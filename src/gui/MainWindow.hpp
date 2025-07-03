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
#ifndef LSAGUIWINDOW_H
#define LSAGUIWINDOW_H

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#ifdef _WIN32
    #include "Windows.h"
#endif

#include <QDateTime>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QModelIndex>
#include <QProcess>
#include <QSettings>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <functional>

#include "ui_mainwindow.h"

#include <AboutSalsa.hpp>
#include <ConfigDialog.hpp>
#include <HistogramDialog.hpp>
#include <DebugTimer.hpp>
#include <exception>
#include <expandpath.hpp>
#include <FindWidget.hpp>
#include <GuiModel.hpp>
#include <LSAFile.hpp>
#include <mapping.hpp>
#include <LSAOperationType.hpp>
#include <PreferencesDialog.hpp>
#include <RecordEditor.hpp>
#include <AdjustedPositions.hpp>
#include <StationDataDialog.hpp>
#include <LSAGuiConstants.hpp>
#include <lsah5.hpp>
#include <guiutils.hpp>
#include <utility>

namespace Ui
{
    class MainWindow;
    class QItemSelection;
}

/// Class MainWindow contains all of code to manage controls for the main gui window.
class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests
    friend class Benchmarks; ///< Test harness class used to execute gui unit and integration tests

public:
    /// Constructor given parent widget
    /// @param parent the parent widget for this object
    explicit MainWindow(QString lsaFile = "", QWidget *parent = 0);

    /// Destructor
    ~MainWindow();

    /// constants
    static const QString QSETTINGS_TREEVIEW_COLUMN_WIDTH_0;
    static const QString QSETTINGS_TREEVIEW_COLUMN_WIDTH_1;
    static const QString QSETTINGS_TREEVIEW_COLUMN_WIDTH_2;
    static const QString QSETTINGS_TREEVIEW_COLUMN_WIDTH_3;
    static const QString QSETTINGS_TREEVIEW_COLUMN_WIDTH_4;

    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_0;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_1;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_2;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_3;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_4;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_5;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_6;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_7;
    static const QString QSETTINGS_RESIDUALS_COLUMN_WIDTH_8;

    static const QString QSETTINGS_CONFIDENCE_COLUMN_WIDTH_0;
    static const QString QSETTINGS_CONFIDENCE_COLUMN_WIDTH_1;
    static const QString QSETTINGS_CONFIDENCE_COLUMN_WIDTH_2;
    static const QString QSETTINGS_CONFIDENCE_COLUMN_WIDTH_3;
    static const QString QSETTINGS_CONFIDENCE_COLUMN_WIDTH_4;
    static const QString QSETTINGS_CONFIDENCE_COLUMN_WIDTH_5;
    static const QString QSETTINGS_CONFIDENCE_COLUMN_WIDTH_6;

    static const QString QSETTINGS_CENTRAL_SPLITTER_SIZE_0;
    static const QString QSETTINGS_CENTRAL_SPLITTER_SIZE_1;
    static const QString QSETTINGS_CENTRAL_SPLITTER_SIZE_2;
    static const QString QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_0;
    static const QString QSETTINGS_OUTPUTTABLES_SPLITTER_SIZE_1;

    static const QString QSETTINGS_WINDOW_GEOMETRY;
    static const QString QSETTINGS_WINDOW_SAVE_STATE;

    static const QString QSETTINGS_WINDOW_MAXIMIZED;
    static const QString QSETTINGS_WINDOW_POSITION;
    static const QString QSETTINGS_WINDOW_SIZE;

    static const QString QSETTINGS_SPC_DEFAULT;
    static const int TIMEOUT_START_MILLISECONDS;

    enum PROCESS_TYPES
    {
       // Unknown must be first, and must = 0
       Unknown = 0,         ///< unknown proc type
       PROCESS_NONE,
       //converters
       PROCESS_IOB2LSA,
       FIRST_CONVERTER = PROCESS_IOB2LSA,
       PROCESS_CUSTOM_GEOID,
       CUSTOM_CONVERTER_SCRIPT,
       PROCESS_CONVERTER,
       LAST_CONVERTER = PROCESS_CONVERTER,
       //preprocessor
       PROCESS_LSAPREPROCESSOR,
       //solvers
       PROCESS_LSASOLVER,
       FIRST_SOLVER = PROCESS_LSASOLVER,
       PROCESS_LSASOLVER_APQUIT,
       PROCESS_LSASOLVER_APQUIT_GEOIDHEIGHTS,
       PROCESS_LSASOLVER_FORCE_STABLE,
       LAST_SOLVER = PROCESS_LSASOLVER_FORCE_STABLE,
       //postprocessor
       PROCESS_LSAPOST,
       FIRST_POST = PROCESS_LSAPOST,
       PROCESS_LSAPOST_GEOIDHEIGHTS,
       LAST_POST = PROCESS_LSAPOST_GEOIDHEIGHTS,
       //reporting scripts
       PROCESS_REPORTING_CSV,
       FIRST_REPORTER = PROCESS_REPORTING_CSV,
       PROCESS_REPORTING_PTS,
       CUSTOM_REPORTING_SCRIPT,
       PROCESS_REPORTING_SOLNCOV,
       PROCESS_REPORTING_MEASRESID,
       PROCESS_REPORTING_UTM,
       PROCESS_REPORTING_SPC,
       PROCESS_REPORTING_NCAT,
       PROCESS_REPORTING_XYZ,
       LAST_REPORTER = PROCESS_REPORTING_XYZ,

       count
    };

    void saveWindowSettings();
    void loadAndApplyWindowSettings();
    void putAdjustedPositionsOnTop();
    bool suppressOutputTableSelectionChanges;
    bool suppressProjectChangedDialog;
    bool suppressErrorNotification = false;
    bool killProcessStatus = false;
    LSAH5File currentHDF5File;

    int numOfUnusedPositions = 0;
    int numOfZeroRedun = 0;

