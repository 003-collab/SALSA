/*
    Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
    at Austin (ARL:UT).

    SALSA is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License version 3 (GPL-3.0-only) as published
    by the Free Software Foundation.

    SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
    A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with
    SALSA.  If not, see https://www.gnu.org/licenses/.
*/
#include "MainWindow.hpp"
#include "lsabinary.hpp"
#include "LSASupportedToVersion.hpp"
#include "lsainvpairdata.h"
#include "expandpath.hpp"
#include "DebugTimer.hpp"

#include <iostream>
#include <algorithm>
#include <Exception.hpp>
#include <cmath>

#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDatabase>
#include <QClipboard>
#include <QTextStream>
#include <QFileInfo>
#include <QInputDialog>
#include <QSettings>
#include <QMimeData>

#include <math.h>
#include <qstandardpaths.h>
#include <StringUtils.hpp>
#include <LabelChangeDialog.hpp>
#include <QtUtilityMethods.hpp>

using namespace std;
using namespace gnsstk::StringUtils;

const QString MainWindow::QSETTINGS_TREEVIEW_COLUMN_WIDTH_0 = "QSETTINGS_TREEVIEW_COLUMN_WIDTH_0";
const QString MainWindow::QSETTINGS_TREEVIEW_COLUMN_WIDTH_1 = "QSETTINGS_TREEVIEW_COLUMN_WIDTH_1";
const QString MainWindow::QSETTINGS_TREEVIEW_COLUMN_WIDTH_2 = "QSETTINGS_TREEVIEW_COLUMN_WIDTH_2";
const QString MainWindow::QSETTINGS_TREEVIEW_COLUMN_WIDTH_3 = "QSETTINGS_TREEVIEW_COLUMN_WIDTH_3";
const QString MainWindow::QSETTINGS_TREEVIEW_COLUMN_WIDTH_4 = "QSETTINGS_TREEVIEW_COLUMN_WIDTH_4";

const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_0 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_0";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_1 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_1";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_2 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_2";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_3 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_3";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_4 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_4";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_5 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_5";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_6 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_6";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_7 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_7";
const QString MainWindow::QSETTINGS_RESIDUALS_COLUMN_WIDTH_8 = "QSETTINGS_RESIDUALS_COLUMN_WIDTH_8";


const QString MainWindow::QSETTINGS_CONFIDENCE_COLUMN_WIDTH_0 = "QSETTINGS_CONFIDENCE_COLUMN_WIDTH_0";
const QString MainWindow::QSETTINGS_CONFIDENCE_COLUMN_WIDTH_1 = "QSETTINGS_CONFIDENCE_COLUMN_WIDTH_1";
const QString MainWindow::QSETTINGS_CONFIDENCE_COLUMN_WIDTH_2 = "QSETTINGS_CONFIDENCE_COLUMN_WIDTH_2";
const QString MainWindow::QSETTINGS_CONFIDENCE_COLUMN_WIDTH_3 = "QSETTINGS_CONFIDENCE_COLUMN_WIDTH_3";
const QString MainWindow::QSETTINGS_CONFIDENCE_COLUMN_WIDTH_4 = "QSETTINGS_CONFIDENCE_COLUMN_WIDTH_4";
const QString MainWindow::QSETTINGS_CONFIDENCE_COLUMN_WIDTH_5 = "QSETTINGS_CONFIDENCE_COLUMN_WIDTH_5";
const QString MainWindow::QSETTINGS_CONFIDENCE_COLUMN_WIDTH_6 = "QSETTINGS_CONFIDENCE_COLUMN_WIDTH_6";


const QString MainWindow::QSETTINGS_CENTRAL_SPLITTER_SIZE_0 = "QSETTINGS_CENTRAL_SPLITTER_SIZE_0";
const QString MainWindow::QSETTINGS_CENTRAL_SPLITTER_SIZE_1 = "QSETTINGS_CENTRAL_SPLITTER_SIZE_1";
const QString MainWindow::QSETTINGS_CENTRAL_SPLITTER_SIZE_2 = "QSETTINGS_CENTRAL_SPLITTER_SIZE_2";
const QString MainWindow::QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_0 = "QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_0";
const QString MainWindow::QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_1 = "QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_1";

const QString MainWindow::QSETTINGS_WINDOW_GEOMETRY = "QSETTINGS_WINDOW_GEOMETRY";
const QString MainWindow::QSETTINGS_WINDOW_SAVE_STATE = "QSETTINGS_WINDOW_SAVE_STATE";
const QString MainWindow::QSETTINGS_WINDOW_MAXIMIZED = "QSETTINGS_WINDOW_MAXIMIZED";
const QString MainWindow::QSETTINGS_WINDOW_POSITION = "QSETTINGS_WINDOW_POSITION";
const QString MainWindow::QSETTINGS_WINDOW_SIZE = "QSETTINGS_WINDOW_SIZE";

const QString MainWindow::QSETTINGS_SPC_DEFAULT = "QSETTINGS_SPC_DEFAULT";
const int MainWindow::TIMEOUT_START_MILLISECONDS = 5000;

MainWindow::MainWindow(QString lsaFile, QWidget *parent) : errorBox(NULL),
    suppressGuiUpdates(false), suppressProjectChangedDialog(false),autoDismissModalDialogs(false),
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    mapWidget = NULL;
    surveyorWorkspace = nullptr;

    progressBarHasBeenShown = false;//for testing purposes only

    ui->setupUi(this);
    if(!LSARELEASE_TYPE.empty())
        setWindowTitle(QString("SALSA ") + QString::fromStdString(LSARELEASE_TYPE));

    // Do some additional menu setup
    setupMenus();

    guiModel = new GuiModel();

    // Configure handling of warnings and errors pushed from the guiModel
    connect(guiModel, SIGNAL(pushError(QString)), this, SLOT(pushError(QString)));
    connect(guiModel, SIGNAL(pushWarning(QString)), this, SLOT(pushWarning(QString)));
    // Handle when something (e.g. a file) is dropped on the tree view
    connect(guiModel, &GuiModel::signalDataDroppedOnTree, this, &MainWindow::handleDataDroppedOnTree);

    // handle multi-threaded access to gui elements
    connect(this,SIGNAL(updateStatusWindow(bool)), this, SLOT(pushToStatusWindow(bool)));
    connect(this,SIGNAL(updateStatusWindow(bool,QString,int,QColor)), this, SLOT(pushToStatusWindow(bool,QString,int,QColor)));
//    connect(this,SIGNAL(setMenuForSubprocess(bool)),this,SLOT(toggleGUISubprocessElements(bool)));

    // Configure components
    setupTreeView();
    setupOutputTables();
    setupRecordEditor();
    setupAdjustedPositions();
    setupStationData();
    setupProgressBar();
    setupStatusWindow();
    setupHistogram();

    mapWidget = new Mapping(modelIsSaved, this);
    setupMapWidget();
    setupSurveyorWorkspace();

    // Configure suppression of gui updates by child widgets
    connect(recordEditor, SIGNAL(signalSuppressGuiUpdates(bool)), this, SLOT(setSuppressGuiUpdates(bool)) );
    connect(mapWidget, SIGNAL(signalSuppressGuiUpdates(bool)), this, SLOT(setSuppressGuiUpdates(bool)) );
    connect(guiModel, SIGNAL(signalSuppressGuiUpdates(bool)), this, SLOT(setSuppressGuiUpdates(bool)) );

    // Set the enabled/disable state of menu actions
    enableDisableMenuActions();

    // Project Menu
    ui->actionOpen->setEnabled(true);
    ui->actionImport_IOB->setEnabled(true);
    ui->actionNewGeoidFile -> setDisabled(true);
    ui->actionSave->setDisabled(true);
    ui->actionSaveAs->setDisabled(true);
    ui->actionGenerate_Initial_Positions->setDisabled(true);
    ui->actionCalculate_Adjustment->setDisabled(true);
    ui->actionAbort_Adjustment->setDisabled(true);
    ui->actionReload->setDisabled(true);
    ui->actionClose->setDisabled(true);
    ui->actionView_Adjusted_Coordinates->setDisabled(true);

    // Record Menu
    ui->menuInsert->setToolTipsVisible(false);
    ui->actionFind->setDisabled(true);
    ui->actionFind_Next->setDisabled(true);
    ui->actionFindNextWarning->setDisabled(true);
    ui->actionFind_Previous->setDisabled(true);

    // Import Menu
    ui->menuImport->setToolTipsVisible(false);
    ui->actionInsertLSAInclude->setDisabled(true);
    ui->actionInsertIOBInclude->setDisabled(true);
    ui->actionInsertGSIInclude->setDisabled(true);
    ui->actionInsertPPPInclude->setDisabled(true);
    ui->actionInsertTrimbleGPSInclude->setDisabled(true);
    ui->actionInsertSetsOfAnglesInclude->setDisabled(true);
    ui->actionInsertLeicaGNSSInclude->setDisabled(true);

    // View Menu
    ui->actionView_Solver_Output->setDisabled(true);
    ui->actionView_Points_File->setDisabled(true);

    // Make map of qaction to their columns
    mapResidualsActionToColumn = { {ui->action_MeasCol, RESID_TABLE_MEAS_COLUMN},
                                   {ui->action_From_AtCol, RESID_TABLE_FROMAT_COLUMN},
                                   {ui->action_ToCol, RESID_TABLE_TO_COLUMN},
                                   {ui->action_RawCol, RESID_TABLE_RAW_COLUMN},
                                   {ui->action_RelCol, RESID_TABLE_REL_COLUMN},
                                   {ui->action_StdCol, RESID_TABLE_STD_COLUMN},
                                   {ui->action_RedundancyCol, RESID_TABLE_REDUND_COLUMN},
                                   {ui->action_Int_RelCol, RESID_TABLE_MAX_BIAS_COLUMN},
                                   {ui->action_Ext_RelCol, RESID_TABLE_EXT_MAG_COLUMN}
                                 };
    mapPointsActionToColumn = { {ui->action_PointCol, POINT_TABLE_POINT_COLUMN},
                                {ui->action_3D_majCol, POINT_TABLE_3DMAJ_COLUMN},
                                {ui->action_2D_majCol, POINT_TABLE_2DMAJ_COLUMN},
                                {ui->action_VerticalCol, POINT_TABLE_VERT_COLUMN},
                                {ui->action_3D_maj_RRCol, POINT_TABLE_3DMAJRR_COLUMN},
                                {ui->action_2D_maj_RRCol, POINT_TABLE_2DMAJRR_COLUMN},
                                {ui->action_Vertical_RRCol, POINT_TABLE_VERTRR_COLUMN}
                              };

    // Connect right-click context menu
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));

    // Connect splitterMoved signals
    connect(ui->centralSplitter, SIGNAL(splitterMoved(int,int)), this, SLOT(handleCentralSplitterMoved(int,int)) );
    connect(ui->splitterOutputTables, SIGNAL(splitterMoved(int,int)), this, SLOT(handleOutputTablesSplitterMoved(int,int)) );


    // Set central splitter positions
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    QList<int> splitterSizes;
    splitterSizes << settings.value(QSETTINGS_CENTRAL_SPLITTER_SIZE_0,551).toInt();
    splitterSizes << settings.value(QSETTINGS_CENTRAL_SPLITTER_SIZE_1,320).toInt();
    splitterSizes << settings.value(QSETTINGS_CENTRAL_SPLITTER_SIZE_2,114).toInt();
    ui->centralSplitter->setSizes(splitterSizes);

    //set output table splitter positions
    QList<int> outputTablesSplitterSizes;
    outputTablesSplitterSizes << settings.value(QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_0,310).toInt();
    outputTablesSplitterSizes << settings.value(QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_1,190).toInt();
    ui->splitterOutputTables->setSizes(outputTablesSplitterSizes);
    connect(ui->checkBoxFilterMeasOnSelected,SIGNAL(toggled(bool)),this,SLOT(handleMeasFilterChanged(bool)));
    connect(ui->checkBoxFilterPointsOnSelected,SIGNAL(toggled(bool)),this,SLOT(handlePointsFilterChanged(bool)));

    connect(ui->statusWindow,SIGNAL(anchorClicked(QUrl)),this,SLOT(handleStatusWindowAnchorClicked(QUrl)));

    mainWindowInitialState = saveState();
    // initialize exe paths
    QStringList searchPaths;
    QString installationPath = QCoreApplication::applicationDirPath();
    searchPaths << installationPath;
    iob2lsaPath = QStandardPaths::findExecutable(QString("iob2lsa"), searchPaths);
    preprocessorPath = QStandardPaths::findExecutable(QString("lsapreprocessor"), searchPaths);
    solverPath = QStandardPaths::findExecutable(QString("lsasolver"), searchPaths);
    cgeoidPath = QStandardPaths::findExecutable(QString("cgeoid"), searchPaths);
    lsapostPath = QStandardPaths::findExecutable(QString("lsapost"), searchPaths);
    lsainverse = QStandardPaths::findExecutable(QString("lsainverse"), searchPaths);

    QString pythonInstallationPath;
#ifdef _WIN32
    pythonInstallationPath = installationPath + QString("/../python/python.exe");
#else
    pythonInstallationPath = qgetenv("SALSA_PYTHON_EXE");
#endif
    pythonInstallationPath = QDir::cleanPath(pythonInstallationPath);
    QFile python_exe(pythonInstallationPath);
    if(python_exe.exists())
        python3Path = pythonInstallationPath;
    else
    {
        pushError(QString("Error - unable to find ")+pythonInstallationPath + QString(".  Unable to import instrumentation output."));
        python3Path = QString("");
    }

    converterPath = QDir::cleanPath(installationPath + QString("/../scripts/converters/converterlauncher.py"));
    reportingPath = QDir::cleanPath(installationPath + QString("/../scripts/reporting"));
    scriptsPath = QDir::cleanPath(installationPath + QString("/../scripts"));

    //re-used, newed global pointers
    errorBox = NULL;
    configDialog = NULL;
    preferencesDialog = NULL;
    proc = NULL;
    procStdErr = QString("");
    procWarnings = QString("");
    procChainStdErr = QString("");
    procChainWarnings = QString("");

    suppressOutputTableSelectionChanges = false;

    ui->findFilterWidget->hide();

    // calling loadLsaFile alone will not display warnings
    openLSAFilepath(lsaFile);

    QVector<QLabel*> labelsToBeTextSelectable;

    labelsToBeTextSelectable.append(ui->valueAPV);
    labelsToBeTextSelectable.append(ui->valueChiSqr);
    labelsToBeTextSelectable.append(ui->valueMaxIterations);
    labelsToBeTextSelectable.append(ui->valueIterations);
    labelsToBeTextSelectable.append(ui->valueConvLimit);
    labelsToBeTextSelectable.append(ui->valueLastUpdate);
    labelsToBeTextSelectable.append(ui->valueMaxExtMag);
    labelsToBeTextSelectable.append(ui->valueNumDOF);
    labelsToBeTextSelectable.append(ui->valueNumUnknowns);
    labelsToBeTextSelectable.append(ui->valueNumObs);
    labelsToBeTextSelectable.append(ui->valueProcessingTime);

    foreach(QLabel *label, labelsToBeTextSelectable)
    {
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }

    installEventFilter(this);

    QString executablePath = QDir::cleanPath(QString::fromStdString(getExecutablePath()) + "/..");
    qputenv("SALSA_PROJECT_DIR", executablePath.toLocal8Bit());

    return;
}

MainWindow::~MainWindow()
{
    //clear lsaRecordBuffer
    foreach(LSARecord* lsaRecord, lsaRecordBuffer)
    {
        delete lsaRecord;
    }

    cleanCustomActions(ui->menuExport);

    delete mapWidget;
    delete recordEditor;
    if(adjustedPositions) delete adjustedPositions;
    if(stationData)       delete stationData;
    if(histogramDialog)   delete histogramDialog;
    delete guiModel;
    delete ui;
}

void MainWindow::setupMenus()
{
    // Project menu
    connect(ui->actionNew,                       SIGNAL(triggered()), this, SLOT(createNewProject())         );
    connect(ui->actionOpen,                      SIGNAL(triggered()), this, SLOT(openLSAProject())           );
    connect(ui->actionImport_IOB,                SIGNAL(triggered()), this, SLOT(importProjectFromIOB())     );
    connect(ui->actionReload,                    SIGNAL(triggered()), this, SLOT(reloadProject())            );
    connect(ui->actionClose,                     SIGNAL(triggered()), this, SLOT(closeProject())             );
    connect(ui->actionSave,                      SIGNAL(triggered()), this, SLOT(saveProject())              );
    connect(ui->actionSaveAs,                    SIGNAL(triggered()), this, SLOT(saveAsProject())              );
    connect(ui->actionCalculate_Adjustment,      SIGNAL(triggered()), this, SLOT(calculateAdjustment())      );
    connect(ui->actionGenerate_Initial_Positions,SIGNAL(triggered()), this, SLOT(generateInitialPositions()) );
    connect(ui->actionQuit,                      SIGNAL(triggered()), this, SLOT(quitApplication())          );
    connect(ui->actionAbort_Adjustment,          SIGNAL(triggered()), this, SLOT(killProcess())              );
    connect(ui->actionRestore_Record_Editor,     SIGNAL(triggered()), this, SLOT(RestoreDockableWidgets())   );
    connect(ui->actionConfigure,                 SIGNAL(triggered()), this, SLOT(openConfigDialog())         );

    connect(ui->actionNewGeoidFile,              SIGNAL(triggered()), this, SLOT(convertToGeoidFile())       );
    connect(ui->actionView_Histogram,            SIGNAL(triggered()), this, SLOT(updateHistogramVisibility()));

    // Edit menu
    connect(ui->actionCopy,                      SIGNAL(triggered()), this, SLOT(handleCopyRecord())	     );
    connect(ui->actionCut,                       SIGNAL(triggered()), this, SLOT(handleCutRecord()));
    connect(ui->actionPaste,                     SIGNAL(triggered()), this, SLOT(handlePasteRecord())	     );
    connect(ui->actionDuplicate,                 SIGNAL(triggered()), this, SLOT(handleDuplicateRecord())      );
    connect(ui->actionUndo,                      SIGNAL(triggered()), this, SLOT(undo()));
    connect(ui->actionRedo,                      SIGNAL(triggered()), this, SLOT(redo()));

    // Import menu
    connect(ui->actionInsertNEWInclude,          SIGNAL(triggered()), this, SLOT(insertNewInclude())           );
    connect(ui->actionInsertLSAInclude,          SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromLSA()) );
    connect(ui->actionInsertIOBInclude,          SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromIOB()) );
    connect(ui->actionInsertSetsOfAnglesInclude, SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromSetsOfAngles()) );
    connect(ui->actionInsertTrimbleGPSInclude,   SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromTrimbleTDEF()) );
    connect(ui->actionInsertPPPInclude,          SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromPPP()) );
    connect(ui->actionInsertGSIInclude,          SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromGSI()) );
    connect(ui->actionInsertOPUSInclude,         SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromOPUS()));
    connect(ui->actionInsertLeicaGNSSInclude,    SIGNAL(triggered()), this, SLOT(handleInsertIncludeFromLeicaGNSS()) );

    connect(ui->actionInsertPOSG,    SIGNAL(triggered()), this, SLOT(insertPOSG()) );
    connect(ui->actionInsertPOSC,    SIGNAL(triggered()), this, SLOT(insertPOSC()) );
    connect(ui->actionInsertDIST,    SIGNAL(triggered()), this, SLOT(insertDIST()) );
    connect(ui->actionInsertDXYZ,    SIGNAL(triggered()), this, SLOT(insertDXYZ()) );
    connect(ui->actionInsertHANG,    SIGNAL(triggered()), this, SLOT(insertHANG()) );
    connect(ui->actionInsertAZIM,    SIGNAL(triggered()), this, SLOT(insertAZIM()) );
    connect(ui->actionInsertVANG,    SIGNAL(triggered()), this, SLOT(insertVANG()) );
    connect(ui->actionInsertZANG,    SIGNAL(triggered()), this, SLOT(insertZANG()) );
    connect(ui->actionInsertHDIF,    SIGNAL(triggered()), this, SLOT(insertHDIF()) );
    connect(ui->actionInsertHDIR,    SIGNAL(triggered()), this, SLOT(insertHDIR()) );
    connect(ui->actionInsertDGRP,    SIGNAL(triggered()), this, SLOT(insertDGRP()) );
    connect(ui->actionInsertHGHT,    SIGNAL(triggered()), this, SLOT(insertHGHT()) );
    connect(ui->actionInsertVSCA,    SIGNAL(triggered()), this, SLOT(insertVSCA()) );
    connect(ui->actionInsertUNCR,    SIGNAL(triggered()), this, SLOT(insertUNCR()) );
    connect(ui->actionInsertComment, SIGNAL(triggered()), this, SLOT(insertComment()) );
    connect(ui->actionInsertSeparator,  SIGNAL(triggered()), this, SLOT(insertSeparator()) );

    connect(ui->actionInsertMEAN,    SIGNAL(triggered()), this, SLOT(insertMEAN()) );
    connect(ui->actionInsertENUO,    SIGNAL(triggered()), this, SLOT(insertENUO()) );

    connect(ui->actionDelete, SIGNAL(triggered()), this, SLOT( deleteCurrentlySelectedRecords()) );
    connect(ui->actionFind, SIGNAL(triggered()), this, SLOT( findRecord()) );
    connect(ui->actionFind_Next, SIGNAL(triggered()), ui->findFilterWidget, SLOT( findNext()) );
    connect(ui->actionFindNextWarning, SIGNAL(triggered()), ui->findFilterWidget, SLOT( findNextWarning()) );
    connect(ui->actionFind_Previous, SIGNAL(triggered()), ui->findFilterWidget, SLOT( findPrevious()) );
    connect(ui->findFilterWidget, SIGNAL(closing(QWidget *)), this, SLOT(handleFindWidgetClose(QWidget *)) );
    connect(ui->findFilterWidget, SIGNAL(filterStringChanged(QString, bool, bool)), this, SLOT(handleFilterStringChanged(QString, bool, bool)));
    connect(ui->findFilterWidget, SIGNAL(boxValueChanged(std::vector<LSAType>,std::function<bool (std::vector<LSAType>,LSARecord*)>)),
            this, SLOT(handleFilterBoxChanged(std::vector<LSAType>, std::function<bool (std::vector<LSAType>, LSARecord *)>)) );
    connect(ui->findFilterWidget, SIGNAL(matchTypeChanged(bool)), this, SLOT(handleMatchTypeChanged(bool)));
    connect(ui->actionFilterPoint, SIGNAL(triggered()), this, SLOT(handleFilterPointTriggered()));
    connect(ui->actionSelect_Referencing, SIGNAL(triggered()), this, SLOT(selectReferencingRecords()));
    connect(ui->actionEnable, SIGNAL(triggered()), this, SLOT( enableCurrentlySelectedRecords()) );
    connect(ui->actionDisable, SIGNAL(triggered()), this, SLOT( disableCurrentlySelectedRecords()) );
    connect(ui->actionMove_Up, SIGNAL(triggered()), this, SLOT(moveRecordUp()));
    connect(ui->actionMove_Down, SIGNAL(triggered()), this, SLOT(moveRecordDown()));

    // Preferences menu
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    bool showComments = qsettings.value(guiModel->QSETTINGS_SHOWCOMMENTS, true).toBool();
    ui->actionShow_Comments->setChecked(showComments);
    connect(ui->actionShow_Comments, SIGNAL(triggered()), this, SLOT(updateRecordVisibility()) );
    connect(ui->actionPreferences,   SIGNAL(triggered()), this, SLOT(openPreferencesDialog())   );

    // Export menu
    connect(ui->actionAdjustedLSA,         SIGNAL(triggered()), this, SLOT(exportPOSGsFromHDF5File())                 );
    connect(ui->actionGeoidHeightsAPriori, SIGNAL(triggered()), this, SLOT(exportGeoidHeightsFromAprioriPositions()) );
    connect(ui->actionExportMap,           SIGNAL(triggered()), this, SLOT(exportMap())                );
    connect(ui->actionExport_KML,           SIGNAL(triggered()), this, SLOT(exportKML())                );
    connect(ui->actionExportPointsXYZ,     SIGNAL(triggered()), this, SLOT(exportPointsXYZ())                );
    connect(ui->actionExportPointsUTM,     SIGNAL(triggered()), this, SLOT(exportPointsUTM())                );
    connect(ui->actionExportSPC,           SIGNAL(triggered()), this, SLOT(exportPointsSPC()) );
    connect(ui->actionExportPointsNCAT,     SIGNAL(triggered()), this, SLOT(exportPointsNCAT())                );
    connect(ui->actionExportSolnCov,     SIGNAL(triggered()), this, SLOT(exportSolnCov())                );
    connect(ui->actionExportMeasResid,     SIGNAL(triggered()), this, SLOT(exportMeasResid())                );
    connect(ui->actionExportMeasResidAsENU,     SIGNAL(triggered()), this, SLOT(exportMeasResidAsENU())                );

    // Help menu
    connect(ui->actionAbout_Salsa,      SIGNAL(triggered()), this, SLOT(showAboutSalsaDialog()) );
    connect(ui->actionView_User_Manual, SIGNAL(triggered()), this, SLOT(viewManual())       );

    // View menu
    connect(ui->actionView_RecordEditor,      SIGNAL(triggered()), this, SLOT(updateRecordEditorVisibility()) );
    connect(ui->actionView_Map,               SIGNAL(triggered()), this, SLOT(updateMapVisibility()) );
    connect(ui->actionView_AdjustedPositions, SIGNAL(triggered()), this, SLOT(updateAdjustedPositionsVisibility()) );
    connect(ui->actionView_StationData,       SIGNAL(triggered()), this, SLOT(updateStationDataVisibility()) );
    connect(ui->actionView_Solver_Output,     SIGNAL(triggered()), this, SLOT(viewOutput())               );
    connect(ui->actionView_Points_File,       SIGNAL(triggered()), this, SLOT(viewAdjustedPoints()) );
    bool visibleRecordEditor = qsettings.value(guiModel->QSETTINGS_VISIBLE_RECORDEDITOR, true).toBool();
    bool visibleMap = qsettings.value(guiModel->QSETTINGS_VISIBLE_MAP, true).toBool();
    ui->actionView_RecordEditor->setChecked(visibleRecordEditor);
    ui->actionView_Map->setChecked(visibleMap);
    ui->actionView_AdjustedPositions->setChecked(false);
    ui->actionView_StationData->setChecked(false);
    ui->actionView_Histogram->setChecked(false);

    connect(ui->actionAdd_Autogen,              SIGNAL(triggered() ), this, SLOT(addAutogeneratedPoints() ) );

    // Recent projects menu
    setupRecentProjectsMenu();
}

void MainWindow::setupRecentProjectsMenu()
{

    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    // Don't allow falling back to the org level settings in the tests !1721
    if (qsettings.applicationName() != lsa::salsaApplicationName)
    {
        qsettings.setFallbacksEnabled(false);
    }
    QStringList recentProjects = qsettings.value(lsa::QSETTINGS_RECENTPROJECTS,QStringList()).toStringList();
    qsettings.setFallbacksEnabled(true);

    ui->RecentMenu->setToolTipsVisible(true);

    // Initialize the recent projects menu. The list is ordered by most recent to oldest, so we need to add the oldest first.
    for (int i = recentProjects.size() - 1; i >= 0; --i)
    {
        addActionRecentProjects(recentProjects.at(i));
    }

    connect(ui->RecentMenu, SIGNAL(aboutToShow()), this, SLOT(removeDeletedProjectsRecentMenu()) );

    return;

}

void MainWindow::setupOutputTables()
{
    // Residuals table
    // NUM_RESID_COLUMNS=10;//| Meas | From(-At )| To | Raw | Rel | Std | Red'cy | Max Bias | Ext Mag | hidden column for LSARecord*
    topResidModel = new QStandardItemModel(5,NUM_RESID_COLUMNS,this);//5 rows
    ui->tableMeasurements->setModel(topResidModel);
    ui->tableMeasurements->setColumnHidden(NUM_RESID_COLUMNS-1,true);

    // Adjustments table
    confidenceModel = new QStandardItemModel(5,NUM_POINT_COLUMNS,this);//5 rows (temp value - overwritten later)
    ui->tablePoints->setModel(confidenceModel);
    lastIndicatorColumn = 4;
    lastIndicatedOrder = Qt::DescendingOrder;

    setOutputTableColumnWidths();
    addOutputTableHeaders();

    // Connect signals for resizing output tables
    connect(ui->tableMeasurements->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(handleTableMeasurementsResized(int,int,int)) );
    connect(ui->tablePoints->horizontalHeader(),       SIGNAL(sectionResized(int,int,int)), this, SLOT(handleTablePointsResized(int,int,int)) );

    // Connect signals to update output tables
    connect(ui->tableMeasurements->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(handleTableMeasurementsSelectionChanged(QItemSelection, QItemSelection)));
    connect(ui->tablePoints->selectionModel(),       SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(handleTablePointsSelectionChanged(QItemSelection, QItemSelection)));

    // Connect signals for user clicks in output tables
    connect(ui->tablePoints,                     SIGNAL(doubleClicked(QModelIndex)),  this, SLOT(handleTablePointsDoubleClicked(QModelIndex)));
    connect(ui->tablePoints->horizontalHeader(), SIGNAL(sectionClicked(int)),         this, SLOT(onTablePointsHeaderClicked(int)));

    // setup context menu signals - Residuals
    ui->tableMeasurements->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableMeasurements->horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenuResiduals(const QPoint &)));

    // setup context menu signals - Points
    ui->tablePoints->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tablePoints->horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenuPoints(const QPoint &)));

    // connect the actions for when the context menu is used - Residuals
    connect(ui->action_MeasCol,       SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_From_AtCol,    SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_ToCol,         SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_RawCol,        SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_RelCol,        SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_StdCol,        SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_RedundancyCol, SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_Int_RelCol,    SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_Ext_RelCol,    SIGNAL(toggled(bool)), this, SLOT(toggleResidualsTableColumn(bool)), Qt::UniqueConnection);

    // connect the actions for when the context menu is used - Points
    connect(ui->action_PointCol,       SIGNAL(toggled(bool)), this, SLOT(togglePointsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_3D_majCol,      SIGNAL(toggled(bool)), this, SLOT(togglePointsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_2D_majCol,      SIGNAL(toggled(bool)), this, SLOT(togglePointsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_VerticalCol,    SIGNAL(toggled(bool)), this, SLOT(togglePointsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_3D_maj_RRCol,   SIGNAL(toggled(bool)), this, SLOT(togglePointsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_2D_maj_RRCol,   SIGNAL(toggled(bool)), this, SLOT(togglePointsTableColumn(bool)), Qt::UniqueConnection);
    connect(ui->action_Vertical_RRCol, SIGNAL(toggled(bool)), this, SLOT(togglePointsTableColumn(bool)), Qt::UniqueConnection);

    QString showHideTooltip = "Right mouse button to show/hide columns.";
    ui->tableMeasurements->horizontalHeader()->setToolTip(showHideTooltip);
    ui->tablePoints->horizontalHeader()->setToolTip(showHideTooltip);
    return;
}

// Setup the RecordEditor
void MainWindow::setupRecordEditor()
{
    recordEditor = new RecordEditor(guiModel, this);
    addDockWidget(Qt::RightDockWidgetArea, recordEditor);
    connect(recordEditor, SIGNAL(exceptionCaught(QString)), this, SLOT(handleException(QString)) );
    connect(recordEditor, SIGNAL(giveFocusToTreeView()),     this, SLOT(giveFocusToTreeView()) );
    connect(recordEditor, SIGNAL(replaceInclude(QModelIndex,QStringList)), this, SLOT(handleReplaceInclude(QModelIndex, QStringList)) );
    connect(recordEditor, SIGNAL(reloadAfterParseWarning()), this, SLOT(reloadProjectAfterParseWarning()));
    connect(recordEditor, SIGNAL(closing()), this, SLOT(handleRecordEditorClose()));

    bool wasLastVisible = ui->actionView_RecordEditor->isChecked();
    recordEditor->setVisible(wasLastVisible);
}

// Setup the map widget
void MainWindow::setupMapWidget()
{
    DebugTimer timer(false);

    splitDockWidget(recordEditor, mapWidget, Qt::Vertical);

    mapWidget->setGuiModel(guiModel);

    connect(mapWidget, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(handleItemSelectionChanged(QItemSelection, QItemSelection)) );
    connect(mapWidget, SIGNAL(pointCoordinatesChanged(QModelIndex)),             this, SLOT(handlePointCoordinateChanged(QModelIndex)) );
    connect(mapWidget, SIGNAL(pushWarning(QString)),                             this, SLOT(pushWarning(QString)) );
    connect(ui->actionMapPreferences, SIGNAL(triggered()),                       mapWidget, SLOT(handleMapOptionsTriggered()) );
    connect(mapWidget, SIGNAL(closing()),                                     this, SLOT(handleMapClose()));

    mapWidget->initialize();

    bool wasLastVisible = ui->actionView_Map->isChecked();
    mapWidget->setVisible(wasLastVisible);
}

// Setup the AdjustedPositions
void MainWindow::setupAdjustedPositions()
{
    adjustedPositions = new AdjustedPositions(guiModel);//widget starts visible regardless of visibility setting
//    adjustedPositions = new AdjustedPositions(guiModel);//widget cannot be moved by dragging
    connect(adjustedPositions, SIGNAL(closing()), this, SLOT(handleAdjustedPositionsClose()));

    bool wasLastVisible = ui->actionView_AdjustedPositions->isChecked();
    adjustedPositions->setVisible(wasLastVisible);
}

void MainWindow::setupHistogram()
{
//    histogramDialog = new HistogramDialog(this);
    histogramDialog = new HistogramDialog();
    connect(histogramDialog, SIGNAL(closing()), this, SLOT(handleHistogramClose()));

    bool wasLastVisible = ui->actionView_Histogram->isChecked();
    histogramDialog->setVisible(wasLastVisible);
    histogramDialog->clearHistogram();
}

void MainWindow::setupStationData()
{
    stationData = new StationDataDialog();
    stationData->setWindowFlags(Qt::CustomizeWindowHint);
    stationData->setWindowFlags(Qt::WindowCloseButtonHint);
    stationData->setWindowTitle ("Station Inverse Dialog");

    connect(stationData, SIGNAL(pushWarning(QString)), this, SLOT(pushWarning(QString)));
    connect(stationData, SIGNAL(pushError(QString)), this, SLOT(pushError(QString)));
    connect(stationData, SIGNAL(pushStatus(QString)), this, SLOT(pushStatus(QString)));
    connect(stationData, SIGNAL(closing()), this, SLOT(handleStationDataClose()) );

    bool wasLastVisible = ui->actionView_StationData->isChecked();
    stationData->setVisible(wasLastVisible);
}

/******************************************************************************
//
// Events
//
******************************************************************************/

void MainWindow::moveEvent(QMoveEvent *event)
{
    saveWindowSettings();

    QMainWindow::moveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    saveWindowSettings();

    QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowSettings();

    if (!quitApplication())
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }
}


/******************************************************************************
//
// Slots
//
******************************************************************************/

void MainWindow::onTablePointsHeaderClicked(int logicalIndex)
{
    bool ascending = (lastIndicatorColumn == logicalIndex) && (lastIndicatedOrder == Qt::DescendingOrder);

    Qt::SortOrder order = ascending ? Qt::AscendingOrder : Qt::DescendingOrder;

    ui->tablePoints->horizontalHeader()->setSortIndicator(logicalIndex,order);
    ui->tablePoints->sortByColumn(logicalIndex,order);
    lastIndicatorColumn = logicalIndex;
    lastIndicatedOrder = order;
}

void MainWindow::importProjectFromIOB()
{

    // Give the user a chance to save changes before closing the current project
    if ( confirmChangesAreSaved() == QMessageBox::Cancel) return;

    QString fileName = "";
    QString selfilter = "GeoLab IOB Files (*.iob)";

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();

    fileName = QFileDialog::getOpenFileName(this, // parent
                                            "Import GeoLab IOB file", // caption
                                            lastLSAPath, // dir
                                            "GeoLab IOB Files (*.iob);; All files (*)", // filter string
                                            &selfilter); // filter
    if(!fileName.isEmpty())
    {
        QString currentDir = QString::fromStdString(getPathWithoutFileName(fileName.toStdString()));
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        settings.setValue(lsa::QSETTINGS_LASTPATH, currentDir);

        convertAndLoadIOBProject(fileName);
    }

    return;
}

void MainWindow::copyConfigFile(QString lsaFileName)
{
    if(lsaFileName.isEmpty())
        lsaFileName = currentFile;
    QFile configFile;
    configFileName = lsaFileName.left(lsaFileName.lastIndexOf(".")) + ".cfg";
    configFile.setFileName(configFileName);

    // If config file doesn't exist, copy it from lsa/data/default.cfg
    if ( !configFile.exists() )
    {
        QString systemConfig = QString::fromStdString(getExecutablePath() + "/../data/default.cfg");
        QFile::copy(systemConfig, configFileName);
    }
}

void MainWindow::handleFocusChanged(QWidget* oldWidget, QWidget* newWidget)
{
    // These lines can be helpful when debugging:
    // if (newWidget == NULL || oldWidget == NULL) return;
    // QString debug_NewWidgetName = (newWidget == NULL) ? "nullWidget" : newWidget->objectName();
    // QString debug_OldWidgetName = (oldWidget == NULL) ? "nullWidget" : oldWidget->objectName();

    // If the TreeView or a widget in the RecordEditor lost focus, push its changes into the LSARecord
    if (oldWidget == ui->treeView || isChildOfWidget(oldWidget, recordEditor))
    {
        // QComboBox widgets lose focus to a QComboBoxListView widget when a user clicks on them
        // We don't want to update when this happens
        std::string oldWidgetType = oldWidget->metaObject()->className();
        std::string newWidgetType = (newWidget == NULL) ? "nullWidget" : newWidget->metaObject()->className();
        bool isQComboBoxSelection = (oldWidgetType == "QComboBox" && newWidgetType == "QComboBoxListView") ||
                                    (newWidgetType == "QComboBox" && oldWidgetType == "QComboBoxListView");

        // If the record editor is currently suppressing updates, we don't want to push changes into a LSARecord
        if (!isQComboBoxSelection && !guiModel->suppressGuiModelUpdates && recordEditor->hasPendingChanges() )
        {
            recordEditor->pushChangesToGuiModel();

            if (newWidget != NULL)
            {
                newWidget->setFocus();
            }
        }
    }

    // If a QSpinBox or QLineEdit in the Record Editor gained focus, select all of its contents for editing
    if ( isChildOfWidget(oldWidget, recordEditor) )
    {
        if (newWidget != NULL)
        {
            if ( newWidget->inherits("QDoubleSpinBox") )
            {
                QDoubleSpinBox *dSpinBox = qobject_cast<QDoubleSpinBox*>(newWidget);
                dSpinBox->selectAll();
            }
            else if( newWidget->inherits("QSpinBox") )
            {
                QSpinBox *spinBox = qobject_cast<QSpinBox*>(newWidget);
                spinBox->selectAll();
            }
            else if( newWidget->inherits("QLineEdit") )
            {
                QLineEdit *lineEdit = qobject_cast<QLineEdit*>(newWidget);
                lineEdit->selectAll();
            }
        }
    }

    /*
     * This statement is used to help record the last widget that is not a child of
     * the find/filter widget. This information is relayed to ui->findFilterWidget so that it
     * will give focus to FindWidget::priorFocusWidget upon closing the findFilterWidget instead
     * of giving default focus to whatever is specified by the tab order. This information is relayed
     * regardless of whether or not the Find/Filter widget is currently open.
     */

    if(!isChildOfWidget(newWidget, ui->findFilterWidget) &&
            (isChildOfWidget(newWidget, this) || isChildOfWidget(newWidget, adjustedPositions)) )
    {
        ui->findFilterWidget->setPriorWidget(newWidget);
    }

    enableDisableCopyPaste();
    showHideAddAutogen();

}

void MainWindow::handleException(QString message)
{
    pushError(message);
}

void MainWindow::handleDataChange(QModelIndex topLeft, QModelIndex botRight, QVector<int> roles)
{
    // Check if an include or VSCA changed
    bool includeOrVSCAChanged = false;
    for (int row = topLeft.row(); row <= botRight.row(); ++row)
    {
        for (int col = topLeft.column(); col <= botRight.column(); ++col)
        {
            QModelIndex index = guiModel->index(row, col, topLeft.parent());
            if (index.isValid())
            {
                LSARecord* lsaRecord = guiModel->getLSARecord(index);
                LSAType    lsaType = lsaRecord->getRecType();
                if (lsaType == LSAType::VSCA || lsaType == LSAType::INCLUDE)
                {
                    includeOrVSCAChanged = true;
                    break;
                }
            }
        }
    }

    if (includeOrVSCAChanged)
    {
        guiModel->synchGuiModelToLSATree();
    }

    bTVRowVisibilityChanged = true;
    updateWidgets();

    return;
}

void MainWindow::createNewProject(QString filename)
{
    // Let the user select a project and parent .lsa file
    bool saveAsLSA = false;
    if (filename.isEmpty())
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();
        QString selfilter = tr("LSA Project Files (*.proj)");
        filename = QFileDialog::getSaveFileName(this,
                                                tr("Create a new project directory and .proj file"),
                                                lastLSAPath,
                                                tr("LSA Project Files (*.proj);; LSA Files (*.lsa);; All files (*)"),
                                                &selfilter);

        // Part of fix to bug 1499 where linux didn't save the file extension
        if(selfilter == "LSA Files (*.lsa)")
        {
            saveAsLSA = true;
        }
    }

    if (!filename.isEmpty())
    {
        #ifndef _WIN32 // fix to bug 1499
        QFileInfo fi(filename);
        QString ext = fi.suffix();
        if(ext == "")
        {
            if(saveAsLSA)
            {
                filename = filename + ".lsa";
            }
            else
            {
                filename = filename + ".proj";
            }
        }
        #endif

        QString currentDir = QString::fromStdString(getPathWithoutFileName(filename.toStdString()));
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        settings.setValue(lsa::QSETTINGS_LASTPATH, currentDir);

        QFileInfo check(filename);
        QFile gonner(filename);
        if(check.exists()) gonner.remove();

        // Create the file on disk
        QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString header("#New project created " + dateTime);

        QFile newFile(filename);
        newFile.open(QIODevice::WriteOnly | QIODevice::Text);

        QTextStream newStream(&newFile);
        newStream << header;
        newFile.close();

        // clear the status window
        emit updateStatusWindow(true);

        // Create a new model with the newly created root include file
        loadLsaFile(filename);
        guiModel->clearRootIncludeUIWarnings();

        guiModel->save();
        QModelIndex rootIndex = guiModel->index(0,0);
        guiModel->clearDescendantIncludeIsModified(rootIndex);

        // Make sure the map is showing initial coordinates
        mapWidget->initMapInitialCoodinates();//switch to input display
        mapWidget->setHome();

        copyConfigFile();

        copyCustomFiles(currentDir, "customExport");
        copyCustomFiles(currentDir, "customImport");

        // Select the auto-generated comment
        guiModel->select(rootIndex.child(0,0));
    }

    return;
}

void MainWindow::openLSAProject(QString filename)
{
    // Give the user a chance to save changes before closing the current project
    if ( confirmChangesAreSaved() == QMessageBox::Cancel) return;

    emit updateStatusWindow(true);

    QString fileName = filename;
    QString selfilter = tr("LSA Project Files (*.proj)");

    if (filename.isEmpty())
    {

        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();

        fileName = QFileDialog::getOpenFileName(this,
                                                tr("Open LSA-compatible file"),
                                                lastLSAPath,
                                                tr("LSA Project Files (*.proj);; LSA Files (*.lsa);; All files (*)"),
                                                &selfilter);
    }

    if(!fileName.isEmpty())
    {
        QString currentDir = QString::fromStdString(getPathWithoutFileName(fileName.toStdString()));
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        settings.setValue(lsa::QSETTINGS_LASTPATH, currentDir);

    }

    openLSAFilepath(fileName);

    return;
}

void MainWindow::removeDeletedProjectsRecentMenu()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    QStringList recentProjects = qsettings.value(lsa::QSETTINGS_RECENTPROJECTS,QStringList()).toStringList();

    QMenu* recentMenu = ui->RecentMenu;

    // If the project no longer exists, remove it from the recent projects menu
    foreach (QAction* action, recentMenu->actions())
    {
        QString fileName = action->data().toString();
        QFileInfo file(fileName);

        if(!file.exists() && action != ui->actionEmptyRecent)
        {
            recentProjects.removeAll(fileName);
            recentMenu->removeAction(action);
        }
    }


    if (recentMenu->actions().size() == 0)
    {
        recentMenu->addAction(ui->actionEmptyRecent);
    }

    qsettings.setValue(lsa::QSETTINGS_RECENTPROJECTS,recentProjects);

    return;
}

void MainWindow::openRecentLSAProject()
{
    // Give the user a chance to save changes before closing the current project
    if ( confirmChangesAreSaved() == QMessageBox::Cancel) return;

    emit updateStatusWindow(true);

    // Get the action sender of triggered signal
    QAction* action = qobject_cast<QAction*>(sender());
    QString fileName = action->data().toString();

    openLSAFilepath(fileName);

    return;
}

void MainWindow::updateRecentProjectsMenu(QString fileName)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    // Don't allow falling back to the org level settings in the tests !1721
    if (qsettings.applicationName() != lsa::salsaApplicationName)
    {
        qsettings.setFallbacksEnabled(false);
    }
    QStringList recentProjects = qsettings.value(lsa::QSETTINGS_RECENTPROJECTS,QStringList()).toStringList();
    qsettings.setFallbacksEnabled(true);

    QMenu* recentMenu = ui->RecentMenu;

    // If recent projects menu already contains this file, reorder the menu list
    if (recentProjects.contains(fileName))
    {
        QList<QAction*> list = recentMenu->actions();
        foreach (QAction* item, list)
        {
            if (item->data().toString() == fileName)
            {
                // Check to see if the file is already in the most recent position in the list.
                if (list.front() != item)
                {
                    recentMenu->removeAction(item);
                    recentMenu->insertAction(list.front(), item);
                }
                break;
            }
        }
        recentProjects.removeAll(fileName);
        recentProjects.prepend(fileName);
        qsettings.setValue(lsa::QSETTINGS_RECENTPROJECTS,recentProjects);

    }
    // Add file to recent projects menu. If over the 5 item threshold, remove the oldest
    else
    {
        if (recentProjects.size() >= 5)
        {
            recentProjects.removeLast();
            QAction* last = recentMenu->actions().last();
            recentMenu->removeAction(last);
        }
        recentProjects.prepend(fileName);
        qsettings.setValue(lsa::QSETTINGS_RECENTPROJECTS,recentProjects);

        addActionRecentProjects(fileName);
    }

    return;
}

void MainWindow::addActionRecentProjects(QString fileName)
{
    QMenu* recentMenu = ui->RecentMenu;

    // Only show the user the file name
    QString text = QFileInfo(fileName).fileName();

    QAction* newAction = new QAction(text, recentMenu);

    // Save the file path
    newAction->setData(fileName);
    newAction->setToolTip(fileName);

    if (recentMenu->actions().size() == 0)
    {
        recentMenu->addAction(ui->actionEmptyRecent);
    }

    recentMenu->insertAction(recentMenu->actions().front(), newAction);

    if (recentMenu->actions().contains(ui->actionEmptyRecent))
    {
        recentMenu->removeAction(ui->actionEmptyRecent);
    }

    connect(newAction, SIGNAL(triggered()), this, SLOT(openRecentLSAProject()));

    return;
}

void MainWindow::openLSAFilepath(QString filepath)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    if(!filepath.isEmpty())
    {
        QString newLastPath = QString::fromStdString(getPathWithoutFileName(filepath.toStdString()));
        settings.setValue(lsa::QSETTINGS_LASTPATH,newLastPath);
        std::vector<std::string> errorsAndWarnings = loadLsaFile(filepath);

        for(size_t i=0; i<errorsAndWarnings.size(); i++)
        {
           pushWarning(errorsAndWarnings[i]);
        }
    }


    popupForMissingIncludes();

    if(failedIncludeList.size()>0)
    {
        if(!isTestHarness())
        {
            for(int i=0;i<failedIncludeList.size();i++)
            {
                QMessageBox *notifyUser = new QMessageBox(QMessageBox::Critical,
                                                          QString("Included file not found!"),
                                                          QString("Unable to parse ")+failedIncludeList.at(i),
                                                          QMessageBox::Ok,
                                                          (QWidget*)this,
                                                          Qt::WindowStaysOnTopHint);
                notifyUser->exec();//blocks
                delete notifyUser;
            }
        }
        failedIncludeList.clear();
    }

    return;
}

void MainWindow::reloadProject()
{
    // Give the user a chance to save changes before closing the current project
    if ( confirmChangesAreSaved() == QMessageBox::Cancel) return;

    reloadProjectAfterSave();
}

void MainWindow::reloadProjectAfterSave()
{
    std::string projectFilename = guiModel->getProjectFilename();

    QString newLastPath = QString::fromStdString(getPathWithoutFileName(projectFilename));
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(lsa::QSETTINGS_LASTPATH,newLastPath);
    std::vector<std::string> badrecs = loadLsaFile(QString::fromStdString(projectFilename));

    for(size_t i=0; i<badrecs.size(); i++)
    {
       pushWarning(badrecs[i]);
    }
}

// Fix to issue #1034. confirmChangesAreSavedParseWarning() is also to fix this issue
void MainWindow::reloadProjectAfterParseWarning()
{
   // Give the user a chance to save changes before closing the current project
   if ( confirmChangesAreSavedParseWarning() == (QMessageBox::Cancel))
   {
      return;
   }

   reloadProjectAfterSave();
}

void MainWindow::closeProject()
{
    // Give the user a chance to save changes before closing the current project
    if ( confirmChangesAreSaved() == QMessageBox::Cancel) return;

    loadLsaFile("");

    // disable some menu options
    enableDisableMenuActions();
    ui->actionReload->setDisabled(true);
    ui->actionConfigure->setDisabled(true);
    ui->actionFind->setDisabled(true);
    ui->actionFindNextWarning->setDisabled(true);
    ui->actionFind_Next->setDisabled(true);
    ui->actionFind_Previous->setDisabled(true);

    // clear the status window
    emit updateStatusWindow(true);

    // clear the Final Positions View
    adjustedPositions->clear();

    // clear the Station Data Dialog View
    stationData->clear();

    // Move the current directory to home (Fix to issue #752)
    QString homeDir = QDir::homePath();
    QDir::setCurrent(homeDir);

    return;

}

void MainWindow::saveProject()
{
    bool change = !guiModel->isFileVersionCurrent(std::string("1.10.0"));
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    bool giveWarning = qsettings.value(lsa::QSETTINGS_WARN_PREC_UPGRADE, true).toBool();

    if(change && giveWarning && !isTestHarness())//c.f. Git Issue #118
    {
        // Get the path to the user manual
        QDir manualDir(QString::fromStdString(getExecutablePath()) + QString("/../doc"));
        QString manualPath = QString::fromStdString("file:") + manualDir.absoluteFilePath("lsaValidationSpec.xlsx");

        QMessageBox confirmBox;
        confirmBox.setWindowTitle("Save Project?");
        confirmBox.setText(QString("The .lsa file spec has been updated to reflect the state of the art precision for measurements "
                                   "(refer to the <a href=\"" + manualPath + "\">LSA Validation Spec workbook</a>).  Your project's values may be reduced in precison."));
        confirmBox.setCheckBox(new QCheckBox("Turn off future warnings"));
        confirmBox.checkBox()->setCheckState(Qt::CheckState::Unchecked);
        confirmBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        confirmBox.setDefaultButton(QMessageBox::Ok);
        int buttonClicked = confirmBox.exec();

        if (buttonClicked == QMessageBox::Cancel)
            return;

        if (confirmBox.checkBox()->isChecked())
        {
            qsettings.setValue(lsa::QSETTINGS_WARN_PREC_UPGRADE, false);
        }
    }

    // Make sure we push any pending changes in the Record Editor into the LSA tree
    recordEditor->pushChangesToGuiModel();
    guiModel->save();

    // check to see if we need to reload the project after saving due to potential hash mismatch (c.f. Git Issue #118)
    if(change)
    {
        //fix to Bug #1521
        parseLSAFileErrorsAndWarnings(true);
        reloadProjectAfterSave();
    }
    else
    {
        QModelIndex rootIndex = guiModel->index(0,0);
        guiModel->clearDescendantIncludeIsModified(rootIndex);

        //generate map of unique IDs for measurement records to corresponding LSARecord pointers
        generateUniqueIDMapToLSARecordPointers();

        //generate the hash code for the current data
        currentHashCode = guiModel->hashTree();

        //fix to Bug #1521
        parseLSAFileErrorsAndWarnings(true);
    }
}

void MainWindow::saveAsProject(QString newDirPath)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    if (newDirPath.isEmpty())
    {
        // Determine the current project directory and parent directory
        QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();
        QDir defaultDir = QDir(lastLSAPath);
        QDir parentDir = QDir(lastLSAPath);
        parentDir.cdUp();

        // Configure a directory browser dialog
        QString caption("Specify a new or existing directory");
        QString filter("");
        QString startingDir = parentDir.absolutePath();
        QFileDialog fileDialog(this, caption, startingDir, filter);
        fileDialog.setFileMode(QFileDialog::Directory);
        fileDialog.setOption(QFileDialog::ShowDirsOnly);
        fileDialog.selectFile(defaultDir.absolutePath());

        // Push the dialog to the user and verify they created and/or selected a directory
        if (fileDialog.exec())
            newDirPath = fileDialog.selectedFiles().first();
        else
            return;

        if (newDirPath.isEmpty())
            return;
    }

    QString oldDirName = QString::fromStdString(guiModel->getProjectDirectory());

    // If the user chose the current directory, perform a regular save
    if (oldDirName == newDirPath)
    {
        saveProject();
        return;
    }

    // Display a warning if chosen directory is not empty

    QDir newDir(newDirPath);
    if (newDir.count() > 2)
    {

        QString newProjFileName = newDir.dirName() + ".proj";
        QString message;
        if (newDir.exists(newProjFileName))
        {
            message = "Warning - this directory already contains a Salsa project. <b>";
            message += newProjFileName + " and associated files will be overwritten.</b>  Continue?";
        }
        else
        {
            message = "Files inside of this directory may be overwritten. Continue?";
        }

        QMessageBox::StandardButton buttonClicked = QMessageBox::question(this, "", message, QMessageBox::Cancel | QMessageBox::Ok);

        // If the user clicked cancel, abort the save as process
        if (buttonClicked == QMessageBox::Cancel)
        {
            return;
        }
    }

    // Check if the directory to be copied is > 100MB or has > 100 files
    int numFiles = getNumFiles(oldDirName);
    qint64 totalDirSize_bytes = recursiveDirSize_bytes(oldDirName);
    double totalDirSize_mb = totalDirSize_bytes/1000.0/1000.0; // convert B to MB
    if (numFiles > 100 || totalDirSize_mb > 100)
    {
        QMessageBox::StandardButton buttonClicked;

        QString formatedDirSize = QString::number(totalDirSize_mb, 'f', 2);
        QString message = QString("Warning: Source project directory is very large (%1 files, %2 MB)! "
                     "\nProceed with copying current project to destination?").arg(numFiles).arg(formatedDirSize);
        buttonClicked = QMessageBox::question(this,
                         "", message,
                         QMessageBox::Cancel | QMessageBox::Ok);

        // If the user clicked cancel, abort the save as process
        if (buttonClicked == QMessageBox::Cancel)
        {
            return;
        }
    }

    // Check that the new directory exists.  If it doesn't, create it.
    if (!newDir.exists())
    {
        QString name = newDir.dirName();
        QDir parentDir(newDirPath);
        parentDir.cdUp();
        parentDir.mkdir(name);
    }

    // Recursively copy the project directory to the new location
    bool success = recursiveCopydir(oldDirName, newDirPath);
    if (!success)
    {
        QString message("Warning - Save As operation failed.  Could not copy project directory to new location.");
        pushWarning(message);
        return;
    }

    // Rename some files
    QModelIndex rootIndex = guiModel->getNextIndex();
    LSAInclude *rootInclude = static_cast<LSAInclude*>(guiModel->getLSARecord(rootIndex));
    QString projFilePath = QString::fromStdString(rootInclude->getAbsolutePath());
    QFileInfo projFileInfo(projFilePath);

    QString oldBaseName = projFileInfo.completeBaseName();
    QString newBaseName = newDir.dirName();

    QString absPath = newDir.canonicalPath();
    QStringList extensions;
    extensions << "proj" << "bin" << "cfg" << "csv" << "pts" << "dat" << "h5" << "out" << "post";

    QString newProjFilePath;
    foreach(QString extension, extensions)
    {
        QString oldFileName = absPath + "/" + oldBaseName + "." + extension;
        QString newFileName = absPath + "/" + newBaseName + "." + extension;
        renameFile(oldFileName, newFileName);

        if (extension == "proj")
        {
            // Save the absolute path to the root include for use once everything is renamed
            newProjFilePath = newFileName;
        }
    }

    guiModel->changeProjectDirectory(newDirPath, oldDirName);
    rootInclude->setAbsolutePath(newProjFilePath.toStdString());

    settings.setValue(lsa::QSETTINGS_LASTPATH, newDirPath);

    saveProject();
    if(guiModel->isFileVersionCurrent(std::string("1.10.0")))//if not, we will have already reloaded in saveProject()
        reloadProjectAfterSave();

    // Delete some files we couldn't previously delete because they were open in the salsa gui
    QString oldProjFileName = absPath + "/" + oldBaseName + ".proj";
    QFile oldProjFile(oldProjFileName);
    oldProjFile.remove();

    QString oldCfgFileName = absPath + "/" + oldBaseName + ".cfg";
    QFile oldCfgFile(oldCfgFileName);
    oldCfgFile.remove();
}

void MainWindow::exportMap()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();

    QString fileName = QFileDialog::getSaveFileName(this,
                                            tr("Save a screenshot of the map"),
                                            lastLSAPath,
                                            tr(".png (*.png);;.jpg (*.jpg);;.jpeg (*.jpeg)"));
//    qDebug() << fileName;
    if (!fileName.isEmpty()) {
        if(!fileName.endsWith(".png") && !fileName.endsWith(".jpg") && !fileName.endsWith(".jpeg"))
        {
            fileName.append(".png");
        }
        QPixmap image = mapWidget->mapScreenShot();
        image.save(fileName, 0, 90); // 90 is the image quality out of 100

        QString fileNameOutput;

        fileNameOutput = getFileNameFromFullPath(fileName);

        QString status_message = "Successfully exported the map as " + fileNameOutput + " to " + lastLSAPath + ".";
        pushStatus(status_message);
    }
//    qDebug() << fileName;
}

void MainWindow::exportKML()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    QString defaultPath = currentFile.left(currentFile.lastIndexOf(".")).append(".kmz");

    QString selfilter = tr("KMZ files (*.kmz)");
    QString fileName = QFileDialog::getSaveFileName(this,
                                            tr("Export map as a kmz file"),
                                            defaultPath,
                                            tr("KMZ files (*.kmz);;All files (*)"),
                                            &selfilter);

    if (!fileName.isEmpty()) {

        // QFileDialog::getSaveFileName only appends the file extension on Windows if the user does
        // not include it in the file name. On Linux, if the file name "test" if chosen and there already
        // exists a "test.kmz", getSaveFileName will not prompt the user of a possible overwrite.
        // Need to manually check if the file with the kmz extension already exists.
        if(!fileName.endsWith(".kmz"))
        {
            fileName.append(".kmz");

            if (QFile(fileName).exists())
            {

                QMessageBox::StandardButton buttonClicked;
                QString message = fileName + " already exists. Proceed to overwrite?";
                buttonClicked = QMessageBox::question(this,
                                     "", message,
                                     QMessageBox::Cancel | QMessageBox::Yes);

                if (buttonClicked == QMessageBox::Cancel) return;
            }

        }
        mapWidget->exportKML(fileName, adjustedPositions);

        QString fileNameOutput;
        fileNameOutput = getFileNameFromFullPath(fileName);

        QString pathOutput;
        pathOutput = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();

        QString status_message = "Successfully exported the map as " + fileNameOutput + " to " + pathOutput + ".";
        pushStatus(status_message);
    }
}

/**
 * @brief MainWindow::quitApplication The user has quit the application, confirm
 * changes are saved, and save off window geometry.
 * @return
 */
bool MainWindow::quitApplication()
{
    // Give the user a chance to save changes before closing the current project
    if(guiModel->projectIsModified())
    {
        if (confirmChangesAreSaved() == QMessageBox::Cancel)
            return false;
    }

    saveWindowSettings();

    qApp->quit();
    return true;
}

void MainWindow::handleCopyRecord()
{
    //clear old buffer
    foreach(LSARecord* lsaRecord, lsaRecordBuffer)
    {
        delete lsaRecord;
    }
    lsaRecordBuffer.clear();

    QString newRecordStr;
    QStringList clipList;
    QModelIndexList selected = guiModel->getSelectedRows();
    foreach(QModelIndex index, selected)
    {
        //copy record to lsaRecordBuffer
        LSARecord* lsaRecord = guiModel->getLSARecord(index);
        LSARecord* newRecord = copyOfLSARecord(lsaRecord);
        lsaRecordBuffer.push_back(newRecord);

        //copy plainText to clipboard
        newRecordStr = QString::fromStdString(newRecord->getSingleLSAString());
        clipList << newRecordStr;
    }

    // Check the tree view is in focus
    QWidget* focusWidget = QApplication::focusWidget();
    bool findWidgetBtnFocus = ui->findFilterWidget->findWidgetButtonClicked();

    if(focusWidget == ui->treeView || isTestHarness() || findWidgetBtnFocus)
    {
        QString recordListTxt = clipList.join("\n");

        // System Clipboard
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->clear();
        clipboard->setText(recordListTxt);
    }
}

void MainWindow::handleCutRecord()
{
    guiModel->undoStack()->beginMacro("Cutting records");

    handleCopyRecord();

    QModelIndexList selectedIndices = guiModel->getSelectedRows();

    deleteCutRecordsList(selectedIndices);

    guiModel->undoStack()->endMacro();
}

void MainWindow::handlePasteRecord()
{
    guiModel->undoStack()->beginMacro("Pasting records");
    DebugTimer timer(false);

    // Convert LSARecordBuffer into a single string (clipboard related)
    QStringList lsaRecBufStr;
    QString recordString;
    foreach(LSARecord* lsaRecord, lsaRecordBuffer)
    {
        recordString = QString::fromStdString(lsaRecord->getSingleLSAString());
        lsaRecBufStr << recordString;
    }
    QString recordListTxt = lsaRecBufStr.join("\n");
    LOG_TIME(timer,"Get list of lsa strings")

    // If the system clipboard and LSARecordBuffer does not match abort paste handler
    // We want the users to think only one clipboard exist
    QClipboard *clipboard = QApplication::clipboard();
    bool clipboardChanged = recordListTxt != clipboard->text();
    if(clipboardChanged)
    {
        lsaRecordBuffer.clear();
        guiModel->undoStack()->endMacro();
        return;
    }
    LOG_TIME(timer,"Clear lsaRecordBuffer")

    QWidget* focusWidget = QApplication::focusWidget();
    bool findWidgetBtnFocus = ui->findFilterWidget->findWidgetButtonClicked();
    if(focusWidget == ui->treeView || isTestHarness() || findWidgetBtnFocus)
    {
        //determine where the user wants to paste
        QModelIndex index = guiModel->getLastSelectedRecord();
        int rowPosition  = index.row()+1;

        // Don't attempt to insert if index is invalid
        if (!index.isValid())
        {
            guiModel->undoStack()->endMacro();
            return;
        }

        QModelIndex parentIndex = guiModel->parent(index);

        if(guiModel->rootIncludeIsSelected())
        {
            rowPosition = 0;
        }


        LOG_TIME(timer,"Perpare to insert record")
        //insert records to be pasted
        guiModel->insertNewRecords(parentIndex, rowPosition, lsaRecordBuffer);


        LOG_TIME(timer,"Insert the record")
    }

    guiModel->undoStack()->endMacro();
}

void MainWindow::deleteCutRecordsList(QModelIndexList cutIndices)
{
    QList<QPersistentModelIndex> persistentIndicesToDelete;
    foreach(QModelIndex index, cutIndices)
    {
        persistentIndicesToDelete.push_back(index);
    }

    QVector<QVector<IndexRowColumnParentChainItem>> removedIndicesChain = guiModel->buildMultipleIndicesChain(persistentIndicesToDelete);

    foreach(QPersistentModelIndex persitentIndex, persistentIndicesToDelete)
    {
        if (persitentIndex.isValid())
        {
            QModelIndex currentIndex = guiModel->index(persitentIndex.row(), persitentIndex.column(), persitentIndex.parent());
            deleteRecord(currentIndex, removedIndicesChain);
        }
    }
}

void MainWindow::handleDuplicateRecord()
{
    QVector <LSARecord*> savedBuffer;
    QString savedClipboard;

    //save what is currently in the buffer
    foreach(LSARecord* lsaRecord, lsaRecordBuffer)
    {
        savedBuffer.push_back(copyOfLSARecord(lsaRecord));
    }

    //save current system clipboard
    QClipboard *clipboard = QApplication::clipboard();
    savedClipboard = clipboard->text();

    //copy the records that are currently selected
    handleCopyRecord();

    //paste below the last index selected
    handlePasteRecord();

    //restore what was originally in the buffer & the clipboard
    foreach(LSARecord* lsaRecord, lsaRecordBuffer)   delete lsaRecord;
    lsaRecordBuffer.clear();

    lsaRecordBuffer = savedBuffer;
    clipboard->setText(savedClipboard);
}

void MainWindow::handleCustomImportActionTriggered()
{
    //get the action sender of triggered signal
    QAction* action = qobject_cast<QAction*>(sender());
    customImportAction(action);

    // prompt user to select files to convert
    getFilesToAddToProject(lsa::CONVERTER_TYPE_CUSTOM);
}

//splitting this up from handleCustomImportActionTriggered before testharness
void MainWindow::customImportAction(QAction* action)
{
    currentImportAction = action;
}

QString MainWindow::convertCustomImport(QString filename)
{
    //run script
    QString pythonScript = currentImportAction->data().toString();
    QString scriptName = currentImportAction->text();

    QStringList arguments;
    arguments << QDir::cleanPath(pythonScript);

    QString instrFilename = QDir::cleanPath(filename);
    QString newOutFilename = instrFilename + ".lsa";
    QString wrnFilePath = instrFilename + ".wrn";
    QString cfgFilePath = QString::fromStdString(getPathWithoutFileName(currentFile.toStdString())) + QString("/converter.cfg");
    QString cvtFilePath = converterPath;

    arguments << "--in" << instrFilename
              << "--out" << newOutFilename
              << "--cfgfile" << cfgFilePath
              << "--wrnfile" << wrnFilePath
              << "--cvtpy" << cvtFilePath;

    //create or update the converter.cfg file
    createConverterConfigFile(cfgFilePath);

//    if(runProcess(python3Path, arguments))
//    {
//        //output status that the script is running
//        QStringList processed = arguments;
//    }

    procType = CUSTOM_CONVERTER_SCRIPT;

    if(python3Path.isEmpty())
        pushError(QString("Error - unable to find python.exe in SALSA installation folder.  Unable to run custom python scripts."));
    else
    {
        pushBreakerStart(QString("Import Include using " + scriptName + " script."));
        pushProcessStart(scriptName.toStdString() + ".py");
        if(runProcess(python3Path,arguments))
        {
            QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
            int CLITimeoutMilliseconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt()*1000;
            if(!proc->waitForFinished(CLITimeoutMilliseconds))
                cleanupProcess(QString(scriptName + "export script"));
        }
        else
            pushError(QString("Error - unable run " + scriptName + " export script."));
    }

    return newOutFilename;
}

void MainWindow::handleCustomExportActionTriggered()
{
    //get the action sender of triggered signal
    QAction* action = qobject_cast<QAction*>(sender());

    customExportAction(action);
}

//helper method for handleCustomExportActionTriggered()
void MainWindow::customExportAction(QAction* action, QStringList additionalArguments)
{
    QString pythonScript = action->data().toString();
    QString scriptName = action->text();

    QString hdf5FilePath = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QStringList arguments;
    arguments << QDir::cleanPath(pythonScript);
    arguments << "--in" << QDir::cleanPath(hdf5FilePath) << "--cgeoid" << QDir::cleanPath(cgeoidPath);

    if(isGeoidFileUsed())
    {
        QString geoidFile2 = QString::fromStdString(getExecutablePath() + string("/../data/geoid/egm1996_2.5m.und"));
        geoidFile2 = QDir::cleanPath(geoidFile2);
        arguments << "--geo" << geoidFile2;
    }

    // Append additional arguments
    arguments += additionalArguments;

    procType = CUSTOM_REPORTING_SCRIPT;

    if(python3Path.isEmpty())
        pushError(QString("Error - unable to find python.exe in SALSA installation folder.  Unable to run custom python scripts."));
    else
    {
        pushBreakerStart(QString("Exporting custom " + scriptName + " script."));
        pushProcessStart(scriptName.toStdString() + ".py");
        if(runProcess(python3Path,arguments))
        {
            QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
            int CLITimeoutMilliseconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt()*1000;
            if(!proc->waitForFinished(CLITimeoutMilliseconds))
                cleanupProcess(QString(scriptName + "export script"));
        }
        else
            pushError(QString("Error - unable run " + scriptName + " export script."));
    }
}

void MainWindow::cleanCustomActions(QMenu* menu)
{
    QList<QAction*> customAction = menu->actions();
    bool customScriptsBegin = false;
    foreach(QAction* item, customAction)
    {
        if(item->text() == "Custom Scripts")
        {
            customScriptsBegin = true;
        }
        if(customScriptsBegin)
        {
            menu->removeAction(item);
            item->deleteLater();
        }
    }
}

LSARecord* MainWindow::copyOfLSARecord(LSARecord *lsaRecord)
{
    LSAType recType = lsaRecord -> getRecType();
    LSARecord* newRecord = NULL;

    switch(recType.getLSAtype())
    {
    case LSAType::POSG:
    {
       LSAPosG* oldPOSG = dynamic_cast<LSAPosG*>(lsaRecord);
       LSAPosG* newPOSG = new LSAPosG(*oldPOSG);
       newRecord = dynamic_cast<LSARecord*>(newPOSG);
       break;
    }
    case LSAType::POSC:
    {
       LSAPosC* oldPOSC = dynamic_cast<LSAPosC*>(lsaRecord);
       LSAPosC* newPOSC = new LSAPosC(*oldPOSC);
       newRecord = dynamic_cast<LSARecord*>(newPOSC);
       break;
    }
    case LSAType::DXYZ:
    {
       LSADelta* oldDXYZ = dynamic_cast<LSADelta*>(lsaRecord);
       LSADelta* newDXYZ = new LSADelta(*oldDXYZ);
       newRecord = dynamic_cast<LSARecord*>(newDXYZ);
       break;
    }
    case LSAType::DIST:
    {
       LSADist* oldDIST = dynamic_cast<LSADist*>(lsaRecord);
       LSADist* newDIST = new LSADist(*oldDIST);
       newRecord = dynamic_cast<LSARecord*>(newDIST);
       break;
    }
    case LSAType::HANG:
    {
       LSAHAngle* oldHANG = dynamic_cast<LSAHAngle*>(lsaRecord);
       LSAHAngle* newHANG = new LSAHAngle(*oldHANG);
       newRecord = dynamic_cast<LSARecord*>(newHANG);
       break;
    }
    case LSAType::VANG:
    {
       LSAVAngle* oldVANG = dynamic_cast<LSAVAngle*>(lsaRecord);
       LSAVAngle* newVANG = new LSAVAngle(*oldVANG);
       newRecord = dynamic_cast<LSARecord*>(newVANG);
       break;
    }
    case LSAType::ZANG:
    {
       LSAZAngle* oldZANG = dynamic_cast<LSAZAngle*>(lsaRecord);
       LSAZAngle* newZANG = new LSAZAngle(*oldZANG);
       newRecord = dynamic_cast<LSARecord*>(newZANG);
       break;
    }
    case LSAType::AZIM:
    {
       LSAAzimuth* oldAZIM = dynamic_cast<LSAAzimuth*>(lsaRecord);
       LSAAzimuth* newAZIM = new LSAAzimuth(*oldAZIM);
       newRecord = dynamic_cast<LSARecord*>(newAZIM);
       break;
    }
    case LSAType::HDIF:
    {
       LSAHeightDiff* oldHDIF = dynamic_cast<LSAHeightDiff*>(lsaRecord);
       LSAHeightDiff* newHDIF = new LSAHeightDiff(*oldHDIF);
       newRecord = dynamic_cast<LSARecord*>(newHDIF);
       break;
    }
    case LSAType::DGRP:
    {
       LSADirGroup* oldDGRP = dynamic_cast<LSADirGroup*>(lsaRecord);
       LSADirGroup* newDGRP = new LSADirGroup(*oldDGRP);
       newRecord = dynamic_cast<LSARecord*>(newDGRP);
       break;
    }
    case LSAType::HDIR:
    {
       LSAHDir* oldHDIR = dynamic_cast<LSAHDir*>(lsaRecord);
       LSAHDir* newHDIR = new LSAHDir(*oldHDIR);
       newRecord = dynamic_cast<LSARecord*>(newHDIR);
       break;
    }
    case LSAType::UNCR:
    {
       LSAUncertainty* oldUNCR = dynamic_cast<LSAUncertainty*>(lsaRecord);
       LSAUncertainty* newUNCR = new LSAUncertainty(*oldUNCR);
       newRecord = dynamic_cast<LSARecord*>(newUNCR);
       break;
    }
    case LSAType::VSCA:
    {
       LSAVarScaling* oldVSCA = dynamic_cast<LSAVarScaling*>(lsaRecord);
       LSAVarScaling* newVSCA = new LSAVarScaling(*oldVSCA);
       newRecord = dynamic_cast<LSARecord*>(newVSCA);
       break;
    }
    case LSAType::HGHT:
    {
       LSAHeight* oldHGHT = dynamic_cast<LSAHeight*>(lsaRecord);
       LSAHeight* newHGHT = new LSAHeight(*oldHGHT);
       newRecord = dynamic_cast<LSARecord*>(newHGHT);
       break;
    }
    case LSAType::MEAN:
    {
       LSAMean* oldMEAN = dynamic_cast<LSAMean*>(lsaRecord);
       LSAMean* newMEAN = new LSAMean(*oldMEAN);
       newRecord = dynamic_cast<LSARecord*>(newMEAN);
       break;
    }
    case LSAType::ENUO:
    {
       LSAEnuo* oldENUO = dynamic_cast<LSAEnuo*>(lsaRecord);
       LSAEnuo* newENUO = new LSAEnuo(*oldENUO);
       newRecord = dynamic_cast<LSARecord*>(newENUO);
       break;
    }
    case LSAType::COMMENT:
    {
       LSAComment* oldCOMMENT = dynamic_cast<LSAComment*>(lsaRecord);
       LSAComment* newCOMMENT = new LSAComment(*oldCOMMENT);
       newRecord = dynamic_cast<LSARecord*>(newCOMMENT);
       break;
    }
    }//end switch

    return newRecord;
}

void MainWindow::handleItemSelectionChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
    handleItemSelectionChanged();
}

void MainWindow::handleItemSelectionChanged(QItemSelection, QItemSelection)
{
    handleItemSelectionChanged();
}

bool MainWindow::isPPP(LSARecord *lsaRecord)
{
    bool isPPP=false;

    LSAType lsaType = lsaRecord->getRecType();
    if(lsaType.isPosition())
    {
        if(lsaType == LSAType::POSC)
        {
            LSAPosC *lsaPosc = static_cast<LSAPosC*>(lsaRecord);
            if(lsaPosc->hasValidCovariance() && lsaPosc->fixedState == LSAFixedState::CONSTRAINED)
                isPPP = true;
        }
        else if(lsaType == LSAType::POSG)
        {
            LSAPosG *lsaPosg = static_cast<LSAPosG*>(lsaRecord);
            if(lsaPosg->hasValidCovariance() && lsaPosg->fixedState == LSAFixedState::CONSTRAINED)
                isPPP = true;
        }
    }
    return isPPP;
}

void MainWindow::handleItemSelectionChanged()
{
    guiModel->enableSelectedRowsCache(true);
    QModelIndexList selectedPoints = getSelectedPoints();
    QModelIndexList selectedMeasurements = getSelectedMeasurements();
    QModelIndex topIndex = findTopSelectedIndex();

    selectOutputTablesRows(selectedPoints,selectedMeasurements);
    adjustedPositions->updateSelection(selectedPoints);

    //added to allow scrolling to tree view record when selecting a point/measurement in the map
    QWidget *focusWidget = QApplication::focusWidget();
    if ( focusWidget == dynamic_cast<QWidget*>(ui->treeView) || focusWidget == dynamic_cast<QWidget*>(ui->findFilterWidget))
    {
        // this is a hack to prevent the horizontal scroll bar from scrolling right when the user clicks
        // column 4 or 5. Ideally, we would configure the tree view to only auto-scroll vertically, but
        // I dont see an easy way to do that.
        QModelIndex currentIndex = guiModel->selectionModel->currentIndex();
        QModelIndex columnZeroIndex = guiModel->index(currentIndex.row(), 0, currentIndex.parent());

        // Scroll the widget
        ui->treeView->scrollTo(columnZeroIndex, QAbstractItemView::EnsureVisible);
    }
    else if( focusWidget != dynamic_cast<QWidget*>(ui->tablePoints) )
    {
        ui->treeView->scrollTo(topIndex,QAbstractItemView::PositionAtCenter);
    }

    // Make sure the correct menu actions are enabled and disabled
    enableDisableMenuActions();

    // Update widgets
    updateWidgets();
    guiModel->enableSelectedRowsCache(false);
}

void MainWindow::handleFindWidgetClose(QWidget *signaledWidget)
{
    // Fix i#1431 Scroll to the last selected record when closing the find widget
    QModelIndex temp = guiModel->getLastSelectedRecord();
    if(temp.isValid())
    {
        ui->treeView->scrollTo(temp);
    }

    signaledWidget->setFocus();
}

void MainWindow::enableDisableMenuActions()
{

    // Get the selected record(s) from the guiModel
    QModelIndexList selectedRows = guiModel->getSelectedRows();
    int numSelectedRows = selectedRows.size();
    bool projectIsLoaded = guiModel->projectIsLoaded();
    bool rootIncludeSelected = guiModel->rootIncludeIsSelected();
    bool zeroRowsSelected = selectedRows.size() == 0;
    bool singleRowSelected = selectedRows.size() == 1;
    bool autogeneratedRowIsSelected = guiModel->hasAutogeneratedRow();

    // Project Menu
    ui->actionClose->setEnabled(projectIsLoaded);
    ui->actionNewGeoidFile -> setEnabled(projectIsLoaded);
    ui->actionGenerate_Initial_Positions->setEnabled(projectIsLoaded);
    ui->actionCalculate_Adjustment->setEnabled(projectIsLoaded);
    ui->actionView_Adjusted_Coordinates->setEnabled(projectIsLoaded);
    ui->actionSave->setEnabled(projectIsLoaded);
    ui->actionSaveAs->setEnabled(projectIsLoaded);

    // Record Menu
    enableDisableCopyPaste();

    ui->actionExpand_All->setEnabled(projectIsLoaded);
    ui->actionCollapse_All->setEnabled(projectIsLoaded);
    ui->actionDelete->setDisabled(zeroRowsSelected);
    ui->actionSelect_Referencing->setDisabled(zeroRowsSelected);
    ui->actionEnable->setDisabled(zeroRowsSelected);
    ui->actionDisable->setDisabled(zeroRowsSelected);
    ui->actionMove_Up->setDisabled(zeroRowsSelected);
    ui->actionMove_Down->setDisabled(zeroRowsSelected);

    // Record->Insert Menu
    ui->menuInsert->setToolTipsVisible(singleRowSelected && !autogeneratedRowIsSelected);
    ui->menuInsert->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertPOSG->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertPOSC->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertDIST->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertDXYZ->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertHANG->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertAZIM->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertVANG->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertZANG->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertHDIF->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertHDIR->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertDGRP->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertHGHT->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertVSCA->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertUNCR->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertComment->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertSeparator->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertNEWInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);

    if (numSelectedRows > 0)
    {
        // Don't allow the rootInclude record to be deleted
        ui->actionDelete->setDisabled(rootIncludeSelected);

        // Enable select referencing when a single record is selected
        ui->actionSelect_Referencing->setEnabled(numSelectedRows == 1);

        // Don't allow the rootInclude record enabled or disabled
        ui->actionEnable->setDisabled(rootIncludeSelected);
        ui->actionDisable->setDisabled(rootIncludeSelected);

        //disable move up/down to keep records within their include file (may change later)
        ui->actionMove_Up->setDisabled(true);
        ui->actionMove_Down->setDisabled(true);
        if (numSelectedRows == 1 && !rootIncludeSelected)
        {
            QModelIndex selectedIndex = selectedRows.at(0);
            QModelIndex parentIndex = guiModel->parent(selectedIndex);
            if(parentIndex.isValid())
            {
                int rowNum = selectedIndex.row();
                LSAInclude *parentInclude = static_cast<LSAInclude*>(guiModel->getLSARecord(parentIndex));
                int numChildren = parentInclude->childRecords.size();

                //disable move up/down if Auto-generated Points file or point is selected
                if(!guiModel->getLSARecord(selectedIndex)->isAutogenerated)
                {
                    ui->actionMove_Up->setEnabled(rowNum != 0);
                    ui->actionMove_Down->setEnabled(rowNum != numChildren - 1);

                    //TODO: Handle case where intervening records between top/bottom are blank comments
                }
            }
        }
    }

    // Record->Insert Derived
    ui->menuInsert_Derived_Position->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertMEAN->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertENUO->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);

    bool singlePositionOrDerivedSelected = false;
    if (numSelectedRows == 1)
    {
        QModelIndex index = selectedRows.first();
        if (index.isValid())
        {
            LSAType lsaType = guiModel->getLSARecord(index)->getRecType();
            singlePositionOrDerivedSelected = lsaType.isPosition() || lsaType.isPostProcessed();
        }
    }
    ui->actionFilterPoint->setEnabled(singlePositionOrDerivedSelected);

    // Import menu
    ui->menuImport->setToolTipsVisible(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertLSAInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertIOBInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertGSIInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertPPPInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertTrimbleGPSInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertLeicaGNSSInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertSetsOfAnglesInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);
    ui->actionInsertOPUSInclude->setEnabled(singleRowSelected && !autogeneratedRowIsSelected);

    enableDisableCustomActions(ui->menuImport, singleRowSelected && !autogeneratedRowIsSelected);

    // Export menu
    ui->actionAdjustedLSA->setEnabled(projectIsLoaded);
    ui->actionGeoidHeightsAPriori->setEnabled(projectIsLoaded);
    ui->actionExportMap->setEnabled(projectIsLoaded);
    ui->actionExport_KML->setEnabled(projectIsLoaded);
    ui->actionExportPointsXYZ->setEnabled(projectIsLoaded);
    ui->actionExportSolnCov->setEnabled(projectIsLoaded);
    ui->actionExportMeasResid->setEnabled(projectIsLoaded);
    ui->actionExportMeasResidAsENU->setEnabled(projectIsLoaded);
    ui->actionExportPointsUTM->setEnabled(projectIsLoaded);
    ui->actionExportSPC->setEnabled(projectIsLoaded);
    ui->actionExportPointsNCAT->setEnabled(projectIsLoaded);

    enableDisableCustomActions(ui->menuExport, projectIsLoaded);
    showHideAddAutogen();

    // View menu
    ui->actionView_Solver_Output->setEnabled(projectIsLoaded);
    ui->actionView_Points_File->setEnabled(projectIsLoaded);
}

void MainWindow::addCustomMenu()
{
    //clear old custom export options
    cleanCustomActions(ui->menuExport);
    cleanCustomActions(ui->menuImport);

    QString customExportPath = QString::fromStdString(guiModel->getProjectDirectory() + "/customExport");
    QDir exportDir(customExportPath);

    QString customImportPath = QString::fromStdString(guiModel->getProjectDirectory() + "/customImport");
    QDir importDir(customImportPath);

    // check project for customExport scripts
    if(exportDir.exists())
    {
        ui->menuExport->addSection("Custom Scripts");
        QDirIterator it(QString::fromStdString(guiModel->getProjectDirectory() + "/customExport"), QStringList() << "*.py", QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext())
        {
            it.next();

            QString scriptName = it.fileName();
            QString actionName = scriptName.left(scriptName.lastIndexOf("."));
            QAction* newAction = new QAction(actionName, ui->menuExport);
            newAction->setData(it.filePath());

            ui->menuExport->addAction(newAction);

            connect(newAction, SIGNAL(triggered()), this, SLOT(handleCustomExportActionTriggered()));

        }
    }

    // check project for customImport scripts
    if(importDir.exists())
    {
        ui->menuImport->addSection("Custom Scripts");
        QDirIterator it(QString::fromStdString(guiModel->getProjectDirectory() + "/customImport"), QStringList() << "*.py", QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext())
        {
            it.next();

            QString scriptName = it.fileName();
            QString actionName = scriptName.left(scriptName.lastIndexOf("."));
            QAction* newAction = new QAction(actionName, ui->menuImport);
            newAction->setData(it.filePath());

            ui->menuImport->addAction(newAction);

            connect(newAction, SIGNAL(triggered()), this, SLOT(handleCustomImportActionTriggered()));

        }

    }
}

void MainWindow::enableDisableCustomActions(QMenu *menu, bool singleRowSelected)
{
    QList<QAction*> customActions = menu->actions();
    bool customScriptsBegin = false;
    foreach(QAction* item, customActions)
    {
        QString itemName = item->text();
        if(itemName == "Custom Scripts")
        {
            customScriptsBegin = true;
            continue;
        }
        if(customScriptsBegin)
        {
            item->setEnabled(singleRowSelected);
        }
    }
}

void MainWindow::enableDisableCopyPaste()
{
    // Treeview has focus || findWidget
    QWidget* focusWidget = QApplication::focusWidget();
    bool treeViewHasFocus = focusWidget == ui->treeView;
    bool findWidgetBtnFocus = ui->findFilterWidget->findWidgetButtonClicked();

    if(treeViewHasFocus || isTestHarness() || findWidgetBtnFocus) {
        // Get the selected record(s) from the guiModel
        bool zeroRowsSelected = guiModel->getSelectedRows().size() == 0;
        bool onlyOneSelected = guiModel->getSelectedRows().size() == 1;
        bool includeRowIsSelected = guiModel->includeRowIsSelected();
        bool autogeneratedRowIsSelected = guiModel->hasAutogeneratedRow();

        // Record Menu
        if(!zeroRowsSelected && !includeRowIsSelected && !autogeneratedRowIsSelected) {ui->RecordMenu->setToolTipsVisible(true);}
        ui->actionCopy->setEnabled(!zeroRowsSelected && !includeRowIsSelected && !autogeneratedRowIsSelected);
        ui->actionCut->setEnabled(!zeroRowsSelected && !includeRowIsSelected && !autogeneratedRowIsSelected);
        ui->actionPaste->setEnabled(!zeroRowsSelected && !autogeneratedRowIsSelected && !lsaRecordBuffer.isEmpty() && onlyOneSelected);
        ui->actionDuplicate->setEnabled(!zeroRowsSelected && !includeRowIsSelected && !autogeneratedRowIsSelected);
    }
    else{
        ui->actionCopy->setDisabled(true);
        ui->actionCut->setDisabled(true);
        ui->actionPaste->setDisabled(true);
        ui->actionDuplicate->setDisabled(true);
        ui->RecordMenu->setToolTipsVisible(false);
    }
}

void MainWindow::showHideAddAutogen()
{
    if(QApplication::focusWidget() != ui->treeView && !isTestHarness())
    {
        ui->actionAdd_Autogen->setVisible(false);
    }

    else
    {
        ui->actionAdd_Autogen->setVisible(guiModel->selectionIsAllAutogen());
    }
}

void MainWindow::calculateAdjustment(PROCESS_TYPES runType)
{
    // Require the user to save changes before proceeding with the adjustment
    QString projChangedMsg = "All changes must be saved before calculating adjustment.\nSave changes?";

    // change message if runType is generate initial position
    if(runType == PROCESS_LSASOLVER_APQUIT)
    {
        projChangedMsg = "All changes must be saved before generating initial position. \nSave changes?";
    }
    if(runType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
    {
        projChangedMsg = "All changes must be saved before exporting Orthometric Heights at pre-adjusted-points. \nSave changes?";
    }

    QMessageBox::StandardButton userReply = confirmChangesAreSaved(projChangedMsg);
    if ( userReply == QMessageBox::Cancel)
    {
        return;
    }

    if(!currentFile.isEmpty())
    {
        if(runType == PROCESS_LSASOLVER_APQUIT)
        {
            pushBreakerStart(QString("Generate Initial Positions"));
        }
        else if(runType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
        {
            pushBreakerStart(QString("Export Geoid Heights at Pre-adjusted Points"));
        }
        else
        {
            pushBreakerStart(QString("Calculate Adjustment"));
            startProgressBar();//run progress bar only if calculating adjustment
        }


        // run the preprocessor
        QString datFileName;
        if(!runPreprocessor(datFileName))
        {
            cleanupProcess(QString("lsapreprocessor"));
            return;
        }

        if(runType != PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
        {
            clearOutputTab();
        }

        // run the solver
        QString outFileName;
        QString binFileName;
        if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
        {
            if(runType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
            {
                binFileName = currentFile + QString(".apb");
                outFileName = currentFile + QString(".apo");
            }
            else
            {
                binFileName = currentFile + QString(".bin");
                outFileName = currentFile + QString(".out");
            }
            if(configFileName.isEmpty())//for gui tests - never started a new project or imported an iob project
                configFileName = currentFile + QString(".cfg");
        }
        else
        {
            if(runType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
            {
                binFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".apb");
                outFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".apo");
            }
            else
            {
                binFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".bin");
                outFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".out");
            }
            if(configFileName.isEmpty())//for gui tests - never started a new project or imported an iob project
                configFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".cfg");
        }

        QStringList arguments;
        QString geoidPath;
        if (isProjDirGeoid()) //custom geoid file in project dir specified
        {
            geoidPath = QString::fromStdString(getPathWithoutFileName(currentFile.toStdString())+"/geoid/");
        }
        else
        {
            geoidPath = QString::fromStdString(getExecutablePath() + string("/../data/geoid"));
        }
        arguments << "--input" << datFileName
                  << "--out" << outFileName
                  << "--bin" << binFileName
                  << "--file" << configFileName
                  << "--progress";
        if((runType == PROCESS_LSASOLVER_APQUIT) || (runType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS))
        {
            arguments << "--apquit";
        }
        if(isGeoidFileUsed())
        {
            arguments << "--geoidpath" << geoidPath;
        }
        if(runType == PROCESS_LSASOLVER_FORCE_STABLE)
        {
            arguments << "--forcestable";
        }

        //store file creation time to only parse warning and errors from current .out files
        QFileInfo infoAboutOutFile(outFileName);
        if(infoAboutOutFile.exists())
#ifdef _WIN32
            lastOutCreationTime = windowsGetLastModifiedTime(outFileName);
        else
        {
            FILETIME firstTime;
            firstTime.dwHighDateTime = 0;
            firstTime.dwLowDateTime = 0;
            lastOutCreationTime = firstTime;
        }
#else
            lastOutCreationTime = infoAboutOutFile.created().toMSecsSinceEpoch();
        else
            lastOutCreationTime = 0;
#endif
        procType = runType;

        QFileInfo binFile(binFileName);
        if(binFile.exists())
            QFile::remove(binFileName);//fix to Bug #1141

        runProcess(solverPath,arguments);

        stationData->StationInverseMap.clear();
    }

    return;
}

QMessageBox::StandardButton MainWindow::confirmChangesAreSaved(QString message)
{
    // If the model is already saved, there is nothing to do
    QModelIndex rootIndex = guiModel->index(0,0);
    if(!rootIndex.isValid())//happens when called from openLSAProject when no existing project open
    {
        return QMessageBox::Yes;
    }

    // Make sure we push any pending changes in the Record Editor into the LSA tree
    recordEditor->pushChangesToGuiModel();

    if (!guiModel->descendantIncludeIsModified(rootIndex).isValid())
    {
        return QMessageBox::Yes;
    }

    QMessageBox::StandardButton buttonClicked;
    if ( message.isEmpty() )
    {
        message = "Save changes before proceeding?";
        buttonClicked = QMessageBox::question(this,
                         "Warning - there are unsaved changes to the LSA project.", message,
                         QMessageBox::Cancel | QMessageBox::No | QMessageBox::Save);
    }
    else
    {
        buttonClicked = QMessageBox::question(this,
                         "Warning - there are unsaved changes to the LSA project.", message,
                         QMessageBox::Cancel | QMessageBox::Save);

    }

    // If the user clicked yes, then save changes
    if (buttonClicked == QMessageBox::Yes || buttonClicked == QMessageBox::Save)
    {
        saveProject();
    }

    return buttonClicked;
}

QMessageBox::StandardButton MainWindow::confirmChangesAreSavedParseWarning(QString message)
{
    QMessageBox::StandardButton buttonClicked;
    if ( message.isEmpty() )
    {
        message = "Record with Parse Warning edited. Would you like to save and reload the project?";
    }

    buttonClicked = QMessageBox::question(this,
                     "Warning - there are unsaved changes to the LSA project.", message,
                     QMessageBox::Cancel | QMessageBox::Save);

    // If the user clicked save, then save changes
    if (buttonClicked == QMessageBox::Save)
    {
        QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

        // Make sure we push any pending changes in the Record Editor into the LSA tree
        guiModel->save();
        QModelIndex rootIndex = guiModel->index(0,0);
        guiModel->clearDescendantIncludeIsModified(rootIndex);

        //generate map of unique IDs for measurement records to corresponding LSARecord pointers
        generateUniqueIDMapToLSARecordPointers();

        //generate the hash code for the current data
        currentHashCode = guiModel->hashTree();

        //fix to Bug #1521
        parseLSAFileErrorsAndWarnings(true);
    }

    return buttonClicked;
}

QMessageBox::StandardButton MainWindow::confirmDeleteInclude(QString message)
{
    QMessageBox::StandardButton buttonClicked;

    // Make sure we push any pending changes in the Record Editor into the LSA tree
    recordEditor->pushChangesToGuiModel();

    if(!isTestHarness())
    {
        buttonClicked = QMessageBox::question(this,"Warning - there are unsaved changes to the LSA project.",
                                              message, QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes);
    }
    else
    {
        buttonClicked = QMessageBox::Yes;
    }

    // The user chose Cancel, so return false to stop processing
    return buttonClicked;
}

void MainWindow::handlePointCoordinateChanged(QModelIndex index)
{
    guiModel->synchItemToRecord(index);

    LSARecord* lsaRecord = guiModel->getLSARecord(index);
    if (lsaRecord->isAutogenerated)
    {
        guiModel->convertAutogeneratedToNormalPoint(index);
    }

    updateWidgets();
}

void MainWindow::handleRecordEditorClose()
{
    ui->actionView_RecordEditor->setChecked(false);
    updateRecordEditorVisibility();
}

void MainWindow::handleMapClose()
{
    ui->actionView_Map->setChecked(false);
    updateMapVisibility();

}

void MainWindow::handleAdjustedPositionsClose()
{
    ui->actionView_AdjustedPositions->setChecked(false);
    updateAdjustedPositionsVisibility();
}

void MainWindow::handleStationDataClose()
{
    ui->actionView_StationData->setChecked(false);
    updateStationDataVisibility();
}

void MainWindow::handleHistogramClose()
{
    ui->actionView_Histogram->setChecked(false);
    updateHistogramVisibility();
}

/******************************************************************************
//
// Private Methods
//
******************************************************************************/

bool MainWindow::runPreprocessor(QString &datFileName)
{
    //assumes currentFile is .lsa files.  .dat files generated in same directory.
    if(currentFile.lastIndexOf(".")<=currentFile.lastIndexOf("/"))//case when file lacks extension
        datFileName = currentFile + ".dat";
    else
        datFileName = currentFile.left(currentFile.lastIndexOf(".")) + ".dat";

    QStringList arguments;
    arguments << "--input" << currentFile
              << "--out" << datFileName;

    procType = PROCESS_LSAPREPROCESSOR;

    pushProcessStart("lsapreprocessor");
    if(runProcess(preprocessorPath,arguments))
    {
        QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        int CLITimeoutMilliseconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt()*1000;
        return proc->waitForFinished(CLITimeoutMilliseconds);
    }

    return false;
}

void MainWindow::onProcessEnd(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool eigenError = false;

    popupForMissingIncludes();

    if(exitStatus != QProcess::NormalExit)//user terminated a process or process failed
    {
        if(procTypeIsSolver())
            emit solverFinished();
        else if(procTypeIsPostprocessor())
            emit postprocessorFinished();
        else if(procTypeIsConverter())
            emit converterFinished();
        else if(procType == PROCESS_LSAPREPROCESSOR)
            emit preprocessorFinished();

        pushProcessFail();
        //Handle error/warning pushing here instead of onProcessStdErr()/onProcessStdOut()
        //see [Bug #896]
        reportCLIWarningsAndErrors();
        pushBreakerEnd();
        finishProgressBar();

        //restore menu options to launch subprocesses
        toggleGUISubprocessElements(true);
        procType = PROCESS_NONE;
    }
    else
    {
        //launch follow-on processes, if necessary
        if(procTypeIsSolver())
        {
            lastSolverRunType = procType;
            emit solverFinished();

            //run lsapost
            if(!runPostprocessor(procType))//aborted launch of lsapost
            {
                pushProcessFail();
                //Handle error/warning pushing here instead of onProcessStdErr()/onProcessStdOut()
                //see [Bug #896]
                reportCLIWarningsAndErrors();
                pushBreakerEnd();

                //restore menu options to launch subprocesses
                toggleGUISubprocessElements(true);
                procType = PROCESS_NONE;
            }
        }
        else if(procTypeIsPostprocessor())
        {
            pushProcessComplete();
            //Handle error/warning pushing here instead of onProcessStdErr()/onProcessStdOut()
            //see [Bug #896]
            reportCLIWarningsAndErrors();
            bool readOK = readHDF5File();

            if(currentHDF5File.solverExitType->getDataPoint() == lsa::EIGENFAIL)
                eigenError = true;

            //rerun solver in force stable mode if fast mode encountered an error
            if((lastSolverRunType == PROCESS_LSASOLVER) && isAllowFast() && eigenError)
            {
                pushWarning(QString("Possible problem with fast solver.  Recalculating adjustment with stable solver."));
                procChainStdErr.clear();
                procChainWarnings.clear();
                calculateAdjustment(PROCESS_LSASOLVER_FORCE_STABLE);
            }
            else
            {
                if(readOK)
                    runReportingScripts();//TODO: Make these separate processes?
                else
                {
                    pushBreakerEnd();
                    finishProgressBar();
                }

                //restore menu options to launch subprocesses
                toggleGUISubprocessElements(true);
                procType = PROCESS_NONE;
                //emit signals for testlsagui to know when a subprocess is done
                emit postprocessorFinished();
            }
        }
        else if(procTypeIsConverter())
        {
            if(procType == PROCESS_CUSTOM_GEOID)
                openConfigDialog();
            else if(procType == CUSTOM_CONVERTER_SCRIPT)
                pushProcessComplete();

            //Handle error/warning pushing here instead of onProcessStdErr()/onProcessStdOut()
            //see [Bug #896]
            reportCLIWarningsAndErrors();
            pushBreakerEnd();

            //restore menu options to launch subprocesses
            toggleGUISubprocessElements(true);
            procType = PROCESS_NONE;
            //emit signals for testlsagui to know when a subprocess is done
            emit converterFinished();
        }
        else if(procType == PROCESS_LSAPREPROCESSOR)
        {
            //emit signals for testlsagui to know when a subprocess is done
            pushProcessComplete();
            //Handle error/warning pushing here instead of onProcessStdErr()/onProcessStdOut()
            //see [Bug #896]
            reportCLIWarningsAndErrors();
            emit preprocessorFinished();
        }
        else if(procTypeIsReporter())
        {
            pushProcessComplete();
            //Handle error/warning pushing here instead of onProcessStdErr()/onProcessStdOut()
            //see [Bug #896]
            reportCLIWarningsAndErrors();
            if((procType == PROCESS_REPORTING_CSV) || (procType == PROCESS_REPORTING_XYZ || procType == CUSTOM_REPORTING_SCRIPT ||
                procType == PROCESS_REPORTING_SOLNCOV || procType == PROCESS_REPORTING_MEASRESID || procType == PROCESS_REPORTING_UTM ||
                                                       procType == PROCESS_REPORTING_SPC || procType == PROCESS_REPORTING_NCAT))
                pushBreakerEnd();
            //restore menu options to launch subprocesses
            toggleGUISubprocessElements(true);
            if(procType == PROCESS_REPORTING_XYZ)
            {
                //emit signals for testlsagui to know when a subprocess is done
                emit postprocessorFinished();
            }

            procType = PROCESS_NONE;
        }
    }

    QApplication::restoreOverrideCursor();
}

void MainWindow::clearAdjustmentSuccessIndicators()
{
    ui->valueNumDOF->setText("--");
    ui->valueNumObs->setText("--");
    ui->valueNumUnknowns->setText("--");
    ui->valueMaxExtMag->setText("<font color = black>--</font>");
    ui->labelMaxExtMag->setText("<font color = black>Max Ext Mag (m):</font>");

    ui->valueConvergenceHeader->setText("<font color = black>--</font>");
    ui->valueLastUpdate->setText("<font color = black>--</font>");
    ui->valueIterations->setText("--");

    ui->valueAPV->setText("--");
    ui->valueChiSqrHeader->setText("<font color = black>--</font>");
    ui->valueChiSqr->setText("--");

    //reset column header text to black for standard residuals and redunancy
    topResidModel->horizontalHeaderItem(3)->setForeground(QBrush(QColor("black")));
    topResidModel->horizontalHeaderItem(4)->setForeground(QBrush(QColor("black")));
    topResidModel->horizontalHeaderItem(5)->setForeground(QBrush(QColor("black")));
    topResidModel->horizontalHeaderItem(6)->setForeground(QBrush(QColor("black")));
}

bool MainWindow::runPostprocessor(int solverRunType)
{
    QString binFileName, datFileName, logFileName/*needed or else contents go to statusWindow*/,
            hdf5FileName, outFileName, cfgFileName;
    QString geoidPath;

    if (isProjDirGeoid())
    {
        geoidPath = QString::fromStdString(getPathWithoutFileName(currentFile.toStdString())+"/geoid/");
    }
    else
    {
        geoidPath = QString::fromStdString(getExecutablePath() + string("/../data/geoid"));
    }
    geoidPath = QDir::cleanPath(geoidPath);

    QString geoidFile1Name;
    QString geoidFile2Name;
    QStringList arguments;
    arguments.clear();


    if(isGeoidFileUsed())
    {
        geoidFile1Name = getGeoidFileName();
        geoidFile2Name = QString("egm1996_2.5m.und");
    }

    if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
    {
        logFileName = currentFile + QString(".post");
        datFileName = currentFile + QString(".dat");
        cfgFileName = currentFile + QString(".cfg");
    }
    else
    {
        logFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".post");
        datFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".dat");
        cfgFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".cfg");
    }

    if(solverRunType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
    {
        if(!isGeoidFileUsed())
        {
            pushError(QString(" Error - unable to generate geoid heights file without a geoid file... "));
            return false;
        }

        if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
        {
            binFileName = currentFile + QString(".apb");
            outFileName = currentFile + QString(".apo");
            hdf5FileName = currentFile + QString(".aph5");
        }
        else
        {
            binFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".apb");
            outFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".apo");
            hdf5FileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".aph5");
        }

        //generate the geoid heights file
        arguments << "--geoidpath" << geoidPath
                  << "--geoidfile" << geoidFile1Name
                  << "--geoidfile2" << geoidFile2Name
                  << "--bin" << binFileName;

        procType = PROCESS_LSAPOST_GEOIDHEIGHTS;
    }
    else
    {
        if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
        {
            binFileName = currentFile + QString(".bin");
            outFileName = currentFile + QString(".out");
            hdf5FileName = currentFile + QString(".h5");
        }
        else
        {
            binFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".bin");
            outFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".out");
            hdf5FileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
        }

        //generate the .pts file
        arguments << "--bin" << binFileName
                  << "--confid" << QString::fromStdString(getConfidenceInterval());

        if(isGeoidFileUsed())
        {
            arguments << "--geoidpath" << geoidPath
                      << "--geoidfile" << geoidFile1Name
                      << "--geoidfile2" << geoidFile2Name;
        }

        procType = PROCESS_LSAPOST;
    }

    //files common to both run types
    arguments << "--cfg" << cfgFileName
              << "--hdf5" << hdf5FileName
              << "--out" << outFileName
              << "--log" << logFileName
              << "--dat" << datFileName;

    //would be nice to just use the .cfg file for some of these arguments (geoidfile, westLon, noAPV)
    //but lsapost rejects non-lsapost arguments (e.g. --eqnout)
    if(usingWestLongitude())
        arguments << "--westLon";
    if(!isScaleByAPV())
        arguments << "--noAPV";
    arguments << "--linprecM" << QString::fromStdString(std::to_string(getLinearMeasurementPrecisionMeters()));
    arguments << "--angprecP" << QString::fromStdString(std::to_string(getAngularPositionPrecisionSOA()));
    arguments << "--angprecM" << QString::fromStdString(std::to_string(getAngularMeasurementPrecisionSOA()));

    pushProcessStart("lsapost");
    runProcess(lsapostPath,arguments);

    return true;
}

QString MainWindow::getGeoidFileName()
{
    QString returnStr = QString("");

    if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
    {
        if(configFileName.isEmpty())//for gui tests - never started a new project or imported an iob project
            configFileName = currentFile + QString(".cfg");
    }
    else
    {
        if(configFileName.isEmpty())//for gui tests - never started a new project or imported an iob project
            configFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".cfg");
    }

    std::ifstream istrm;
    istrm.open(configFileName.toStdString().c_str(),std::ios::in);
    if(!istrm.is_open())
       return QString("");
    std::string line;
    std::vector<std::string> parts;

    while(1)
    {
       line = std::string("");
       getline(istrm,line);
       gnsstk::StringUtils::stripTrailing(line,'\r');
       gnsstk::StringUtils::stripTrailing(line,'\n');
       if((istrm.eof() || !istrm.good()) && line.empty()) break;
       if(line.empty())
           continue;

       if(line.at(0)!='#')
       {
           if(line.find("--geoidfile")!=std::string::npos)
           {
               parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
               if(parts.size() < 2)
               {
                   pushWarning(QString("No geoid file specified in the project .cfg file."));
               }
               else
               {
                   returnStr = QString::fromStdString(parts[1]);
               }
               break;
           }
       }
    }
    istrm.close();
    return returnStr;
}

void MainWindow::onProcessStdOut()
{
    QRegularExpression whitespace("[\\s]");
    procStdOut = QString(proc->readAllStandardOutput());//get proc output here?
#ifdef _WIN32
    QStringList outputList = procStdOut.split("\r\n");
#else
    QStringList outputList = procStdOut.split("\n");
#endif
    foreach(QString line, outputList)
    {
        if (line.isEmpty())
            continue;

        //line.split(" ") on ' Warning - ....' doesn't parse 'Warning' as the first element. Weird.
        //string the leading space
        string line_str = line.toStdString();
        gnsstk::StringUtils::stripLeading(line_str,' ');
        line = QString::fromStdString(line_str);

        QString firstWord = line.split(whitespace).at(0);

        //used to look for Error, but those should be written to stderr
        if(firstWord.toUpper().contains("WARNING"))
        {
            procWarnings += line + QString("\n");
        }
        else
        {
            if(procTypeIsSolver())//only display non-warning/error stdout from the solver
                pushStatus(line);
        }
        //update progress bar
        updateProgressBar(firstWord, line);

        if(line.contains("lsasolver timing"))
        {
            std::string timetag = line.toStdString().substr(line.toStdString().find("lsasolver timing")+18);
            timetag = timetag.substr(0,timetag.find(" ")-timetag.find_first_of("0123456789"));
            ui->valueProcessingTime->setText(QString::fromStdString(timetag));
        }
    }
}

void MainWindow::onProcessStdErr()
{
    procStdErr += QString(proc->readAllStandardError());
}

void MainWindow::onProcessError()
{
    QProcess::ProcessError errorType = proc->error();

    if(suppressErrorNotification)
        suppressErrorNotification = false;
    else
    {
        if(errorType==QProcess::FailedToStart)
            pushError("Error - unable to find or run " + proc->program());
        else if(errorType==QProcess::Crashed)
            pushError("Error - " + proc->program() + " terminated unexpectedly.");
        else if(errorType==QProcess::Timedout)
            pushError("Error - " + proc->program() + " timed out.");
        else if(errorType==QProcess::WriteError)
            pushError("Error - failure writing to " + proc->program());
        else if(errorType==QProcess::ReadError)
            pushError("Error - failure to read from " + proc->program());
        else if(errorType==QProcess::UnknownError)
            pushError(proc->errorString());
        else
            pushError(proc->errorString());
    }
}

void MainWindow::viewOutput()
{
    QString outFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".out");
    QFileInfo file(outFileName);
    if(!file.exists())
    {
        QMessageBox *notifyUser = new QMessageBox(QMessageBox::Critical, // icon
                                                  QString("Unable to open solver output!"), // title
                                                  outFileName + QString(" not found."), // text
                                                  QMessageBox::Ok, // buttons
                                                  (QWidget*)this, // parent
                                                  Qt::WindowStaysOnTopHint);

        notifyUser->exec();//blocks
        delete notifyUser;
    }
    else
    {
        QUrl outfileUrl(QString("file:")+outFileName);
        QDesktopServices::openUrl(outfileUrl);
    }
}

void MainWindow::viewAdjustedPoints()
{
    QString ptsFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".pts");
    QFileInfo file(ptsFileName);
    if(!file.exists())
    {
        QMessageBox *notifyUser = new QMessageBox(QMessageBox::Critical, // icon
                                                  QString("Unable to open adjusted points file!"), // title
                                                  ptsFileName + QString(" not found."), // text
                                                  QMessageBox::Ok, // buttons
                                                  (QWidget*)this, // parent
                                                  Qt::WindowStaysOnTopHint);

        notifyUser->exec();//blocks
        delete notifyUser;
    }
    else
    {
        QUrl ptsfileUrl(QString("file:")+ptsFileName);
        QDesktopServices::openUrl(ptsfileUrl);
    }
}

void MainWindow::viewManual()
{
    // Get the path to the user manual
    QDir manualDir(QString::fromStdString(getExecutablePath()) + QString("/../doc"));
    QString manualPath;
    if(manualDir.exists(QString::fromStdString("SalsaUserManual.pdf")))
    {
        manualPath = QString::fromStdString("file:") + manualDir.absoluteFilePath("SalsaUserManual.pdf");
    }

    else
    {
        QDir executableDir(QString::fromStdString(getExecutablePath()));
        manualPath = QString::fromStdString("file:") + executableDir.absoluteFilePath("SalsaUserManual.pdf");
    }

    // Open it
    QUrl manualUrl(manualPath);
    QDesktopServices::openUrl(manualUrl);
}

void MainWindow::killProcess()
{
    proc->kill();
    finishProgressBar();
    pushKillProcess();
}

bool MainWindow::DisplaySolverWarningsAndErrors(bool firstOpen)
{
    QString outFileName;
    string error = string(""), eigen_error=string("");
    bool foundEigenError = false;
    bool foundNormalError = false;


    if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
    {
        outFileName = currentFile + QString(".out");
    }
    else
    {
        outFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".out");
    }

    // search for eigenerrors
    for(int i=0; i<currentHDF5File.solverErrors->getSize(); i++)
    {

        if(currentHDF5File.solverErrors->getDataPoint(i).find(lsa::FASTSOLVER_ERRORSTRING)!=string::npos)
        {
            eigen_error = currentHDF5File.solverErrors->getDataPoint(i);
            foundEigenError = true;
            break;
        }
    }

    // push warnings before throwing an eigenerror
    for(int i=0;i<currentHDF5File.solverWarnings->getSize();i++)
    {
        if(!firstOpen)
        {
            std::string warningString = currentHDF5File.solverWarnings->getDataPoint(i);
            procChainWarnings += QString::fromStdString(warningString) + QString("\n");
        }
        pushWarning(currentHDF5File.solverWarnings->getDataPoint(i));
    }

    //don't pop-up eigen error if rerunning with stable solver
    if(foundEigenError)
    {
        if(!isAllowFast())//forced fast
        {
            if(errorBox != NULL)
            {
                delete errorBox;
                errorBox = NULL;
            }
            errorBox = new QMessageBox(QString("lsasolver ERROR"),QString::fromStdString(eigen_error),
                                       QMessageBox::Critical,QMessageBox::Ok,0,0,(QWidget*)this,Qt::WindowStaysOnTopHint);
            errorBox->show();
            clearOutputTab();
        }
        else//will attempt to rerun to solver in forced stable mode
        {
            // seeing inconsistent .out file parsing behavior when doing reruns
            QFile outFile(outFileName);
            outFile.remove();
        }
    }
    else if(currentHDF5File.solverErrors->getSize()>0)
    {
        for(int i=0; i<currentHDF5File.solverErrors->getSize(); i++)
            error += currentHDF5File.solverErrors->getDataPoint(i) + string("\n");
        if(error.size()>lsa::MAX_ERROR_STRING_SIZE)//Fix to Bug #943
            error = error.substr(0,lsa::MAX_ERROR_STRING_SIZE) + std::string(" ...");

        if(firstOpen)//output last run's errors from old project as warnings
        {
            pushWarning(( QString("Warning - last solver run exited with ").append(QString::fromStdString(error))));
        }
        else
        {
            if(errorBox != NULL)
            {
                delete errorBox;
                errorBox = NULL;
            }
            errorBox = new QMessageBox(QString("lsasolver Error"),QString::fromStdString(error),
                                               QMessageBox::Critical,QMessageBox::Ok,0,0,(QWidget*)this,Qt::WindowStaysOnTopHint);
            errorBox->show();

            procChainStdErr += QString::fromStdString(error);
            pushError(error);
            clearOutputTab();
        }

        foundNormalError = true;
    }

    return foundNormalError;
}

#ifdef _WIN32
FILETIME MainWindow::windowsGetLastModifiedTime(QString filename)
{
    FILETIME ftCreate, ftAccess, ftWrite, firstTime;
    firstTime.dwHighDateTime = 0;
    firstTime.dwLowDateTime = 0;
    HANDLE hFile = CreateFile(filename.toStdString().c_str(),GENERIC_READ,
                              FILE_SHARE_READ, NULL,OPEN_EXISTING,0,NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return firstTime;
    if(!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
    {
        CloseHandle(hFile);
        return firstTime;
    }
    CloseHandle(hFile);
    return ftWrite;
}
#endif

bool MainWindow::updateOutputPanel()
{
    double maxExtMagWarningThreshold = getMaxExtMagWarningThreshold();
    double maxExtMagErrorThreshold = getMaxExtMagErrorThreshold();

    std::set<int> elevatedColumnsSet;
    /********************************************************
     *
     *  Parse num of unknowns, observations and DOF
     *
     ********************************************************/
    //put try/catch wrapper around .bin file parsing.  If encounter nans due eigen failure, may rerun solver
    try
    {
        bool redundZeroExists = false;
        for(int i=0; i<currentHDF5File.measurements->getSize();i++){
           if(currentHDF5File.measurements->getDataPoint(i).redundancyZero)
           {
              redundZeroExists = true;
              break;
           }
        }

        if(currentHDF5File.probDescript->getSize() > 0)
        {
            ui->valueNumUnknowns->setText(QString::number(currentHDF5File.probDescript->getDataPoint().unknownCount));
            ui->valueNumObs->setText(QString::number(currentHDF5File.probDescript->getDataPoint().NMeas));
            ui->valueNumDOF->setText(QString::number(currentHDF5File.probDescript->getDataPoint().degOfFreed));
        }

        /********************************************************
         *
         *  Parse convergence panel data
         *
         ********************************************************/
        if(currentHDF5File.convergenceHist->getSize()>0)
        {
            //convergence test
            if(currentHDF5File.convergenceHist->back().status == string("too many iterations"))
            {
                pushWarning(QString("No solution. Max iteration limit exceeded."));
                ui->valueConvergenceHeader->setText("<font color = red>FAIL (Max Iterations)</font>");
            }
            else if(currentHDF5File.convergenceHist->back().status == string("diverged"))
            {
                pushWarning(QString("Solution diverged."));
                ui->valueConvergenceHeader->setText("<font color = red>FAIL (Diverged)</font>");
            }
            else
            {
                ui->valueConvergenceHeader->setText("<font color = green>PASS</font>");
            }

            std::ostringstream ossConvergence;
            ossConvergence.precision(2);
            ossConvergence << std::scientific << currentHDF5File.convergenceHist->back().RMSadj;
            ui->valueLastUpdate->setText(QString::fromStdString(ossConvergence.str()));

            std::ostringstream ossConvergenceLimit;
            ossConvergenceLimit.precision(2);
            ossConvergenceLimit << std::scientific << currentHDF5File.projCfg->getDataPoint().converge;
            ui->valueConvLimit->setText(QString::fromStdString(ossConvergenceLimit.str()));

            ui->valueIterations->setText(QString::number(currentHDF5File.convergenceHist->getSize()));
        }

         /********************************************************
         *
         *  Parse chi squared panel data
         *
         ********************************************************/

        if(currentHDF5File.probDescript->getDataPoint().degOfFreed == 0)
        {
            pushWarning(QString(" Warning - the network has zero degrees of freedom. Error ellipses will not be drawn on the map."));
            ui->valueChiSqrHeader->setText("<font color = red>FAIL (zero DoF)</font>");
            ui->valueChiSqr->setText("--");
            ui->valueAPV->setText("Undefined");
        }
        else
        {
            // chi-squared test
            std::ostringstream oss;
            int numDigits = 3;

            oss.str("");
            double lowerBound;
            double upperBound;

            if(currentHDF5File.statistics->getDataPoint().APV>0)
            {
                lowerBound = currentHDF5File.statistics->getDataPoint().lowerChiSq/currentHDF5File.probDescript->getDataPoint().degOfFreed;

                oss << std::fixed << setprecision(numDigits) << lowerBound;
                std::string chiSquaredLowerText = oss.str();

                oss.str("");
                oss << std::fixed << setprecision(numDigits) << currentHDF5File.statistics->getDataPoint().APV;
                std::string chiSquaredText = oss.str();

                oss.str("");
                Q_ASSERT(currentHDF5File.probDescript->getDataPoint().degOfFreed > 0);
                upperBound = currentHDF5File.statistics->getDataPoint().upperChiSq/currentHDF5File.probDescript->getDataPoint().degOfFreed;
                oss << std::fixed << setprecision(numDigits) << upperBound;
                std::string chiSquaredUpperText = oss.str();

                bool lowerBoundFailed = (currentHDF5File.statistics->getDataPoint().APV < lowerBound) || (lowerBound < 1e-9);
                bool upperBoundFailed = (currentHDF5File.statistics->getDataPoint().APV > upperBound) || (upperBound < 1e-9);

                QString chiSquaredHeaderText;
                QString chiSquaredLabelText;
                if(lowerBoundFailed)
                {
                    chiSquaredLabelText = QString::fromStdString("<font color = red>" + chiSquaredLowerText + " &lt; " + chiSquaredText + "</font> &lt; " + chiSquaredUpperText);
                    chiSquaredHeaderText = QString::fromStdString("<font color = red>FAIL (Lower Bound Exceeded)</font>");
                }
                else if (upperBoundFailed)
                {
                    chiSquaredLabelText = QString::fromStdString(chiSquaredLowerText + " &lt; <font color = red>" + chiSquaredText + " &lt; " + chiSquaredUpperText + "</font>");
                    chiSquaredHeaderText = QString::fromStdString("<font color = red>FAIL (Upper Bound Exceeded)</font>");
                }
                else
                {
                    chiSquaredLabelText = QString::fromStdString(chiSquaredLowerText + " < " + chiSquaredText + " < " + chiSquaredUpperText);
                    chiSquaredHeaderText = QString::fromStdString("<font color = green>PASS</font>");
                }

                ui->valueChiSqrHeader->setText(chiSquaredHeaderText);
                ui->valueChiSqr->setText(chiSquaredLabelText);

                oss.str("");
                oss << setprecision(numDigits) << currentHDF5File.statistics->getDataPoint().APV;
                std::string APVText = oss.str();
                ui->valueAPV->setText(QString::fromStdString(APVText));
            }

            if(currentHDF5File.projCfg->getDataPoint().alpha>0)
            {
                string confidenceInterval1 = getChiSquared();
                ui->labelChiSqr->setText("Test Limits (" + QString::fromStdString(confidenceInterval1) + "%): ");
            }
        }

        /********************************************************
         *
         *  Parse measurement residual table data
         *
         ********************************************************/

        if(currentHDF5File.measurements->getSize()>0)
        {
            //model->clear() removes the headers
            if(topResidModel != NULL)
            {
                delete topResidModel;
                topResidModel = NULL;
            }
            topResidModel = new QStandardItemModel(currentHDF5File.measurements->getSize(),NUM_RESID_COLUMNS,this);

            addResidualTableHeaders();

            topResidModel->setHorizontalHeaderItem(NUM_RESID_COLUMNS-1, new QStandardItem(QString("")));

            int ctr=0;
            hashCodesAreValid = true;
            double maxExtMagtemp = 0.0, maxExtMag = 0.0;
            for(int i=0;i<currentHDF5File.measurements->getSize();i++)
            {
                measurementsExternal currentExternal = currentHDF5File.measurements->getDataPoint(i);
                maxExtMagtemp = currentExternal.extVectMag;
                if(maxExtMagtemp > maxExtMag)
                {
                    maxExtMag = maxExtMagtemp;
                }
                QStandardItem *Measurement = new QStandardItem(QString::fromStdString(currentExternal.measType));
                QFont itemFont = Measurement->font();
                auto pntsize = itemFont.pointSize();
                itemFont.setPointSize(getIdealFontSize());
                Measurement->setFont(itemFont);
                pntsize = itemFont.pointSize();
                QStandardItem *raw = new QStandardItem();
                raw->setFont(itemFont);
                raw->setData(std::fabs(currentExternal.rawResid),Qt::DisplayRole);
                raw->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                QStandardItem *rel = new QStandardItem();
                rel->setFont(itemFont);
                rel->setData(std::fabs(currentExternal.relResid),Qt::DisplayRole);
                rel->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                QStandardItem *standard = new QStandardItem();
                standard->setFont(itemFont);
                standard->setData(std::fabs(currentExternal.stdResid),Qt::DisplayRole);
                standard->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                QStandardItem *Redundancy = new QStandardItem();
                Redundancy->setFont(itemFont);
                Redundancy->setData(currentExternal.redund,Qt::DisplayRole);
                Redundancy->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                QStandardItem *MinDectBias = new QStandardItem();
                MinDectBias->setFont(itemFont);
                MinDectBias->setData(currentExternal.minDectBias,Qt::DisplayRole);
                MinDectBias->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                if(currentExternal.redundancyZero)
                {
                   Redundancy->setData(QVariant(QColor("red")),Qt::ForegroundRole);
                   MinDectBias->setData(QVariant(QColor("red")),Qt::ForegroundRole);

                }
                if(currentExternal.isBlunder)
                {
                   Measurement->setData(QVariant(QColor("red")),Qt::ForegroundRole);
                   standard->setData(QVariant(QColor("red")),Qt::ForegroundRole);
                }
                QStandardItem *ExtVectMag = new QStandardItem();
                ExtVectMag->setFont(itemFont);

                if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect)
                {
                   ExtVectMag->setData(currentExternal.extVectMag,Qt::DisplayRole);
                   ExtVectMag->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                   if(roundDoubleToPrecision(currentExternal.extVectMag,lsa::DEFAULT_REDUNDANCY_PRECISION) > maxExtMagErrorThreshold)
                   {
                       Measurement->setData(QVariant(QColor("red")),Qt::ForegroundRole);
                       ExtVectMag->setData(QVariant(QColor("red")),Qt::ForegroundRole);
                   }
                   else if(roundDoubleToPrecision(currentExternal.extVectMag,lsa::DEFAULT_REDUNDANCY_PRECISION) > maxExtMagWarningThreshold)
                   {
                       Measurement->setData(QVariant(QColor("orange")),Qt::ForegroundRole);
                       ExtVectMag->setData(QVariant(QColor("orange")),Qt::ForegroundRole);
                   }
                  topResidModel->setItem(ctr,RESID_TABLE_EXT_MAG_COLUMN,ExtVectMag);

                  // Measurement residual header colors
                  if(maxExtMag > maxExtMagErrorThreshold)
                  {
                     topResidModel->horizontalHeaderItem(RESID_TABLE_EXT_MAG_COLUMN)->setForeground(QBrush(QColor("red")));
                     elevatedColumnsSet.insert(RESID_TABLE_EXT_MAG_COLUMN);
                  }
                  else if(maxExtMag > maxExtMagWarningThreshold)
                  {
                     topResidModel->horizontalHeaderItem(RESID_TABLE_EXT_MAG_COLUMN)->setForeground(QBrush(QColor("orange")));
                     elevatedColumnsSet.insert(RESID_TABLE_EXT_MAG_COLUMN);
                  }
                }
                else
                {
                   QStandardItem *noExtMag = new QStandardItem();
                   noExtMag->setFont(itemFont);
                   noExtMag->setData(QString::fromStdString("--"), Qt::DisplayRole);
                   noExtMag->setData(Qt::AlignCenter + Qt::AlignVCenter,Qt::TextAlignmentRole);
                   topResidModel->setItem(ctr,RESID_TABLE_EXT_MAG_COLUMN, noExtMag);

                }

                topResidModel->setItem(ctr,RESID_TABLE_MEAS_COLUMN,Measurement);
                topResidModel->setItem(ctr,RESID_TABLE_RAW_COLUMN,raw);
                topResidModel->setItem(ctr,RESID_TABLE_REL_COLUMN,rel);
                topResidModel->setItem(ctr,RESID_TABLE_STD_COLUMN,standard);
                topResidModel->setItem(ctr,RESID_TABLE_REDUND_COLUMN,Redundancy);
                topResidModel->setItem(ctr,RESID_TABLE_MAX_BIAS_COLUMN,MinDectBias);

                LSARecord *lsaRecord;

                if(currentExternal.measType.find("PPP")!=string::npos)
                {
                    lsaRecord = guiModel->getLSARecord(guiModel->getIndexFromPointLabel(currentExternal.at));
                }
                else
                {
                    QModelIndex newIndex = guiModel->getIndexFromTag(currentExternal.tag);
                    lsaRecord = guiModel->getLSARecord(newIndex);
                }

                if(lsaRecord != NULL)
                {
                    string FromAt, To;

                    if(currentExternal.measType.find("PPP")!=string::npos)
                    {
                        FromAt = currentExternal.at;
                        To = string("");
                    }
                    else
                    {
                        FromAt = currentExternal.from;
                        if(!currentExternal.at.empty())
                            FromAt += string("-") + currentExternal.at;
                        To = currentExternal.to;
                    }
                    QStandardItem *FromAtStations = new QStandardItem(QString::fromStdString(FromAt));
                    FromAtStations->setFont(itemFont);
                    QStandardItem *ToStations = new QStandardItem(QString::fromStdString(To));
                    ToStations->setFont(itemFont);
                    topResidModel->setItem(ctr,RESID_TABLE_FROMAT_COLUMN,FromAtStations);
                    topResidModel->setItem(ctr,RESID_TABLE_TO_COLUMN,ToStations);

                    QStandardItem *guiModelID = new QStandardItem(QString::number((qlonglong)lsaRecord));
                    guiModelID->setFont(itemFont);
                    topResidModel->setItem(ctr,NUM_RESID_COLUMNS-1,guiModelID);
                }
                else
                {
                    hashCodesAreValid = false;
                }
                ctr++;
            }

            ui->tableMeasurements->setModel(topResidModel);
            ui->tableMeasurements->setColumnHidden(NUM_RESID_COLUMNS-1,true);
            connect(ui->tableMeasurements->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(handleTableMeasurementsSelectionChanged(QItemSelection, QItemSelection)));

            //set text of standard residual column to red if any exceeded the blunder limit
            if(currentHDF5File.blundersExist->getDataPoint())
            {
                topResidModel->horizontalHeaderItem(RESID_TABLE_STD_COLUMN)->setForeground(QBrush(QColor("red")));
                elevatedColumnsSet.insert(RESID_TABLE_STD_COLUMN);
            }
            else
            {
                topResidModel->horizontalHeaderItem(RESID_TABLE_STD_COLUMN)->setForeground(QBrush(QColor("black")));
            }
            if(redundZeroExists)
            {
               topResidModel->horizontalHeaderItem(RESID_TABLE_REDUND_COLUMN)->setForeground(QBrush(QColor("red")));
               topResidModel->horizontalHeaderItem(RESID_TABLE_MAX_BIAS_COLUMN)->setForeground(QBrush(QColor("red")));
               topResidModel->horizontalHeaderItem(RESID_TABLE_EXT_MAG_COLUMN)->setForeground(QBrush(QColor("red")));
               elevatedColumnsSet.insert(RESID_TABLE_REDUND_COLUMN);
               elevatedColumnsSet.insert(RESID_TABLE_MAX_BIAS_COLUMN);
               elevatedColumnsSet.insert(RESID_TABLE_EXT_MAG_COLUMN);
            }

            //perform string formatting on the numerical items
            ui->tableMeasurements->setItemDelegateForColumn(RESID_TABLE_RAW_COLUMN,new NumberFormatDelegate(this));
            ui->tableMeasurements->setItemDelegateForColumn(RESID_TABLE_REL_COLUMN,new NumberFormatDelegate(this));
            ui->tableMeasurements->setItemDelegateForColumn(RESID_TABLE_STD_COLUMN,new NumberFormatDelegate(this));
            ui->tableMeasurements->setItemDelegateForColumn(RESID_TABLE_REDUND_COLUMN,new NumberFormatDelegate(this));
            ui->tableMeasurements->setItemDelegateForColumn(RESID_TABLE_MAX_BIAS_COLUMN,new NumberFormatDelegate(this));
            if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect)
            {
               ui->tableMeasurements->setItemDelegateForColumn(RESID_TABLE_EXT_MAG_COLUMN,new NumberFormatDelegate(this));
            }
            else
            {
               ui->tableMeasurements->setItemDelegateForColumn(RESID_TABLE_EXT_MAG_COLUMN,new StringFormatDelegate(this));
            }
            ui->tableMeasurements->sortByColumn(RESID_TABLE_STD_COLUMN,Qt::DescendingOrder);
            ui->tableMeasurements->horizontalHeader()->setSortIndicator(RESID_TABLE_STD_COLUMN,Qt::DescendingOrder);
            //resizeEvent(NULL);//added because Meas column was blowing up if rerunning the calc.  Couldn't find the reason.

            setResidualTableColumnWidths();
            if(elevatedColumnsSet.size())
            {
                QStringList listOfHiddenColumns;
                QString warningString(" Warning - hidden column(s) contain elevated values:");
                for(const auto& entry : elevatedColumnsSet)
                {
                    if(ui->tableMeasurements->isColumnHidden(entry))
                    {
                        listOfHiddenColumns.append(residColumnToColumnHeaderMap.at(entry));
                    }
                }
                if (listOfHiddenColumns.count())
                {
                    pushWarning(warningString + " " + listOfHiddenColumns.join(", "));
                }
            }
        }



        /********************************************************
         *
         *  Parse confidence region table data
         *
         ********************************************************/
        if(currentHDF5File.llh->getSize()>0)
        {
           double maxExtMag = 0.0, maxExtMagtemp = 0.0;
           for(int i=0;i<currentHDF5File.measurements->getSize();++i)
           {
               measurementsExternal currentExternal = currentHDF5File.measurements->getDataPoint(i);
               maxExtMagtemp = currentExternal.extVectMag;
               if(maxExtMagtemp > maxExtMag)
               {
                   maxExtMag = maxExtMagtemp;
               }
           }

           /// Determine the reliability rectangle headers and colors
           QString plain_additional_text, additional_text;
           int reliabilityRedundancyFlag=0;
           if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect && !redundZeroExists)
           {
              reliabilityRedundancyFlag = 1;
           }
           else if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect && redundZeroExists)
           {
              reliabilityRedundancyFlag = 2;
           }
           else
           {
              reliabilityRedundancyFlag = 3;
           }

           switch(reliabilityRedundancyFlag)
           {
              case 1 :
                 additional_text = " Reliability Rectangles"; // They are calculated, no redundancy equal to zero found
                 if(maxExtMag >= maxExtMagErrorThreshold)
                 {
                     ui->labelMaxExtMag->setText(QString("<font color=red><b>Max Ext Mag (m)</b></font>"));
                     ui->valueMaxExtMag->setText(QString("<font color=red><b>") + QString::number(maxExtMag,'f',lsa::DEFAULT_REDUNDANCY_PRECISION)
                                                        + QString(" &gt; ") + QString::number(maxExtMagErrorThreshold,'f',lsa::DEFAULT_REDUNDANCY_PRECISION) + QString("</b></font>"));

                 }
                 else if(maxExtMag > maxExtMagWarningThreshold && maxExtMag < maxExtMagErrorThreshold)
                 {
                     ui->labelMaxExtMag->setText(QString("<font color=orange><b>Max Ext Mag (m)</b></font>"));
                     ui->valueMaxExtMag->setText(QString("<font color=orange><b>") + QString::number(maxExtMag,'f',lsa::DEFAULT_REDUNDANCY_PRECISION)
                                                        + QString(" &gt; ") + QString::number(maxExtMagWarningThreshold,'f',lsa::DEFAULT_REDUNDANCY_PRECISION) + QString("</b></font>"));
                 }
                 else
                 {
                     ui->labelMaxExtMag->setText(QString("<font color=black>Max Ext Mag (m)</font>"));
                     ui->valueMaxExtMag->setText(QString("<font color=black>") + QString::number(maxExtMag,'f',lsa::DEFAULT_REDUNDANCY_PRECISION)
                                                        + QString(" &lt; ") + QString::number(maxExtMagWarningThreshold,'f',lsa::DEFAULT_REDUNDANCY_PRECISION) + QString("</font>"));
                 }
                 break;
              case 2 :
                 plain_additional_text = " Reliability Rectangles (Redundancy = 0)"; // They are calculated, but a redundancy equal to zero exists
                 additional_text = QString("<span style=' color:red;'>%1</span>").arg(plain_additional_text);
                 if(maxExtMag >= maxExtMagErrorThreshold)
                 {
                     ui->labelMaxExtMag->setText(QString("<font color=red><b>Max Ext Mag (m)</b></font>"));
                     ui->valueMaxExtMag->setText(QString("<font color=red><b>") + QString::number(maxExtMag,'f',lsa::DEFAULT_REDUNDANCY_PRECISION)
                                                        + QString(" &gt; ") + QString::number(maxExtMagErrorThreshold,'f',lsa::DEFAULT_REDUNDANCY_PRECISION) + QString(" (Redundancy = 0)</b></font>"));
                 }
                 else if(maxExtMag > maxExtMagWarningThreshold && maxExtMag < maxExtMagErrorThreshold)
                 {
                     ui->labelMaxExtMag->setText(QString("<font color=orange><b>Max Ext Mag (m)</b></font>"));
                     ui->valueMaxExtMag->setText(QString("<font color=orange><b>") + QString::number(maxExtMag,'f',lsa::DEFAULT_REDUNDANCY_PRECISION)
                                                        + QString(" &gt; ") + QString::number(maxExtMagWarningThreshold,'f',lsa::DEFAULT_REDUNDANCY_PRECISION) + QString(" (Redundancy = 0)</b></font>"));
                 }
                 else
                 {
                     ui->labelMaxExtMag->setText(QString("<font color=black>Max Ext Mag (m)</font>"));
                     ui->valueMaxExtMag->setText(QString("<font color=black>") + QString::number(maxExtMag,'f',lsa::DEFAULT_REDUNDANCY_PRECISION)
                                                        + QString(" &lt; ") + QString::number(maxExtMagWarningThreshold,'f',lsa::DEFAULT_REDUNDANCY_PRECISION) + QString(" (Redundancy = 0)</font>"));
                 }
                 break;
              case 3 :
                 additional_text = " Reliability Rectangles (Not Calculated)"; // They are not calculated
                 ui->labelMaxExtMag->setText(QString("<font color=black>Max Ext Mag (m)</font>"));
                 ui->valueMaxExtMag->setText(QString("<font color=black>Not Calculated</font>"));
                 break;
           }

            //adjusted points
            if(confidenceModel != NULL)
            {
                delete confidenceModel;
                confidenceModel = NULL;
            }

            int numRows = 0;
            confidenceModel = new QStandardItemModel(numRows, NUM_POINT_COLUMNS, this);//N rows, 7 columns
            ui->tablePoints->setModel(confidenceModel);
            connect(ui->tablePoints->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(handleTablePointsSelectionChanged(QItemSelection, QItemSelection)));
            connect(ui->tablePoints, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(handleTablePointsDoubleClicked(QModelIndex)));

            addConfidenceTableHeaders();

            string confidenceInterval = getConfidenceInterval();
            guiModel->confidenceInterval = QString::fromStdString(confidenceInterval);
            //Set table title
            if(confidenceInterval == "1sig")
            {
                ui->labelPointConfRegions->setText(QString::fromStdString("Point Confidence Regions (1-Sigma) |") + additional_text);
            }
            else
            {
                ui->labelPointConfRegions->setText(QString::fromStdString("Point Confidence Regions (" + confidenceInterval + "%) |") + additional_text);
            }

            for(int i=0;i<currentHDF5File.llh->getSize();i++)
            {
                // Don't include fixed points in the point confidence table
                std::string pointLabel = currentHDF5File.llh->getDataPoint(i).label;
                LSARecordMap *pointMap = guiModel->getPoints();
                LSARecordMap::iterator recordIter = pointMap->find(pointLabel);
                if (recordIter != pointMap->end())
                {
                    LSARecord *lsaRecord = recordIter->second;
                    LSAType lsaType = lsaRecord->getRecType();
                    if (lsaType == LSAType::POSG)
                    {
                        LSAPosG *lsaPosG = static_cast<LSAPosG*>(lsaRecord);
                        if (lsaPosG->isFixed() )
                            continue;
                    }
                    else if (lsaType == LSAType::POSC)
                    {
                        LSAPosC *lsaPosC = static_cast<LSAPosC*>(lsaRecord);
                        if (lsaPosC->isFixed() )
                            continue;
                    }
                }

                QStandardItem *station = new QStandardItem(QString::fromStdString(pointLabel));
                QStandardItem *threeDMajor = new QStandardItem();
                QStandardItem *twoDMajor = new QStandardItem();
                QStandardItem *vertical = new QStandardItem();
                QStandardItem *maj2Drr = new QStandardItem();
                QStandardItem *maj3Drr = new QStandardItem();
                QStandardItem *vertrr = new QStandardItem();

                // set font size
                QFont itemFont = station->font();
                itemFont.setPointSize(getIdealFontSize());
                station->setFont(itemFont);
                threeDMajor->setFont(itemFont);
                twoDMajor->setFont(itemFont);
                vertical->setFont(itemFont);
                maj2Drr->setFont(itemFont);
                maj3Drr->setFont(itemFont);
                vertrr->setFont(itemFont);

                //assign values to the items
                double ellipseScaleFactor = 1.0;
                if(isScaleByAPV())
                    ellipseScaleFactor *= ::sqrt(currentHDF5File.statistics->getDataPoint().APV);

                threeDMajor->setData(currentHDF5File.llh->getDataPoint(i).maj3D*ellipseScaleFactor,Qt::DisplayRole);


                // Fix to bug issue254
                if(isnan(currentHDF5File.llh->getDataPoint(i).maj2D*ellipseScaleFactor))
                {
                   std::string constraint_string = currentHDF5File.llh->getDataPoint(i).constraints;
                   if(constraint_string == "")
                   {
                      constraint_string = currentHDF5File.initialLlh->getDataPoint(i).constraints;
                   }
                   // Check that the problem is 1D
                   if(constraint_string == "EN" || constraint_string == "EU" || constraint_string == "NU" ||
                      constraint_string == "NE" || constraint_string == "UE" || constraint_string == "UN")
                   {
                      twoDMajor->setData(0,Qt::DisplayRole);
                   }
                   else
                   {
                      twoDMajor->setData(currentHDF5File.llh->getDataPoint(i).maj2D*ellipseScaleFactor,Qt::DisplayRole);
                   }
                }
                else
                {
                   twoDMajor->setData(currentHDF5File.llh->getDataPoint(i).maj2D*ellipseScaleFactor,Qt::DisplayRole);
                }

                vertical->setData(currentHDF5File.llh->getDataPoint(i).vert*ellipseScaleFactor,Qt::DisplayRole);
                if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect)
                {
                   maj2Drr->setData(currentHDF5File.llh->getDataPoint(i).maj2Drr,Qt::DisplayRole);
                   maj3Drr->setData(currentHDF5File.llh->getDataPoint(i).maj3Drr,Qt::DisplayRole);
                   vertrr->setData(currentHDF5File.llh->getDataPoint(i).vertrr,Qt::DisplayRole);
                   maj2Drr->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                   maj3Drr->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                   vertrr->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                }
                else
                {
                   std::string noExtRel = "--";
                   maj2Drr->setData(QString::fromStdString(noExtRel),Qt::DisplayRole);
                   maj3Drr->setData(QString::fromStdString(noExtRel),Qt::DisplayRole);
                   vertrr->setData(QString::fromStdString(noExtRel),Qt::DisplayRole);
                   maj2Drr->setData(Qt::AlignCenter + Qt::AlignVCenter,Qt::TextAlignmentRole);
                   maj3Drr->setData(Qt::AlignCenter + Qt::AlignVCenter,Qt::TextAlignmentRole);
                   vertrr->setData(Qt::AlignCenter + Qt::AlignVCenter,Qt::TextAlignmentRole);
                }

                //do right alignment for numbers
                threeDMajor->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                twoDMajor->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                vertical->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
                QList<QStandardItem*> newRow;
                newRow << station << threeDMajor << twoDMajor  << vertical << maj3Drr << maj2Drr << vertrr;
                confidenceModel->appendRow(newRow);

            }

            //perform string formatting on the numerical items
            ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_3DMAJ_COLUMN,new NumberFormatDelegate(this));
            ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_2DMAJ_COLUMN,new NumberFormatDelegate(this));
            ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_VERT_COLUMN,new NumberFormatDelegate(this));
            if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect)
            {
               ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_3DMAJRR_COLUMN,new NumberFormatDelegate(this));
               ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_2DMAJRR_COLUMN,new NumberFormatDelegate(this));
               ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_VERTRR_COLUMN,new NumberFormatDelegate(this));
            }
            else
            {
               ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_3DMAJRR_COLUMN,new StringFormatDelegate(this));
               ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_2DMAJRR_COLUMN,new StringFormatDelegate(this));
               ui->tablePoints->setItemDelegateForColumn(POINT_TABLE_VERTRR_COLUMN,new StringFormatDelegate(this));
            }

            //initial display sorted by RSS Adj
            ui->tablePoints->sortByColumn(POINT_TABLE_3DMAJ_COLUMN,Qt::DescendingOrder);
            ui->tablePoints->horizontalHeader()->setSortIndicator(POINT_TABLE_3DMAJ_COLUMN,Qt::DescendingOrder);
            //resizeEvent(NULL);//added because issue with TopResid table (see above)
            setConfidenceTableColumnWidths();
        }

        //honor any selections in the tree view prior to hitting F5 (part of Fix to Bug #1483)
        QItemSelection dummy;
        emit ui->treeView->selectionModel()->selectionChanged(dummy, dummy);
    }

    catch(...)//problem parsing .bin file.  Possible eigen failure
    {
        clearOutputTab();
        pushError(QString("Failure parsing HDF5 file.  Unable to map outputs to inputs."));
        return false;
    }

    return true;
}

//Enable/disable GUI elements that can launch subprocesses (i.e. proc member)
void MainWindow::toggleGUISubprocessElements(bool enable)
{
    // Project Menu
    ui->actionNew->setEnabled(enable);
    ui->actionOpen->setEnabled(enable);
    ui->RecentMenu->setEnabled(enable);
    ui->actionConfigure->setEnabled(enable);
    ui->actionImport_IOB->setEnabled(enable);
    ui->actionSave->setEnabled(enable);
    ui->actionSaveAs->setEnabled(enable);
    ui->actionGenerate_Initial_Positions->setEnabled(enable);
    ui->actionReload->setEnabled(enable);
    ui->actionClose->setEnabled(enable);
    ui->actionNewGeoidFile->setEnabled(enable);
    ui->actionCalculate_Adjustment->setEnabled(enable);
    ui->actionAbort_Adjustment->setEnabled(!enable);

    // Import Menu
    ui->menuImport->setToolTipsVisible(enable);
    ui->actionInsertLSAInclude->setEnabled(enable);
    ui->actionInsertIOBInclude->setEnabled(enable);
    ui->actionInsertGSIInclude->setEnabled(enable);
    ui->actionInsertPPPInclude->setEnabled(enable);
    ui->actionInsertTrimbleGPSInclude->setEnabled(enable);
    ui->actionInsertLeicaGNSSInclude->setEnabled(enable);
    ui->actionInsertSetsOfAnglesInclude->setEnabled(enable);
}

bool MainWindow::runProcess(QString process, QStringList args)
{
    proc = new QProcess(this);
    proc->connect( proc, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished, this, &MainWindow::onProcessEnd);
    proc->connect( proc, SIGNAL( readyReadStandardOutput() ),    this, SLOT( onProcessStdOut() ) );
    proc->connect( proc, SIGNAL( readyReadStandardError() ),     this, SLOT( onProcessStdErr() ) );
    proc->connect( proc, SIGNAL( error(QProcess::ProcessError)), this, SLOT( onProcessError() ) );
//  TBD: Add back in. Removed the above error handler connect because of seeing this at runtime:
//QObject::connect: No such slot MainWindow::onProcessError(QString process) in /home/dtucker.snow/source/lsa/src/gui/MainWindow.cpp:614
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    toggleGUISubprocessElements(false);
    proc->start(process,args);
    if(!proc->waitForStarted(TIMEOUT_START_MILLISECONDS))//blocks GUI for this long
        return false;
    else
        return true;
}

QString MainWindow::convertIOBProject(QString fileName)
{
    return convertIOB(fileName, true);
}

QString MainWindow::convertIOBFile(QString fileName)
{
    return convertIOB(fileName, false);
}

QString MainWindow::convertIOB(QString fileName, bool isProject)
{
    QString sourceFileName = getFileNameFromFullPath(fileName);
    QString targetProjDir = QString::fromStdString(getPathWithoutFileName(currentFile.toStdString()));
    QString lsaFileName = "";
    QString warnFileName = "";
    QStringList arguments;

    arguments << "--iob" << fileName;
    if (isProject)
    {
        lsaFileName = fileName.left(fileName.lastIndexOf(".")) + ".proj";
        warnFileName = fileName.left(fileName.lastIndexOf(".")) + ".wrn";
    }
    else
    {
        lsaFileName = targetProjDir + "/" + sourceFileName + ".lsa";
        warnFileName = targetProjDir + "/" + sourceFileName + ".wrn";
    }

    if(!isTestHarness())
    {
        bool overWriteStatus = checkOverwrite(lsaFileName);
        if(overWriteStatus)
        {
            QMessageBox::StandardButton overWriteChoice = giveOverwriteMessage(lsaFileName);
            if(overWriteChoice == QMessageBox::Cancel)
            {
                return QString("");
            }
        }
    }

    arguments << "--lsa" << lsaFileName;
    arguments << "--warn" << warnFileName;
    if (isProject)
    {
        arguments << "--project";
        pushBreakerStart(QString("Open GeoLab Project"));
    }
    else
    {
        arguments << "--projdir" << targetProjDir;
    }

    pushBreakerStart(QString("Import Include from IOB"));
    procType = PROCESS_IOB2LSA;
    if(runProcess(iob2lsaPath,arguments))
    {
        QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        int CLITimeoutMilliseconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt()*1000;
        if(!proc->waitForFinished(CLITimeoutMilliseconds))
        {
            cleanupProcess(QString("iob2lsa"));
            return QString("");//unable to convert .iob file
        }

        if(isProject)
            copyConfigFile(lsaFileName);

        return lsaFileName;
    }
    else
        return QString("");//unable to convert .iob file
}

void MainWindow::cleanupProcess(QString CLIprocessName)
{
    suppressErrorNotification = true;
    proc->kill();
    procType = PROCESS_NONE;
    QApplication::restoreOverrideCursor();
    pushError(CLIprocessName + QString(" terminated - exceeded timeout.  To increase the allowable run-time for CLIs, select Preferences->User Preferences."));
    //restore menu options to launch subprocesses
    toggleGUISubprocessElements(true);
}

QString MainWindow::convertInstrumentFile(QString filename, lsa::CONVERTER_TYPE fileType)
{
    QFileInfo info(configFileName);
    QString cfgFileName;
    QString lsaFileName;
    QString wrnFileName;
    QString instrumentFileName = getFileNameFromFullPath(filename);
    QStringList arguments;

    if(currentFile.isEmpty() || isFileWithinProject(filename))//Fix to Bug #1052
    {
        lsaFileName = QString::fromStdString(getPathWithoutFileName(filename.toStdString()));
    }
    else
    {
        lsaFileName = QString::fromStdString(getPathWithoutFileName(currentFile.toStdString()));
    }
    cfgFileName = info.dir().path() + QString("/converter.cfg");
    wrnFileName = lsaFileName + QString("/") + instrumentFileName + QString(".wrn");
    lsaFileName += QString("/") + instrumentFileName + QString(".lsa");

    //create or update the converter.cfg file
    createConverterConfigFile(cfgFileName);

    arguments << converterPath;
    arguments << "--in"      << filename;
    arguments << "--out"     << lsaFileName;
    arguments << "--cfgfile" << cfgFileName;
    arguments << "--wrnfile" << wrnFileName;

    switch(fileType)
    {
        case lsa::CONVERTER_TYPE_SETSOFANGLES:
            pushBreakerStart(QString("Import Include from Sets of Angles log"));
            arguments << "--type" << "SETS_OF_ANGLES";
            break;
        case lsa::CONVERTER_TYPE_TRIMBLETDEF:
            pushBreakerStart(QString("Import Include from Trimble Data Exchange"));
            arguments << "--type" << "TRIMBLE";
            break;
        case lsa::CONVERTER_TYPE_LEICAGNSS:
            pushBreakerStart(QString("Import Include from Leica Geo Office"));
            arguments << "--type" << "LEICA_GNSS";
            break;
        case lsa::CONVERTER_TYPE_TRIMBLEROUNDS:
            pushBreakerStart(QString("Import Include from Trimble Rounds"));
            arguments << "--type" << "TRIMBLE_ROUNDS";
            break;
        case lsa::CONVERTER_TYPE_GSI:
            pushBreakerStart(QString("Import Include from Leica GSI"));
            arguments << "--type" << "GSI";
            break;
        case lsa::CONVERTER_TYPE_PPP:
            pushBreakerStart(QString("Import Include from grape|merge output"));
            arguments << "--type" << "PPP";
            break;
        case lsa::CONVERTER_TYPE_OPUS:
            pushBreakerStart(QString("Import Include from OPUS output"));
            arguments << "--type" << "OPUS";
            break;
    }

    procType = PROCESS_CONVERTER;
    if(python3Path.isEmpty())
    {
        pushError(QString("Error - unable to find python.exe in SALSA installation folder.  Unable to import instrumentation output."));
        removeConverterConfigFile(cfgFileName);
        return QString("");//unable to convert .log file
    }
    else
    {
        if(runProcess(python3Path,arguments))
        {
            QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
            int CLITimeoutMilliseconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt()*1000;
            if(!proc->waitForFinished(CLITimeoutMilliseconds))
            {
                cleanupProcess(QString("Converter script"));
                removeConverterConfigFile(cfgFileName);
                return QString("");//unable to convert instrument file
            }
            else
            {
                removeConverterConfigFile(cfgFileName);
                return lsaFileName;
            }
        }
        else
        {
            removeConverterConfigFile(cfgFileName);
            return QString("");//unable to convert instrument file
        }
    }
}

void MainWindow::removeConverterConfigFile(QString filename)
{
    QFileInfo check_file(filename);

    if (check_file.exists() && check_file.isFile())
    {
        QFile tmp_file(filename);
        tmp_file.remove();
    }
}

QString MainWindow::openInputGeoidFile (void){
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    // filter means only select files of that specific type
    QString selfilter = tr("Custom Geoid Files (*.geoid)","IOB Project Files (*.iob)");
    // search beginning
    QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();
    // open up file browser, retrieve filename on OK/close
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open geoid-compatible file"), lastLSAPath, "Custom Geoid Files (*.geoid);; IOB with Geoid Heights (*.iob)", &selfilter);
    return fileName;
}

void MainWindow::convertToGeoidFile(){
    QString fileName = openInputGeoidFile();
    if (fileName.isEmpty())
        return;
    // script name with path, file to be converted with path
    QString scriptDir = QDir::cleanPath(QString(scriptsPath  + "/geoid/generate_custom_geoid.py"));
    QString egmFile = QDir::cleanPath(QString(scriptsPath  + "/../data/geoid/egm2008_2.5m.und"));
    string projDir = getPathWithoutFileName(currentFile.toStdString())+"/";
    QString destDir = QDir::cleanPath(QString::fromStdString(projDir+"geoid/"));
    QStringList arguments;
    arguments<< scriptDir
             <<"--data"<<fileName
             <<"--outDir"<<destDir
             <<"--cgeoid"<<cgeoidPath
             <<"--egmFile"<<egmFile;

    procType = PROCESS_CUSTOM_GEOID;
    if(python3Path.isEmpty() )
        pushError(QString("Error - unable to find python.exe in SALSA installation folder.  Unable to import instrumentation output."));
    else{
        runProcess(python3Path,arguments);
//        emit converterFinished();
    }
}

void MainWindow::clearOutputTab()
{
    ui->valueProcessingTime->setText("--");
    ui->valueNumDOF->setText("--");
    ui->valueNumObs->setText("--");
    ui->valueNumUnknowns->setText("--");
    ui->valueMaxExtMag->setText("<font color = black>--</font>");
    ui->labelMaxExtMag->setText("<font color = black>Max Ext Mag (m):</font>");

    ui->valueConvergenceHeader->setText("<font color = black>--</font>");
    ui->valueLastUpdate->setText("<font color = black>--</font>");
    ui->valueConvLimit->setText("--");
    ui->valueIterations->setText("--");
    ui->valueMaxIterations->setText("--");

    ui->valueAPV->setText("--");
    ui->valueChiSqrHeader->setText("<font color = black>--</font>");
    ui->valueChiSqr->setText("--");

    topResidModel->clear();//also removes headers, have to re-add
    confidenceModel->clear();//also removes headers, have to re-add
    addOutputTableHeaders();
    setOutputTableColumnWidths();

    ui->labelPointConfRegions->setText("Point Confidence Regions");
}

/// Open a file browser for the user to select a new .lsa file.  Update the treeview with the file contents.
std::vector<std::string> MainWindow::loadLsaFile(QString filename, bool convertingFromIOB)
{
    bool beginStatus = setSuppressGuiUpdates(true);
    DebugTimer timer(false);

    currentFile = filename;
    QString currentDir = QString::fromStdString(getPathWithoutFileName(currentFile.toStdString()));
    QDir::setCurrent(currentDir);
    bool readHDF5OK = false;

    // Cache the filename
    if (!filename.isEmpty())
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        settings.setValue(lsa::QSETTINGS_LASTPATH, currentDir);
        updateRecentProjectsMenu(filename);

    }

    //these methods need to be implemented in-order
    //removeLegacyAutogenPointsFile() selects records, which requires the selectionModel exist, which is set in setupTreeView.
    setupGuiModel();

    //inform user of missing includes
    parseLSAFileErrorsAndWarnings();

    // Set up the Record Editor
    recordEditor->setGuiModel(guiModel);
    // Set up the tree view
    setupTreeView();
    //handle legacy AutogeneratedPoints.lsa file from v1.0.0 to v1.1.2
    if(guiModel->removeLegacyAutogenPointsFile())
        pushWarning(QString("Removed outdated AutogeneratedPoints.lsa file."));

    ui->findFilterWidget->setGuiModel(guiModel);

    // Clear the output tab
    clearOutputTab();

    histogramDialog->clearHistogram();

    //clear the Final Positions View
    adjustedPositions->clear();

    //clear the Station Data Dialog
    stationData->clear();

    if(!filename.isEmpty())
    {
        //assign the config file name, in case we've opened an existing lsa file
        //and we're not creating a new project or importing an iob project
        copyConfigFile();

        //eliminate obsolete parameters, replace them with updated parameters
        updateLegacyConfigFile();

        //make sure the config values are populated throughout the application
        applyConfigFile();

        //check for the existence of the customExport directory and populate if necessary
        populateCustomExportDir(currentDir);

        //generate map of unique IDs for measurement records to corresponding tree view QModelIndexes
        generateUniqueIDMapToLSARecordPointers();

        //generate the hash code for the current data
        currentHashCode = guiModel->hashTree();

        //attempt to load the last HDF5 file, if it exists
        lastSolverRunType = PROCESS_LSASOLVER;//Fix to Bug #1284
        //Do this before setupMapWidget to get a priori points into the map
        if(!convertingFromIOB)//don't read past output upon converting from IOB.  Start fresh.
        {
            readHDF5OK = readHDF5File(true);
        }
    }

    // Enable/disable menu actions based on the selected record(s)
    addCustomMenu();
    enableDisableMenuActions();
    ui->actionReload->setEnabled(!filename.isEmpty());
    ui->actionConfigure->setEnabled(true);
    ui->actionFind->setEnabled(true);
    ui->actionFind_Next->setEnabled(true);
    ui->actionFindNextWarning->setEnabled(true);
    ui->actionFind_Previous->setEnabled(true);

    // Update up the MapWidget
    mapWidget->setGuiModel(guiModel);

    if(readHDF5OK)
    {
        mapWidget->initMapAdjustedCoordinates();
    }
    else
    {
        mapWidget->initMapInitialCoodinates();
    }

    setSuppressGuiUpdates(beginStatus);
    mapWidget->setHome(true);

    // Clear the Find/Filter widget
    ui->findFilterWidget->clearControls();

    // We are done loading everything, so update the UI one time
    updateWidgets();

    // Set the model saved flag to true
    QModelIndex rootIndex = guiModel->index(0,0);
    guiModel->clearDescendantIncludeIsModified(rootIndex);

    // Reset file modified timestamps
    timeStampsAreInitialized = false;
    fileChangedTimes.clear();

    //alert user of missing includes, and then clear the list
    popupForMissingIncludes();

    if(failedIncludeList.size()>0)
    {
        if(!isTestHarness())
        {
            QString message("Unable to parse");

            for(int i=0;i<failedIncludeList.size();i++)
            {
                message += QString(" ") + failedIncludeList.at(i);
            }
            if(message.size() > lsa::MAX_ERROR_STRING_SIZE)
            {
                message = message.left(lsa::MAX_ERROR_STRING_SIZE) + QString("...");
            }
            QMessageBox *notifyUser = new QMessageBox(QMessageBox::Critical, // icon
                                                      QString("Included file not found!"), // title
                                                      message, // text
                                                      QMessageBox::Ok, // buttons
                                                      (QWidget*)this, // parent
                                                      Qt::WindowStaysOnTopHint);

            notifyUser->exec();//blocks
            delete notifyUser;
        }
        failedIncludeList.clear();
    }

    QModelIndex parentIndex = guiModel->getNextIndex();
    pushParseWarningsForDescendants(parentIndex);

    unsigned int numDXYZ_HGHT = getNumDXYZwithHGHT();
    if(numDXYZ_HGHT != 0)
    {
        std::string dxyzWarning = "Warning - " + std::to_string(numDXYZ_HGHT) + " DXYZ records have a height correction applied to them.";
        pushWarning(dxyzWarning);
    }

    return guiModel->getBadRecords();
}

void MainWindow::setupGuiModel()
{
    // Get a new GuiModel
    GuiModel *oldModel = guiModel;
    if (currentFile.isEmpty())
    {
        guiModel = new GuiModel();
    }
    else
    {
        guiModel = new GuiModel(currentFile.toStdString() ); // read the .lsa file into memory
    }

    // Delete the old model if it exists
    if (oldModel) delete oldModel;
    oldModel = NULL;

    guiModel->synchGuiModelToLSATree();

    if (surveyorWorkspace)
    {
        surveyorWorkspace->setProjectModel(guiModel);
    }

    connect(guiModel, SIGNAL(dataChanged(QModelIndex,QModelIndex, QVector<int>)), this, SLOT(handleDataChange(QModelIndex,QModelIndex,QVector<int>)));
    connect(guiModel, SIGNAL(pushWarning(QString)), this, SLOT(pushWarning(QString)));
    connect(guiModel, SIGNAL(pushError(QString)), this, SLOT(pushError(QString)));
    // Handle when something (e.g. a file) is dropped on the tree view
    connect(guiModel, &GuiModel::signalDataDroppedOnTree, this, &MainWindow::handleDataDroppedOnTree);
}

void MainWindow::setupSurveyorWorkspace()
{
    if (surveyorWorkspace)
    {
        return;
    }

    surveyorWorkspace = new SurveyorWorkspace(guiModel, this);
    addDockWidget(Qt::RightDockWidgetArea, surveyorWorkspace);
    ui->menuView->addAction(surveyorWorkspace->toggleViewAction());

    connect(surveyorWorkspace, &SurveyorWorkspace::runAdjustmentRequested,
            this, [this]() { calculateAdjustment(); });
}

void MainWindow::setupProgressBar()
{
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(0);
    ui->progressBar->hide();
}

void MainWindow::setupStatusWindow()
{
    // Set the font to fixed width to account for variable-length strings
    // in the start breakers    connect(guiModel, SIGNAL(pushWarning(QString)), this, SLOT(pushWarning(QString)));
    connect(guiModel, SIGNAL(pushError(QString)), this, SLOT(pushError(QString)));

    // TODO: Evaluate SALSA's general font paradigm #1193
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    int pSize = getIdealFontSize();
    font.setPointSize(pSize);
    font.setWeight(QFont::Normal);
    ui->statusWindow->setFont(font);
}

void MainWindow::setupTreeView()
{
    ui->treeView->setAlternatingRowColors(true);

    // Style sheets are now set in main.cpp

    // Set the font
    QFont font(lsa::TREE_VIEW_FONT);
    font.setStyleHint(QFont::Courier);
    int pSize = std::max(getIdealFontSize(), 10);
    font.setPointSize(pSize);
    font.setWeight(QFont::Normal);
    font.setKerning(false);
    font.setFixedPitch(true);
    ui->treeView->setFont(font);

    // Set the model on the treeview
    ui->treeView->setModel(guiModel);

    // Set a reference to the selection model in the guiModel;
    guiModel->setSelectionModel(ui->treeView->selectionModel());

    // Connect the selectionChanged signal
    connect(ui->treeView->selectionModel(), SIGNAL(selectionChanged( QItemSelection, QItemSelection)), this, SLOT(handleItemSelectionChanged(QItemSelection, QItemSelection)) );

    // Connect the column width changed signal
    connect(ui->treeView->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(handleTreeViewColumnWidthChange(int,int,int)) );
    connect(ui->actionExpand_All, SIGNAL(triggered()), ui->treeView, SLOT(expandAll()) );
    connect(ui->actionCollapse_All, SIGNAL(triggered()), this, SLOT(handleTreeViewCollapseAll()) );

    // Set column widths to contents so users don't have to adjust col widths to see everything.
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    int columnWidth0 = settings.value(QSETTINGS_TREEVIEW_COLUMN_WIDTH_0, 130).toInt();
    int columnWidth1 = settings.value(QSETTINGS_TREEVIEW_COLUMN_WIDTH_1, 250).toInt();
    int columnWidth2 = settings.value(QSETTINGS_TREEVIEW_COLUMN_WIDTH_2, 150).toInt();
    int columnWidth3 = settings.value(QSETTINGS_TREEVIEW_COLUMN_WIDTH_3, 150).toInt();
    int columnWidth4 = settings.value(QSETTINGS_TREEVIEW_COLUMN_WIDTH_4, 150).toInt();
    ui->treeView->setColumnWidth(0, columnWidth0);
    ui->treeView->setColumnWidth(1, columnWidth1);
    ui->treeView->setColumnWidth(2, columnWidth2);
    ui->treeView->setColumnWidth(3, columnWidth3);
    ui->treeView->setColumnWidth(4, columnWidth4);

    // Select and expand the top include record
    QModelIndex rootIndex = guiModel->index(0,0);
    ui->treeView->expand(rootIndex);
    ui->treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->treeView->setAcceptDrops(true);
    ui->treeView->setDragDropMode(QAbstractItemView::DropOnly);
    ui->treeView->setDropIndicatorShown(true);

    updateRecordVisibility();
    updateColumnSpans();

    return;
}

void MainWindow::updateRecordVisibility()
{
    // Store the current showComments settings to use next time the application starts up
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(guiModel->QSETTINGS_SHOWCOMMENTS, ui->actionShow_Comments->isChecked());

    QModelIndex index = guiModel->getNextIndex();
    index = guiModel->getNextIndex(index); // root include is always visible

    while ( index.isValid() )
    {
        bool recordIsHidden = guiModel->isRecordHidden(index);
        QModelIndex parentIndex = guiModel->parent(index);
        setTreeViewRowHidden(index.row(), parentIndex, recordIsHidden);

        if (!recordIsHidden)
        {
            // Make all ancestor includes visible
            setParentsVisible(index);
        }

        index = guiModel->getNextIndex(index);
    }
    bTVRowVisibilityChanged = false;

    return;
}

void MainWindow::setParentsVisible(QModelIndex index)
{
    QModelIndex parentIndex = guiModel->parent(index);

    if (parentIndex.isValid())
    {
        setTreeViewRowHidden(index.row(), parentIndex, false);
        setParentsVisible(parentIndex);
    }

    return;
}

void MainWindow::updateRecordEditorVisibility()
{
    // Store the current settings to use next time the application starts up
    bool visible = ui->actionView_RecordEditor->isChecked();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(guiModel->QSETTINGS_VISIBLE_RECORDEDITOR, visible);

    recordEditor->setVisible(visible);
}

void MainWindow::updateMapVisibility()
{
    // Store the current settings to use next time the application starts up
    bool visible = ui->actionView_Map->isChecked();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(guiModel->QSETTINGS_VISIBLE_MAP, visible);

    mapWidget->setVisible(visible);
}

void MainWindow::updateAdjustedPositionsVisibility()
{
    // Store the current settings to use next time the application starts up
    bool visible = ui->actionView_AdjustedPositions->isChecked();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(guiModel->QSETTINGS_VISIBLE_ADJUSTEDPOSITIONS, visible);

    adjustedPositions->setVisible(visible);
}

void MainWindow::updateStationDataVisibility()
{
    bool visible = ui->actionView_StationData->isChecked();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(guiModel->QSETTINGS_VISIBLE_STATIONDATA, visible);

    stationData->setVisible(visible);

    if (visible)
    {
        // If two or more positions are selected, set the first one to From and second one to To
        QModelIndexList selectedRows = guiModel->getSelectedRows();
        QString fromLabel, toLabel;
        bool fromLabelFound = false;
        bool toLabelFound = false;
        foreach (QModelIndex index, selectedRows)
        {
            if (!index.isValid())
                continue;
            LSARecord *lsaRecord = guiModel->getLSARecord(index);
            LSAType lsaType = lsaRecord->getRecType();
            if (lsaType.isPosition())
            {
                QString label = QString::fromStdString(lsaRecord->getLabel());
                if (!label.isEmpty())
                {
                    if (!fromLabelFound)
                    {
                        fromLabel = label;
                        fromLabelFound = true;
                    }
                    else if (!toLabelFound)
                    {
                        toLabel = label;
                        toLabelFound = true;
                        break; // stop looking for positions once from and to are found
                    }
                }
            }
        }

        if (fromLabelFound && toLabelFound)
        {
            stationData->setNewFromTo(fromLabel, toLabel);
        }
    }

    stationData->setupPairTable(currentFile);
}

void MainWindow::updateHistogramVisibility()
{
    bool visible = ui->actionView_Histogram->isChecked();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(guiModel->QSETTINGS_VISIBLE_HISTOGRAM, visible);

    histogramDialog->setVisible(visible);
}

void MainWindow::updateColumnSpans()
{
    QModelIndex index = guiModel->getNextIndex();
    while ( index.isValid() )
    {

        LSAType lsaType = guiModel->getLSARecord(index)->getRecType();

        if (lsaType == LSAType::COMMENT || lsaType == LSAType::UNCR || index == guiModel->index(0,0))
        {
            QModelIndex parentIndex = guiModel->parent(index);
            ui->treeView->setFirstColumnSpanned(index.row(), parentIndex, true);
        }

        index = guiModel->getNextIndex(index);
    }
    return;
}

void MainWindow::onCustomContextMenu(const QPoint &point)
{
    QModelIndex index = ui->treeView->indexAt(point);

    if ( index.isValid() )
    {
        ui->RecordMenu->exec(ui->treeView->mapToGlobal(point));
    }

    ui->RecordMenu->close();
}

void MainWindow::onCustomContextMenuResiduals(const QPoint & point)
{
    // set the menu item as checked based on its column width
    // the user could have hidden (which sets width to 0), reopened salsa, which sets the colum width as 0.
    for(auto& entry : mapResidualsActionToColumn)
    {
        entry.first->setChecked(ui->tableMeasurements->columnWidth(entry.second));
    }

    QMenu menu(this);
    menu.addAction(ui->action_MeasCol);
    menu.addAction(ui->action_From_AtCol);
    menu.addAction(ui->action_ToCol);
    menu.addAction(ui->action_RawCol);
    menu.addAction(ui->action_RelCol);
    menu.addAction(ui->action_StdCol);
    menu.addAction(ui->action_RedundancyCol);
    menu.addAction(ui->action_Int_RelCol);
    menu.addAction(ui->action_Ext_RelCol);
    menu.exec(ui->tableMeasurements->mapToGlobal(point));
}

void MainWindow::onCustomContextMenuPoints(const QPoint & point)
{
    // set the menu item as checked based on its column width
    // the user could have hidden (which sets width to 0), reopened salsa, which sets the colum width as 0.
    for(auto& entry : mapPointsActionToColumn)
    {
        entry.first->setChecked(ui->tablePoints->columnWidth(entry.second));
    }

    QMenu menu(this);
    menu.addAction(ui->action_PointCol);
    menu.addAction(ui->action_3D_majCol);
    menu.addAction(ui->action_2D_majCol);
    menu.addAction(ui->action_VerticalCol);
    menu.addAction(ui->action_3D_maj_RRCol);
    menu.addAction(ui->action_2D_maj_RRCol);
    menu.addAction(ui->action_Vertical_RRCol);
    menu.exec(ui->tablePoints->mapToGlobal(point));
}

void MainWindow::handleTreeViewColumnWidthChange(int columnNumber, int oldWidth, int newWidth)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    switch (columnNumber)
    {
    case 0:
        settings.setValue(QSETTINGS_TREEVIEW_COLUMN_WIDTH_0, newWidth);
        break;
    case 1:
        settings.setValue(QSETTINGS_TREEVIEW_COLUMN_WIDTH_1, newWidth);
        break;
    case 2:
        settings.setValue(QSETTINGS_TREEVIEW_COLUMN_WIDTH_2, newWidth);
        break;
    case 3:
        settings.setValue(QSETTINGS_TREEVIEW_COLUMN_WIDTH_3, newWidth);
        break;
    case 4:
        settings.setValue(QSETTINGS_TREEVIEW_COLUMN_WIDTH_4, newWidth);
        break;
    default:
        break;

    }

    return;
}

void MainWindow::handleTablePointsResized(int columnNumber, int oldWidth, int newWidth)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    switch (columnNumber)
    {
    case 0:
        settings.setValue(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_0, newWidth);
        break;
    case 1:
        settings.setValue(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_1, newWidth);
        break;
    case 2:
        settings.setValue(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_2, newWidth);
        break;
    case 3:
        settings.setValue(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_3, newWidth);
        break;
    case 4:
        settings.setValue(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_4, newWidth);
        break;
    case 5:
        settings.setValue(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_5, newWidth);
        break;
    case 6:
        settings.setValue(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_6, newWidth);
        break;
    default:
        break;

    }

    return;
}

void MainWindow::handleTableMeasurementsResized(int columnNumber, int oldWidth, int newWidth)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    switch (columnNumber)
    {
    case 0:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_0, newWidth);
        break;
    case 1:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_1, newWidth);
        break;
    case 2:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_2, newWidth);
        break;
    case 3:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_3, newWidth);
        break;
    case 4:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_4, newWidth);
        break;
    case 5:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_5, newWidth);
        break;
    case 6:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_6, newWidth);
        break;
    case 7:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_7, newWidth);
        break;
    case 8:
        settings.setValue(QSETTINGS_RESIDUALS_COLUMN_WIDTH_8, newWidth);
        break;
    default:
        break;

    }

    return;
}

void MainWindow::handleCentralSplitterMoved(int position, int index)
{
    QList<int> splitterSizes = ui->centralSplitter->sizes();

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(QSETTINGS_CENTRAL_SPLITTER_SIZE_0, splitterSizes.at(0));
    settings.setValue(QSETTINGS_CENTRAL_SPLITTER_SIZE_1, splitterSizes.at(1));
    settings.setValue(QSETTINGS_CENTRAL_SPLITTER_SIZE_2, splitterSizes.at(2));

    return;
}

void MainWindow::handleOutputTablesSplitterMoved(int position, int index)
{
    QList<int> splitterSizes = ui->splitterOutputTables->sizes();

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_0, splitterSizes.at(0));
    settings.setValue(QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_1, splitterSizes.at(1));

    return;
}

void MainWindow::pushStatus(std::string input)
{
    pushStatus(QString::fromStdString(input));
}

void MainWindow::pushStatus(QString msg)
{
    msg = cleanQString(msg);
    emit updateStatusWindow(false,msg,50,Qt::black);
}

void MainWindow::pushProcessStart(std::string processName)
{
    QString msg = QTime::currentTime().toString(QString("HH:mm:ss.zzz")) + QString(" ") +
            QString::fromStdString(processName) + QString(" starting... ");

    msg = cleanQString(msg);
    emit updateStatusWindow(false,msg,70,Qt::black);
}

void MainWindow::pushKillProcess()
{
   QString msg = "Calculate Adjustment aborted by user.";
   msg = cleanQString(msg);
   emit updateStatusWindow(false,msg,87,Qt::red);

   // Removes `lsasolver.exe crashed unexpectedly` error message
   suppressErrorNotification = true;

   // Removes `FAIL` following the kill process message
   killProcessStatus = true;
}

void MainWindow::pushProcessFail()
{
    //save the last line in the status window and delete it
    QTextCursor cursor = ui->statusWindow->textCursor();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    cursor.select(QTextCursor::LineUnderCursor);
    QString lastMessage = cursor.selectedText();
    cursor.removeSelectedText();
    cursor.deletePreviousChar();
    ui->statusWindow->setTextCursor(cursor);

    //replace the last line with the original plus added status
    QString msg = lastMessage + QString("FAIL");
    msg = cleanQString(msg);
   //if statement added by B. Beal on 08/03/2017 Bug #1441
    if (msg.contains("Error"))
    {
        emit updateStatusWindow(false,msg,70,Qt::red);
    }
    else
    {
        if(killProcessStatus)
        {
           emit updateStatusWindow(false,lastMessage,87,Qt::red);
        }
        else
        {
           emit updateStatusWindow(false,msg,70,Qt::black);
        }
    }
}

void MainWindow::pushProcessComplete()
{
    //save the last line in the status window and delete it
    QTextCursor cursor = ui->statusWindow->textCursor();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    cursor.select(QTextCursor::LineUnderCursor);
    QString lastMessage = cursor.selectedText();
    cursor.removeSelectedText();
    cursor.deletePreviousChar();
    ui->statusWindow->setTextCursor(cursor);

    //replace the last line with the original plus added status
    QString msg = lastMessage + QString("COMPLETED");
    msg = cleanQString(msg);
    emit updateStatusWindow(false,msg,70,Qt::black);
}

void MainWindow::pushBreakerStart(QString menuItemString)
{
    const int SEPARATOR_LENGTH = 60;
    QString separator("");

    for(int i=0;i<SEPARATOR_LENGTH - menuItemString.size();i++)
        separator += QString("-");


    QString msg = QString("--- ") + QDateTime::currentDateTime().toString() +
                   QString(" ") + menuItemString + QString(" ") + separator;

    msg = cleanQString(msg);
    emit updateStatusWindow(false,msg,50,Qt::darkGreen);
}

void MainWindow::pushBreakerEnd()
{
    reportCLIWarningsAndErrors(true);
    QString msg = QString("--- ") + QDateTime::currentDateTime().toString() +
                   QString(" -------------------------------------------------------------");

    msg = cleanQString(msg);
    emit updateStatusWindow(false,msg,50,Qt::darkGreen);
}

QString MainWindow::cleanQString(QString msg)
{
    std::string msg_string = msg.toStdString();
    gnsstk::StringUtils::stripTrailing(msg_string,'\r');
    gnsstk::StringUtils::stripTrailing(msg_string,'\f');
    gnsstk::StringUtils::stripTrailing(msg_string,'\n');
    return QString::fromStdString(msg_string);
}

void MainWindow::pushError(std::string input)
{
    pushError(QString::fromStdString(input));
}

void MainWindow::pushError(QString msg)
{
    msg = cleanQString(msg);
    //Make URL for converter warning file
    if(procType == PROCESS_IOB2LSA && msg.contains(" See "))//look for reference to warnings file
    {
        //extract the warning file name and create a URL to it
        int splitPoint = msg.indexOf(" See ");
        int pathStart = splitPoint + 5; // 5 = length of string " See ". If that string is changed, update the length accordingly
        int pathEnd = msg.indexOf(" for details.");
        QString firstPart = msg.left(splitPoint);
        QString filePath = msg.mid(pathStart, pathEnd - pathStart);
        msg = firstPart + " Click <a href=\"" + QString("file:") + filePath + "\">here</a> for details.";
        //text formatting doesn't get applied for text with URL.  Wrap with HTML tags.
        msg = QString("<strong><font color=red>") + msg + QString("</font></strong>");
    }
    else if( (procType == CUSTOM_CONVERTER_SCRIPT || procType == PROCESS_CONVERTER) && msg.contains(" Refer to "))
    {
        //extract the warning file name and create a URL to it
        int splitPoint = msg.indexOf(" Refer to ");
        int pathStart = splitPoint + 10; // 10 = length of string " Refer to ". If that string is changed, update the length accordingly
        int pathEnd = msg.indexOf(" for details.");
        QString firstPart = msg.left(splitPoint);
        QString filePath = msg.mid(pathStart, pathEnd - pathStart);
        msg = firstPart + " Click <a href=\"" + QString("file:") + filePath + "\">here</a> for details.";
        //text formatting doesn't get applied for text with URL.  Wrap with HTML tags.
        msg = QString("<strong><font color=red>") + msg + QString("</font></strong>");
    }

    emit updateStatusWindow(false,msg,87,Qt::red);
}

void MainWindow::pushWarning(std::string input)
{
    pushWarning(QString::fromStdString(input));
}

void MainWindow::pushWarning(QString msg)
{
    msg = cleanQString(msg);

    // get the numer of unused positions
    QStringList msgStr = msg.split("\n");
    foreach(QString str, msgStr)
    {
        if(str.contains(" unused positions"))
        {
            QStringList unusedPosStr = str.split(" ");
            numOfUnusedPositions = unusedPosStr[5].toInt();
        }
        else if(str.contains(" zero redundancy"))
        {
           QStringList zeroRedunStr = str.split(" ");
           if(zeroRedunStr.size() < 7) // Check to prevent out-of-bounds error
           {
              break;
           }
           else
           {
              numOfZeroRedun = zeroRedunStr.value(7).toInt();
           }
        }
    }

    //Make URL for converter warning file
    if(procType == PROCESS_IOB2LSA)
    {
        QStringList messages = msg.split("\n");
        foreach(QString message, messages)
        {
            if(message.indexOf(" See ") != -1)
            {
                //extract the warning file name and create a URL to it
                int splitPoint = message.indexOf(" See ");
                int pathStart = splitPoint + 5; // 5 = length of string " See ". If that string is changed, update the length accordingly
                int pathEnd = message.indexOf(" for details.");
                QString firstPart = message.left(splitPoint);
                QString filePath = message.mid(pathStart, pathEnd - pathStart);
                message = firstPart + " Click <a href=\"" + QString("file:") + filePath + "\">here</a> for details.";
                //text formatting doesn't get applied for text with URL.  Wrap with HTML tags.
                message = QString("<strong><font color=orange>") + message + QString("</font></strong>");
            }
            QColor orange(255,128,0);
            emit updateStatusWindow(false,message,87,orange);
        }
    }
    else if( (procType == CUSTOM_CONVERTER_SCRIPT || procType == PROCESS_CONVERTER) && msg.contains(" Refer to "))
    {
        if(msg.indexOf(" Refer to ") != -1)
        {
            //extract the warning file name and create a URL to it
            int splitPoint = msg.indexOf(" Refer to ");
            int pathStart = splitPoint + 10; // 10 = length of string " Refer to ". If that string is changed, update the length accordingly
            int pathEnd = msg.indexOf(" for details.");
            QString firstPart = msg.left(splitPoint);
            QString filePath = msg.mid(pathStart, pathEnd - pathStart);
            msg = firstPart + " Click <a href=\"" + QString("file:") + filePath + "\">here</a> for details.";
            //text formatting doesn't get applied for text with URL.  Wrap with HTML tags.
            msg = QString("<strong><font color=orange>") + msg + QString("</font></strong>");

            QColor orange(255,128,0);
            emit updateStatusWindow(false,msg,87,orange);
        }
    }
    else
    {
        QStringList messages = msg.split("\n");
        foreach(QString message, messages)
        {
            // Push message to view solver output if there is more than 5 warnings
            if(message.contains(" unused positions") && numOfUnusedPositions > 5)
            {
                message = message + " Please view SOLVER OUTPUT for details.";
            }

            // Do not print unused position warning if there are more than 5
            if(message.contains(" is unused.") && numOfUnusedPositions > 5) break;

            // Push message to view solver output if there is more than 5 redundancy warnings
            if(message.contains(" zero redundancy") && numOfZeroRedun > 5)
            {
                message.append(" Please view SOLVER OUTPUT for details.");
            }

            // Do not print redundancy warnings if there are more than 5
            if(message.contains(" insufficient redundancy") && numOfZeroRedun > 5) break;

            QColor orange(255,128,0);
            emit updateStatusWindow(false,message,87,orange);
        }
    }

    if(msg.indexOf("Could not open included file:") != -1)
    {
        QString fileName = msg.mid(msg.indexOf("file:")+6);
        string fileNameStr = gnsstk::StringUtils::splitWithDoubleQuotes(fileName.toStdString(),' ')[0];
        fileName = QString::fromStdString(fileNameStr);
        failedIncludeList.push_back(fileName);
    }
}

void MainWindow::pushToStatusWindow(bool clear, QString msg, int fontWeight, QColor fontColor)
{
    if(clear)
        ui->statusWindow->clear();
    else if(!msg.isEmpty())
    {
        ui->statusWindow->moveCursor(QTextCursor::End);//necessary workaround for QT Bug 539
        ui->statusWindow->setTextColor(fontColor);
        ui->statusWindow->setFontWeight(fontWeight);
        msg = msg.simplified();

        ui->statusWindow->append(msg);
    }
}

void MainWindow::handleTreeViewCollapseAll()
{
    // collapse everything
    ui->treeView->collapseAll();

    // Expand the project record
    QModelIndex rootIndex = guiModel->index(0,0);
    ui->treeView->expand(rootIndex);
}

void MainWindow::handleReplaceInclude(QModelIndex index, QStringList filenameList)
{
    guiModel->undoStack()->beginMacro("Replacing Include.");
    // Cache the index to delete
    QPersistentModelIndex indexToDelete(index);

    // Insert the new include
    QModelIndex insertedIndex = convertFilesAndAddToProject(filenameList, lsa::CONVERTER_TYPE_NONE, index.parent(), index.row());

    // Delete the old include
    if (insertedIndex.isValid())
    {
        deleteRecord(indexToDelete);
        guiModel->select(insertedIndex);
    }
    else
    {
        // the insert failed, so don't delete the old record.  Just make sure it is still selected.
        guiModel->select(index);
    }
    guiModel->undoStack()->endMacro();
}

void MainWindow::handleDataDroppedOnTree(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parentIndex)
{
    // We only care about the column 0 index
    auto parentIndexCpy = parentIndex.sibling(parentIndex.row(), 0);

    auto urls = data->urls();
    // While no file is open
    if(currentFile.isEmpty())
    {
        // If they are dropping whole files
        if(urls.size() > 1)
        {
            createNewProject();
        }
        if(urls.size() == 1)
        {
            QFileInfo fileInfo(urls[0].toLocalFile());
            auto filePath = fileInfo.filePath();

            lsa::CONVERTER_TYPE converterType = getConverterTypeForFile(fileInfo);
            // IOB files have their own conversion method
            if(converterType == lsa::CONVERTER_TYPE_IOB)
            {
                convertAndLoadIOBProject(filePath);
            }
            else
            {
                // causes warning "QFileInfo::absolutePath: Constructed with empty filename" in tests
                filePath = convertFile(filePath, converterType);
            }

            // Try to open the converted file
            fileInfo = filePath;
            auto extension = fileInfo.suffix();
            extension = extension.toLower();
            if(extension == "lsa" || extension == "proj")
            {
                loadLsaFile(filePath);
            }
            return;
        }
    }

    if (!currentFile.isEmpty())
    {
        // WHILE A FILE IS ALREADY OPEN
        for(const auto& url : urls)
        {
            auto fileInfo = QFileInfo(url.toLocalFile());
            auto extension = fileInfo.suffix();
            extension = extension.toLower();

            // Logic to get the correct position for this include
            int rowCpy = row;
            correctIncludeDropPosition(row, rowCpy, parentIndexCpy);
            if(guiModel->isAutogenIndex(parentIndexCpy))
            {
                pushWarning(QString("Warning - Records cannot be added to an autogenerated Include."));
                return;
            }

            // Try to infer the type of file
            lsa::CONVERTER_TYPE converterType = getConverterTypeForFile(fileInfo);

            // This adds the file as an lsa file to the project, at the position we figured out above
            parentIndexCpy = convertFilesAndAddToProject({fileInfo.filePath()}, converterType, parentIndexCpy, rowCpy);

        }

        // If the user is dropping plain text, not a file
        if (!data->hasUrls() && data->hasText())
        {
            std::stringstream ss;
            ss << data->text().toStdString();

            int rowCpy = row;
            correctIncludeDropPosition(row, rowCpy, parentIndexCpy);
            if(guiModel->isAutogenIndex(parentIndexCpy))
            {
                pushWarning(QString("Warning - Records cannot be added to an autogenerated Include."));
                return;
            }
            guiModel->insertFromStreamAfter(ss, parentIndexCpy, rowCpy);
        }
    }

}

void MainWindow::updateWidgets()
{
    guiModel->validateModelAndRegenerateMaps();

    updateColumnSpans();

    // Update Record Editor
    recordEditor->update();

    // Update tree view
    if(bTVRowVisibilityChanged)
    {
        updateRecordVisibility();
    }
    ui->treeView->update();

    // Update num selected records
    int numSelectedRecords = guiModel->selectionModel->selectedRows().count();
    QString numString = QString::fromStdString( gnsstk::StringUtils::asString(numSelectedRecords) );
    ui->labelNumSelectedRecords->setText(numString);
}

void MainWindow::insertNewInclude(QString testFileName)
{
    QString fileName = createNewEmptyLSAFile(testFileName);

    if (fileName.isEmpty())
    {
        return;
    }
    // fix to bug #1075: send through the insertIndclude path where subdirectory handling is already correct
    else
    {
        // this setup based on the use of insertInclude in selectAndInsertIncludes
        QStringList fileList;
        fileList << fileName;
        QModelIndex currentIndex = guiModel->getSingleSelectedRecord();
        int rowNum = currentIndex.row() + 1;
        convertFilesAndAddToProject(fileList, lsa::CONVERTER_TYPE_NONE, currentIndex.parent(), rowNum);
    }

}

QModelIndex MainWindow::convertFilesAndAddToProject(QStringList fileNameList, lsa::CONVERTER_TYPE fileType, QModelIndex parentIndex, int rowNum)
{
    if(fileType == lsa::CONVERTER_TYPE_NULL)
    {
        return QModelIndex();
    }

    QModelIndex insertedIndex;

    QStringList overWriteFiles;
    QMessageBox::StandardButton overWriteChoice = QMessageBox::Yes;

    // performCheck will check for overrides under two conditions.
    // One, this action isn't being called from the test harness
    // Two, the converter type is some other value besides none.
    bool performCheck = fileType != lsa::CONVERTER_TYPE_NONE;
    if(performCheck)
    {
        overWriteFiles = checkOverwrite(fileNameList);
        if(!overWriteFiles.isEmpty())
        {
            overWriteChoice = giveOverwriteMessage(overWriteFiles);
        }
    }

    // Convert files from _ to .lsa
    foreach (QString fileName, fileNameList)
    {
        if (fileName.isEmpty() )
        {
            continue;
        }
        else if(performCheck)
        {
            if((overWriteChoice == QMessageBox::Cancel) && overWriteFiles.contains(fileName) )
            {
                continue;
            }
        }
        QString absolutePath;
        if (overWriteChoice == QMessageBox::Yes)
        {
            absolutePath = convertFile(fileName, fileType);
        }
        else
        {
            // The user does not want to overwrite their previously converted file. Including correct
            // file path.
            QFileInfo fileInfo(fileName);
            QString convertedBaseLsaName = fileInfo.fileName() + ".lsa";
            auto projdir = getPathWithoutFileName(currentFile.toStdString());
            absolutePath = QString::fromStdString(projdir) + "/" + convertedBaseLsaName;
        }
        // #1532|This is to prevent blank includes being created when the user declines to
        // overwrite an existing converted .iob.
        if(absolutePath.isEmpty())
        {
            continue;
        }

        // Create copies of the modifiers maps that can later be used to de-duplicate labels
        // Fix to Bug #1004

        LSAUNCRMap oldUncrMap = guiModel->getFullUncrMap();
        LSAHGHTMap oldHeightMap = guiModel->getFullHeightMap();
        LSAVSCAMap oldVarMap = guiModel->getFullVarScalingMap();
        LSADirGroupMap oldDirGroupMap = guiModel->getFullDirGroupMap();

        // Make sure parentIndex is valid
        if (!parentIndex.isValid())
        {
            QModelIndex selectedIndex = guiModel->getSingleSelectedRecord();

            QModelIndex rootProjectIndex = guiModel->getNextIndex();
            if (selectedIndex == rootProjectIndex)
            {
                rowNum = -1;
            }
            else
            {
                parentIndex = selectedIndex.parent();
                rowNum = selectedIndex.row();
            }

            // if parentIndex is still not valid, insert the new record as the last child of the root include
            if (!parentIndex.isValid())
            {
                parentIndex = guiModel->getNextIndex();
                rowNum = -1; // insert as last child
            }
        }

        Q_ASSERT(parentIndex.isValid());

        // Attempt to insert the new record
        std::string lsaPath;
        if (parentIndex.isValid())
        {
            LSARecord* parentRecord = guiModel->getLSARecord(parentIndex);
            LSAInclude* parentInclude = static_cast<LSAInclude*>(parentRecord);

            std::string parentAbsolutePath = parentInclude->getAbsolutePath();
            lsaPath = getLSAPath( guiModel->getProjectDirectory(), absolutePath.toStdString(), parentAbsolutePath);

            if (rowNum == 0)
            {
                guiModel->select(parentIndex);
                insertedIndex = guiModel->insertFirstChild(LSAType::INCLUDE, lsaPath);
            }
            else if (rowNum > 0)
            {
                QModelIndex siblingIndex = guiModel->index(rowNum -1, 0, parentIndex);
                guiModel->select(siblingIndex);
                insertedIndex = guiModel->insertSiblingAfter(LSAType::INCLUDE, lsaPath);
            }
            else
            {
                insertedIndex = guiModel->insertLastChild(LSAType::INCLUDE, lsaPath, parentIndex);
            }
        }

        //Fix to Bug #1158
        //We used to loop over guiModel->getErrorsAndWarnings HERE to push parse
        //warnings to the status window, but now GuiModel::InsertNewRecords does this
        if(fileType == lsa::CONVERTER_TYPE_NONE)
        {
            // Check for duplicate labels and autocorrect them if the user so chooses
            vector<string> warningMessages = guiModel->findNonPosDuplicateLabelWarnings(insertedIndex);
            if(warningMessages.size() > 0)
            {
                QString dialogTitle = "Warning - label conflict while inserting " + QString::fromStdString(lsaPath) + ".";
                QString message = QString("Records were found with labels that already exist in the project:\n");
                for(int i=0;i<warningMessages.size();i++)
                    message += QString::fromStdString(warningMessages.at(i)) + QString("\n");
                message += "\nWould you like to make the labels in the file about to be inserted unique (recommended)?";
                QMessageBox::StandardButton reply = QMessageBox::question(this, dialogTitle, message, QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
                if (reply == QMessageBox::Yes)
                   makeModifiersLabelsUnique(insertedIndex,&oldUncrMap,&oldHeightMap,&oldVarMap,&oldDirGroupMap);
            }
        }
        else
        {
            makeModifiersLabelsUnique(insertedIndex,&oldUncrMap,&oldHeightMap,&oldVarMap,&oldDirGroupMap);
        }
    }

    // Fix to Bug #1136. Move this line after validateModelAndRegenerateMaps
//    guiModel->synchGuiModelToLSATree(); // make sure tree view is refreshed, especially scale factors

    //remove "Child record has warning." warning messages from all include records inherited through duplicate labels
    guiModel->validateModelAndRegenerateMaps();

    // Fix to Bug #1136. Moved this line from before validateModelAndRegenerateMaps
    guiModel->synchGuiModelToLSATree(); // make sure tree view is refreshed, especially scale factors

    updateRecordVisibility();
    updateColumnSpans();

    // make sure we push any parse warnings to the status window
    pushParseWarningsForDescendants(insertedIndex);

    return insertedIndex;
}

QString MainWindow::convertFile(QString fileName, lsa::CONVERTER_TYPE fileType)
{
    if (fileName.isEmpty() || fileType == lsa::CONVERTER_TYPE_NULL )
    {
        return "";
    }

    QString absolutePath;

    // Check fileType and call appropriate converter
    if(fileType == lsa::CONVERTER_TYPE_NONE)
    {
        absolutePath = fileName;
    }
    else if(fileType == lsa::CONVERTER_TYPE_IOB)
    {
        // Note that there is a seperate method for converting IOB PROJECTS!
        absolutePath = convertIOBFile(fileName);
    }
    else if(fileType == lsa::CONVERTER_TYPE_SETSOFANGLES ||
            fileType == lsa::CONVERTER_TYPE_TRIMBLETDEF ||
            fileType == lsa::CONVERTER_TYPE_TRIMBLEROUNDS ||
            fileType == lsa::CONVERTER_TYPE_LEICAGNSS ||
            fileType == lsa::CONVERTER_TYPE_PPP ||
            fileType == lsa::CONVERTER_TYPE_OPUS ||
            fileType == lsa::CONVERTER_TYPE_GSI)
    {
        absolutePath = convertInstrumentFile(fileName, fileType);
    }
    else if(fileType == lsa::CONVERTER_TYPE_CUSTOM)
    {
        absolutePath = convertCustomImport(fileName);
    }

    return absolutePath;
}

void MainWindow::getFilesToAddToProject(lsa::CONVERTER_TYPE fileType, QString filename)
{
    guiModel->undoStack()->beginMacro("Inserting files");
    QStringList fileList;

    if (filename.isEmpty())
    {
        //prompt user to select files from file explorer
        fileList = getExistingFilenames(fileType);
    }
    else
    {
        //add this filename to the list to be converted (supporting multiselect)
        fileList << filename;
    }

    // insert list of new includes to existing project
    if (fileList.isEmpty())
        return;
    else
    {
        QModelIndex currentIndex = guiModel->getSingleSelectedRecord();
        convertFilesAndAddToProject(fileList, fileType, currentIndex.parent(), currentIndex.row() + 1);
    }
    guiModel->undoStack()->endMacro();
}

void MainWindow::insertPOSG()
{
    QModelIndex newIndex;

    if (guiModel->rootIncludeIsSelected())
    {
        newIndex = guiModel->insertFirstChild(LSAType::POSG);
    }
    else
    {
        newIndex = guiModel->insertSiblingAfter(LSAType::POSG);
    }

    // New LSAPosG records default to East longitude.  If the user has set config to default to W longitude, set that now.
    if (newIndex.isValid() && usingWestLongitude())
    {
        LSARecord *lsaRecord = guiModel->getLSARecord(newIndex);
        LSAPosG *posg = static_cast<LSAPosG*>(lsaRecord);

        posg->lonDir = "W";

        guiModel->synchSelectedItemsToRecords();
    }

    recordEditor->prepareToEditNewRecord();
}


void MainWindow::insertNewBlankRecord(LSAType lsaType)
{
    if(guiModel->rootIncludeIsSelected())
    {
        guiModel->insertFirstChild(lsaType);
    }
    else
    {
        guiModel->insertSiblingAfter(lsaType);
    }

    recordEditor->prepareToEditNewRecord();

    updateRecordVisibility();
}

void MainWindow::insertComment()
{
    QModelIndex newIndex;

    if (guiModel->rootIncludeIsSelected())
    {
        newIndex = guiModel->insertFirstChild(LSAType::COMMENT);
    }
    else
    {
        newIndex = guiModel->insertSiblingAfter(LSAType::COMMENT);
    }
    ui->actionShow_Comments->setChecked(true);
    updateRecordVisibility();
    updateColumnSpans();

    recordEditor->prepareToEditNewRecord();

    return;
}

void MainWindow::insertSeparator()
{
    // Note: if you change the contents of separatorBar, you need to update the setSelection parameters
    // in RecordEditor::prepareToEditSeparatorBar()
    QString separatorBar("#──────────────── ─ ─────────────────────────────────────────────────────");
    QModelIndex newIndex;

    std::string debugString = separatorBar.toStdString();
    if (guiModel->rootIncludeIsSelected())
    {
        newIndex = guiModel->insertFirstChild(LSAType::COMMENT, separatorBar.toStdString());
    }
    else
    {
        newIndex = guiModel->insertSiblingAfter(LSAType::COMMENT, separatorBar.toStdString());
    }
    ui->actionShow_Comments->setChecked(true);
    updateRecordVisibility();
    updateColumnSpans();

    recordEditor->prepareToEditSeparatorBar();
}

void MainWindow::findRecord()
{
    ui->findFilterWidget->show();
    ui->findFilterWidget->setFocus();
}

void MainWindow::selectReferencingRecords()
{
    guiModel->selectReferencingRecords();

    return;
}

QString MainWindow::createNewEmptyLSAFile(QString testFileName)
{
    // Let the user select a project and parent .lsa file
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();
    QString selfilter = tr("LSA Files (*.lsa)");
    QString filename;

    suppressProjectChangedDialog = true;
    if(testFileName.isEmpty())
    {
        {
            filename = QFileDialog::getSaveFileName(this,
                                                    tr("Create a new .lsa file"),
                                                    lastLSAPath,
                                                    tr("LSA Files (*.lsa);; All files (*)"),
                                                    &selfilter);
        }
    }
    else
    {
        filename = testFileName;
    }
    suppressProjectChangedDialog = false;

    // Don't try to do anything else if the user cancelled
    if (filename.isEmpty())
    {
        return "";
    }

    // Create the file on disk
    QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString header("# New .lsa file created " + dateTime);

    QFile newFile(filename);
    newFile.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream newStream(&newFile);
    newStream << QString::fromStdString(LSAVERSION_FILE_STRING) << endl;
    newStream << header << endl;
    newFile.close();

    return filename;
}

QStringList MainWindow::getExistingFilenames(lsa::CONVERTER_TYPE fileType)
{

    QString caption, filter, selFilter;
    switch(fileType)
    {
        case lsa::CONVERTER_TYPE_NONE:
            caption = "Select a .lsa file";
            filter = "LSA Files (*.lsa);; All files (*)";
            selFilter = "LSA Files (*.lsa)";
        break;
        case lsa::CONVERTER_TYPE_IOB:
            caption = "Select an .iob file";
            filter = "GeoLab Files (*.iob *.gps *.cob *.apx *.plh);;iob Files (*.iob);; GPS Files (*.gps);; Conventional Files (*.cob);; Initial Positions (*.apx *.plh);; All files (*)";
            selFilter = "GeoLab Files (*.iob)";
        break;
        case lsa::CONVERTER_TYPE_SETSOFANGLES:
            caption = "Select a .log or .txt file";
            filter = "Sets Of Angles Files (*.log *.txt);; All files (*)";
            selFilter = "Sets Of Angles Files (*.log *.txt)";
        break;
        case lsa::CONVERTER_TYPE_TRIMBLETDEF:
            caption = "Select a .asc file";
            filter = "Trimble Files (*.asc);; All files (*)";
            selFilter = "Trimble Files (*.asc)";
        break;
        case lsa::CONVERTER_TYPE_LEICAGNSS:
            caption = "Select a .asc file";
            filter = "Leica GNSS Files (*.asc);; All files (*)";
            selFilter = "Leica GNSS Files (*.asc)";
        break;
        case lsa::CONVERTER_TYPE_TRIMBLEROUNDS:
            caption = "Select a .jxl file";
            filter = "Trimble job file (*.jxl);;Trimble xml file (*.xml);; All files(*)";
            selFilter = "Trimble job file (*.jxl)";
        break;
        case lsa::CONVERTER_TYPE_PPP:
            caption = "Select a .txt file";
            filter = "grape|merge log files (*.txt *.log);; All files (*)";
            selFilter = "grape|merge log files (*.txt *.log)";
        break;
        case lsa::CONVERTER_TYPE_OPUS:
            caption = "Select a .opus file";
            filter = "OPUS output files (*.txt *.opus);; All files (*)";
            selFilter = "OPUS output files (*.txt *.opus)";
        break;
        case lsa::CONVERTER_TYPE_GSI:
            caption = "Select a .GSI file";
            filter = "GSI files (*.GSI);; All files (*)";
            selFilter = "GSI files (*.txt)";
        break;
        case lsa::CONVERTER_TYPE_CUSTOM:
            caption = "Select a file to convert with " + currentImportAction->text() + " import";
            filter = "Sets Of Angles Files (*.log);; "
                     "Trimble Files (*.asc);;\
                      grape|merge log files (*.txt);; \
                      GSI files (*.GSI);; "
                      "All files (*)";
            selFilter = "All files (*)";
        break;
    }

    QStringList filenames;
    suppressProjectChangedDialog = true;
    {
        // Let the user select a project and parent .lsa file
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();
        filenames = QFileDialog::getOpenFileNames(this, caption, lastLSAPath, filter, &selFilter);
        if(filenames.size()>0)
        {
            QString fileName = filenames.at(filenames.size()-1);
            QString currentDir = QString::fromStdString(getPathWithoutFileName(fileName.toStdString()));
            settings.setValue(lsa::QSETTINGS_LASTPATH, currentDir);

        }
    }
    suppressProjectChangedDialog = false;

    return filenames;
}

void MainWindow::deleteCurrentlySelectedRecords()
{
    guiModel->undoStack()->beginMacro("Removing records.");
    QModelIndexList itemsToDelete = guiModel->getSelectedRows();

    // Confirm user wants to delete multiple records
    int numRecordsToDelete = itemsToDelete.size();
    if (numRecordsToDelete > 1)
    {
        QMessageBox confirmBox;
        confirmBox.setText(QString("This operation will delete ") + QString::number(numRecordsToDelete) + " records." );
        confirmBox.setInformativeText("Do you wish to proceed?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        confirmBox.setDefaultButton(QMessageBox::Yes);
        int buttonClicked = confirmBox.exec();

        if (buttonClicked == QMessageBox::No)
            return;
    }

    QList<QPersistentModelIndex> persistentIndicesToDelete;
    foreach (QModelIndex index, itemsToDelete)
    {
        persistentIndicesToDelete.push_back(QPersistentModelIndex(index));
    }

    QVector<QVector<IndexRowColumnParentChainItem>> removedIndicesChain = guiModel->buildMultipleIndicesChain(persistentIndicesToDelete);

    foreach(QPersistentModelIndex persitentIndex, persistentIndicesToDelete)
    {
        if (persitentIndex.isValid())
        {
            QModelIndex currentIndex = guiModel->index(persitentIndex.row(), persitentIndex.column(), persitentIndex.parent());
            deleteRecord(currentIndex, removedIndicesChain);
        }
    }

    guiModel->undoStack()->endMacro();
}

void MainWindow::enableCurrentlySelectedRecords()
{
    QModelIndexList indices = guiModel->getSelectedRows();
    guiModel->undoStack()->beginMacro("Enabling records");

    // suppress gui updates while we make the changes
    bool oldState = setSuppressGuiUpdates(true);
    {
        foreach(QModelIndex index, indices)
        {
            guiModel->setData(index, Qt::Checked, Qt::CheckStateRole);
            guiModel->setParentIncludeModified(index);
        }
    }
    setSuppressGuiUpdates(oldState);

    // Refresh the gui model and UI once after all changes are made
    guiModel->synchGuiModelToLSATree();
    mapWidget->handleGuiModelDataChanged();
    updateWidgets();
    guiModel->undoStack()->endMacro();

    return;
}

void MainWindow::disableCurrentlySelectedRecords()
{
    QModelIndexList indices = guiModel->getSelectedRows();

    // suppress gui updates while we make the changes
    bool oldState = setSuppressGuiUpdates(true);
    {
        foreach(QModelIndex index, indices)
        {
            guiModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
            guiModel->setParentIncludeModified(index);
        }
    }
    setSuppressGuiUpdates(oldState);

    // Refresh the gui model and UI once after all changes are made
    guiModel->synchGuiModelToLSATree();
    mapWidget->handleGuiModelDataChanged();
    updateWidgets();

    return;
}

void MainWindow::handleFilterBoxChanged(std::vector<LSAType> filterTypes, std::function<bool (std::vector<LSAType>, LSARecord *)> logicalFunc)
{
    guiModel->setFilterLogic(filterTypes, logicalFunc);
    updateRecordVisibility();
}

void MainWindow::handleFilterStringChanged(QString newString, bool bExact, bool bMatchAll)
{
    guiModel->filterString = newString.toStdString();
    guiModel->filterExact = bExact;
    guiModel->bMatchAllFilter = bMatchAll;
    updateRecordVisibility();
}

void MainWindow::handleFilterPointTriggered()
{
    // Get the currently selected index and check that it is valid
    QModelIndex currentIndex = guiModel->getSingleSelectedRecord();
    if (!currentIndex.isValid())
        return;

    // Get the currently selected record and check that it is a position
    LSARecord *currentRecord = guiModel->getLSARecord(currentIndex);
    LSAType currentType = currentRecord->getRecType();
    if (!currentType.isPosition() && !currentType.isPostProcessed())
            return;

    // Set the filter label
    QString pointLabel = QString::fromStdString(currentRecord->getLabel());
    ui->findFilterWidget->show();
    ui->findFilterWidget->setFilterText(pointLabel);
    ui->findFilterWidget->setFocus();


}

void MainWindow::handleMatchTypeChanged(bool bMatchAll)
{
    guiModel->bMatchAllFilter = bMatchAll;
    updateRecordVisibility();
}

void MainWindow::deleteRecord(QModelIndex index, QVector<QVector<IndexRowColumnParentChainItem>> removedIndicesTree)
{
    if (!index.isValid()) return;
    LSARecord* lsaRecord = guiModel->getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();

    // If the record is a modifier or DGRP, ask user if they want to delete references to this record
    bool deleteReferences = false;
    if (lsaType == LSAType::DGRP || lsaType.isModifier())
    {
        std::string label = lsaRecord->getLabel();
        LabelChangeDialog dialog(this, guiModel, lsaType, label, LSAOperationType::Delete);
        QMessageBox::StandardButton reply = (QMessageBox::StandardButton)dialog.exec();
        if (reply == QMessageBox::Cancel)
        {
            return;
        }
        else
        {
            deleteReferences = (reply == QMessageBox::Yes);
        }
    }
    else if(lsaType == LSAType::INCLUDE && !lsaRecord->isAutogenerated)
    {
        QModelIndex modifiedDescendantIncludeIndex = guiModel->descendantIncludeIsModified(index);
        if(modifiedDescendantIncludeIndex.isValid())
        {
            LSAInclude *modifiedInclude = static_cast<LSAInclude*>(guiModel->getLSARecord(modifiedDescendantIncludeIndex));
            QString message = QString::fromStdString(modifiedInclude->getLSAPath()) + QString(" has modified child records.  Do you wish to save the file before removing it from the project?");
            QMessageBox::StandardButton response = confirmDeleteInclude(message);
            if(response == QMessageBox::Cancel)
            {
                return;
            }
            else if(response == QMessageBox::Yes)
            {
                guiModel->saveBranch(index);
            }
        }
    }

    // Delete the record
    guiModel->removeRecord(index, deleteReferences, false, false, removedIndicesTree);

    // Refresh all the widgets
    updateWidgets();

    return;
}

bool MainWindow::isChildOfWidget(QObject* input, QWidget* widget) const
{
    if (input == NULL)
    {
        return false;
    }
    else if (input == widget)
    {
        return true;
    }

    return isChildOfWidget(input->parent(), widget);
}

double MainWindow::computeConfidenceRegion(gnsstk::Matrix<double> &covMat)
{
    double confReg = 0.0;
    double scale_factor = 2.7955;// sqrt of critical value for chi-square dist with
                                 // three degrees of freedom and with 1-alpha = 0.95
    gnsstk::Vector<double> eigenvalues(3);
    gnsstk::Matrix<double> eigenvectors(3,3);

    //get eigenvalues of the covariance matrix
    EigenDecomp(covMat,eigenvalues,eigenvectors);

    //RSS the eigenvalues
    confReg = sqrt(eigenvalues[0]*eigenvalues[0] +
                   eigenvalues[1]*eigenvalues[1] +
                   eigenvalues[2]*eigenvalues[2]);

    //convert from variance to standard error
    confReg = sqrt(confReg);

    //scale by the critical value
    return confReg*scale_factor;
}

void MainWindow::RestoreDockableWidgets()
{
//This restored the GUI to the state that the dockable widgets were
//in when SALSA started, which (now) may have some docwids invisible
//    restoreState(mainWindowInitialState);

    //This restores the docwids state to the default visibility state, where
    //the map and record editor are visible, but other docwids are invisible
    //Also redock Map and Record Editor (i#1019)
    ui->actionView_RecordEditor->setChecked(true);
    updateRecordEditorVisibility();
    recordEditor->setFloating(false);
    ui->actionView_Map->setChecked(true);
    updateMapVisibility();
    mapWidget->setFloating(false);
    ui->actionView_AdjustedPositions->setChecked(false);
    updateAdjustedPositionsVisibility();
    ui->actionView_StationData->setChecked(false);
    updateStationDataVisibility();
    ui->actionView_Histogram->setChecked(false);
    updateHistogramVisibility();
}

void MainWindow::openConfigDialog()
{
    if(configDialog != NULL)
    {
        delete configDialog;
        configDialog = NULL;
    }
    configDialog = new ConfigDialog(configFileName);
    configDialog->show();
}

void MainWindow::openPreferencesDialog()
{
    if(preferencesDialog != NULL)
    {
        delete preferencesDialog;
        preferencesDialog = NULL;
    }
    preferencesDialog = new PreferencesDialog(this);
    preferencesDialog->show();
}

void MainWindow::viewHistogram()//This is only called from testlsagui.cpp
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QFileInfo file(hdfFileName);
    if(!file.exists())
    {
        QMessageBox *notifyUser = new QMessageBox(QMessageBox::Critical, // icon
                                                  QString("Unable to find adjustment output file!"), // title
                                                  hdfFileName + QString(" not found."), // text
                                                  QMessageBox::Ok, // buttons
                                                  (QWidget*)this, // parent
                                                  Qt::WindowStaysOnTopHint);

        notifyUser->exec();//blocks
        delete notifyUser;
    }
    else
    {

        histogramDialog->setupHistogram(currentHDF5File, guiModel, currentFile);
        histogramDialog->show();
    }
}

void MainWindow::giveFocusToTreeView()
{
    ui->treeView->setFocus();
}

bool MainWindow::setSuppressGuiUpdates(bool value)
{
    bool oldValue = guiModel->suppressGuiModelUpdates;

    suppressGuiUpdates = value;
    guiModel->suppressGuiModelUpdates = value;
    mapWidget->suppressMapWidgetUpdates = value;
    recordEditor->suppressRecordEditorUpdates = value;

    return oldValue;
}

void MainWindow::handleTableMeasurementsSelectionChanged(QItemSelection selected, QItemSelection deselected)
{
    if(hashCodesAreValid && !suppressOutputTableSelectionChanges)
    {
        int row=0;
        QString tableLsaRecordPtr;
        QModelIndexList selectedRows = ui->tableMeasurements->selectionModel()->selectedRows();

        QModelIndexList treeViewIndices;
        foreach(QModelIndex index, selectedRows)
        {
            row = index.row();
            tableLsaRecordPtr = ui->tableMeasurements->model()->data(ui->tableMeasurements->model()->index(row,NUM_RESID_COLUMNS-1)).toString();
            if(tableLsaRecordPtr.isEmpty())
                continue;//no data in the row

            //find the corresponding element in the tree
            QModelIndex currentIndex = guiModel->getNextIndex();
            while (true)
            {
                LSARecord * lsaRecord = guiModel->getLSARecord(currentIndex);
                if(tableLsaRecordPtr == QString::number((qlonglong)lsaRecord))
                        break;
                currentIndex = guiModel->getNextIndex(currentIndex);
                if(currentIndex == QModelIndex())//traversed the tree and found no matching lsaRecord (shouldn't happen)
                    break;
            }
            if(currentIndex !=QModelIndex())
                treeViewIndices << currentIndex;
        }

        if(treeViewIndices.size()>0)
        {
            suppressOutputTableSelectionChanges = true;
            guiModel->select(treeViewIndices);
            suppressOutputTableSelectionChanges = false;
        }

        // Update the current index of the selection model so that user interaction with the view will work appropriately
        if(selectedRows.size() > 0)
        {
            ui->tableMeasurements->selectionModel()->setCurrentIndex(selectedRows.last(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
}

void MainWindow::handleTablePointsSelectionChanged(QItemSelection selected, QItemSelection deselected)
{
    if(!suppressOutputTableSelectionChanges)
    {
        //extract the point name from the first column
        int row=0;
        QModelIndexList selectedRows = ui->tablePoints->selectionModel()->selectedRows();
        QModelIndexList treeViewIndices;
        foreach(QModelIndex index, selectedRows)
        {
            row = index.row();
            string pointName = ui->tablePoints->model()->data(ui->tablePoints->model()->index(row,POINT_TABLE_POINT_COLUMN)).toString().toStdString();
            if(pointName.empty())
                continue;//no point in the row

            QModelIndex currentIndex = guiModel->getIndexFromPointLabel(pointName);
            if(currentIndex.isValid())
                treeViewIndices << currentIndex;
        }

        if(treeViewIndices.size()>0)
        {
            suppressOutputTableSelectionChanges = true;
            guiModel->select(treeViewIndices);
            suppressOutputTableSelectionChanges = false;
        }

        // Update the current index of the selection model so that user interaction with the view will work appropriately
        if(selectedRows.size() > 0)
        {
            ui->tablePoints->selectionModel()->setCurrentIndex(selectedRows.last(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        updateWidgets();
    }
}

//generates a mapping of measurement record indices (found by counting through the tree)
//to the corresponding LSARecord pointer values
void MainWindow::generateUniqueIDMapToLSARecordPointers()
{
    LSAType meas[] = {LSAType::AZIM,LSAType::DIST,LSAType::DXYZ,LSAType::HANG,
                      LSAType::HDIF,LSAType::HDIR,LSAType::VANG,LSAType::ZANG,LSAType::POSC,LSAType::POSG};
    std::set<LSAType> measurements(meas,meas+sizeof(meas)/sizeof(meas[0]));

    guiModel->inputOutputMap.clear();
    QModelIndex currentIndex = guiModel->getNextIndex();
    LSARecord *lsaRecord;
    unsigned int treeDatIndex=0;

    while (currentIndex.isValid())
    {
        lsaRecord = guiModel->getLSARecord(currentIndex);
        if((measurements.find(lsaRecord->getRecType()) != measurements.end()) &&
           guiModel->isActive(currentIndex) && isPassedToDAT(lsaRecord))
        {
            treeDatIndex++;
            guiModel->inputOutputMap.insert(std::pair<unsigned int, QModelIndex>(treeDatIndex, currentIndex));
        }
        currentIndex = guiModel->getNextIndex(currentIndex);
    }
}


bool MainWindow::isPassedToDAT(LSARecord *lsaRecord)
{
    if(lsaRecord->isAutogenerated)
        return false;
    if(lsaRecord->getRecType() == LSAType::HDIR)//Fix to Bug #941
    {
        LSAHDir *lsaHDir(static_cast<LSAHDir *>(lsaRecord));
        LSADirGroupMap *DGRPMap = guiModel->getDirGroupMap();
        if(DGRPMap->find(lsaHDir->dirGroupLabel) == DGRPMap->end())
            return false;
    }

    return true;
}

bool MainWindow::isHashConsistent()
{
    //this will change when the binary .bin file has the hash code in it
    QString outFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".out");
    ifstream istrm;
    std::string line;

    istrm.open(outFileName.toStdString().c_str(), ios::in);
    if(!istrm.is_open())
       return false;

    while(getline(istrm,line))
    {
        if(line.find("# Hash string is ")!= string::npos)
        {
            stripTrailing(line,'\r');
            stripTrailing(line,'\n');
            if(QString::number(currentHashCode) == QString::fromStdString(line.substr(17)))
            {
                istrm.close();
                return true;
            }
            else
            {
                istrm.close();
                return false;
            }
        }
    }
    istrm.close();
    return false;
}

bool MainWindow::ingestSolverOutputs(bool onlyApriori)
{
    DebugTimer timer(false);

    // cache the previous model save state
    bool previousSaveState = guiModel->getIsRootModified();
    bool prevSupressGuiUpdates = setSuppressGuiUpdates(true);
    {
        parseAutogeneratedPoints();

        if(!onlyApriori)
        {
            // Populate final adjusted positions vector
            guiModel->clearOutputPositions();
            vector<FinalAdjustedPosition> newAdjustedValues;
            for(int i=0; i<currentHDF5File.llh->getSize(); i++)
            {
                //get final position info
                FinalAdjustedPosition newValue;
                newValue.position = currentHDF5File.llh->getDataPoint(i).label;
                newValue.latDecDeg = currentHDF5File.llh->getDataPoint(i).lat;
                newValue.latDir = "N";
                newValue.lonDecDeg = currentHDF5File.llh->getDataPoint(i).lon;
                newValue.lonDir = "E";
                newValue.height = currentHDF5File.llh->getDataPoint(i).ht;
                newValue.heightUnits = "M";
                newValue.deltaN = currentHDF5File.llh->getDataPoint(i).adjN;
                newValue.deltaE = currentHDF5File.llh->getDataPoint(i).adjE;
                newValue.deltaU = currentHDF5File.llh->getDataPoint(i).adjU;
                newValue.covNN = currentHDF5File.llh->getDataPoint(i).Cnn;
                newValue.covEE = currentHDF5File.llh->getDataPoint(i).Cee;
                newValue.covUU = currentHDF5File.llh->getDataPoint(i).Cuu;
                newValue.covNE = currentHDF5File.llh->getDataPoint(i).Cne;
                newValue.covNU = currentHDF5File.llh->getDataPoint(i).Cnu;
                newValue.covEU = currentHDF5File.llh->getDataPoint(i).Ceu;
                newValue.maj2Drr = currentHDF5File.llh->getDataPoint(i).maj2Drr;
                newValue.min2Drr = currentHDF5File.llh->getDataPoint(i).min2Drr;
                newValue.azrr = currentHDF5File.llh->getDataPoint(i).azrr;
                newValue.vertrr = currentHDF5File.llh->getDataPoint(i).vertrr;
                newAdjustedValues.push_back(newValue);
            }

            if(newAdjustedValues.size()>0)
            {
                guiModel->setOutputPositions(newAdjustedValues);
            }

            guiModel->solutionNumDOF = currentHDF5File.probDescript->getDataPoint().degOfFreed;
            LOG_TIME(timer, "Populate final adjusted positions vector");
        }
    }
    setSuppressGuiUpdates(prevSupressGuiUpdates);

    guiModel->setIsRootModified(previousSaveState);

    if(currentHDF5File.llh->getSize()>0)
        return true;
    else
        return false;
}


void MainWindow::parseAutogeneratedPoints()
{
    DebugTimer timer(false);

    // If an autogenerated include file is present, remove it from the gui model
    QModelIndex autoIncludeIndex = QModelIndex(guiModel->getAutogenIndex());
    if(autoIncludeIndex.isValid())
    {
        guiModel->removeRecord(autoIncludeIndex);
    }
    LOG_TIME(timer, "Remove old include");

    //look for a priori points
    bool autoGeneratedPointExists = false;
    for(int i=0; i<currentHDF5File.initialLlh->getSize();i++)
    {
        if(currentHDF5File.initialLlh->getDataPoint(i).sourceType == AUTOGENERATED)
        {
            autoGeneratedPointExists = true;
            break;
        }
    }
    LOG_TIME(timer, "Do autogen exist?");

    if (autoGeneratedPointExists)
    {
        autoIncludeIndex = guiModel->insertAutogeneratedInclude();
        QModelIndex selectedIndex = guiModel->getSingleSelectedRecord();
        //Q_ASSERT(autoIncludeIndex == selectedIndex);
        LOG_TIME(timer, "Insert new include");

        // Insert a POSG record for each autogenerated point
        QList<std::string> POSGstrings;
        QList<LSAType> recordTypes;
        QList<bool> isAutogenList;
        QList<bool> isCommentedList;
        for(int i=0; i<currentHDF5File.initialLlh->getSize();i++)
        {
            if(currentHDF5File.initialLlh->getDataPoint(i).sourceType == AUTOGENERATED)
            {
                string label = currentHDF5File.initialLlh->getDataPoint(i).label;
                double latDecDeg = currentHDF5File.initialLlh->getDataPoint(i).lat;
                double lonDecDeg = currentHDF5File.initialLlh->getDataPoint(i).lon;
                double height = currentHDF5File.initialLlh->getDataPoint(i).ht;
                string autogenFixedState = currentHDF5File.initialLlh->getDataPoint(i).fixedType.asLSAFileString();
                string autogenPointConstraints = currentHDF5File.initialLlh->getDataPoint(i).constraints;

                // Build the string to parse into a POSG
                stringstream POSGstrm;
                if(usingWestLongitude())
                {
                    lonDecDeg = 360.0 - lonDecDeg;//convert from east lon to west lon
                    if(lonDecDeg >= 360.0)//Fix to Bug #1430
                        lonDecDeg -= 360.0;

                    // Convert latDecDeg to DMS
                    bool latDMSIsNeg;
                    int dmsLatDegrees = 0;
                    int dmsLatMinutes = 0;
                    double dmsLatSeconds = 0.0;
                    degToDMS(latDecDeg, latDMSIsNeg, dmsLatDegrees, dmsLatMinutes, dmsLatSeconds, getAngularPositionPrecisionSOA());

                    // Convert lonDecDeg to DMS
                    bool lonDMSIsNeg;
                    int dmsLonDegrees = 0;
                    int dmsLonMinutes = 0;
                    double dmsLonSeconds = 0.0;
                    degToDMS(lonDecDeg, lonDMSIsNeg, dmsLonDegrees, dmsLonMinutes, dmsLonSeconds, getAngularPositionPrecisionSOA());

                    POSGstrm << "POSG " << addQuotes(label) << " " << autogenFixedState << " " << autogenPointConstraints << " "
                             << (latDMSIsNeg ?  "-" : "") << dmsLatDegrees << " " << dmsLatMinutes << " " << fixed << setprecision(getAngularPositionPrecisionSOA()) << dmsLatSeconds << " N "
                             << (lonDMSIsNeg ?  "-" : "") << dmsLonDegrees << " " << dmsLonMinutes << " " << fixed << setprecision(getAngularPositionPrecisionSOA()) << dmsLonSeconds << " W "
                             << fixed << setprecision(getLinearPositionPrecisionMeters()) << height << " m";
                }
                else
                {
                    // Convert latDecDeg to DMS
                    bool latDMSIsNeg;
                    int dmsLatDegrees = 0;
                    int dmsLatMinutes = 0;
                    double dmsLatSeconds = 0.0;
                    degToDMS(latDecDeg, latDMSIsNeg, dmsLatDegrees, dmsLatMinutes, dmsLatSeconds);

                    // Convert lonDecDeg to DMS
                    bool lonDMSIsNeg;
                    int dmsLonDegrees = 0;
                    int dmsLonMinutes = 0;
                    double dmsLonSeconds = 0.0;
                    degToDMS(lonDecDeg, lonDMSIsNeg, dmsLonDegrees, dmsLonMinutes, dmsLonSeconds);

                    POSGstrm << "POSG " << addQuotes(label) << " " << autogenFixedState << " " << autogenPointConstraints << " "
                             << (latDMSIsNeg ?  "-" : "") << dmsLatDegrees << " " << dmsLatMinutes << " " << fixed << setprecision(getAngularPositionPrecisionSOA()) << dmsLatSeconds << " N "
                             << (lonDMSIsNeg ?  "-" : "") << dmsLonDegrees << " " << dmsLonMinutes << " " << fixed << setprecision(getAngularPositionPrecisionSOA()) << dmsLonSeconds << " E "
                             << fixed << setprecision(getLinearPositionPrecisionMeters()) << height << " m";
                }
                string POSGString = POSGstrm.str();

                POSGstrings.append(POSGString);
                recordTypes.append(LSAType::POSG);
                isAutogenList.append(true);
                isCommentedList.append(false);
            }
        }

        guiModel->insertNewRecords(autoIncludeIndex, 1, recordTypes, POSGstrings, isAutogenList, isCommentedList);//Fix to Bug #1088
        LOG_TIME(timer, "insertNewRecords");
    }
}


/**
 * @brief MainWindow::saveWindowSettings Saves off various settings of Main Window such as size, position, etc
 * so that the application can be adjusted accordingly the next time it is launched.
 */
void MainWindow::saveWindowSettings()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    // Saves off the current geometry of the main window
    qsettings.setValue( QSETTINGS_WINDOW_GEOMETRY, saveGeometry() );
    // Saves off the current state of the dockable widgets in the main window
    qsettings.setValue( QSETTINGS_WINDOW_SAVE_STATE, saveState() );
    // Saves off whether or not the main window is maximized
    qsettings.setValue( QSETTINGS_WINDOW_MAXIMIZED, isMaximized() );
    // Saves off the position and size on the screen, if not maximized
    if (!isMaximized())
    {
        qsettings.setValue( QSETTINGS_WINDOW_POSITION, pos() );
        qsettings.setValue( QSETTINGS_WINDOW_SIZE, size() );
    }

    return;
}

/**
 * @brief MainWindow::loadAndApplyWindowSettings
 * Restores various settings to adjust the size, position, etc of
 * the Main Window to what it was the last time it was closed.
 */
void MainWindow::loadAndApplyWindowSettings()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    // Restores the geometry
    restoreGeometry(qsettings.value( QSETTINGS_WINDOW_GEOMETRY, saveGeometry() ).toByteArray());
    // Restores the default state of the MainWindow
    restoreState(qsettings.value( QSETTINGS_WINDOW_SAVE_STATE, saveState() ).toByteArray());
    // Restores whether or not it was maximized
    if (qsettings.value( QSETTINGS_WINDOW_MAXIMIZED, isMaximized()).toBool())
    {
        showMaximized();
    }
    else
    {
        // Not maximized, restore the position and size on the screen
        move(qsettings.value( QSETTINGS_WINDOW_POSITION, pos()).toPoint());
        resize(qsettings.value( QSETTINGS_WINDOW_SIZE, size()).toSize());
    }

    return;
}

template<> bool MainWindow::parseCFGFile<bool>(lsa::CFG_PARSE_TYPE cfgParseType)
{
   bool result = false;

   if(cfgParseType == lsa::CFG_PARSE_IS_ALLOW_FAST)
   {
      result = true;
   }

   ifstream configFile;
   configFile.open(configFileName.toStdString().c_str(),ios::in);
   if(!configFile.is_open())
   {
      return result;
   }

   string line;
   while(1)
   {
      line = string("");
      getline(configFile,line);
      stripTrailing(line,'\r');

      if((configFile.eof() || !configFile.good()) && line.empty())
      {
         break;
      }

      switch(cfgParseType)
      {
         case lsa::CFG_PARSE_USING_WEST_LON:
         {
            if(line.substr(0,9) == string("--westLon"))
            {
               result = true;
            }
            break;
         }
         case lsa::CFG_PARSE_IS_SCALE_BY_APV:
         {
            if(line.substr(0,5) == string("--APV"))
            {
               result = true;
            }
            break;
         }
         case lsa::CFG_PARSE_INCLUDE_UNUSED:
         {
            if(line.substr(0,15) == string("--includeUnused"))
            {
               result = true;
            }
            break;
         }
         case lsa::CFG_PARSE_IS_GEOID_USED:
         {
            if(line.substr(0,11) == string("--geoidfile"))
            {
               result = true;
            }
            break;
         }
         case lsa::CFG_PARSE_IS_PROJ_DIR_GEOID:
         {
            if(line.substr(0,11) == string("--geoidfile"))
            {
               QString projPath=QString::fromStdString(getPathWithoutFileName(currentFile.toStdString())+"/geoid");
               QDir projDir(projPath);
               QStringList projFiles = projDir.entryList();
               QString geoidFile = QString::fromStdString(line.substr(12));
               if (projFiles.contains(geoidFile))
               {
                  result = true;
               }
               else
               {
                  result = false;
               } //end if
            } //end if
            break;
         }
         case lsa::CFG_PARSE_IS_ALLOW_FAST:
         {
            if(line.substr(0,11) == string("--forcefast"))
            {
                result = false;
            }
            else if(line.substr(0,13) == string("--forcestable"))
            {
                result = false;
            }
            break;
         }
      }//end switch
   }//end while
   configFile.close();
   return result;
}

template<> string MainWindow::parseCFGFile<std::string>(lsa::CFG_PARSE_TYPE cfgParseType)
{
   string result = "";

   if(cfgParseType == lsa::CFG_PARSE_GET_CONFIDENCE_INTERVAL)
   {
      result = string("1sig");
   }

   ifstream configFile;
   configFile.open(configFileName.toStdString().c_str(),ios::in);
   if(!configFile.is_open())
   {
      return result;
   }

   string line;
   while(1)
   {
      line = string("");
      getline(configFile,line);
      stripTrailing(line,'\r');

      if((configFile.eof() || !configFile.good()) && line.empty())
      {
         break;
      }

      switch(cfgParseType)
      {
      case lsa::CFG_PARSE_GET_CONFIDENCE_INTERVAL:
      {
         if(line.substr(0,8) == string("--confid"))
         {
             result = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ').at(1);
         }
         break;
      }
      case lsa::CFG_PARSE_GET_CHI_SQUARED:
      {
          if(line.substr(0,8) == string("--alpha "))
          {
              result = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ').at(1);
              double numericalResult = 100*(1 - QString::fromStdString(result).toDouble());
              int intResult = int (numericalResult);
              result = std::to_string(intResult);
          }
          break;
      }

      }//end switch
   }//end while
   configFile.close();
   return result;
}

bool MainWindow::usingWestLongitude()
{
   return MainWindow::parseCFGFile<bool>(lsa::CFG_PARSE_USING_WEST_LON);
}

bool MainWindow::isScaleByAPV()
{
   return MainWindow::parseCFGFile<bool>(lsa::CFG_PARSE_IS_SCALE_BY_APV);
}

bool MainWindow::includeUnused()
{
   return MainWindow::parseCFGFile<bool>(lsa::CFG_PARSE_INCLUDE_UNUSED);
}

bool MainWindow::isAllowFast()
{
   return MainWindow::parseCFGFile<bool>(lsa::CFG_PARSE_IS_ALLOW_FAST);
}

bool MainWindow::isGeoidFileUsed()
{
   return MainWindow::parseCFGFile<bool>(lsa::CFG_PARSE_IS_GEOID_USED);
}

//determines if there is a specified geoid file in the proj dir
bool MainWindow::isProjDirGeoid()
{
   return MainWindow::parseCFGFile<bool>(lsa::CFG_PARSE_IS_PROJ_DIR_GEOID);
}

string MainWindow::getConfidenceInterval()
{
    return MainWindow::parseCFGFile<std::string>(lsa::CFG_PARSE_GET_CONFIDENCE_INTERVAL);
}

string MainWindow::getChiSquared()
{
    return MainWindow::parseCFGFile<std::string>(lsa::CFG_PARSE_GET_CHI_SQUARED);
}

bool MainWindow::makeModifiersLabelsUnique(QModelIndex parentIncludeIndex, LSAUNCRMap *oldUncrMap, LSAHGHTMap *oldHeightMap,
                                           LSAVSCAMap *oldVarMap, LSADirGroupMap *oldDirGroupMap)
{
    bool madeLabelsUnique = false;

    if(makeUncrLabelsUnique(parentIncludeIndex, oldUncrMap))
        madeLabelsUnique = true;
    if(makeHeightLabelsUnique(parentIncludeIndex, oldHeightMap))
        madeLabelsUnique = true;
    if(makeVarScalingLabelsUnique(parentIncludeIndex, oldVarMap))
        madeLabelsUnique = true;
    if(makeDirGroupLabelsUnique(parentIncludeIndex, oldDirGroupMap))
        madeLabelsUnique = true;

    if(madeLabelsUnique)
        guiModel->synchGuiModelToLSATree();

    return madeLabelsUnique;
}

bool MainWindow::makeUncrLabelsUnique(QModelIndex parentIncludeIndex, LSAUNCRMap *oldUncrMap)
{
    bool madeLabelsUnique = false;
    QMultiMap<string, QModelIndex> newMultiMap;
    QModelIndex currentIndex = parentIncludeIndex;
    LSAUNCRMap *uncrMap = guiModel->getUncrMap();
    LSAUNCRMap combinedMap;

    //build up a multimap of all Uncr records beneath the parentIncludeIndex
    while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
    {
        LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
        if(lsaRecord->getRecType() == LSAType::UNCR)
        {
            LSAUncertainty *lsaUncr = static_cast<LSAUncertainty*>(lsaRecord);
            string new_key = lsaUncr->label;
            newMultiMap.insert(new_key, currentIndex);
        }
    }

    //create map of all new enabled AND old disabled UNCRs
    LSAUNCRMap::iterator iter;
    for(iter = uncrMap->begin(); iter != uncrMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSAUncertainty*>(iter->first,iter->second));
    for(iter = oldUncrMap->begin(); iter != oldUncrMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSAUncertainty*>(iter->first,iter->second));

    //loop over the oldUncrMap to look for duplicate labels
    LSAUNCRMap::iterator oldIter;
    for(oldIter = oldUncrMap->begin(); oldIter != oldUncrMap->end(); oldIter++)
    {
        string dupLabel = oldIter->first;
        //check for duplicate labels
        if(newMultiMap.keys().contains(dupLabel))
        {
            //get label base if exists or create a new one (UNCR#N_MEASTYPE or UNCR#N_USERLABEL)
            string labelBase;
            size_t indx = dupLabel.find("UNCR#");
            if(indx == string::npos)
                labelBase = dupLabel;
            else if(indx == 0)
                labelBase = dupLabel.substr(dupLabel.find_first_of('_')+1);

            //loop over the new uncrMap
            LSAUNCRMap::iterator newIter;
            int labelIncrement=1;
            for(newIter = combinedMap.begin(); newIter != combinedMap.end(); newIter++)
            {
                //find all Uncr records in uncrMap with the same label base
                if(newIter->first.find(labelBase) != string::npos)
                {
                    string labelNumberStr;
                    int labelNumber=0;
                    if(newIter->first.find("UNCR#") != string::npos)
                    {
                        size_t beg = newIter->first.find_first_of('#');
                        size_t end = newIter->first.find_first_of('_');
                        labelNumberStr = newIter->first.substr(beg+1,end-beg-1);
                        labelNumber = (int)gnsstk::StringUtils::asInt(labelNumberStr);
                    }
                    //find the largest existing increment number to the label base
                    if((labelNumber != 0) && (labelNumber > labelIncrement))
                        labelIncrement = labelNumber;
                }
            }
            //make the new label Increment one more than the highest existing increment
            labelIncrement += 1;

            //assign new labels to all duplicates in newMultiMap
            QList<QModelIndex> dups = newMultiMap.values(dupLabel);
            for(int i=0; i< dups.size(); ++i)
            {
                string newLabel = string("UNCR#") + gnsstk::StringUtils::asString(labelIncrement) + string("_") + labelBase;
                LSAUncertainty *lsaUncr = static_cast<LSAUncertainty*>(guiModel->getLSARecord(dups.at(i)));
                //update the label on the UNCR record
                lsaUncr->label = newLabel;
                //add the new Uncr record to uncrMap
                uncrMap->insert(std::pair<std::string, LSAUncertainty*>(newLabel, lsaUncr));
                //update the labels on referencing records
                QModelIndexList refRecsIndexList = guiModel->getRecordsReferencingModifier(LSAType::UNCR, dupLabel, parentIncludeIndex);
                for(int i=0;i<refRecsIndexList.size();i++)
                {
                    LSARecord* lsaRecord = guiModel->getLSARecord(refRecsIndexList.at(i));
                    lsaRecord->renameModifierLabel(LSAType::UNCR, dupLabel, newLabel);
                }
                //update the newMultiMap
                newMultiMap.clear();
                currentIndex = parentIncludeIndex;
                while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
                {
                    LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
                    if(lsaRecord->getRecType() == LSAType::UNCR)
                    {
                        LSAUncertainty *lsaUncr = static_cast<LSAUncertainty*>(lsaRecord);
                        string new_key = lsaUncr->label;
                        newMultiMap.insert(new_key, currentIndex);
                    }
                }
                //update combinedMap
                uncrMap = guiModel->getUncrMap();
                for(iter = uncrMap->begin(); iter != uncrMap->end(); iter++)
                    combinedMap.insert(std::pair<string,LSAUncertainty*>(iter->first,iter->second));
                madeLabelsUnique = true;
                labelIncrement++;
            }
        }
    }

    //clean-up now-invalid warning messages on Uncr records
    //Seems risky - could mask bugs
    LSAUNCRMap::iterator newIter;
    for(newIter = uncrMap->begin(); newIter != uncrMap->end(); newIter++)
    {
        LSARecord *lsaRecord = newIter->second;
        if(lsaRecord->warningMessages_generated.find("Duplicate") != lsaRecord->warningMessages_generated.end())
            lsaRecord->warningMessages_generated.clear();
    }

    return madeLabelsUnique;
    /*
    Pseudocode:

    Data Structures:
     LSAUncrMap oldUncrMap: a map of enabled and disabled UNCR records prior to inserting the new include
     LSAUncrMap combinedUncrMap: the combined map of enabled UNCR records in the project after the insert, together with
                                 disabled UNCR records in the project
     LSAUncrMap newUncrMap: map used to index into the newly inserted uncr records

    Algorithm:
     1) Create LSAUncrMap oldUncrMap
     2) Insert the new include record and all of its children
     3) Populate newMultiMap by traversing the tree using index = guiModel->getNextDescendant(parentIncludeIndex)
     4) For each oldLabel in oldUncrMap
        {
            if( newMultiMap.keys().contains(label) )
            {
                a) generate a newLabel to use
                b) make sure that combinedUncrMap doesn't contain newLabel, if it does change it and check combinedUncrMap again
                c) edit all indexes in newMultiMap for oldLabel
                    - replace oldLabel with newLabel
                d) Update newMultiMap to be current with the changes in (c)
                d) traverse the new tree again
                    - if any record in the new tree references sigma oldLabel, change the reference to newLabel
                        A few notes about this step:
                        - there's already some very similar code to this somewhere in MainWindow
                        - this operation will need to be 'undoable' when we implement undo
                        - alternatively, we could build up a list of oldLabel-newLabel pairs, then traverse
                          the tree one time to change all oldLabel instances to the corresponding newLabel
            }
        }
      5) Alternative to step 4d, traverse the tree and change oldLabels to newLabels
    */
}

bool MainWindow::makeHeightLabelsUnique(QModelIndex parentIncludeIndex, LSAHGHTMap *oldHeightMap)
{
    bool madeLabelsUnique = false;
    QMultiMap<string, QModelIndex> newMultiMap;
    QModelIndex currentIndex = parentIncludeIndex;
    LSAHGHTMap *heightMap = guiModel->getHeightMap();
    LSAHGHTMap combinedMap;

    //build up a multimap of all Height records beneath the parentIncludeIndex
    while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
    {
        LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
        if(lsaRecord->getRecType() == LSAType::HGHT)
        {
            LSAHeight *lsaHeight = static_cast<LSAHeight*>(lsaRecord);
            string new_key = lsaHeight->label;
            newMultiMap.insert(new_key, currentIndex);
        }
    }

    //create map of all new enabled AND old disabled HGHTS
    LSAHGHTMap::iterator iter;
    for(iter = heightMap->begin(); iter != heightMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSAHeight*>(iter->first,iter->second));
    for(iter = oldHeightMap->begin(); iter != oldHeightMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSAHeight*>(iter->first,iter->second));

    //loop over the oldHeightMap to look for duplicate labels
    LSAHGHTMap::iterator oldIter;
    for(oldIter = oldHeightMap->begin(); oldIter != oldHeightMap->end(); oldIter++)
    {
        string dupLabel = oldIter->first;
        //check for duplicate labels
        if(newMultiMap.keys().contains(dupLabel))
        {
            //get label base if exists or create a new one (HGHT#N_STATION, or HGHT#N_USERLABEL)
            string labelBase;
            size_t indx = dupLabel.find("HGHT#");
            if(indx == string::npos)
                labelBase = dupLabel;
            else if(indx == 0)
                labelBase = dupLabel.substr(dupLabel.find_first_of('_')+1);

            //loop over the new enabled and old disabled heights
            LSAHGHTMap::iterator newIter;
            int labelIncrement=1;
            for(newIter = combinedMap.begin(); newIter != combinedMap.end(); newIter++)
            {
                //find all Height records in heightMap with the same label base
                if(newIter->first.find(labelBase) != string::npos)
                {
                    string labelNumberStr;
                    int labelNumber = 0;
                    if(newIter->first.find("HGHT#") != string::npos)
                    {
                        size_t beg = newIter->first.find_first_of('#');
                        size_t end = newIter->first.find_first_of('_');
                        labelNumberStr = newIter->first.substr(beg+1,end-beg-1);
                        labelNumber = (int)gnsstk::StringUtils::asInt(labelNumberStr);
                    }
                    //find the largest existing increment number to the label base
                    if(labelNumber != 0 && labelNumber > labelIncrement)
                        labelIncrement = labelNumber;
                }
            }
            //make the new label Increment one more than the highest existing increment
            labelIncrement += 1;

            //assign new labels to all duplicates in newMultiMap
            QList<QModelIndex> dups = newMultiMap.values(dupLabel);
            for(int i=0; i< dups.size(); ++i)
            {
                string newLabel = string("HGHT#") + gnsstk::StringUtils::asString(labelIncrement) + string("_") + labelBase;
                LSAHeight *lsaHeight = static_cast<LSAHeight*>(guiModel->getLSARecord(dups.at(i)));
                //update the label on the Height record
                lsaHeight->label = newLabel;
                //add the new Height record to heightMap
                heightMap->insert(std::pair<std::string, LSAHeight*>(newLabel, lsaHeight));
                //update the labels on referencing records
                QModelIndexList refRecsIndexList = guiModel->getRecordsReferencingModifier(LSAType::HGHT, dupLabel, parentIncludeIndex);
                for(int i=0;i<refRecsIndexList.size();i++)
                {
                    LSARecord* lsaRecord = guiModel->getLSARecord(refRecsIndexList.at(i));
                    lsaRecord->renameModifierLabel(LSAType::HGHT, dupLabel, newLabel);
                }
                //update the newMultiMap
                newMultiMap.clear();
                currentIndex = parentIncludeIndex;
                while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
                {
                    LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
                    if(lsaRecord->getRecType() == LSAType::HGHT)
                    {
                        LSAHeight *lsaHeight = static_cast<LSAHeight*>(lsaRecord);
                        string new_key = lsaHeight->label;
                        newMultiMap.insert(new_key, currentIndex);
                    }
                }
                //update combinedMap
                heightMap = guiModel->getHeightMap();
                for(iter = heightMap->begin(); iter != heightMap->end(); iter++)
                    combinedMap.insert(std::pair<string,LSAHeight*>(iter->first,iter->second));
                madeLabelsUnique = true;
                labelIncrement++;
            }
        }
    }

    //clean-up now-invalid warning messages on Height records
    //Seems risky - could mask bugs
    LSAHGHTMap::iterator newIter;
    for(newIter = heightMap->begin(); newIter != heightMap->end(); newIter++)
    {
        LSARecord *lsaRecord = newIter->second;
        if(lsaRecord->warningMessages_generated.find("Duplicate") != lsaRecord->warningMessages_generated.end() )
            lsaRecord->warningMessages_generated.clear();
    }

    return madeLabelsUnique;
}

bool MainWindow::makeVarScalingLabelsUnique(QModelIndex parentIncludeIndex, LSAVSCAMap *oldVarMap)
{
    bool madeLabelsUnique = false;
    QMultiMap<string, QModelIndex> newMultiMap;
    QModelIndex currentIndex = parentIncludeIndex;
    LSAVSCAMap *varMap = guiModel->getVarScalingMap();
    LSAVSCAMap combinedMap;

    //build up a multimap of all VSCA records beneath the parentIncludeIndex
    while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
    {
        LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
        if(lsaRecord->getRecType() == LSAType::VSCA)
        {
            LSAVarScaling *lsaVar = static_cast<LSAVarScaling*>(lsaRecord);
            string new_key = lsaVar->label;
            newMultiMap.insert(new_key, currentIndex);
        }
    }

    //create map of all new enabled AND old disabled VSCAs
    LSAVSCAMap::iterator iter;
    for(iter = varMap->begin(); iter != varMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSAVarScaling*>(iter->first,iter->second));
    for(iter = oldVarMap->begin(); iter != oldVarMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSAVarScaling*>(iter->first,iter->second));

    //loop over the oldVarMap to look for duplicate labels
    LSAVSCAMap::iterator oldIter;
    for(oldIter = oldVarMap->begin(); oldIter != oldVarMap->end(); oldIter++)
    {
        string dupLabel = oldIter->first;
        //check for duplicate labels
        if(newMultiMap.keys().contains(dupLabel))
        {
            //get label base if exists or create a new one
            string labelBase;
            size_t indx = dupLabel.find_first_of(".")+1;
            if(indx == string::npos)
                labelBase = dupLabel;
            else if(indx == 0)
                labelBase = string("LSA");
            else
                labelBase = dupLabel.substr(0,indx);

            //loop over the new varMap
            LSAVSCAMap::iterator newIter;
            int labelIncrement=0;
            for(newIter = combinedMap.begin(); newIter != combinedMap.end(); newIter++)
            {
                //find all VSCA records in varMap with the same label base
                if(newIter->first.find(labelBase) == 0)
                {
                    string labelNumberStr = newIter->first.substr(labelBase.size());
                    int labelNumber = (int)gnsstk::StringUtils::asInt(labelNumberStr);
                    //find the largest existing increment number to the label base
                    if(labelNumber != 0 && labelNumber > labelIncrement)
                        labelIncrement = labelNumber;
                }
            }
            //make the new label Increment one more than the highest existing increment
            labelIncrement += 1;

            //assign new labels to all duplicates in newMultiMap
            QList<QModelIndex> dups = newMultiMap.values(dupLabel);
            for(int i=0; i< dups.size(); ++i)
            {
                string newLabel = labelBase + gnsstk::StringUtils::asString(labelIncrement);
                LSAVarScaling *lsaVar = static_cast<LSAVarScaling*>(guiModel->getLSARecord(dups.at(i)));
                //update the label on the VSCA record
                lsaVar->label = newLabel;
                //add the new VSCA record to varMap
                varMap->insert(std::pair<std::string, LSAVarScaling*>(newLabel, lsaVar));
                //update the labels on referencing records
                QModelIndexList refRecsIndexList = guiModel->getRecordsReferencingModifier(LSAType::VSCA, dupLabel, parentIncludeIndex);
                for(int i=0;i<refRecsIndexList.size();i++)
                {
                    LSARecord* lsaRecord = guiModel->getLSARecord(refRecsIndexList.at(i));
                    lsaRecord->renameModifierLabel(LSAType::VSCA, dupLabel, newLabel);
                }
                //update newMultiMap to reflect current state of newly inserted VSCAs
                newMultiMap.clear();
                currentIndex = parentIncludeIndex;
                while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
                {
                    LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
                    if(lsaRecord->getRecType() == LSAType::VSCA)
                    {
                        LSAVarScaling *lsaVar = static_cast<LSAVarScaling*>(lsaRecord);
                        string new_key = lsaVar->label;
                        newMultiMap.insert(new_key, currentIndex);
                    }
                }
                //update combinedMap
                varMap = guiModel->getVarScalingMap();
                for(iter = varMap->begin(); iter != varMap->end(); iter++)
                    combinedMap.insert(std::pair<string,LSAVarScaling*>(iter->first,iter->second));
                madeLabelsUnique = true;
                labelIncrement++;
            }
        }
    }

    //clean-up now-invalid warning messages on VSCA records
    //Seems risky - could mask bugs
    LSAVSCAMap::iterator newIter;
    for(newIter = varMap->begin(); newIter != varMap->end(); newIter++)
    {
        LSARecord *lsaRecord = newIter->second;
        if(lsaRecord->warningMessages_generated.find("Duplicate")!=lsaRecord->warningMessages_generated.end() )
            lsaRecord->warningMessages_generated.clear();
    }

    return madeLabelsUnique;
}

bool MainWindow::makeDirGroupLabelsUnique(QModelIndex parentIncludeIndex, LSADirGroupMap *oldDirGroupMap)
{
    bool madeLabelsUnique = false;
    QMultiMap<string, QModelIndex> newMultiMap;
    QModelIndex currentIndex = parentIncludeIndex;
    LSADirGroupMap *dirGroupMap = guiModel->getDirGroupMap();
    LSADirGroupMap combinedMap;

    //build up a multimap of all DirGroup records beneath the parentIncludeIndex
    while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
    {
        LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
        if(lsaRecord->getRecType() == LSAType::DGRP)
        {
            LSADirGroup *lsaDirGroup = static_cast<LSADirGroup*>(lsaRecord);
            string new_key = lsaDirGroup->label;
            newMultiMap.insert(new_key, currentIndex);
        }
    }

    //create map of all new enabled AND old disabled DirGroups
    LSADirGroupMap::iterator iter;
    for(iter = dirGroupMap->begin(); iter != dirGroupMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSADirGroup*>(iter->first,iter->second));
    for(iter = oldDirGroupMap->begin(); iter != oldDirGroupMap->end(); iter++)
        combinedMap.insert(std::pair<string,LSADirGroup*>(iter->first,iter->second));

    //loop over the oldDirGroupMap to look for duplicate labels
    LSADirGroupMap::iterator oldIter;
    for(oldIter = oldDirGroupMap->begin(); oldIter != oldDirGroupMap->end(); oldIter++)
    {
        string dupLabel = oldIter->first;
        //check for duplicate labels
        if(newMultiMap.keys().contains(dupLabel))
        {
            //get label base if exists or create a new one (DGRP#N or USERLABEL#N)
            string labelBase;
            size_t indx = dupLabel.find_first_of("#");
            if(indx == string::npos)
                labelBase = dupLabel;
            else
                labelBase = dupLabel.substr(0,indx+1);

            //loop over the new DirGroupMap
            LSADirGroupMap::iterator newIter;
            int labelIncrement=1;//converter starts incrementing DGRPs at 1
            for(newIter = combinedMap.begin(); newIter != combinedMap.end(); newIter++)
            {
                //find all DGRP records in dirGroupMap with the same label base
                if(newIter->first.find(labelBase) == 0)
                {
                    string labelNumberStr = newIter->first.substr(labelBase.size());
                    int labelNumber = (int)gnsstk::StringUtils::asInt(labelNumberStr);
                    //find the largest existing increment number to the label base
                    if(labelNumber != 0 && labelNumber > labelIncrement)
                        labelIncrement = labelNumber;
                }
            }
            //make the new label Increment one more than the highest existing increment
            labelIncrement += 1;

            //assign new labels to all duplicates in newMultiMap
            QList<QModelIndex> dups = newMultiMap.values(dupLabel);
            for(int i=0; i< dups.size(); ++i)
            {
                string newLabel = labelBase + gnsstk::StringUtils::asString(labelIncrement);
                LSADirGroup *lsaDirGroup = static_cast<LSADirGroup*>(guiModel->getLSARecord(dups.at(i)));
                //update the label on the DirGroup record
                lsaDirGroup->label = newLabel;
                //add the new DirGroup record to dirGroupMap
                dirGroupMap->insert(std::pair<std::string, LSADirGroup*>(newLabel, lsaDirGroup));
                //update the labels on referencing records
                QModelIndexList refRecsIndexList = guiModel->getRecordsReferencingDirGroup(dupLabel, parentIncludeIndex);
                for(int i=0;i<refRecsIndexList.size();i++)
                {
                    LSARecord *lsaRecord = guiModel->getLSARecord(refRecsIndexList.at(i));
                    if(lsaRecord->getRecType() == LSAType::HDIR)
                    {
                        LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);
                        lsaHDir->dirGroupLabel = newLabel;
                    }
                }
                //update newMultiMap to reflect the current state of the newly inserted DGRPs
                newMultiMap.clear();
                currentIndex = parentIncludeIndex;
                while((currentIndex = guiModel->getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
                {
                    LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
                    if(lsaRecord->getRecType() == LSAType::DGRP)
                    {
                        LSADirGroup *lsaDirGroup = static_cast<LSADirGroup*>(lsaRecord);
                        string new_key = lsaDirGroup->label;
                        newMultiMap.insert(new_key, currentIndex);
                    }
                }
                //update combinedMap
                dirGroupMap = guiModel->getDirGroupMap();
                for(iter = dirGroupMap->begin(); iter != dirGroupMap->end(); iter++)
                    combinedMap.insert(std::pair<string,LSADirGroup*>(iter->first,iter->second));
                madeLabelsUnique = true;
                labelIncrement++;
            }
        }
    }

    //clean-up now-invalid warning messages on DirGroup records
    //Seems risky - could mask bugs
    LSADirGroupMap::iterator newIter;
    for(newIter = dirGroupMap->begin(); newIter != dirGroupMap->end(); newIter++)
    {
        LSARecord *lsaRecord = newIter->second;
        if(lsaRecord->warningMessages_generated.find("Duplicate") != lsaRecord->warningMessages_generated.end() )
            lsaRecord->warningMessages_generated.clear();
    }

    return madeLabelsUnique;
}

void MainWindow::runExportScript(QString defaultOutputName, QString selectionFilter, const char *saveFilePrompt, const char *selectionFilterOptions,
                                 QStringList commandLine, PROCESS_TYPES processType, std::string processStart, QString breakerStart, QString breakerEnd,
                                 QString errorString)
{
    if(!h5FileFound())
    {
        return;
    }

    QString outputName;

    if(!isTestHarness())
    {
        // Let the user select a project and parent .lsa file
        suppressProjectChangedDialog = true;
        {
            outputName = QFileDialog::getSaveFileName(this,
                                                    tr(saveFilePrompt),
                                                    defaultOutputName,
                                                    tr(selectionFilterOptions),
                                                    &selectionFilter);
        }
        suppressProjectChangedDialog = false;
    }
    else
        outputName = defaultOutputName;
    commandLine << outputName;

    // Don't try to do anything else if the user cancelled
    if (outputName.isEmpty())
        return;

    runExportScript(commandLine, processType, processStart, breakerStart, breakerEnd, errorString, false);
}

void MainWindow::runExportScript(QStringList commandLine, PROCESS_TYPES processType, std::string processStart, QString breakerStart,
                     QString breakerEnd, QString errorString, bool checkH5Status)
{
    if(checkH5Status)
    {
        if(!h5FileFound())
        {
            return;
        }
    }

    //launch the python script that reads the .h5 file and creates the .xyz file
    procType = processType;
    if(python3Path.isEmpty())
        pushError(QString("Error - unable to find python.exe in SALSA installation folder.  Unable to import instrumentation output."));
    else
    {
        pushBreakerStart(breakerStart);
        pushProcessStart(processStart);
        if(runProcess(python3Path,commandLine))
        {
            QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
            int CLITimeoutMilliseconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt()*1000;
            if(!proc->waitForFinished(CLITimeoutMilliseconds))
                cleanupProcess(breakerEnd);
        }
        else
            pushError(errorString);
    }
}

bool MainWindow::h5FileFound() const
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QFileInfo file(hdfFileName);

    bool fileExists = file.exists();
    if(!fileExists)
    {
        QMessageBox *notifyUser = new QMessageBox(QMessageBox::Critical, // icon
                                                  QString("Unable to open adjustment output file!"), // title
                                                  hdfFileName + QString(" not found."), // text
                                                  QMessageBox::Ok, // buttons
                                                  (QWidget*)this, // parent
                                                  Qt::WindowStaysOnTopHint);
        notifyUser->exec();//blocks
        delete notifyUser;
    }

    return fileExists;
}

void MainWindow::exportPointsXYZ()
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QString cfgFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".cfg");
    QString defaultOutputName = currentFile.left(currentFile.lastIndexOf(".")) + QString("_xyz.csv");
    QString selectionFilter("XYZ CSV Files (*_xyz.csv)");
    const char* saveFilePrompt = "Create a new *_xyz.csv file";
    const char* selectionFilterOptions = "XYZ CSV Files (*_xyz.csv);; All files (*)";
    std::string processStart("h5XYZ.py");
    QString breakerStart("Export Points in XYZ Format");
    QString breakerEnd("XYZ export script");
    QString errorString("Error - unable run XYZ export script.");

    QStringList arguments;
    arguments << QDir::cleanPath(reportingPath + QString("/h5XYZ.py"));
    arguments << "--h5" << QDir::cleanPath(hdfFileName);
    arguments << "--cfg" << QDir::cleanPath(cfgFileName);
    arguments << "--xyz";//the last part of the arguments, the output file name, is added in runExportScript()

    runExportScript(defaultOutputName, selectionFilter, saveFilePrompt, selectionFilterOptions, arguments,
                    PROCESS_REPORTING_XYZ, processStart, breakerStart, breakerEnd, errorString);
}

void MainWindow::exportPointsUTM()
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QString defaultOutputName = currentFile.left(currentFile.lastIndexOf(".")) + QString("_UTM.csv");
    QString selectionFilter("UTM CSV Files (*_UTM.csv)");
    const char* saveFilePrompt = "Create a new *_UTM.csv file";
    const char* selectionFilterOptions = "UTM CSV Files (*_UTM.csv);; All files (*)";
    std::string processStart("h5UTM.py");
    QString breakerStart("Export Points in UTM Format");
    QString breakerEnd("UTM export script");
    QString errorString("Error - unable run UTM export script.");

    QStringList arguments;
    arguments << QDir::cleanPath(reportingPath + QString("/h5UTM.py"));
    arguments << "--in" << QDir::cleanPath(hdfFileName);
    arguments << "--out";//the last part of the arguments, the output file name, is added in runExportScript()

    runExportScript(defaultOutputName, selectionFilter, saveFilePrompt, selectionFilterOptions, arguments,
                    PROCESS_REPORTING_UTM, processStart, breakerStart, breakerEnd, errorString);
}

void MainWindow::exportPointsSPC(QString epsgCode)
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    std::string processStart("h5pyProj.py");
    QString breakerStart("Export Points in SPC Format");
    QString breakerEnd("SPC export script");
    QString errorString("Error - unable run SPC export script.");

    bool ok;
    QString text;
    QString defaultSPC = settings.value(QSETTINGS_SPC_DEFAULT, QString()).toString();
    if(isTestHarness())
    {
        ok = true;
        text = epsgCode;
    }

    else
    {
        text = QInputDialog::getText(this, QString("Projected Coordinates Export Code"),
                                             QString("Enter EPSG Code"), QLineEdit::Normal,
                                             defaultSPC, &ok);
    }


    QString planeCode = QString();
    if(ok && !text.isEmpty())
    {
        planeCode = text;
        settings.setValue(QSETTINGS_SPC_DEFAULT, planeCode);
    }

    else
    {
        // User pressed ok but text is empty
        if(ok)
        {
            pushWarning(QString("Warning - EPSG code not specified. Projected coordinates export script not run."));
        }
        return;
    }

    QStringList arguments;
    arguments << QDir::cleanPath(reportingPath + QString("/h5pyProj.py"));
    arguments << "--in" << QDir::cleanPath(hdfFileName) << "--epsg" << planeCode;

    runExportScript(arguments, PROCESS_REPORTING_SPC, processStart, breakerStart, breakerEnd, errorString, true);
}

void MainWindow::exportPointsNCAT()
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QString defaultOutputName = currentFile.left(currentFile.lastIndexOf(".")) + QString("_NCAT.csv");
    QString selectionFilter("NGS NCAT CSV Files (*_NCAT.csv)");
    const char* saveFilePrompt = "Create a new *_NCAT.csv file";
    const char* selectionFilterOptions = "NGS NCAT CSV Files (*_NCAT.csv);; All files (*)";
    std::string processStart("h5NGS.py");
    QString breakerStart("Export Points in NGS NCAT Format");
    QString breakerEnd("NGS NCAT export script");
    QString errorString("Error - unable run NGS NCAT export script.");

    QStringList arguments;
    arguments << QDir::cleanPath(reportingPath + QString("/h5NGS.py"));
    arguments << "--in" << QDir::cleanPath(hdfFileName);
    arguments << "--out";//the last part of the arguments, the output file name, is added in runExportScript()

    runExportScript(defaultOutputName, selectionFilter, saveFilePrompt, selectionFilterOptions, arguments,
                    PROCESS_REPORTING_NCAT, processStart, breakerStart, breakerEnd, errorString);
}

void MainWindow::exportSolnCov()
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QString defaultOutputName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".cov");
    QString selectionFilter("Soln Cov CSV Files (*.cov)");
    const char* saveFilePrompt = "Create a new *.cov file";
    const char* selectionFilterOptions = "Soln Cov CSV Files (*.cov);; All files (*)";
    std::string processStart("h5SolnCov.py");
    QString breakerStart("Export Solution Covariance Matrix");
    QString breakerEnd("Soln Cov export script");
    QString errorString("Error - unable run Soln Cov export script.");

    QStringList arguments;
    arguments << QDir::cleanPath(reportingPath + QString("/h5SolnCov.py"));
    arguments << "--in" << QDir::cleanPath(hdfFileName);
    if(isScaleByAPV())
        arguments << "--scaleByAPV";
    arguments << "--cov";//the last part of the arguments, the output file name, is added in runExportScript()

    runExportScript(defaultOutputName, selectionFilter, saveFilePrompt, selectionFilterOptions, arguments,
                    PROCESS_REPORTING_SOLNCOV, processStart, breakerStart, breakerEnd, errorString);
}

void MainWindow::exportMeasResid()
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QString defaultOutputName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".resid.csv");
    QString selectionFilter("Meas Resid CSV Files (*.resid.csv)");
    const char* saveFilePrompt = "Create a new *.resid.csv file";
    const char* selectionFilterOptions = "Meas Resid CSV Files (*.resid.csv);; All files (*)";
    std::string processStart("h5SolnMeasurements.py");
    QString breakerStart("Export Measurement Residuals");
    QString breakerEnd("Meas Resid export script");
    QString errorString("Error - unable to run Measurement Residuals export script.");

    QStringList arguments;
    arguments << QDir::cleanPath(reportingPath + QString("/h5SolnMeasurements.py"));
    arguments << "--in" << QDir::cleanPath(hdfFileName);
    arguments << "--csv";//the last part of the arguments, the output file name, is added in runExportScript()

    runExportScript(defaultOutputName, selectionFilter, saveFilePrompt, selectionFilterOptions, arguments,
                    PROCESS_REPORTING_MEASRESID, processStart, breakerStart, breakerEnd, errorString);
}

/**
 * @brief MainWindow::exportMeasResidAsENU Calls the h5SolnMeasurementsENU.py script to
 * export the raw residuals in ENU to a csv file.
 */
void MainWindow::exportMeasResidAsENU()
{
    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    QString defaultOutputName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".enuresid.csv");
    QString selectionFilter("ENU Meas Resid CSV Files (*.enuresid.csv)");
    const char* saveFilePrompt = "Create a new *.enuresid.csv file";
    const char* selectionFilterOptions = "ENU Meas Resid CSV Files (*.enuresid.csv);; All files (*)";
    std::string processStart("h5SolnMeasurementsENU.py");
    QString breakerStart("Export ENU Measurement Residuals");
    QString breakerEnd("ENU Meas Resid export script");
    QString errorString("Error - unable to run ENU Measurement Residuals export script.");

    QStringList arguments;
    arguments << QDir::cleanPath(reportingPath + QString("/h5SolnMeasurementsENU.py"));
    arguments << "--in" << QDir::cleanPath(hdfFileName);
    arguments << "--csv";//the last part of the arguments, the output file name, is added in runExportScript()

    runExportScript(defaultOutputName, selectionFilter, saveFilePrompt, selectionFilterOptions, arguments,
                    PROCESS_REPORTING_MEASRESID, processStart, breakerStart, breakerEnd, errorString);
}

bool MainWindow::runReportingScripts()
{
    QStringList arguments;
    QString hdfFileName, cfgFileName, pcrFileName, ptsFileName, csvFileName, geoidFileName;
    bool reportScriptsTimedOut = false;

    //assign input and output file names for script arguments
    if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
    {
        cfgFileName = currentFile + QString(".cfg");
        pcrFileName = currentFile + QString(".pcr");
        ptsFileName = currentFile + QString(".pts");
        csvFileName = currentFile + QString(".csv");
    }
    else
    {
        cfgFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".cfg");
        pcrFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".pcr");
        ptsFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".pts");
        csvFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".csv");
    }

    if(lastSolverRunType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)//export geoid heights
    {
        if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
        {
            hdfFileName = currentFile + QString(".aph5");
        }
        else
        {
            hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".aph5");
        }

        if(!isTestHarness())
        {
            QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
            QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~").toString();
            QString selfilter = tr("Geoid height file (*.geo)");
            csvFileName = QFileDialog::getSaveFileName(this,
                                                       tr("Create a new .geo file"),
                                                       lastLSAPath,
                                                       tr("Geoid height files (*.geo);; All files (*)"),
                                                          &selfilter);
        }

        else
        {
            QDir projectDir = QFileInfo(currentFile).absoluteDir();
            csvFileName = projectDir.filePath("geoidheights.geo");
        }
    }
    else
    {
        if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
        {
            hdfFileName = currentFile + QString(".h5");
        }
        else
        {
            hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
        }
    }

    hdfFileName = QDir::cleanPath(hdfFileName);
    cfgFileName = QDir::cleanPath(cfgFileName);
    pcrFileName = QDir::cleanPath(pcrFileName);
    ptsFileName = QDir::cleanPath(ptsFileName);
    csvFileName = QDir::cleanPath(csvFileName);

    if(isGeoidFileUsed())
    {
        geoidFileName = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../data/geoid/egm1996_2.5m.und");
    }

    // Grab current station labels from h5 File
    std::vector<std::string> stationList;


    for(int i=0;i<currentHDF5File.llh->getSize();i++)
    {
        std::string stationLabel = currentHDF5File.llh->getDataPoint(i).label;
        stationList.push_back(stationLabel);
    }

    std::sort(stationList.begin(), stationList.end());

    // Construct LSAInvPair data object
    QFileInfo hdf5Info(hdfFileName);
    QString fileDir = hdf5Info.path();
    QString fileBase = hdf5Info.baseName();
    QString normalFileOut = fileBase + ".inv";
    LSAInvPairData lsaPairInfo(fileDir, fileBase);

    std::vector<LSAInvPairData::fromToPair> stationRepeats = lsaPairInfo.giveDuplicateList();
    std::map<LSAInvPairData::fromToPair, bool> statRepeatMap;

    for(const auto &repeatElem : stationRepeats)
    {
        statRepeatMap[repeatElem] = false;
    }

    std::vector<std::string>::const_iterator fromIter, toIter;
    std::map<LSAInvPairData::fromToPair, bool>::iterator repeatIter;
    QStringList lsaInverseArgs;

    QProcess localProc(this);
    QStringList invalidPairList;

    std::vector<LSAInvPairData::fromToPair> fromToPairList = lsaPairInfo.giveStationPairList();

    for(std::vector<LSAInvPairData::fromToPair>::const_iterator sPairsIter = fromToPairList.begin();
        sPairsIter != fromToPairList.end(); ++sPairsIter)
    {
        // If first element in container, check for output files and clear them if they exist
        if(sPairsIter == fromToPairList.begin())
        {
            QFileInfo baseOutput(QDir(fileDir), normalFileOut);

            if(baseOutput.exists())
            {
                QFile baseOutFile(baseOutput.filePath());
                baseOutFile.open(QIODevice::ReadWrite | QIODevice::Text);
                baseOutFile.resize(0);
                baseOutFile.close();
            }

            QFileInfo csvOutput(QDir(fileDir), fileBase + QString::fromStdString("Inv.csv"));

            if(csvOutput.exists())
            {
                QFile csvOutputFile(csvOutput.filePath());
                csvOutputFile.open(QIODevice::ReadWrite | QIODevice::Text);
                csvOutputFile.resize(0);
                csvOutputFile.close();
            }
        }

        // Check to see that both from and to label provided are in stationList.
        // If so, valid pair otherwise invalid pair
        fromIter = std::find(stationList.begin(), stationList.end(), sPairsIter->first);
        toIter = std::find(stationList.begin(), stationList.end(), sPairsIter->second);

        if(fromIter == stationList.end() || toIter == stationList.end())
        {
            invalidPairList << "<" + QString::fromStdString(sPairsIter->first) + ", " + QString::fromStdString(sPairsIter->second) + ">";
            continue;
        }

        // Now check to see if pair is a repeat or not. If so check mapped boolean.
        // If true do continue. Otherwise mark true so other repeats don't have
        // their values added to csv

        repeatIter = statRepeatMap.find(*sPairsIter);
        if(repeatIter != statRepeatMap.end())
        {
            if(repeatIter->second)
            {
                continue;
            }

            else
            {
                repeatIter->second = true;
            }
        }

        // At this point if continue statement hasn't executed safe to pass info to lsa inverse
        // Construct args in a similar manner to StationDataDialog::runLSAInverse() in instance
        // where exportTriggered = true

        lsaInverseArgs << "--from" << QString::fromStdString(sPairsIter->first);
        lsaInverseArgs << "--to" << QString::fromStdString(sPairsIter->second);
        lsaInverseArgs << "--hdf5" << hdfFileName;

        if(isScaleByAPV())
        {
            lsaInverseArgs << "--scaleByAPV";
        }

        if(usingWestLongitude())
        {
            lsaInverseArgs << "--westLon";
        }

        lsaInverseArgs << "--log" << normalFileOut;
        lsaInverseArgs << "--logpath" << hdf5Info.absolutePath();

        localProc.start(lsainverse, lsaInverseArgs);
        if(localProc.waitForFinished())
        {
            std::cout << "LSAinverse executed fine." << std::endl;
        }

        else
        {
            std::cout << "LSAinverse did not execute." << std::endl;
        }

        lsaInverseArgs.clear();
    }

    if(!invalidPairList.empty())
    {
        QString warningMessage = "Could not produce station pair data for following pair(s): " + invalidPairList.join("; ");
        emit pushWarning(warningMessage);
    }

    //launch the .csv file generation script
    arguments.clear();
    arguments << reportingPath + QString("/h5ReportGenerator.py");
    arguments << "--h5" << hdfFileName;
    arguments << "--cfg" << cfgFileName;
    arguments << "--csv" << csvFileName;
    arguments << "--cgeoid" << cgeoidPath;
    if(lastSolverRunType != PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)//don't generate .pts/.pcr files when exporting geoid heights
    {
        arguments << "--pcr" << pcrFileName;
        arguments << "--pts" << ptsFileName;
    }
    if(isGeoidFileUsed())
        arguments << "--geo" << geoidFileName;
    if(lastSolverRunType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
        arguments << "--apriori";

    if(python3Path.isEmpty())
        pushError(QString("Error - unable to find python.exe in SALSA installation folder.  Unable to import instrumentation output."));
    else
    {
        procType = PROCESS_REPORTING_CSV;
        pushProcessStart("h5ReportGenerator.py");

        if(runProcess(python3Path, arguments))
        {
            QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
            int CLITimeoutMilliseconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt()*1000;
            if(!proc->waitForFinished(CLITimeoutMilliseconds))
                reportScriptsTimedOut = true;
        }
        else
            pushError(QString("Error - unable run h5ReportGenerator script."));
    }

    if(reportScriptsTimedOut) cleanupProcess(QString("h5ReportGenerator script"));

    if(reportScriptsTimedOut)
        return false;
    else
        return true;

}

void MainWindow::startProgressBar()
{
    //toggle progress bar and OutputData visibility
    ui->progressBar->show();
    ui->progressBar->setValue(0);
    ui->frameOutputData->hide();
    ui->frameOutputTables->hide();
    progressBarHasBeenShown = true; //this variable is only for testing purposes

    //adjust central splitter positions to shrink to fit progress bar
    QList<int> currentSizes = ui->centralSplitter->sizes();
    //save current sizes
    previousSizes = currentSizes;
    int progressBarHeight = ui->progressBar->height();
    int progressBarDiff = currentSizes[1] - progressBarHeight;
    currentSizes[1] = progressBarHeight;
    currentSizes[2] = currentSizes[2] + progressBarDiff;
    ui->centralSplitter->setSizes(currentSizes);
}

void MainWindow::updateProgressBar(QString firstWord, QString line)
{
    if ( firstWord.contains("convergence", Qt::CaseInsensitive) )
    {
        QRegularExpression whitespace("[\\s]");
        QString iterationConvergence = line.split(whitespace).at(6);
        double V =  iterationConvergence.toDouble();
        QString convergenceThreshold = line.split(whitespace).at(8);
        double Eps =  convergenceThreshold.toDouble();
        double Power = 6.0; //Tuning - how linear progress will look
        double progress = pow((Eps/V),(1/Power));
        progress = progress * 100 / 2;  // Progress bar should be at 50% once the solver converges
        ui->progressBar->setValue(progress);
    }
    else if ( line.contains("lsapost, Ver", Qt::CaseInsensitive) )
    {
        ui->progressBar->setValue(80);
    }
    else if ( line.contains("h5CSV, Ver", Qt::CaseInsensitive) )
    {
        ui->progressBar->setValue(90);
    }
    else if ( line.contains("h5PTS, Ver", Qt::CaseInsensitive) )
    {
        ui->progressBar->setValue(95);
    }
    else if ( line.contains("h5PTS Timing", Qt::CaseInsensitive) )
    {
        finishProgressBar();
    }
}

void MainWindow::finishProgressBar()
{
    //solver finished, hide progress bar
    ui->progressBar->setValue(0);
    //toggle progress bar and output data visibility
    ui->progressBar->hide();
    ui->frameOutputData->show();
    ui->frameOutputTables->show();

    //adjust central splitter positions to expand to fit frame output data/tables
    //if statement catches case where finishProgressBar was called before startProgressBar() was called
    if(previousSizes.empty()){
        previousSizes = ui->centralSplitter->sizes();
    }
    else
        ui->centralSplitter->setSizes(previousSizes);
}

void MainWindow::exportPOSGsFromHDF5File(bool apriori)
{
    //Attempt to load the projects *.h5 file, if it exists
    lastSolverRunType = PROCESS_LSASOLVER;//Fix to Bug #258 where the currentHDF5 file was a *.aph5 so exporting the points would fail
    bool readHDF5Success = readHDF5File(false);

    QString hdfFileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
    if(!readHDF5Success)
    {
        QMessageBox *notifyUser = new QMessageBox(QMessageBox::Critical, // icon
                                                  QString("Unable to open adjustment output file!"), // title
                                                  hdfFileName + QString(" not found."), // text
                                                  QMessageBox::Ok, // buttons
                                                  (QWidget*)this, // parent
                                                  Qt::WindowStaysOnTopHint);

        notifyUser->exec();//blocks
        delete notifyUser;
    }
    else
    {
        QString filename;
        if(!isTestHarness())
        {
            //prompt user for name of exported file
            filename = createNewEmptyLSAFile();
            if(filename.isEmpty())
                return;
        }
        else
        {
            QDir projectDir = QFileInfo(currentFile).absoluteDir();
            filename = projectDir.filePath("adjusted.lsa");
        }

        //open file
        ofstream oflsa;
        oflsa.open(filename.toStdString().c_str(), ios::out);
        if(!oflsa.is_open())
        {
            pushError(QString("The file ") + filename +  QString(" could not be opened."));
            return;
        }
        oflsa << LSAVERSION_FILE_STRING << endl;
        QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        oflsa << "# Exported from SALSA using project " << currentFile.toStdString() << " on " << dateTime.toStdString() << endl << endl;

        if(apriori)
        {
            //TODO
        }
        else
        {
            //loop over all final points (adjusted, fixed, derived, unused)
            for(int i=0; i<currentHDF5File.llh->getSize(); i++)
            {
               //honor user's choice on the config panel to include unused points or not
               if(!currentHDF5File.llh->getDataPoint(i).utilized && !includeUnused())
                       continue;

                LSAPosG *outputPosG = new LSAPosG();

                //check if point was part of the input.  If so, update its LSAString with the output values.
                std::map<std::string, LSARecord*>::iterator itr;
                LSARecordMap *pointMap = guiModel->getPoints();
                itr = pointMap->find(currentHDF5File.llh->getDataPoint(i).label);
                if(itr != pointMap->end())
                {
                    if(itr->second->getRecType() == LSAType::POSC)
                    {
                        LSAPosC *lsaPosC = static_cast<LSAPosC*>(itr->second);
                        outputPosG->fixedState = lsaPosC->fixedState;
                        outputPosG->isEastFixed = lsaPosC->isEastFixed;
                        outputPosG->isNorthFixed = lsaPosC->isNorthFixed;
                        outputPosG->isUpFixed = lsaPosC->isUpFixed;
                    }
                    else if(itr->second->getRecType() == LSAType::POSG)
                    {
                        LSAPosG *lsaPosG = static_cast<LSAPosG*>(itr->second);
                        outputPosG->fixedState = lsaPosG->fixedState;
                        outputPosG->isEastFixed = lsaPosG->isEastFixed;
                        outputPosG->isNorthFixed = lsaPosG->isNorthFixed;
                        outputPosG->isUpFixed = lsaPosG->isUpFixed;
                    }
                }

                outputPosG->label = currentHDF5File.llh->getDataPoint(i).label;
                outputPosG->isAutogenerated = false;//these are adjusted positions
                //update the position and covariance info
                outputPosG->useDecimalDegrees = false;
                int dmsDegrees = 0;
                int dmsMinutes = 0;
                double dmsSeconds = 0.0;
                bool is_neg;
                degToDMS(currentHDF5File.llh->getDataPoint(i).lat, is_neg, dmsDegrees, dmsMinutes, dmsSeconds);
                outputPosG->latDeg = dmsDegrees;
                if(is_neg)
                    outputPosG->latDeg *= -1;
                outputPosG->latMin = dmsMinutes;
                outputPosG->latSec = dmsSeconds;
                outputPosG->latDir = string("N");

                double lonDecDeg = currentHDF5File.llh->getDataPoint(i).lon;
                if(usingWestLongitude())
                {
                   lonDecDeg = 360.0 - lonDecDeg;
                    if(lonDecDeg >= 360.0)
                        lonDecDeg -= 360.0;
                    outputPosG->lonDir = string("W");
                }
                else
                {
                    outputPosG->lonDir = string("E");
                }
                degToDMS(lonDecDeg, is_neg, dmsDegrees, dmsMinutes, dmsSeconds);
                outputPosG->lonDeg = dmsDegrees;
                if(is_neg)
                    outputPosG->lonDeg *= -1;
                outputPosG->lonMin = dmsMinutes;
                outputPosG->lonSec = dmsSeconds;

                outputPosG->height = currentHDF5File.llh->getDataPoint(i).ht;
                if(outputPosG->fixedState != LSAFixedState::FIXED)
                {
                    outputPosG->hasCovariance = true;
                    outputPosG->covnn = currentHDF5File.llh->getDataPoint(i).Cnn;
                    outputPosG->covne = currentHDF5File.llh->getDataPoint(i).Cne;
                    outputPosG->covnu = currentHDF5File.llh->getDataPoint(i).Cnu;
                    outputPosG->covee = currentHDF5File.llh->getDataPoint(i).Cee;
                    outputPosG->coveu = currentHDF5File.llh->getDataPoint(i).Ceu;
                    outputPosG->covuu = currentHDF5File.llh->getDataPoint(i).Cuu;
                }
                else
                    outputPosG->hasCovariance = false;
                outputPosG->modifiers.setHasVSCACorrection(false);

                //write modified string to output file
                outputPosG->writeLSAString(oflsa);

                delete(outputPosG);
            }//end of loop over currentHDF5File.llhContainer
        }//end of else on a priori

        oflsa.close();
    }//end of else of HDF5 file exists
}

void MainWindow::generateInitialPositions()
{
    calculateAdjustment(PROCESS_LSASOLVER_APQUIT);
}

void MainWindow::addAutogeneratedPoints()
{
    QModelIndexList selectedRows = guiModel->getSelectedRows();
    if(selectedRows.size() == 0)
        return;

    guiModel->addAutogenPointsToProject(selectedRows);
}

void MainWindow::exportGeoidHeightsFromAprioriPositions()
{
    calculateAdjustment(PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS);
}

void MainWindow::handleTablePointsDoubleClicked(QModelIndex doubleclickedIndex)
{
    //NOTE: for some reason double-clicking on the table row seems to call this twice
    QModelIndex currentIndex = ui->tablePoints->selectionModel()->currentIndex();
    int row = currentIndex.row();
    string pointName = ui->tablePoints->model()->data(ui->tablePoints->model()->index(row,POINT_TABLE_POINT_COLUMN)).toString().toStdString();
    if(pointName.empty())
        return;//no point in the row
    QModelIndex pointIndex = guiModel->getIndexFromPointLabel(pointName);
    if(pointIndex.isValid())
    {
        ui->treeView->scrollTo(pointIndex, QAbstractItemView::PositionAtCenter);
        updateWidgets();
    }
}

void MainWindow::handleStatusWindowAnchorClicked(QUrl link)
{
    QDesktopServices::openUrl(link);
}

void MainWindow::selectOutputTablesRows(QModelIndexList selectedPoints, QModelIndexList selectedMeasurements)
{
    selectPointsTableRows(selectedPoints);
    selectMeasurementTableRows(selectedMeasurements);
}

void MainWindow::getFromAtToStringsFromLSARecord(LSARecord *lsaRecord, string &FromAt, string &To)
{
    LSAType lsaType = lsaRecord->getRecType();
    if(lsaType ==  LSAType::AZIM)
    {
        LSAAzimuth *lsaAzimuth = static_cast<LSAAzimuth*>(lsaRecord);
        FromAt = lsaAzimuth->From;
        To = lsaAzimuth->To;
    }
    else if(lsaType == LSAType::DIST)
    {
        LSADist *lsaDist = static_cast<LSADist*>(lsaRecord);
        FromAt = lsaDist->From;
        To = lsaDist->To;
    }
    else if(lsaType == LSAType::DXYZ)
    {
        LSADelta *lsaDelta = static_cast<LSADelta*>(lsaRecord);
        FromAt = lsaDelta->From;
        To = lsaDelta->To;
    }
    else if(lsaType == LSAType::HANG)
    {
        LSAHAngle *lsaHangle = static_cast<LSAHAngle*>(lsaRecord);
        FromAt = lsaHangle->From + string("-") + lsaHangle->At;
        To = lsaHangle->To;
    }
    else if(lsaType == LSAType::HDIF)
    {
        LSAHeightDiff *lsaHeightDiff = static_cast<LSAHeightDiff*>(lsaRecord);
        FromAt = lsaHeightDiff->From;
        To = lsaHeightDiff->To;
    }
    else if(lsaType == LSAType::HDIR)
    {
        LSAHDir *lsaHdir = static_cast<LSAHDir*>(lsaRecord);
        To = lsaHdir->toLabel;

        LSADirGroupMap *dgrpMap = guiModel->getDirGroupMap();
        if (dgrpMap->find(lsaHdir->dirGroupLabel) != dgrpMap->end())
        {
            LSADirGroup *dirGroup = dgrpMap->find(lsaHdir->dirGroupLabel)->second;
            FromAt = dirGroup->fromLabel;
        }
    }
    else if(lsaType == LSAType::VANG)
    {
        LSAVAngle *lsaVangle = static_cast<LSAVAngle*>(lsaRecord);
        FromAt = lsaVangle->From;
        To = lsaVangle->To;
    }
    else if(lsaType == LSAType::ZANG)
    {
        LSAZAngle *lsaZangle = static_cast<LSAZAngle*>(lsaRecord);
        FromAt = lsaZangle->From;
        To = lsaZangle->To;
    }
}

void MainWindow::addOutputTableHeaders()
{
    addResidualTableHeaders();
    addConfidenceTableHeaders();
}

void MainWindow::addResidualTableHeaders()
{
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_MEAS_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_MEAS_COLUMN)));
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_FROMAT_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_FROMAT_COLUMN)));
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_TO_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_TO_COLUMN)));
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_RAW_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_RAW_COLUMN)));
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_REL_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_REL_COLUMN)));
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_STD_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_STD_COLUMN)));
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_REDUND_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_REDUND_COLUMN)));
    topResidModel->setHorizontalHeaderItem(RESID_TABLE_MAX_BIAS_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_MAX_BIAS_COLUMN)));
    if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect)
    {
       topResidModel->setHorizontalHeaderItem(RESID_TABLE_EXT_MAG_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_EXT_MAG_COLUMN)));
    }
    else
    {
       topResidModel->setHorizontalHeaderItem(RESID_TABLE_EXT_MAG_COLUMN, new QStandardItem(residColumnToColumnHeaderMap.at(RESID_TABLE_EXT_MAG_COLUMN) + " (N/A)"));
    }
    auto header = ui->tableMeasurements->horizontalHeader();
    auto currFont = header->font();
    currFont.setPointSize(getIdealFontSize());
    header->setFont(currFont);
}

void MainWindow::addConfidenceTableHeaders()
{
    confidenceModel->setHorizontalHeaderItem(POINT_TABLE_POINT_COLUMN, new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_POINT_COLUMN)));
    confidenceModel->setHorizontalHeaderItem(POINT_TABLE_3DMAJ_COLUMN, new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_3DMAJ_COLUMN)));
    confidenceModel->setHorizontalHeaderItem(POINT_TABLE_2DMAJ_COLUMN, new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_2DMAJ_COLUMN)));
    confidenceModel->setHorizontalHeaderItem(POINT_TABLE_VERT_COLUMN,  new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_VERT_COLUMN)));

    if(currentHDF5File.projCfg->getDataPoint().calcExtRelVect)
    {
       confidenceModel->setHorizontalHeaderItem(POINT_TABLE_3DMAJRR_COLUMN, new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_3DMAJRR_COLUMN)));
       confidenceModel->setHorizontalHeaderItem(POINT_TABLE_2DMAJRR_COLUMN, new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_2DMAJRR_COLUMN)));
       confidenceModel->setHorizontalHeaderItem(POINT_TABLE_VERTRR_COLUMN,  new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_VERTRR_COLUMN)));
    }
    else
    {
       confidenceModel->setHorizontalHeaderItem(POINT_TABLE_3DMAJRR_COLUMN, new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_3DMAJRR_COLUMN) + " (N/A)"));
       confidenceModel->setHorizontalHeaderItem(POINT_TABLE_2DMAJRR_COLUMN, new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_2DMAJRR_COLUMN) + " (N/A)"));
       confidenceModel->setHorizontalHeaderItem(POINT_TABLE_VERTRR_COLUMN,  new QStandardItem(pointColumnToColumnHeaderMap.at(POINT_TABLE_VERTRR_COLUMN)  + " (N/A)"));
    }
    auto header = ui->tablePoints->horizontalHeader();
    auto currFont = header->font();
    currFont.setPointSize(getIdealFontSize());
    header->setFont(currFont);
}


void MainWindow::setOutputTableColumnWidths()
{
    setResidualTableColumnWidths();
    setConfidenceTableColumnWidths();
}

void MainWindow::toggleResidualsTableColumn(bool bShow)
{
    // count the number of visible columns
    int nShown = 0;
    for (const auto& column : mapResidualsActionToColumn)
    {
        if(!ui->tableMeasurements->isColumnHidden(column.second))
        {
            ++nShown;
        }
    }
    // Don't allow the user to hide the last column, because there's no way for them
    // to get them back.
    if(nShown == 1 && !bShow)
    {
        return;
    }

    // Figure out which action sent the signal
    if(QAction* sender = qobject_cast<QAction*>( QObject::sender()))
    {
        // Find the action's column's index in the table
        auto column = mapResidualsActionToColumn.find(sender);
        if(column != mapResidualsActionToColumn.end())
        {
            ui->tableMeasurements->setColumnHidden(column->second, !bShow);
            if (bShow)
            {
                ui->tableMeasurements->resizeColumnToContents(column->second);
            }
        }
    }
}

void MainWindow::togglePointsTableColumn(bool bShow)
{
    int nShown = 0;
    for (const auto& column : mapPointsActionToColumn)
    {
        if(!ui->tablePoints->isColumnHidden(column.second))
        {
            ++nShown;
        }
    }
    // Don't allow the user to hide the last column, because there's no way for them
    // to get them back.
    if(nShown == 1 && !bShow)
    {
        return;
    }

    // Figure out which action sent the signal
    if(QAction* sender = qobject_cast<QAction*>( QObject::sender()))
    {
        // Find the action's column's index in the table
        auto column = mapPointsActionToColumn.find(sender);
        if(column != mapPointsActionToColumn.end())
        {
            ui->tablePoints->setColumnHidden(column->second, !bShow);
            if (bShow)
            {
                ui->tablePoints->resizeColumnToContents(column->second);
            }
        }
    }
}

void MainWindow::setConfidenceTableColumnWidths()
{
    // Set column widths to previous values
    ui->tablePoints->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    int confidenceWidth0 = settings.value(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_0, 75).toInt();
    int confidenceWidth1 = settings.value(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_1, 75).toInt();
    int confidenceWidth2 = settings.value(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_2, 75).toInt();
    int confidenceWidth3 = settings.value(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_3, 75).toInt();
    int confidenceWidth4 = settings.value(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_4, 100).toInt();
    int confidenceWidth5 = settings.value(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_5, 100).toInt();
    int confidenceWidth6 = settings.value(QSETTINGS_CONFIDENCE_COLUMN_WIDTH_6, 100).toInt();


    ui->tablePoints->setColumnWidth(POINT_TABLE_POINT_COLUMN, confidenceWidth0);
    ui->tablePoints->setColumnWidth(POINT_TABLE_3DMAJ_COLUMN, confidenceWidth1);
    ui->tablePoints->setColumnWidth(POINT_TABLE_2DMAJ_COLUMN, confidenceWidth2);
    ui->tablePoints->setColumnWidth(POINT_TABLE_VERT_COLUMN, confidenceWidth3);
    ui->tablePoints->setColumnWidth(POINT_TABLE_3DMAJRR_COLUMN, confidenceWidth4);
    ui->tablePoints->setColumnWidth(POINT_TABLE_2DMAJRR_COLUMN, confidenceWidth5);
    ui->tablePoints->setColumnWidth(POINT_TABLE_VERTRR_COLUMN, confidenceWidth6);

    // if the column width is 0, then set the column as hidden
    if (!confidenceWidth0) {ui->tablePoints->hideColumn(POINT_TABLE_POINT_COLUMN);}
    if (!confidenceWidth1) {ui->tablePoints->hideColumn(POINT_TABLE_3DMAJ_COLUMN);}
    if (!confidenceWidth2) {ui->tablePoints->hideColumn(POINT_TABLE_2DMAJ_COLUMN);}
    if (!confidenceWidth3) {ui->tablePoints->hideColumn(POINT_TABLE_VERT_COLUMN);}
    if (!confidenceWidth4) {ui->tablePoints->hideColumn(POINT_TABLE_3DMAJRR_COLUMN);}
    if (!confidenceWidth5) {ui->tablePoints->hideColumn(POINT_TABLE_2DMAJRR_COLUMN);}
    if (!confidenceWidth6) {ui->tablePoints->hideColumn(POINT_TABLE_VERTRR_COLUMN);}
}

void MainWindow::setResidualTableColumnWidths()
{
    // get initial column widths
    ui->tableMeasurements->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    int residualWidth0 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_0, 75).toInt();
    int residualWidth1 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_1, 75).toInt();
    int residualWidth2 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_2, 75).toInt();
    int residualWidth3 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_3, 75).toInt();
    int residualWidth4 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_4, 75).toInt();
    int residualWidth5 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_5, 75).toInt();
    int residualWidth6 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_6, 75).toInt();
    int residualWidth7 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_7, 75).toInt();
    int residualWidth8 = settings.value(QSETTINGS_RESIDUALS_COLUMN_WIDTH_8, 100).toInt();

    // Set columns widths to initial values
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_MEAS_COLUMN, residualWidth0);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_FROMAT_COLUMN, residualWidth1);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_TO_COLUMN, residualWidth2);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_RAW_COLUMN, residualWidth3);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_REL_COLUMN, residualWidth4);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_STD_COLUMN, residualWidth5);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_REDUND_COLUMN, residualWidth6);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_MAX_BIAS_COLUMN, residualWidth7);
    ui->tableMeasurements->setColumnWidth(RESID_TABLE_EXT_MAG_COLUMN, residualWidth8);

    // if the column width is 0, then set the column as hidden
    if (!residualWidth0) {ui->tableMeasurements->hideColumn(RESID_TABLE_MEAS_COLUMN);}
    if (!residualWidth1) {ui->tableMeasurements->hideColumn(RESID_TABLE_FROMAT_COLUMN);}
    if (!residualWidth2) {ui->tableMeasurements->hideColumn(RESID_TABLE_TO_COLUMN);}
    if (!residualWidth3) {ui->tableMeasurements->hideColumn(RESID_TABLE_RAW_COLUMN);}
    if (!residualWidth4) {ui->tableMeasurements->hideColumn(RESID_TABLE_REL_COLUMN);}
    if (!residualWidth5) {ui->tableMeasurements->hideColumn(RESID_TABLE_STD_COLUMN);}
    if (!residualWidth6) {ui->tableMeasurements->hideColumn(RESID_TABLE_REDUND_COLUMN);}
    if (!residualWidth7) {ui->tableMeasurements->hideColumn(RESID_TABLE_MAX_BIAS_COLUMN);}
    if (!residualWidth8) {ui->tableMeasurements->hideColumn(RESID_TABLE_EXT_MAG_COLUMN);}
}


QString MainWindow::getFileNameFromFullPath(QString fullPath)
{
    QFile File(fullPath);
    QFileInfo fileInfo(File.fileName());
    QString fileName = fileInfo.fileName();

    return fileName;
}

bool MainWindow::updateLegacyConfigFile()
{
    QFile configFile;
    QString cfgFileName = currentFile.left(currentFile.lastIndexOf(".")) + ".cfg";
    QString tmpFileName = cfgFileName.left(cfgFileName.lastIndexOf(".")) + QString(".tmp");
    QString changes;
    configFile.setFileName(cfgFileName);
    // Check for the export scripts
    QString exportScriptDirectoryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../scripts/reporting/extras");
    QDir exportScriptDir(exportScriptDirectoryPath);
    bool updated = false;
    bool customExportHandled = false;


    // If config file exists, check for obsolete parameters
    if ( configFile.exists() )
    {
        std::ifstream istrm;
        istrm.open(cfgFileName.toStdString().c_str(),std::ios::in);
        if(!istrm.is_open())
        {
            pushWarning(QString("Unable to check for obsolete .cfg file."));
            return updated;
        }
        std::ofstream ostrm;
        ostrm.open(tmpFileName.toStdString().c_str(),std::ios::out);
        if(!ostrm.is_open())
        {
            istrm.close();
            pushWarning(QString("Unable to check for obsolete .cfg file."));
            return updated;
        }
        ostrm << "# " << LSAVERSION_FILE_STRING << std::endl;

        std::string line = std::string("");
        while(1)
        {
           line = std::string("");
           getline(istrm,line);
           if((istrm.eof() || !istrm.good()) && line.empty()) break;
           if(line.empty())
               continue;

           //preserve comments, except previous version string
           if(line.at(0)=='#')
           {
               if((line.find("Created") != std::string::npos) || (line.find("Deployed") != std::string::npos))
                   continue;
               else
                   ostrm << line << std::endl;
           }
           //replace obsolete parameters
           else if(line.find(std::string("--linprec "))!=std::string::npos)
           {
               ostrm << "--linprecP " << lsa::DEFAULT_NUM_DECIMALS_LINEAR_POSITION_METERS << std::endl;
               ostrm << "--linprecM " << lsa::DEFAULT_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS << std::endl;
               changes += QString("Replaced --linprec with --linprecP and --linprecM.\n");
               updated = true;
           }
           else if(line.find(std::string("--customExportCopy ")) !=std::string::npos)
           {
              customExportHandled = true;
           }
           else//pass through everything else
           {
               ostrm << line << std::endl;
           }
        }
        if(exportScriptDir.exists() && !customExportHandled)
        {
              ostrm << "--customExportCopy %SALSA_PROJECT_DIR%/scripts/reporting/extras/h5XLSM.py" << std::endl;
              ostrm << "--customExportCopy %SALSA_PROJECT_DIR%/scripts/reporting/extras/GSDI_v2.20.xlsm" << std::endl;
              ostrm << "--customExportCopy %SALSA_PROJECT_DIR%/scripts/reporting/extras/h5Sinex.py" << std::endl;
              changes += QString("Updated .cfg to include new export capabilities.\n");

              updated = true;
        }

        istrm.close();
        ostrm.close();
    }


    if(updated)
    {
        //replace previous config file
        remove(cfgFileName.toStdString().c_str());
        rename(tmpFileName.toStdString().c_str(),cfgFileName.toStdString().c_str());
        pushWarning(QString("Updated obsolete .cfg file: ")+changes);
    }
    else
    {
        //remove .tmp file
        remove(tmpFileName.toStdString().c_str());
    }

    return updated;
}

void MainWindow::copyCustomFiles(QString projectDir, string typeString)
{
    std::ifstream istrm;
    istrm.open(configFileName.toStdString().c_str(), std::ios::in);
    if(!istrm.is_open())
        return;

    std::string line;
    std::vector<std::string> splitComponents;
    std::string scriptName;
    QFileInfo scriptFile;
    QString tagSubDir = QString::fromStdString("/" + typeString);
    std::string tag = "--" + typeString + "Copy";
    QDir copyDir(projectDir + tagSubDir);

    while(!istrm.eof())
    {
        getline(istrm, line);
        gnsstk::StringUtils::stripTrailing(line, '\r');
        gnsstk::StringUtils::stripTrailing(line, '\n');

        if(line.empty())
            continue;
        if(line.at(0) == '#')
            continue;

        splitComponents = gnsstk::StringUtils::splitWithDoubleQuotes(line, ' ');
        if(splitComponents.size() == 2)
        {
            if(splitComponents[0] == tag)
            {
                scriptName = replaceEnvVars(splitComponents[1]);

                scriptFile.setFile(QString::fromStdString(scriptName));
                if(!scriptFile.exists())
                {
                    QString warningMessage = QString("Warning - custom ") + QString::fromStdString(typeString) + QString(" file: ")
                            + QString::fromStdString(scriptName) + QString(" does not exist.");
                    pushWarning(warningMessage);
                    continue;
                }
                if(!copyDir.exists())
                {
                    copyDir.mkpath(".");
                }
                QString copyToFileName = copyDir.absolutePath() + QString::fromStdString("/") + scriptFile.fileName();
                QFile::copy(scriptFile.filePath(), copyToFileName);
            }
        }
    }
}

void MainWindow::createConverterConfigFile(QString filename)
{
    ConverterDialog dufus(filename, this);//Can't call ConverterDialog::createConverterConfigFile().  Compiler complains of non-static method without object.
    dufus.createConverterConfigFile();
}

void MainWindow::reportCLIWarningsAndErrors(bool isEndOfProcessChain)
{
    QDir d = QFileInfo(currentFile).absoluteDir();
    QString preproc_out = QDir::cleanPath(d.absolutePath() + QString(QDir::separator()) + QString("preprocessor.out"));
    string fileName = preproc_out.toStdString();
    QFileInfo check_file(preproc_out);
    QString msg = "Preprocessor warnings. Click <a href=\"" + QString("file:") + QString::fromStdString(fileName) + "\">here</a> for details.";
    //text formatting doesn't get applied for text with URL.  Wrap with HTML tags.
    msg = QString("<strong><font color=red>") + msg + QString("</font></strong>");

    if(isEndOfProcessChain)
    {
        if(!procChainStdErr.isEmpty())
        {
            pushStatus(QString("Errors encountered:"));
            pushError(procChainStdErr);
            procChainStdErr.clear();
        }
        if(!procChainWarnings.isEmpty())
        {
            pushStatus(QString("Warnings encountered:"));
            pushWarning(procChainWarnings);
            procChainWarnings.clear();
        }
        //Display link to preprocessor.out file, if it exists
        if(check_file.exists())
        {
            pushStatus(msg);
        }
    }
    else
    {
        if(!procStdErr.isEmpty())
        {
            pushError(procStdErr);
            procChainStdErr += procStdErr;
            procStdErr.clear();
        }
        if(!procWarnings.isEmpty())
        {
            pushWarning(procWarnings);

            // write warnings from stdout of preprocessor to a preprocessor.out file
            if(procType == PROCESS_LSAPREPROCESSOR)
            {
                bool new_file = false;
                if(!check_file.exists())
                    new_file = true;
                QFile f(preproc_out);
                if (f.open(QIODevice::WriteOnly | QIODevice::Append))
                {
                    QTextStream stream(&f);
                    QStringList messages = procWarnings.split("\n");
                    if(new_file)
                    {
                        stream << "lsapreprocessor.exe from SALSA version " << QString::fromStdString(LSAVERSION_FILE_STRING);
                        stream << " run at " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "." << endl;
                        stream << endl;
                    }
                    foreach(QString message, messages)
                        stream << message << endl;
                }
                f.close();
            }

            procChainWarnings += procWarnings;
            procWarnings.clear();
        }
    }
}

void MainWindow::parseLSAFileErrorsAndWarnings(bool saving)
{
    std::vector<std::string> errorsAndWarnings = guiModel->getLSAFile()->getErrorsAndWarnings();
    guiModel->getLSAFile()->clearErrorsAndWarnings();
    for(int i=0;i<errorsAndWarnings.size();i++)
    {
        QString warning = QString::fromStdString(errorsAndWarnings[i]);
        if( (warning.indexOf("Could not open included file:") != -1) ||
            (warning.indexOf(QString::fromStdString(lsa::PARSE_WARNING)) != -1) )
        {
            pushWarning(warning);
        }
        //Fix to Bug #1521
        if(saving && (warning.indexOf("Warning -") != -1))
        {
            pushWarning(warning);
        }
    }
}

void MainWindow::popupForMissingIncludes()
{
    if(failedIncludeList.size()>0)
    {
        if(!isTestHarness())
        {
            QString missingIncludesMsg("Unable to parse ");
            for(int i=0;i<failedIncludeList.size();i++)
            {
                missingIncludesMsg += failedIncludeList.at(i) + QString(", ");
            }
            missingIncludesMsg = missingIncludesMsg.left(missingIncludesMsg.size()-2);//remove last ", "
            QMessageBox *notifyUser = new QMessageBox(QString("Included file not found!"),
                                                      missingIncludesMsg,
                                                      QMessageBox::Critical,
                                                      QMessageBox::Ok,0,0,(QWidget*)this,Qt::WindowStaysOnTopHint);
            notifyUser->exec();//blocks
            delete notifyUser;
        }
        failedIncludeList.clear();
    }
}

void MainWindow::removeOldOutputFiles(QString projectFileName)
{
    //*currentFile.out, .pts, .csv, .bin, .apb, .post, .dat, .pcr
    QString outFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".out");
    QString binFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".bin");
    QString aprioriBinFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".apb");
    QString csvFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".csv");
    QString postLogFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".post");
    QString datFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".dat");
    QString ptsFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".pts");
    QString pcrFileName = projectFileName.left(projectFileName.lastIndexOf(".")) + QString(".pcr");

    QFile::remove(outFileName);
    QFile::remove(binFileName);
    QFile::remove(aprioriBinFileName);
    QFile::remove(csvFileName);
    QFile::remove(postLogFileName);
    QFile::remove(datFileName);
    QFile::remove(ptsFileName);
    QFile::remove(pcrFileName);
}

unsigned int MainWindow::getNumDXYZwithHGHT() const
{
    unsigned int numRecords = 0;

    QModelIndex currentIndex = guiModel->getNextIndex();
    while(currentIndex.isValid())
    {
        LSARecord * currentRecord = guiModel->getLSARecord(currentIndex);

        if(currentRecord->getRecType() == LSAType::DXYZ)
        {
            LSADelta * tmpDelta = static_cast<LSADelta *>(currentRecord);
            if(tmpDelta->modifiers.hasFromHeight() || tmpDelta->modifiers.hasToHeight())
            {
                numRecords++;
            }
        }

        currentIndex = guiModel->getNextIndex(currentIndex);
    }

    return  numRecords;
}

// Parameter fileName may or may not have a .proj or .lsa extension depending on the
// call stack leading up to this method. Add in an if statement that will check
// the name of the converted .lsa file that would result.
bool MainWindow::checkOverwrite(const QString &fileName) const
{
    QFileInfo fileInfo(fileName);
    if(fileInfo.suffix()!="lsa" && fileInfo.suffix()!="proj")
    {
        QString convertedBaseLsaName = fileInfo.fileName() + ".lsa";
        auto projdir = getPathWithoutFileName(currentFile.toStdString());
        fileInfo.setFile(QString::fromStdString(projdir), convertedBaseLsaName);
    }
    return fileInfo.exists();
}

QStringList MainWindow::checkOverwrite(const QStringList &fileList) const
{
    QStringList overlapList;
    foreach (QString fileName, fileList)
    {
        bool fileExist = checkOverwrite(fileName);
        if(fileExist)
        {
            overlapList << fileName;
        }
    }

    return overlapList;
}

QMessageBox::StandardButton MainWindow::provideOverwriteWarningMessageBox(const QString &label, const QString &message)
{
    QMessageBox::StandardButton buttonClicked = QMessageBox::question(this,
                                                                      label,
                                                                      message,
                                                                      QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel);
    return buttonClicked;
}

QMessageBox::StandardButton MainWindow::giveOverwriteMessage(const QString &fileName)
{
    QString warningLabel = "Warning - converted lsa file overwrite.";
    QString message = "Do you wish to overwrite an existing .proj and/or .lsa file?\n" + fileName;
    return provideOverwriteWarningMessageBox(warningLabel, message);
}

QMessageBox::StandardButton MainWindow::giveOverwriteMessage(const QStringList &fileList)
{
    QString warningLabel = "Warning - converted lsa file(s) overwrite.";
    int numFiles;
    fileList.size() <= 5 ? numFiles = fileList.size() : numFiles = 5;
    QString message = "Do you wish to overwrite existing .lsa files?\n";
    for(int fileIndex = 0; fileIndex < numFiles; fileIndex++)
    {
        fileIndex != (numFiles - 1) ? message += (fileList[fileIndex] + "\n")
                : message += fileList[fileIndex];
    }

    return provideOverwriteWarningMessageBox(warningLabel, message);
}

bool MainWindow::isTestHarness() const
{
    QString applicationName = QCoreApplication::applicationFilePath();
    return (applicationName.contains("testlsagui") || applicationName.contains("lsatestpublic"));
}

QString MainWindow::convertAndLoadIOBProject(QString iobFileName)
{
    // clear the status window
    emit updateStatusWindow(true);

    QString lsaFileName = convertIOBProject(iobFileName);
    if(!lsaFileName.isEmpty())
    {
        removeOldOutputFiles(lsaFileName);//Fix to Bug #962
        std::vector<std::string> errorsAndWarnings = loadLsaFile(lsaFileName,true);

        for(size_t i=0; i<errorsAndWarnings.size(); i++)
        {
            pushWarning(errorsAndWarnings[i]);
        }
    }

    return lsaFileName;
}

bool MainWindow::readHDF5File(bool openingLSAProject)
{
    QString hdf5FileName;
    bool updateOK = false;
    bool ingestionOK = false;
    bool binFileFailure = false;

    if(lastSolverRunType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
    {
        if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
        {
            hdf5FileName = currentFile + QString(".aph5");
        }
        else
        {
            hdf5FileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".aph5");
        }
    }
    else
    {
        if(currentFile.lastIndexOf(".") <= currentFile.lastIndexOf("/"))//case when no extension on file
        {
            hdf5FileName = currentFile + QString(".h5");
        }
        else
        {
            hdf5FileName = currentFile.left(currentFile.lastIndexOf(".")) + QString(".h5");
        }
    }

    QFileInfo check(hdf5FileName);
    if(!check.exists())
    {
        if(!openingLSAProject)
            pushWarning(QString("Unable to find solver results: ") + hdf5FileName);
        return false;
    }
    //check the file version
    currentHDF5File.createFile(hdf5FileName.toStdString());
    if(currentHDF5File.ReadFileAttributes() != 0)
    {
        if(!openingLSAProject)
            pushWarning(QString("Failure reading HDF5 file version from ") + hdf5FileName);
        currentHDF5File.closeH5File();
        return false;
    }
    if(currentHDF5File.h5VersionContainer != LSAH5File::HDF5_FILE_VERSION)
    {
        pushWarning(QString("Unsupported HDF5 file version! ") + hdf5FileName +
                    QString(" is version ") + QString::fromStdString(currentHDF5File.h5VersionContainer) +
                    QString(" but the currently supported version is ") +
                    QString::fromStdString(LSAH5File::HDF5_FILE_VERSION) +
                    QString(". Re-calculate adjustment to regenerate a current .h5 file."));
        currentHDF5File.closeH5File();
        return false;
    }
    currentHDF5File.closeH5File();

    //read the HDF5 file
    int HDF5ReadStatus = currentHDF5File.ReadH5File(hdf5FileName.toStdString());
    if(HDF5ReadStatus != 0)
    {
        if(!openingLSAProject)
            pushWarning(QString("Failure reading HDF5 file: ") + hdf5FileName);
        return false;
    }

    //Display solver warnings and errors
    bool foundSolverError = DisplaySolverWarningsAndErrors(openingLSAProject);

    //don't ingest solver output for exporting geoid heights from initial positions
    if(lastSolverRunType == PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)//fix to Bug #1242
    {
        if(foundSolverError)
            return false;
        else
            return true;
    }

    //verify that the .bin file exists/has contents
    if(currentHDF5File.postProcInput->getDataPoint().binFileStatus == BIN_FILE_NOT_FOUND)
    {
        pushWarning(QString("lsapost unable to find .bin file output from solver."));
        binFileFailure = true;
    }
    else if(currentHDF5File.postProcInput->getDataPoint().binFileStatus == BIN_FILE_NOT_PARSED)
    {
        pushWarning(QString("lsapost unable to parse .bin file output from solver."));
        binFileFailure = true;
    }
    else if(currentHDF5File.postProcInput->getDataPoint().binFileStatus == BIN_FILE_UNABLE_TO_OPEN)
    {
        pushWarning(QString("lsapost unable to open .bin file output from solver."));
        binFileFailure = true;
    }
    else if(currentHDF5File.postProcInput->getDataPoint().binFileStatus == BIN_FILE_FORMAT_OBSOLETE)
    {
        pushWarning(QString(".bin file output from solver is obsolete."));
        binFileFailure = true;
    }

    if(binFileFailure && !openingLSAProject)//fix to Bug #1141
    {
        guiModel->clearOutputPositions();
        mapWidget->initMapInitialCoodinates();
        return false;
    }

    //verify the .bin file hash is valid
    unsigned long file_hash = std::strtoul(currentHDF5File.postProcInput->getDataPoint().hashString.c_str(),NULL,0);
    if(file_hash != currentHashCode)
    {
        pushWarning(QString("Project has changed since last 'Calculate Adjustment;' output panels and tables are suppressed."));
        clearOutputTab();
        return false;
    }

    //update points in Station Data Dialog
    stationData->setup(&currentHDF5File, hdf5FileName, isScaleByAPV(), usingWestLongitude());

    //update the map and insert auto-generated points into the tree
    if(lastSolverRunType == PROCESS_LSASOLVER_APQUIT)
    {
        ingestionOK = ingestSolverOutputs(true);
        if(ingestionOK)
        {
            mapWidget->initMapInitialCoodinates();
            mapWidget->setHome();//Fix to Bug #1014
        }
    }
    else if(lastSolverRunType != PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS)
    {
        // get F distribution scale factor based on the following:
        // - degress of freedom of the adjustment
        // - dimensionality of the ellipse (1D for vertial error bar, 2D for a 2d ellipse)
        // - confidence interval (50%, 1 sigma, 90%, 95%, 99%) specified by user in config file
        double confidenceFactor_1d = getConfidenceFactor(getConfidenceInterval(),1,currentHDF5File.probDescript->getDataPoint().degOfFreed);//sqrt already done on the critical values
        double confidenceFactor_2d = getConfidenceFactor(getConfidenceInterval(),2,currentHDF5File.probDescript->getDataPoint().degOfFreed);//sqrt already done on the critical values

        if(isScaleByAPV())//if the user selects --APV in the .cfg file
        {
            confidenceFactor_1d *= sqrt(currentHDF5File.statistics->getDataPoint().APV);
            confidenceFactor_2d *= sqrt(currentHDF5File.statistics->getDataPoint().APV);
        }
        mapWidget->setEllipseConfidenceFactor(confidenceFactor_2d, confidenceFactor_1d);

        ingestionOK = ingestSolverOutputs();

        //regenerate map in case autogen include was inserted (Issue #92/Bug #1535)
        generateUniqueIDMapToLSARecordPointers();

        if(ingestionOK && !openingLSAProject)//have to wait for the guimodel to be set in the mapWidget in loadLSAFile()
        {
            mapWidget->initMapAdjustedCoordinates();

            // Don't call setHome() here.  Users may want to fine tune a small area of an adjustment, so
            // we don't want them to have to keep zooming into the map every time they calculate adjustment.
        }

        //populate the output panel tables and labels
        updateOK = updateOutputPanel();
        histogramDialog->setupHistogram(currentHDF5File, guiModel, currentFile);

        //update the AdjustedPositions widget
        double ellipseScaleFactor = 1.0;
        if(isScaleByAPV())
            ellipseScaleFactor *= ::sqrt(currentHDF5File.statistics->getDataPoint().APV);

        adjustedPositions->updateView(currentHDF5File, ellipseScaleFactor, guiModel, usingWestLongitude(), includeUnused());
    }

    if (ingestionOK)
        emit h5ImportSuccessful();

    return (updateOK && ingestionOK);
}

bool MainWindow::isFileWithinProject(QString filename)
{
    bool isSubDirectory = false;
    const QString projectPath = QString::fromStdString(getPathWithoutFileName(currentFile.toStdString()));
    QString sourcePath = QString::fromStdString(getPathWithoutFileName(filename.toStdString()));
    QDirIterator it(projectPath, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        if(it.next() == sourcePath)
        {
            isSubDirectory = true;
            break;
        }
    }
    return isSubDirectory;
}

void MainWindow::pushParseWarningsForDescendants(QModelIndex parentIndex)
{
    QModelIndex index = parentIndex;
    while(index.isValid())
    {
        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        if (lsaRecord->hasParseWarnings())
        {
            pushWarning(lsaRecord->getSingleLSAString());
        }

        index = guiModel->getNextDescendant(parentIndex, index);
    }
    return;
}

//Fix to Bug #1244
//This seems hacky, but I couldn't find anything else that worked
void MainWindow::putAdjustedPositionsOnTop()
{
    if(ui->actionView_AdjustedPositions->isChecked())
    {
        //toggle visibility
        adjustedPositions->setVisible(false);
        adjustedPositions->setVisible(true);
    }
}

//Fix to Bug #1483
void MainWindow::handleMeasFilterChanged(bool checked)
{
    if(hashCodesAreValid)
    {
        if(!checked)//don't filter
        {
            //unhide all
            int numRows = ui->tableMeasurements->model()->rowCount();
            suppressOutputTableSelectionChanges = true;
            for(int row=0;row<numRows;row++)
            {
                ui->tableMeasurements->setRowHidden(row,false);
            }
            suppressOutputTableSelectionChanges = false;
        }
        QModelIndexList selectedMeasurements = getSelectedMeasurements();
        selectMeasurementTableRows(selectedMeasurements);

        QTimer::singleShot(0,this, SLOT(scrollToSelectedMeasurement()));

    }
}

//Fix to Bug #1483
void MainWindow::handlePointsFilterChanged(bool checked)
{
    if(hashCodesAreValid)
    {
        if(!checked)//don't filter
        {
            int numRows = ui->tablePoints->model()->rowCount();
            suppressOutputTableSelectionChanges = true;
            for(int row=0;row<numRows;row++)
            {
                ui->tablePoints->setRowHidden(row,false);
            }
            suppressOutputTableSelectionChanges = false;
        }

        QModelIndexList selectedPoints = getSelectedPoints();
        selectPointsTableRows(selectedPoints);

        QTimer::singleShot(0,this, SLOT(scrollToSelectedPoint()));
    }
}

// Fix to issue #1072. This allows the first selected point to be visible after
// toggling the Filter checkbox
void MainWindow::scrollToSelectedPoint()
{
   //scroll to item in the table
   QModelIndexList selectedRows = ui->tablePoints->selectionModel()->selectedIndexes();
   if(selectedRows.size()>0)
   {
       QModelIndex topIndex = selectedRows.at(0);
       int topRow = topIndex.row();
       for (const auto& index : qAsConst(selectedRows))
       {
           if(index.row()<topRow)
           {
               topIndex = index;
               topRow = topIndex.row();
           }
       }
       ui->tablePoints->scrollTo(topIndex, QAbstractItemView::PositionAtCenter);
   }
   return;
}

// Fix to issue #1072. This allows the first selected measurement to be visible after
// toggling the Filter checkbox
void MainWindow::scrollToSelectedMeasurement()
{
   //scroll to item in the table
   QModelIndexList selectedRows = ui->tableMeasurements->selectionModel()->selectedIndexes();
   if(selectedRows.size()>0)
   {
       QModelIndex topIndex = selectedRows.at(0);
       int topRow = topIndex.row();
       for (const auto& index : qAsConst(selectedRows))
       {
           if(index.row()<topRow)
           {
               topIndex = index;
               topRow = topIndex.row();
           }
       }
       ui->tableMeasurements->scrollTo(topIndex, QAbstractItemView::PositionAtCenter);
   }
   return;
}

QModelIndex MainWindow::findTopSelectedIndex()
{
    // Get the selected record(s) from the guiModel
    QModelIndexList selectedRows = guiModel->getSelectedRows();
    int topRow = 1000000, row=0;
    QModelIndex topIndex;
    foreach (QModelIndex index, selectedRows)
    {
        if(index.row()<topRow)
        {
            topIndex = index;
            row = index.row();
        }
        ui->treeView->expand(index.parent() );
    }

    return topIndex;
}

QModelIndexList MainWindow::getSelectedMeasurements()
{
    // Get the selected record(s) from the guiModel
    QModelIndexList selectedRows = guiModel->getSelectedRows();

    return getSelectedMeasurements(selectedRows);
}

QModelIndexList MainWindow::getSelectedMeasurements(const QItemSelection &selected)
{
    QModelIndexList selectedIndices = selected.indexes();

    return getSelectedMeasurements(selectedIndices);
}

QModelIndexList MainWindow::getSelectedMeasurements(const QModelIndexList &paramMIL)
{
    QModelIndexList selectedMeasurements;

    selectedMeasurements.clear();

    foreach (QModelIndex index, paramMIL)
    {
        ui->treeView->expand(index.parent() );

        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        LSAType lsaType = lsaRecord->getRecType();
        if(lsaType.isMeasurement() || isPPP(lsaRecord))
            selectedMeasurements.append(index);
    }

    return selectedMeasurements;
}

QModelIndexList MainWindow::getSelectedPoints()
{
    // Get the selected record(s) from the guiModel
    QModelIndexList selectedRows = guiModel->getSelectedRows();

    return getSelectedPoints(selectedRows);
}

QModelIndexList MainWindow::getSelectedPoints(const QItemSelection &selected)
{
    QModelIndexList selectedIndices = selected.indexes();

    return getSelectedPoints(selectedIndices);
}

QModelIndexList MainWindow::getSelectedPoints(const QModelIndexList &paramMIL)
{
    QModelIndexList selectedPoints;

    selectedPoints.clear();

    // Expand the parent include for each selected record
    foreach (QModelIndex index, paramMIL)
    {
        ui->treeView->expand(index.parent() );

        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        LSAType lsaType = lsaRecord->getRecType();
        if(lsaType.isPosition() || lsaType.isPostProcessed())
            selectedPoints.append(index);
    }

    return selectedPoints;
}



void MainWindow::selectMeasurementTableRows(const QModelIndexList& selectedMeasurements)
{
    if(hashCodesAreValid)
    {
        suppressOutputTableSelectionChanges = true;
        int row, numRows = ui->tableMeasurements->model()->rowCount();
        bool onetoone;
        //fix to Bug #1483
        if(ui->checkBoxFilterMeasOnSelected->isChecked())
        {
            for(row=0;row<numRows;row++)
            {
                //hide all
                ui->tableMeasurements->setRowHidden(row,true);
            }
        }

        struct RowData
        {
            QString rowRecordPtrString = "";
            QItemSelectionRange itemRange;
        };
        std::vector<std::unique_ptr<RowData>> rowDataCache(numRows);

        QItemSelection selection;
        for(const QModelIndex& index : selectedMeasurements)
        {

            LSARecord * lsaRecord = guiModel->getLSARecord(index);
            if((lsaRecord->getRecType() == LSAType::DXYZ) || (lsaRecord->getRecType() == LSAType::POSC) || (lsaRecord->getRecType() == LSAType::POSG))
                onetoone = false;
            else
                onetoone = true;
            QString RecordPtrStr = QString::number((qlonglong)lsaRecord);
            int match_count=0;

            for(row=0;row<numRows;row++)
            {
                // See if there is a cache entry for this row.
                if(!rowDataCache[row])
                {
                    rowDataCache[row] = std::make_unique<RowData>();
                    rowDataCache[row]->rowRecordPtrString = ui->tableMeasurements->model()->index(row,NUM_RESID_COLUMNS-1).data().toString();
                    QModelIndex topLeft = ui->tableMeasurements->model()->index(row,0);
                    QModelIndex bottomRight = ui->tableMeasurements->model()->index(row,NUM_RESID_COLUMNS-1);
                    rowDataCache[row]->itemRange = QItemSelectionRange(topLeft,bottomRight);
                }
                QString& tableRecordPtrStr = rowDataCache[row]->rowRecordPtrString;

                if (tableRecordPtrStr.isEmpty())
                {
                    continue;
                }

                if(RecordPtrStr == tableRecordPtrStr)
                {
                    // appending should be faster than merging.
                    // selection.merge(rowDataCache[row]->selectionRep,QItemSelectionModel::Select);
                    selection.append(rowDataCache[row]->itemRange);

                    //fix to Bug #1483
                    if(ui->checkBoxFilterMeasOnSelected->isChecked())
                    {
                        //unhide
                        ui->tableMeasurements->setRowHidden(row,false);
                    }
                    if(onetoone)
                        break;
                    else
                        match_count++;
                    if(match_count == 3)
                        break;
                }
            } // END for(row=0;row<numRows;row++)
        } // END for(const QModelIndex& index : selectedMeasurements)

        // We've found all of the rows, so select them.
        ui->tableMeasurements->selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);

        //scroll to item in the table
        QModelIndexList selectedRows = ui->tableMeasurements->selectionModel()->selectedIndexes();
        if(selectedRows.size()>0)
        {
            QModelIndex topIndex = selectedRows.at(0);
            int topRow = topIndex.row();
            for(const QModelIndex& index : qAsConst(selectedRows))
            {
                if(index.row()<topRow)
                {
                    topIndex = index;
                    topRow = topIndex.row();
                }
            }
            QWidget *focusWidget = QApplication::focusWidget();
            if(focusWidget != dynamic_cast<QWidget*>(ui->tableMeasurements))
            {
                ui->tableMeasurements->scrollTo(topIndex,QAbstractItemView::PositionAtCenter);
            }
            ui->tableMeasurements->selectionModel()->setCurrentIndex(topIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        suppressOutputTableSelectionChanges = false;
    }

}

void MainWindow::selectPointsTableRows(const QModelIndexList& selectedPoints)
{
    if(hashCodesAreValid)
    {
        suppressOutputTableSelectionChanges = true;
        int numRows = ui->tablePoints->model()->rowCount();
        int row;
        //fix to Bug #1483
        if(ui->checkBoxFilterPointsOnSelected->isChecked())
        {
            //hide all
            for(row=0;row<numRows;row++)
            {
                ui->tablePoints->setRowHidden(row,true);
            }
        }

        struct RowData
        {
            QString rowRecordPtrString = "";
            QItemSelectionRange itemRange;
        };
        std::vector<std::unique_ptr<RowData>> rowDataCache(numRows);

        QItemSelection selection;
        for(const QModelIndex& index : selectedPoints)
        {
            LSARecord * lsaRecord = guiModel->getLSARecord(index);
            QString pointName = QString::fromStdString(lsaRecord->getLabel());
            for(row=0;row<numRows;row++)
            {
                // See if there is a cache entry for this row.
                if(!rowDataCache[row])
                {
                    rowDataCache[row] = std::make_unique<RowData>();
                    rowDataCache[row]->rowRecordPtrString = ui->tablePoints->model()->data(ui->tablePoints->model()->index(row,POINT_TABLE_POINT_COLUMN)).toString();
                    QModelIndex topLeft = ui->tablePoints->model()->index(row,0);
                    QModelIndex bottomRight = ui->tablePoints->model()->index(row,NUM_POINT_COLUMNS-1);
                    rowDataCache[row]->itemRange = QItemSelectionRange(topLeft,bottomRight);
                }
                QString& tablePointName = rowDataCache[row]->rowRecordPtrString;
                if(tablePointName.isEmpty())
                {
                    continue;
                }

                if(tablePointName == pointName)
                {
                    selection.append(rowDataCache[row]->itemRange);

                    //fix to Bug #1483
                    if(ui->checkBoxFilterPointsOnSelected->isChecked())
                    {
                        ui->tablePoints->setRowHidden(row,false);
                    }
                    break;
                }
            }
        }

        // We've found all of the rows, so select them.
        ui->tablePoints->selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);

        //scroll to item in the table
        QModelIndexList selectedRows = ui->tablePoints->selectionModel()->selectedIndexes();
        if(selectedRows.size()>0)
        {
            QModelIndex topIndex = selectedRows.at(0);
            int topRow = topIndex.row();
            for (const auto& index : qAsConst(selectedRows))
            {
                if(index.row()<topRow)
                {
                    topIndex = index;
                    topRow = topIndex.row();
                }
            }

            QWidget *focusWidget = QApplication::focusWidget();
            if(focusWidget != dynamic_cast<QWidget*>(ui->tablePoints))
            {
                ui->tablePoints->scrollTo(topIndex,QAbstractItemView::PositionAtCenter);
            }
            ui->tablePoints->selectionModel()->setCurrentIndex(topIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        suppressOutputTableSelectionChanges = false;
    }
}

//Used to allow abandoning of a Record->Cut operation and de-selecting records
bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Escape)
        {
            guiModel->selectionModel->clearSelection();
            ui->treeView->clearSelection();
            ui->treeView->clearFocus();
        }
    }
    return QObject::eventFilter(target,event);
}

void MainWindow::hideRecordsFromIndexes(QList<QPersistentModelIndex> indexes, bool hide)
{
    foreach(QPersistentModelIndex index, indexes)
    {
        QModelIndex parentIndex = guiModel->parent(index);
        setTreeViewRowHidden(index.row(), parentIndex, hide);
    }
}

void MainWindow::populateCustomExportDir(QString currentDir)
{
    QDir::setCurrent(currentDir);
    QString tagSubDir = QString::fromStdString("/customExport");
    QDir copyDir(currentDir + tagSubDir);
    QString exportScriptDirectoryPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../scripts/reporting/extras");
    QDir exportScriptDir(exportScriptDirectoryPath);
    if(!copyDir.exists() && exportScriptDir.exists())
    {
        copyCustomFiles(currentDir, "customExport");
        pushWarning(QString("Added custom export scripts.\n"));
    }
}

void MainWindow::setTreeViewRowHidden(int row, const QModelIndex &parent, bool bHidden)
{
    ui->treeView->setRowHidden(row, parent, bHidden);
    bTVRowVisibilityChanged = true;
}

// This method should give the correct row and parent so that a file dropped onto the treeview
// should be included where the indicator shows. The rowIn CAN change in the case that there
// is more than one file being dropped, so that every file after the first is include after the first
void MainWindow::correctIncludeDropPosition(int& rowIn, int& rowOut, QModelIndex &parentIdx)
{
    // Logic to get the correct position for this include
    rowOut = rowIn;
    QModelIndex recordParentIdx = QModelIndex();
    bool expanded = false;
    LSARecord* parentRecord = guiModel->getLSARecord(parentIdx);

    // If we are dropping onto an include, see if it's expanded. For some reason Qt's
    // isExpanded always returns false, so do it like by checking if the next record is
    // on of the include's children
    if(parentRecord && parentRecord->getRecType() == LSAType::INCLUDE &&
            parentIdx != guiModel->index(0,0) && rowOut == -1)
    {
        auto parentInclude = static_cast<LSAInclude*>(parentRecord);
        const auto& children = parentInclude->childRecords;
        auto childIter = std::find(children.begin(), children.end(), guiModel->getLSARecord(ui->treeView->indexBelow(parentIdx)));
        expanded = childIter != children.end();
    }

    // These conditionals are where we decide the record should go.
    if(expanded)
    {
        recordParentIdx = parentIdx;
        rowOut = 0;
    }
    else if(parentIdx.isValid() && rowIn == -1 ) // dropped on an index
    {
        rowOut = parentIdx.row()+1; // row index where we will insert the new record
        recordParentIdx = parentIdx.parent();
    }
    else if(parentIdx.isValid() && rowIn > -1) // dropped between records. the passed in row is relative to parent include
    {
        recordParentIdx = parentIdx;
        // In the case of dropping multiple records, set the row to -1 so they are all placed after the first added include.
        rowIn = -1;
    }

    // If we get here and the record's parent isn't valid, the the user dropped outside of the tree, but
    // still in the viewport for the tree, or on the root record.
    if (recordParentIdx == QModelIndex())
    {
        recordParentIdx = guiModel->index(0,0);
        if(parentIdx.isValid()) // on root record
        {
            rowOut = 0;
        }
    }

    parentIdx = recordParentIdx;
}

void MainWindow::applyConfigFile()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    std::ifstream istrm;
    istrm.open(configFileName.toStdString().c_str(), std::ios::in);
    if(!istrm.is_open())
       return;
    std::string line;
    std::vector<std::string> parts;

    //TODO: exception handling for parse errors
    while(1)
    {
       line = std::string("");
       getline(istrm,line);
       gnsstk::StringUtils::stripTrailing(line,'\r');
       gnsstk::StringUtils::stripTrailing(line,'\n');
       if((istrm.eof() || !istrm.good()) && line.empty()) break;
       if(line.empty())
           continue;

       if(line.at(0)!='#')
       {
           try
           {
               if(line.find("--linprecM")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS, atoi(parts[1].c_str()) );

                   // Determine the values of the associated qsettings
                   if(atoi(parts[1].c_str()) >= 3)
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_KILOMETERS, atoi(parts[1].c_str()) + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_CENTIMETERS, atoi(parts[1].c_str()) - 2 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET, atoi(parts[1].c_str()) - 2 );
                   }
                   else
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_KILOMETERS, atoi(parts[1].c_str()) + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_CENTIMETERS, 1 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET, 1 );
                   }
               }
               else if(line.find("--linprecP")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_METERS, atoi(parts[1].c_str()) );

                   // Determine the values of the associated qsettings
                   if(atoi(parts[1].c_str()) >= 3)
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_KILOMETERS, atoi(parts[1].c_str()) + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_CENTIMETERS, atoi(parts[1].c_str()) - 2 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_FEET, atoi(parts[1].c_str()) - 2 );
                   }
                   else
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_KILOMETERS, atoi(parts[1].c_str()) + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_CENTIMETERS, 1 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_FEET, 1 );
                   }
               }
               else if(line.find("--angprecP")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_SOA, atoi(parts[1].c_str()) );

                   // Determine the values of the associated qsettings
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_DEGREES, atoi(parts[1].c_str()) + 4 );
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_RADIANS, atoi(parts[1].c_str()) + 5 );
               }
               else if(line.find("--angprecM")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA, atoi(parts[1].c_str()) );
                   // Determine the values of the associated qsettings
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_DEGREES, atoi(parts[1].c_str()) + 3 );
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_RADIANS, atoi(parts[1].c_str()) + 5 );
               }
               else if(line.find("--warningExtRelVect")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   qsettings.setValue( lsa::QSETTINGS_MAX_EXT_MAG_WARNING, stod(parts[1].c_str()) );
               }
               else if(line.find("--errorExtRelVect")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   qsettings.setValue( lsa::QSETTINGS_MAX_EXT_MAG_ERROR, stod(parts[1].c_str()) );
               }
           }
           catch(...)
           {
               continue;
           }
       }
    }
    istrm.close();
}

void MainWindow::undo()
{
    // Check that a project is loaded
    if(!currentFile.isEmpty())
    {
        guiModel->undo();
    }
}

void MainWindow::redo()
{
    // Check that a project is loaded
    if(!currentFile.isEmpty())
    {
        guiModel->redo();
    }
}