public slots:
    void handleFocusChanged(QWidget* oldWidget, QWidget* newWidget);
    void openConfigDialog();
    void openPreferencesDialog();
    void viewHistogram();
    void giveFocusToTreeView();
    bool setSuppressGuiUpdates(bool value);

    void handleInsertIncludeFromLSA()          { getFilesToAddToProject(lsa::CONVERTER_TYPE_NONE); }
    void handleInsertIncludeFromIOB()          { getFilesToAddToProject(lsa::CONVERTER_TYPE_IOB); }
    void handleInsertIncludeFromSetsOfAngles() { getFilesToAddToProject(lsa::CONVERTER_TYPE_SETSOFANGLES); }
    void handleInsertIncludeFromTrimbleTDEF()   { getFilesToAddToProject(lsa::CONVERTER_TYPE_TRIMBLETDEF); }
    void handleInsertIncludeFromTrimbleRounds() { getFilesToAddToProject(lsa::CONVERTER_TYPE_TRIMBLEROUNDS); };
    void handleInsertIncludeFromPPP()          { getFilesToAddToProject(lsa::CONVERTER_TYPE_PPP); }
    void handleInsertIncludeFromOPUS()         { getFilesToAddToProject(lsa::CONVERTER_TYPE_OPUS); }
    void handleInsertIncludeFromGSI()          { getFilesToAddToProject(lsa::CONVERTER_TYPE_GSI); }
    void handleInsertIncludeFromLeicaGNSS()    { getFilesToAddToProject(lsa::CONVERTER_TYPE_LEICAGNSS); }

    // Error, warning, status messages
    void pushWarning(QString msg); ///< Push a 'warning' formatted string to the status window
    void pushError(QString msg);   ///< Push a 'error' formatted string to the status window
    void pushStatus(QString msg);  ///< Push a 'status' formatted string to the status window
    void pushToStatusWindow(bool clear, QString msg=QString(), int fontWeight=0, QColor fontColor=Qt::black);

    void handleTreeViewCollapseAll();
    void handleReplaceInclude(QModelIndex index, QStringList filenameList);

    void handleDataDroppedOnTree(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

private slots:

    /// handle an exception caught in a child widget
    void handleException(QString message);
    void handleDataChange(QModelIndex, QModelIndex, QVector<int> roles);

    void handleStatusWindowAnchorClicked(QUrl);

    /// Disable/Enable GUI subprocess elements
    /// @param enable - enable elements if true, disable if false
    void toggleGUISubprocessElements(bool enable);


    /// Slots triggered from Project Menu
    void createNewProject(QString filename = "");     ///< Create a new empty project with a default options.lsa file
    void openLSAProject(QString filename = "");       ///< Open a project by selecting the parent .lsa file
    void openRecentLSAProject(); ///< Open a recent project by selecting the parent .lsa file
    void removeDeletedProjectsRecentMenu(); ///< Remove any recent projects that were deleted from the recent projects menu
    void openLSAFilepath(QString filepath);               ///< Wrapper for loadLsaFile which will display warnings
    void importProjectFromIOB(); ///< Open an .iob project by the parent .iob, convert it to lsa, open the lsa project
    void reloadProject();        ///< Reload the project
    void reloadProjectAfterSave();///< Reload the project without checking if changes need saving
    void reloadProjectAfterParseWarning();///< Reload the project after attempting to fix a parse warning
    void closeProject();         ///< Close the currently opened project.
    void saveProject();                 ///< Save the current opened project to lsa files
    void saveAsProject(QString newDirPath = "");   ///< Save the current opened project to lsa files at the user specified location
    void exportMap();            ///< Save a screenshot of the map with a file save dialog
    void exportKML();            ///< export marble state at kml file
    bool quitApplication();      ///< Provide an opportunity to save changes (if needed), then quit the application
    void closeEvent(QCloseEvent *);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);
    bool eventFilter(QObject *target, QEvent *event);
    void convertToGeoidFile();

    void updateRecordEditorVisibility();
    void updateMapVisibility();
    void updateAdjustedPositionsVisibility();
    void updateStationDataVisibility();
    void updateHistogramVisibility();
    void handleRecordEditorClose();
    void handleMapClose();
    void handleAdjustedPositionsClose();
    void handleStationDataClose();
    void handleHistogramClose();

    void updateRecordVisibility();
    void setParentsVisible(QModelIndex index);

    void updateColumnSpans();

    void onCustomContextMenu(const QPoint &);
    void onCustomContextMenuResiduals(const QPoint &);
    void onCustomContextMenuPoints(const QPoint &);
    void toggleResidualsTableColumn(bool bShow);
    void togglePointsTableColumn(bool bShow);

    void handleTreeViewColumnWidthChange(int, int, int);
    void handleTablePointsResized(int columnNumber, int oldWidth, int newWidth);
    void handleTableMeasurementsResized(int columnNumber, int oldWidth, int newWidth);
    void handleCentralSplitterMoved(int position, int index);
    void handleOutputTablesSplitterMoved(int position, int index);
    void handleMeasFilterChanged(bool checked);
    void handlePointsFilterChanged(bool checked);

    /// Slot handled from copy and paste menu
    void handleCopyRecord();
    void handleCutRecord();
    void handlePasteRecord();
    void handleDuplicateRecord();
    void deleteCutRecordsList(QModelIndexList cutIndices);
    void hideRecordsFromIndexes(QList<QPersistentModelIndex> indexes, bool hide);
    LSARecord* copyOfLSARecord(LSARecord *lsaRecord);

    void handleCustomExportActionTriggered();
    void customExportAction(QAction* action, QStringList additionalArguments = QStringList());
    void handleCustomImportActionTriggered();
    void customImportAction(QAction* action);

    void cleanCustomActions(QMenu* menu);

    /// Slot triggered by changing selection inside the tree view
    /// @param index QModelIndex indicating which node in the tree was clicked
    void handleItemSelectionChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
    void handleItemSelectionChanged(QItemSelection, QItemSelection);
    void handleItemSelectionChanged();

    void handleFindWidgetClose(QWidget *);

    /// Slot triggered by clicking the calculate adjustment button
    void calculateAdjustment(MainWindow::PROCESS_TYPES runType=PROCESS_LSASOLVER);

    /// Slot triggered by clicking on a row in the residuals output table
    void handleTableMeasurementsSelectionChanged(QItemSelection selected, QItemSelection deselected);
    void selectOutputTablesRows(QModelIndexList selectedPoints, QModelIndexList selectedMeasurements);
    void selectMeasurementTableRows(const QModelIndexList& selectedMeasurements);
    void selectPointsTableRows(const QModelIndexList& selectedPoints);
    void scrollToSelectedPoint();
    void scrollToSelectedMeasurement();
    QModelIndex findTopSelectedIndex();
    QModelIndexList getSelectedMeasurements();
    QModelIndexList getSelectedMeasurements(const QItemSelection &);
    QModelIndexList getSelectedMeasurements(const QModelIndexList &);

    QModelIndexList getSelectedPoints();
    QModelIndexList getSelectedPoints(const QItemSelection &);
    QModelIndexList getSelectedPoints(const QModelIndexList &);


    /// Slot triggered by clicking on a row in the points displacement output table
    void handleTablePointsSelectionChanged(QItemSelection selected, QItemSelection deselected);
    void handleTablePointsDoubleClicked(QModelIndex doubleclickedIndex);

    void handlePointCoordinateChanged(QModelIndex index);

    void moveRecordUp() {guiModel->moveRecord(true);}
    void moveRecordDown() {guiModel->moveRecord(false);}

    void findRecord();
    void selectReferencingRecords();

    void insertNewInclude(QString testFileName = "");
    void getFilesToAddToProject(lsa::CONVERTER_TYPE fileType, QString filename = QString(""));

    QModelIndex convertFilesAndAddToProject(QStringList fileNameList, lsa::CONVERTER_TYPE fileType, QModelIndex parentIndex, int rowNum);
    QString convertFile(QString fileNameList, lsa::CONVERTER_TYPE fileType);

    void insertPOSG();     ///< Insert a new POSG record after the currently selected record
    void insertPOSC() { insertNewBlankRecord(LSAType::POSC); }
    void insertDIST() { insertNewBlankRecord(LSAType::DIST); }
    void insertDXYZ() { insertNewBlankRecord(LSAType::DXYZ); }
    void insertHANG() { insertNewBlankRecord(LSAType::HANG); }
    void insertAZIM() { insertNewBlankRecord(LSAType::AZIM); }
    void insertVANG() { insertNewBlankRecord(LSAType::VANG); }
    void insertZANG() { insertNewBlankRecord(LSAType::ZANG); }
    void insertHDIF() { insertNewBlankRecord(LSAType::HDIF); }
    void insertHDIR() { insertNewBlankRecord(LSAType::HDIR); }
    void insertDGRP() { insertNewBlankRecord(LSAType::DGRP); }
    void insertHGHT() { insertNewBlankRecord(LSAType::HGHT); }
    void insertVSCA() { insertNewBlankRecord(LSAType::VSCA); }
    void insertUNCR() { insertNewBlankRecord(LSAType::UNCR); }

    void insertMEAN() { insertNewBlankRecord(LSAType::MEAN); }
    void insertENUO() { insertNewBlankRecord(LSAType::ENUO); }

    void insertComment();   ///< Insert a new COMMENT record after the currently selected record
    void insertSeparator(); ///< Insert a new COMMENT containing a separator bar

    void deleteCurrentlySelectedRecords(); ///< Delete the currently selected record(s)
    void enableCurrentlySelectedRecords(); ///< Enable the currently selected record(s)
    void disableCurrentlySelectedRecords(); ///< Disable the currently selected record(s)

    void showAboutSalsaDialog() { AboutSalsaDialog aboutDialog;  aboutDialog.exec(); }

    void handleFilterBoxChanged(std::vector<LSAType>, std::function<bool (std::vector<LSAType>, LSARecord *)>);
    void handleFilterStringChanged(QString newString, bool bExact, bool bMatchAll);
    void handleFilterPointTriggered();
    void handleMatchTypeChanged(bool bMatchAll);

    void onProcessStdErr();
    void onProcessStdOut();
    void onProcessError();
    /// Launch a text editor on the current .out file
    void viewOutput();
    void viewAdjustedPoints();
    void viewManual();
    /// Kill the currently running subprocess (most likely lsasolver)
    void killProcess();
    void cleanupProcess(QString processName);
    void RestoreDockableWidgets();
    /// allow sorting in the Adjustments table on the output tab
    void onTablePointsHeaderClicked(int logicalIndex);
    void exportPOSGsFromHDF5File(bool apriori=false);
    void exportGeoidHeightsFromAprioriPositions();
    void exportPointsXYZ();
    void exportPointsUTM();
    void exportPointsSPC(QString epsgCode="");
    void exportPointsNCAT();
    void exportSolnCov();
    void exportMeasResid();
    void exportMeasResidAsENU();
    void runExportScript(QString defaultOutputName, QString selectionFilter, const char *saveFilePrompt, const char *selectionFilterOptions,
                         QStringList commandLine, MainWindow::PROCESS_TYPES processType, std::string processStart, QString breakerStart, QString breakerEnd,
                         QString errorString);
    void runExportScript(QStringList commandLine, MainWindow::PROCESS_TYPES processType, std::string processStart, QString breakerStart,
                         QString breakerEnd, QString errorString, bool checkH5Status);
    void generateInitialPositions();

    void addAutogeneratedPoints();

    void undo();
    void redo();

signals:
    void updateStatusWindow(bool clear, QString msg=QString(), int fontWeight=0, QColor fontColor=Qt::black);
    void setMenuForSubprocess(bool isNoSubprocess);
    void converterFinished();
    void preprocessorFinished();
    void solverFinished();
    void postprocessorFinished();
    void h5ImportSuccessful();

private:
    // members
    Ui::MainWindow *ui;              ///< pointer to mainwindow ui.  Contains all controls generated by Qt Designer
    GuiModel       *guiModel;        ///< pointer to the model that captures the current state of the network
    QString         currentFile;     ///< full path and file name for the file currently opened for editing
    QString         configFileName;  ///< full path and file name for the project configuration file

    // dynamically populated menu actions
    QAction* currentImportAction;

    QVector<LSARecord*> lsaRecordBuffer;  ///buffer of LSARecord pointers, used for copy and paste
    // TODO: refactor modelIsSaved to be a member of LSAInclude.  This will enable tracking changes to individual files.
    bool            modelIsSaved;    ///< flag indicating the contents of the lsafile have not changed since the last save

    std::vector<QString> failedIncludeList;

    QStandardItemModel *topResidModel;///< class containing data for the output tab top residuals table
    QStandardItemModel *confidenceModel;///< class containing data for the output tab adjustments table
    QStandardItemModel *reliabilityRectangleModel;///< class containing data for the output tab reliability rectangle table
    int                 lastIndicatorColumn;
    Qt::SortOrder       lastIndicatedOrder;

    const int NUM_RESID_COLUMNS=10;//| Meas | From(-At )| To | Raw | Rel | Std | Red'cy | Int Rel | Ext Rel | hidden column for LSARecord*
    const int RESID_TABLE_MEAS_COLUMN=0;
    const int RESID_TABLE_FROMAT_COLUMN=1;
    const int RESID_TABLE_TO_COLUMN=2;
    const int RESID_TABLE_RAW_COLUMN=3;
    const int RESID_TABLE_REL_COLUMN=4;
    const int RESID_TABLE_STD_COLUMN=5;
    const int RESID_TABLE_REDUND_COLUMN=6;
    const int RESID_TABLE_MAX_BIAS_COLUMN=7;
    const int RESID_TABLE_EXT_MAG_COLUMN=8;
    const int RESID_TABLE_HIDDEN_COLUMN=9;
    const std::map<int, QString> residColumnToColumnHeaderMap = {{RESID_TABLE_MEAS_COLUMN, QString("Meas")},
                                                                 {RESID_TABLE_FROMAT_COLUMN, QString("From-(At)")},
                                                                 {RESID_TABLE_TO_COLUMN, QString("To")},
                                                                 {RESID_TABLE_RAW_COLUMN, QString("|Raw|")},
                                                                 {RESID_TABLE_REL_COLUMN, QString("|Rel|")},
                                                                 {RESID_TABLE_STD_COLUMN, QString("|Std|")},
                                                                 {RESID_TABLE_REDUND_COLUMN, QString("Red'cy")},
                                                                 {RESID_TABLE_MAX_BIAS_COLUMN, QString("Int Rel")},
                                                                 {RESID_TABLE_EXT_MAG_COLUMN, QString("Ext Rel")}};

    const int NUM_POINT_COLUMNS=7;
    const int POINT_TABLE_POINT_COLUMN=0;
    const int POINT_TABLE_3DMAJ_COLUMN=1;
    const int POINT_TABLE_2DMAJ_COLUMN=2;
    const int POINT_TABLE_VERT_COLUMN=3;
    const int POINT_TABLE_3DMAJRR_COLUMN=4;
    const int POINT_TABLE_2DMAJRR_COLUMN=5;
    const int POINT_TABLE_VERTRR_COLUMN=6;
    const std::map<int, QString> pointColumnToColumnHeaderMap = {{POINT_TABLE_POINT_COLUMN, QString("Point")},
                                                                 {POINT_TABLE_3DMAJ_COLUMN, QString("3D maj")},
                                                                 {POINT_TABLE_2DMAJ_COLUMN, QString("2D maj")},
                                                                 {POINT_TABLE_VERT_COLUMN, QString("Vertical")},
                                                                 {POINT_TABLE_3DMAJRR_COLUMN, QString("3D maj RR")},
                                                                 {POINT_TABLE_2DMAJRR_COLUMN, QString("2D maj RR")},
                                                                 {POINT_TABLE_VERTRR_COLUMN, QString("Vertical RR")}};

    bool hashCodesAreValid;              ///< indicates if the map from unique IDs in the .bin file to LSA records has broken
    unsigned int currentHashCode;   ///< hash code of the last loaded/saved LSAFile tree

    // executable paths
    QString iob2lsaPath;       ///< path to the iob2lsa converter executable
    QString cgeoidPath;        ///< path to cgeoid executable
    QString preprocessorPath;  ///< path to the lsa to dat preprocessor
    QString solverPath;        ///< path to the solver
    QString lsapostPath;       ///< path to the lsapost utility
    QString python3Path;
    QString converterPath;     ///< path to converterlauncher.py
    QString reportingPath;     ///< path to reporting scripts directory
    QString scriptsPath;     ///< path to reporting scripts directory
    QString lsainverse;     ///< path to the lsainverse executable

    // TODO: add path to data directory here

    // dynamically allocated widgets and views

    Mapping             *mapWidget;    ///< widget the encapsulates a MarbleWidget and displays lsaModel
    RecordEditor        *recordEditor; ///< dynamically allocated widget that displays contents of a single LSARecord
    AdjustedPositions   *adjustedPositions;
    StationDataDialog   *stationData;

    bool autoDismissModalDialogs; ///< For test harness use ONLY

    QByteArray mainWindowInitialState;
    bool       suppressGuiUpdates;

    // Config and Preferences dialogs
    ConfigDialog      *configDialog;
    PreferencesDialog *preferencesDialog;
    HistogramDialog   *histogramDialog;
    QWidget           *findWidget;

    bool progressBarHasBeenShown;//for testing purposes only

    //QProcess members
    QProcess *proc;           ///< pointer to spawned external processes (one at a time)
    PROCESS_TYPES procType;   ///< enum that refers to currently running process type
    PROCESS_TYPES lastSolverRunType; ///< used when parsing HDF5 file to determine whether --apquit was used
    QString procStdOut;       ///< stdout of spawned external process
    QString procStdErr;       ///< stderr of spawned external process
    QString procWarnings;     ///< Buffer used to store warning strings when parsing stdout
    QString procChainStdErr;       ///< repeat of procStdErr for entire process chain
    QString procChainWarnings;     ///< repeat of procWarnings for entire process chain
    QMessageBox *errorBox;
    bool procTypeIsConverter()     { return FIRST_CONVERTER <= procType && procType <= LAST_CONVERTER; }
    bool procTypeIsSolver()        { return FIRST_SOLVER    <= procType && procType <= LAST_SOLVER; }
    bool procTypeIsPostprocessor() { return FIRST_POST      <= procType && procType <= LAST_POST; }
    bool procTypeIsReporter()      { return FIRST_REPORTER  <= procType && procType <= LAST_REPORTER; }

#ifdef _WIN32
    FILETIME lastOutCreationTime;///< Time the last .out file was created

    FILETIME windowsGetLastModifiedTime(QString filename);
#else
    qint64 lastOutCreationTime;///< Time the last .out file was created in msec since 1970-Jan-1
#endif

    // QProcess methods
    /// Start a process, verify it finishes before the specified timeout expires.  Blocks the gui during
    /// processing and pushes errors and warnings to the status window.
    /// @param the name of the exe the process will launch
    /// @param args list of arguments to pass to the exe
    /// @return true on successful finish without error
    bool runProcess(QString process, QStringList args);

    /// Load an .iob file, convert and save it as a .lsa file
    /// @param filename the path and filename of the iob file to load
    /// @return true on successful finish without error
    QString convertIOBProject(QString filename);
    QString convertIOBFile(QString filename);
    QString convertIOB(QString fileName, bool isProject);
    QString convertAndLoadIOBProject(QString iobFileName);
    QString convertInstrumentFile(QString filename, lsa::CONVERTER_TYPE fileType);
    QString convertCustomImport(QString filename);
    QString openInputGeoidFile(void);
    void createConverterConfigFile(QString filename);
    void removeConverterConfigFile(QString filename);

    void updateFileChangedTimeStamps();
    void validateFileChangedTimeStamps();


    /// Load an .lsa file and create a new LSAModel with its contents
    /// @param filename the path and filename of the lsa file to load
    /// @returns a vector<string> containing every line of the lsafile tree that could not be parsed
    std::vector<std::string> loadLsaFile(QString filename, bool convertingFromIOB=false);

    /// Handle a process finished signal.  Push status, warning and error messages to the status window.
    /// @param exitCode - exit code of the process
    /// @param editStatus - exit status of the process
    void onProcessEnd(int exitCode, QProcess::ExitStatus exitStatus);

    bool DisplaySolverWarningsAndErrors(bool firstRun);
    void popupForMissingIncludes();

    /// preps a string for pushing to the status window; Windows has extra end-of-line characters.
    QString cleanQString(QString msg);

    //Status Window methods
    void pushStatus(std::string msg);  ///< Push a 'status' formatted string to the status window
    void pushWarning(std::string msg); ///< Push a 'warning' formatted string to the status window
    void pushError(std::string msg);   ///< Push a 'error' formatted string to the status window
    void pushProcessStart(std::string processName);
    void pushKillProcess();
    void pushProcessFail();
    void pushProcessComplete();
    void pushBreakerStart(QString menuItemString); ///< Push a green timestamp with many --- signs and menu item for process chain
    void pushBreakerEnd();             ///< Push a green timestamp with many --- signs
    void reportCLIWarningsAndErrors(bool isEndOfProcessChain=false);

    QString createNewEmptyLSAFile(QString testFileName = "");
    QStringList getExistingFilenames(lsa::CONVERTER_TYPE fileType);

    // private methods
    /// Update all of the widgets in the main window
    /// Intended use is that all slots handling signals from various widgets can do the following:
    /// 1) Update lsaModel with any new data contained in the signal
    /// 2) Once lsaModel is updated, call updateWidgets to refresh all the widgets with contents of lsaModel
    void updateWidgets();

    void setupGuiModel();
    void setupTreeView();
    void setupProgressBar();
    void setupStatusWindow();
    void setupRecordEditor();
    void setupAdjustedPositions();
    void setupStationData();
    void setupMapWidget();
    void setupHistogram();
    void setupMainWindowGeometry();
    void persistMainWindowGeometry();

    QMessageBox::StandardButton confirmChangesAreSaved( QString message = QString() );
    QMessageBox::StandardButton confirmChangesAreSavedParseWarning( QString message = QString() );
    QMessageBox::StandardButton confirmDeleteInclude(QString message);

    void clearOutputTab();
    void clearAdjustmentSuccessIndicators();
    void enableDisableMenuActions();
    void enableDisableCustomActions(QMenu *menu, bool singleRowSelected);
    void enableDisableCopyPaste();
    void showHideAddAutogen();
    void addCustomMenu();


    bool    runPreprocessor(QString &datFile);
    bool    runPostprocessor(int solverRunType);
    bool    runReportingScripts();

    //ProgressBar functions
    QList<int> previousSizes;
    void startProgressBar();
    void updateProgressBar(QString firstWord, QString line);
    void finishProgressBar();

    void    setupMenus();
    void    setupRecentProjectsMenu();
    void    setupOutputTables();
    void    addOutputTableHeaders();
    void    addResidualTableHeaders();
    void    addConfidenceTableHeaders();
    void    setResidualTableColumnWidths();
    void    setConfidenceTableColumnWidths();
    void    setOutputTableColumnWidths();

    void updateRecentProjectsMenu(QString fileName);
    void addActionRecentProjects(QString fileName);

    bool isChildOfWidget(QObject *oldWidget, QWidget *widget) const;
    bool isPPP(LSARecord *lsaRecord);

    void deleteRecord(QModelIndex index, QVector<QVector<IndexRowColumnParentChainItem>> removedIndicesChain = QVector<QVector<IndexRowColumnParentChainItem>>());

    /// Returns the RSS of a covariance matrix eigenvalues, rescaled by the critical value of a
    /// chi-square distribution with 3 degrees of freedom, and cumulative probability of confidence
    /// @param covMat - 3x3 covariance matrix
    /// @return estimate of the confidence region
    double computeConfidenceRegion(gnsstk::Matrix<double> &covMat);

    ///creates a map of unique IDs in the .bin file to LSARecord pointers corresponding to records in the tree view
    void generateUniqueIDMapToLSARecordPointers();
    bool isPassedToDAT(LSARecord *lsaRecord);

    void getFromAtToStringsFromLSARecord(LSARecord *lsaRecord, std::string &FromAt, std::string &To);

    ///reads the .out file and compares it's hash against currentHashCode.  Returns true if equal, false otherwise.
    bool isHashConsistent();

    void insertNewBlankRecord(LSAType lsaType);

    ///Checks if a config file exists in the project, and if not, copies the default config file into the project and updates the geoidfile path
    void copyConfigFile(QString lsaFileName=QString(""));
    bool updateLegacyConfigFile();
    void applyConfigFile();

    void copyCustomFiles(QString projectDir, std::string typeString);

    void parseLSAFileErrorsAndWarnings(bool saving=false);

    /// Fills structures used by the map with output from the solver and inserts autogen include file in the treeview
    /// @return - true if final results from the solver exist, false if not
    bool ingestSolverOutputs(bool onlyApriori=false);
    void parseAutogeneratedPoints();

    bool readHDF5File(bool openingLSAProject=false);
    bool updateOutputPanel();

    //methods to get user preferences from the .cfg file
    bool usingWestLongitude();
    bool isScaleByAPV();
    bool includeUnused();
    bool isAllowFast();
    std::string getConfidenceInterval();
    std::string getChiSquared();
    QString getGeoidFileName();
    bool isGeoidFileUsed();
    bool isProjDirGeoid(); //true if there is a specified geoid file in the proj dir
    double getMaxExtMagWarningThreshold() {QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName")); return qsettings.value( lsa::QSETTINGS_MAX_EXT_MAG_WARNING, lsa::DEFAULT_MAX_EXT_MAG_WARNING).toDouble();}
    double getMaxExtMagErrorThreshold() {QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName")); return qsettings.value( lsa::QSETTINGS_MAX_EXT_MAG_ERROR, lsa::DEFAULT_MAX_EXT_MAG_ERROR).toDouble();}
    template <typename T>
    T parseCFGFile(lsa::CFG_PARSE_TYPE cfgParseType);

    /// Ensure that all modifiers descended from parentIncludeIndex don't already exist in the rest of the project
    /// @param parentIncludeIndex rootIndex to identify the part of the tree that needs to be checked for duplicates
    bool makeModifiersLabelsUnique(QModelIndex parentIncludeIndex, LSAUNCRMap *oldUncrMap, LSAHGHTMap *oldHeightMap,
                                   LSAVSCAMap *oldVarMap, LSADirGroupMap *oldDirGroupMap);
    bool makeUncrLabelsUnique(QModelIndex parentIncludeIndex, LSAUNCRMap *oldUncrMap);
    bool makeHeightLabelsUnique(QModelIndex parentIncludeIndex, LSAHGHTMap *oldHeightMap);
    bool makeVarScalingLabelsUnique(QModelIndex parentIncludeIndex, LSAVSCAMap *oldVarMap);
    bool makeDirGroupLabelsUnique(QModelIndex parentIncludeIndex, LSADirGroupMap *oldDirGroupMap);

    void pushParseWarningsForDescendants(QModelIndex parentIndex);
    QString getFileNameFromFullPath(QString fullPath);
    bool isFileWithinProject(QString filename);

    bool timeStampsAreInitialized;

    void removeOldOutputFiles(QString projectFileName);

    QMap<QString, QDateTime> fileChangedTimes;
    unsigned int getNumDXYZwithHGHT() const;

    bool checkOverwrite(const QString &fileName) const;
    QStringList checkOverwrite(const QStringList &fileList) const;

    QMessageBox::StandardButton provideOverwriteWarningMessageBox(const QString &label, const QString &message);

    QMessageBox::StandardButton giveOverwriteMessage(const QString &fileName);
    QMessageBox::StandardButton giveOverwriteMessage(const QStringList &fileList);

    std::map<QAction*, int> mapResidualsActionToColumn;
    std::map<QAction*, int> mapPointsActionToColumn;

    bool isTestHarness() const;

    void populateCustomExportDir(QString currentDir);
    void setTreeViewRowHidden(int row, const QModelIndex& parent, bool bHidden);
    bool bTVRowVisibilityChanged = true;

    // This method should give the correct row and parent so that a file dropped onto the treeview
    // should be included where the indicator shows. The rowIn CAN change in the case that there
    // is more than one file being dropped, so that every file after the first is include after the first
    void correctIncludeDropPosition(int& rowIn, int& rowOut, QModelIndex &parentIdx);

    bool h5FileFound() const;
};

#endif // LSAGUIWINDOW_H
