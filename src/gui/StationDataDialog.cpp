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
#include "StationDataDialog.hpp"
#include "ui_StationDataDialog.h"
#include "lsah5.hpp"
#include "MainWindow.hpp"
#include "lsaUtils.hpp"

#include <QColor>
#include <QProcess>
#include <QCompleter>
#include <QDir>
#include <QStandardPaths>
#include <QIODevice>
#include <QTextStream>
#include <QRegExp>
#include <QModelIndex>
#include <QPair>
#include <QFileInfo>
#include <QKeySequence>
#include <QPoint>
#include <QPalette>

#include <fstream>
#include <string>
#include <map>
#include <algorithm>

StationDataDialog::StationDataDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StationDataDialog)
{
    initialized = false;
    ui->setupUi(this);

    suppressOutputUpdates = false;

    connect(ui->comboBoxFrom,       SIGNAL(currentTextChanged(QString)),    this, SLOT(comboBoxChanged()) );
    connect(ui->comboBoxTo,         SIGNAL(currentTextChanged(QString)),    this, SLOT(comboBoxChanged()) );
    connect(ui->btnSwapFromTo,      SIGNAL(clicked()),                      this, SLOT(btnSwapClicked()) );
    connect(ui->btnExport,          SIGNAL(clicked()),                      this, SLOT(btnExportClicked()) );
    connect(ui->btnExportResultsAs, SIGNAL(clicked()),                      this, SLOT(btnExportAsClicked()));

    connect(ui->btnPairAdd,         SIGNAL(clicked()),                      this, SLOT(handleAddBtnClicked()) );
    connect(ui->stationPairsTable,  SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)) );

    ui->btnExport->installEventFilter(this);
    ui->btnExportResultsAs->installEventFilter(this);

    QPalette lineEditPal;
    lineEditPal.setColor(QPalette::Base, Qt::transparent);
    lineEditPal.setColor(QPalette::Text, Qt::black);

    ui->fromLatD->setPalette(lineEditPal);
    ui->fromLatM->setPalette(lineEditPal);
    ui->fromLatS->setPalette(lineEditPal);

    ui->toLatD->setPalette(lineEditPal);
    ui->toLatM->setPalette(lineEditPal);
    ui->toLatS->setPalette(lineEditPal);

    ui->fromLonD->setPalette(lineEditPal);
    ui->fromLonM->setPalette(lineEditPal);
    ui->fromLonS->setPalette(lineEditPal);

    ui->toLonD->setPalette(lineEditPal);
    ui->toLonM->setPalette(lineEditPal);
    ui->toLonS->setPalette(lineEditPal);

    ui->fromEllipHt->setPalette(lineEditPal);
    ui->toEllipHt->setPalette(lineEditPal);

    ui->fromOrthoHt->setPalette(lineEditPal);
    ui->toOrthoHt->setPalette(lineEditPal);

    ui->fromUnd->setPalette(lineEditPal);
    ui->toUnd->setPalette(lineEditPal);

    ui->fromNSDefl->setPalette(lineEditPal);
    ui->toNSDefl->setPalette(lineEditPal);

    ui->fromEWDefl->setPalette(lineEditPal);
    ui->toEWDefl->setPalette(lineEditPal);

    ui->slantDistance->setPalette(lineEditPal);
    ui->uncrSlantDistance->setPalette(lineEditPal);

    ui->geodesicDistance->setPalette(lineEditPal);
    ui->uncrGeoDistance->setPalette(lineEditPal);

    ui->horizontalDistance->setPalette(lineEditPal);
    ui->uncrHorizontalDistance->setPalette(lineEditPal);

    ui->azimD->setPalette(lineEditPal);
    ui->azimM->setPalette(lineEditPal);
    ui->azimS->setPalette(lineEditPal);
    ui->uncrAzimuth->setPalette(lineEditPal);

    ui->vangD->setPalette(lineEditPal);
    ui->vangM->setPalette(lineEditPal);
    ui->vangS->setPalette(lineEditPal);
    ui->uncrVerticalAngle->setPalette(lineEditPal);

    ui->heightDifference->setPalette(lineEditPal);
    ui->uncrHeightDifference->setPalette(lineEditPal);

    ui->DX->setPalette(lineEditPal);
    ui->DY->setPalette(lineEditPal);
    ui->DZ->setPalette(lineEditPal);

    ui->covarianceXX->setPalette(lineEditPal);
    ui->covarianceXY->setPalette(lineEditPal);
    ui->covarianceXZ->setPalette(lineEditPal);
    ui->covarianceYY->setPalette(lineEditPal);
    ui->covarianceYZ->setPalette(lineEditPal);
    ui->covarianceZZ->setPalette(lineEditPal);

    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    restoreGeometry(qsettings.value(QSETTINGS_SDD_GEOMETRY, saveGeometry() ).toByteArray());
    initialized = true;
}

StationDataDialog::~StationDataDialog()
{
    delete ui;
}

void StationDataDialog::setup(LSAH5File *h5file, QString hdf5FileName, bool apv, bool westLon)
{
    setupComboBoxes(h5file, hdf5FileName, apv, westLon);
}

void StationDataDialog::setupComboBoxes(LSAH5File *h5file, QString hdf5FileName, bool apv, bool westLon)
{
    if(h5file == NULL)
    {
        h5FilePath.clear();
        scaleByAPV = false;
        usingWestLon = false;
        return;
    }

    h5FilePath = hdf5FileName;
    scaleByAPV = apv;
    usingWestLon = westLon;

    suppressOutputUpdates = true;
    {
        stationList.clear();
        stationList << "";

        for(int i=0;i<h5file->llh->getSize();i++)
        {
            // Don't include fixed points in the point confidence table
            QString stationLabel = QString::fromStdString(h5file->llh->getDataPoint(i).label);
            stationList << stationLabel;
        }

        //Fix to Bug #1396
        stationList.sort();

        QCompleter *fromStationCompleter = new QCompleter(stationList, this);
        fromStationCompleter->setCaseSensitivity(Qt::CaseInsensitive);

        QCompleter *toStationCompleter = new QCompleter(stationList, this);
        toStationCompleter->setCaseSensitivity(Qt::CaseInsensitive);

        ui->comboBoxFrom->clear();
        ui->comboBoxFrom->addItems(stationList);
        ui->comboBoxFrom->setEditable(true);
        ui->comboBoxFrom->setCompleter(fromStationCompleter);

        ui->comboBoxTo->clear();
        ui->comboBoxTo->addItems(stationList);
        ui->comboBoxTo->setEditable(true);
        ui->comboBoxTo->setCompleter(toStationCompleter);


    }
    suppressOutputUpdates = false;

    if(h5FilePath == oldH5File && validStations(oldFromStation, oldToStation))
    {
        suppressOutputUpdates = true;
        ui->comboBoxFrom->setCurrentText(oldFromStation);
        ui->comboBoxTo->setCurrentText(oldToStation);

        updateOutput();
        suppressOutputUpdates = false;
    }
}

void StationDataDialog::setupPairTable(const QString &projectDir, const QString &projectName)
{
    // Allocate and initialize model
    invPairsModel = new QStandardItemModel(0, NUM_PAIR_COLUMNS, this);
    QStringList headerList;
    headerList.append(QString("From Station"));
    headerList.append(QString("To Station"));
    invPairsModel->setHorizontalHeaderLabels(headerList);

    ui->stationPairsTable->setModel(invPairsModel);

    setTableColumnWidths();

    for(int columnIndex = 0; columnIndex < ui->stationPairsTable->horizontalHeader()->count(); columnIndex++)
    {
        ui->stationPairsTable->horizontalHeader()->setSectionResizeMode(columnIndex, QHeaderView::Stretch);
    }

    allStationPairs = LSAInvPairData(projectDir, projectName);

    std::vector<LSAInvPairData::fromToPair> fromToPairList = allStationPairs.giveStationPairList();
    for(const auto &pairElem : fromToPairList)
    {
        validateAndAddPair(QString::fromStdString(pairElem.first), QString::fromStdString(pairElem.second));
    }

    ui->stationPairsTable->setContextMenuPolicy(Qt::CustomContextMenu);

    rightClickMenu = new QMenu(ui->stationPairsTable);
    rightClickMenu->setObjectName("rightClickMenu");

    actionDelete = new QAction(rightClickMenu);
    actionDelete->setText("Delete");
    actionDelete->setObjectName("actionDelete");
    actionDelete->setShortcut(QKeySequence(tr("Del")));

    rightClickMenu->addAction(actionDelete);

    ui->stationPairsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    connect(ui->stationPairsTable->horizontalHeader(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(handleTableStationPairsResized(int,int,int)));
    connect(actionDelete,           SIGNAL(triggered()),                    this, SLOT(handleDeleteAction()) );
    connect(ui->stationPairsTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(handleSelectionChanged(QItemSelection , QItemSelection)) );
}

void StationDataDialog::setupPairTable(const QString &projectFile)
{
    QFileInfo projectFileInfo(projectFile);
    setupPairTable(projectFileInfo.path(), projectFileInfo.baseName());
}

void StationDataDialog::handleStationsChanged()
{
    updateOutput();
}

void StationDataDialog::handleSwapBtnClicked()
{
    QString newFrom = toStation;
    QString newTo = fromStation;

    suppressOutputUpdates = true;
    {
        ui->comboBoxFrom->setCurrentText(newFrom);
        ui->comboBoxTo->setCurrentText(newTo);
    }
    updateOutput();
    suppressOutputUpdates = false;
}

void StationDataDialog::handleExportBtnClicked()
{
    bool exportTriggered = true;
    bool clearOutputFile;

    QString currentFrom = fromStation, currentTo = toStation;

    // Gathers all stations that have repeats
    std::vector<LSAInvPairData::fromToPair> repeatStations = allStationPairs.giveDuplicateList();

    // make a map with those station pairs. Initialize all values to false
    std::map<LSAInvPairData::fromToPair, bool> repeatStationCounter;
    std::map<LSAInvPairData::fromToPair, bool>::iterator repeat_iter;

    for(const auto &stationElem : repeatStations)
    {
        repeatStationCounter[stationElem] = false;
    }

    std::vector<LSAInvPairData::fromToPair> fromToPairList = allStationPairs.giveStationPairList();
    QStringList invalidPairList;

    // Avoid writing duplicates in the output files
    for(std::vector<LSAInvPairData::fromToPair>::const_iterator pairList_iter = fromToPairList.begin();
        pairList_iter != fromToPairList.end(); ++pairList_iter)
    {
        // Set output file to be cleared if it's the first element
        pairList_iter != fromToPairList.begin() ? clearOutputFile = false : clearOutputFile = true;

        repeat_iter = repeatStationCounter.find(*pairList_iter);

        // If station pair is part of duplicate
        if(repeat_iter!=repeatStationCounter.end())
        {
            // If been marked true (occurs after encountering first instance of duplicate station pair)
            if(repeat_iter->second)
            {
                continue;
            }

            // At first instance set to true so future duplicate pairs will hit continue statement
            else
            {
                repeat_iter->second = true;
            }
        }

        fromStation = QString::fromStdString(pairList_iter->first);
        toStation = QString::fromStdString(pairList_iter->second);

        if(!validStations(fromStation, toStation))
        {
            invalidPairList << "<" + fromStation + ", " + toStation + ">";
            continue;
        }

        updateOutput(exportTriggered, clearOutputFile);
    }

    if(!invalidPairList.empty())
    {
        QString warningMessage = "Warning - could not produce station pair data for following pair(s): " + invalidPairList.join("; ");
        emit pushWarning(warningMessage);
    }

    setNewFromTo(currentFrom, currentTo);
}

bool StationDataDialog::eventFilter(QObject *object, QEvent *event)
{
    if(event->type() == QEvent::FocusOut)
    {
        if(object == ui->btnExport || object == ui->btnExportResultsAs)
        {
            ui->labelExportPath->clear();
        }
    }
    return  false;
}

QString StationDataDialog::utilizeFileBrowser()
{
    QFileInfo h5FileInfo(h5FilePath);
    QString selfilter = tr("inv output files (*.inv)");
    QString currentDir = h5FileInfo.dir().dirName();
    auto fileName = QFileDialog::getSaveFileName(this,
                                                 tr("Create inverse file output name"),
                                                 currentDir,
                                                 tr("inv output files (*.inv);; All files(*)"),
                                                 &selfilter);

    // If the user pressed cancel, then fileName will be an empty QString.
    return fileName;
}

void StationDataDialog::handleAddBtnClicked()
{

    // Validation done in method below. Won't skip
    // invalid pairs but will highlight.

    validateAndAddPair(fromStation, toStation, true);
    allStationPairs.writeConfig();
}

void StationDataDialog::handleDeleteAction()
{
    QModelIndexList currentRowList = ui->stationPairsTable->selectionModel()->selectedRows();

    if(currentRowList.size() == 0)
    {
        return;
    }

    for(QModelIndexList::reverse_iterator deleteIterator = currentRowList.rbegin();
        deleteIterator != currentRowList.rend(); ++deleteIterator)
    {
        deletePair(deleteIterator->row());
    }

    allStationPairs.writeConfig();
}

void StationDataDialog::handleSelectionChanged(const QItemSelection &updatedSelection, const QItemSelection &oldSelection)
{
    // Check if selection has been updated. If it's stayed the same return
    if(updatedSelection==oldSelection)
    {
        return;
    }

    QModelIndexList rowList = ui->stationPairsTable->selectionModel()->selectedRows();

    if(rowList.size() == 1)
    {
        QString fromLabel = invPairsModel->item(rowList[0].row(), 0)->text();
        QString toLabel = invPairsModel->item(rowList[0].row(), 1)->text();
        setNewFromTo(fromLabel, toLabel);
    }

}

void StationDataDialog::validateAndAddPair(const QString &fromText, const QString &toText, bool fromAddButton)
{
    int numRows = invPairsModel->rowCount();
    int rowCounter = 0;
    bool pairExists = false;

    QStandardItem *fromChecker, *toChecker;

    QColor textColorFrom = Qt::black, textColorTo = Qt::black;

    // Works with hard coded assumpation that there are two columns

    // Check to see if there are iterators associated with the QStandardItemModel class
    // Prefer not to use indices

    // Check to see if pair existed previously in table. If so, set label text for both
    // from and to station to red.
    while(rowCounter < numRows && !pairExists)
    {
        fromChecker = invPairsModel->item(rowCounter, 0);
        toChecker = invPairsModel->item(rowCounter, 1);

        if(fromText == fromChecker->text() && toText == toChecker->text())
        {
            pairExists = true;
        }

        else
        {
            rowCounter++;
        }
    }

    if(pairExists)
    {
        textColorFrom = Qt::red;
        textColorTo = Qt::red;
    }

    // Otherwise, double check to make sure they're valid. If either is invalid
    // then make the text for the invalid station(s) red.

    else if(!validStations(fromText, toText))
    {
        bool fromIsValid = (!fromText.isEmpty() && stationList.contains(fromText));
        bool toIsValid = (!toText.isEmpty() && stationList.contains(toText));

        if(!fromIsValid)
        {
            textColorFrom = Qt::red;
        }

        if(!toIsValid)
        {
            textColorTo = Qt::red;
        }
    }


    // At this point we've verified that the station pair did not exist previously so we can
    // go ahead and add it to the model as well as the configuration file.

    QStandardItem *fromItem = new QStandardItem(fromText);
    QStandardItem *toItem = new QStandardItem(toText);

    fromItem->setForeground(QBrush(textColorFrom));
    toItem->setForeground(QBrush(textColorTo));

    QList<QStandardItem *> newRow;
    newRow << fromItem << toItem;


    if(fromAddButton)
    {
        LSAInvPairData::fromToPair stationPair;
        stationPair.first = fromText.toStdString();
        stationPair.second = toText.toStdString();
        allStationPairs.addPair(stationPair);
    }

    invPairsModel->appendRow(newRow);
}

void StationDataDialog::deletePair(int deleteIndex)
{
    invPairsModel->removeRow(deleteIndex);

    if(!allStationPairs.deletePair(static_cast<unsigned int>(deleteIndex)))
    {
        pushGuiWarning("Warning - could not delete at station dialog pair at specified index.");
    }
}

void StationDataDialog::saveWidgetSettings()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    qsettings.setValue(QSETTINGS_SDD_GEOMETRY, saveGeometry());
}

void StationDataDialog::updateOutput(bool exportTriggered, bool clearOutputFile)
{
    if(fromToStations_Changed)
    {
        //from Station value
        fromStation = ui->comboBoxFrom->currentText();
        //to Station value
        toStation = ui->comboBoxTo->currentText();
    }

    if(validStations(fromStation, toStation))
    {
        oldH5File = h5FilePath;
        oldFromStation = fromStation;
        oldToStation = toStation;
        QString fromToStation = fromStation + toStation;
        QStringList inverseData;
        bool alreadyCalculated = false;
        std::map<QString, QStringList>::const_iterator it;

        it = StationInverseMap.find(fromToStation);
        if(it != StationInverseMap.end() && !exportTriggered)
        {
           alreadyCalculated = true;
           inverseData = it->second;
           clearOutput();
           parseToGui(inverseData);
        }

        if(!alreadyCalculated)
        {
           runLSAInverse(exportTriggered, clearOutputFile);
        }

    }
    else
    {
        clearOutput();
    }

    checkBtnAddStatus();
}

bool StationDataDialog::validStations(QString from, QString to)
{
    if (from == to)
        return false;

    bool fromIsValid = (!from.isEmpty() && stationList.contains(from));
    bool toIsValid =   (!to.isEmpty() && stationList.contains(to));

    return fromIsValid && toIsValid;
}

void StationDataDialog::setNewFromTo(QString fromLabel, QString toLabel)
{
    (fromLabel == ui->comboBoxFrom->currentText() && toLabel == ui->comboBoxTo->currentText()) ?
                fromToStations_Changed = false : fromToStations_Changed = true;

    if(!fromToStations_Changed)
    {
        return;
    }

    ui->comboBoxFrom->setCurrentText(fromLabel);
    ui->comboBoxTo->setCurrentText(toLabel);

    updateOutput();
}

void StationDataDialog::runLSAInverse(bool exportTriggered, bool clearOutputFile)
{
    QProcess proc(this);

    // setup executable path
    QStringList searchPaths;
    QString lsaApplicationPath = QDir::cleanPath(QCoreApplication::applicationDirPath());
    searchPaths << lsaApplicationPath;
    lsainverse = QStandardPaths::findExecutable(QString("lsainverse"), searchPaths);


    // set up arguments for executable
    QStringList lsaInverseArgs;
    lsaInverseArgs << "--from" << fromStation
               << "--to"   << toStation
               << "--hdf5" << h5FilePath;

    if(!exportTriggered)
    {
        lsaInverseArgs << "--GUIOutput";
    }
    if(scaleByAPV)
    {
        lsaInverseArgs << "--scaleByAPV";
    }

    if(usingWestLon)
    {
        lsaInverseArgs << "--westLon";
    }

    QString exportFileName;
    QString exportPath;
    //export button was clicked, run process with --log
    if(exportTriggered)
    {
        QString base = giveFileBase();
        exportFileName = base + ".inv";
        exportPath = giveFileAbsPath();
        lsaInverseArgs << "--log" << exportFileName
                       << "--logpath" << exportPath;

        if(clearOutputFile)
        {
            QFile outputFile(exportPath + QDir::separator() + exportFileName);
            outputFile.open(QIODevice::ReadWrite | QIODevice::Text);
            outputFile.resize(0);
            outputFile.close();

            QFile csvOutput(exportPath + QDir::separator() + base + "Inv.csv");
            csvOutput.open(QIODevice::ReadWrite | QIODevice::Text);
            csvOutput.resize(0);
            csvOutput.close();
        }
    }
    else
    {
        //clear exisiting gui data to prepare for new gui data
        clearOutput();
    }

    // Grab the timestamp
    QString exportFullPath = QDir::cleanPath(exportPath +  QDir::separator() + exportFileName);
    QDateTime timestamp = QFileInfo(exportFullPath).lastModified();

    // run process
    proc.start(lsainverse, lsaInverseArgs);
    proc.waitForFinished();

    // process Errors from lsaInverse
    QString procStdErr = QString(proc.readAllStandardError());
    bool errorsPresent = false;
    bool warningsPresent = false;
    if(!procStdErr.isEmpty())
    {
        errorsPresent = true;
        emit pushGuiError(procStdErr);
        procStdErr.clear();
    }

    // process warnings from lsaInverse
    QString procStdOut = QString(proc.readAllStandardOutput());
#ifdef _WIN32
    QStringList inverseData = procStdOut.split("\r\n");
#else
    QStringList inverseData = procStdOut.split("\n");
#endif
    QString fromToLabel = fromStation + toStation;

    foreach(QString line, inverseData)
    {
        if(line.toUpper().contains("WARNING"))
        {
            emit pushGuiWarning(line);
            warningsPresent = true;
        }
    }

    // update gui data in station data dialog
    if(exportTriggered && clearOutputFile)
    {
        QDir exportDir(exportPath);
        bool exportFileExists = exportDir.exists(exportFileName);

        QDateTime updatedTimestamp = QFileInfo(exportFullPath).lastModified();
        QString message;

        if (errorsPresent || warningsPresent)
        {
            message = "Error or warning encountered. See status window for details.";
        }
        else if (exportFileExists && timestamp != updatedTimestamp)
        {
            message = "File exported to: " + exportDir.absoluteFilePath(exportFileName);
        }
        else
        {
            message = "Inverse export failed.";
        }
        ui->labelExportPath->setText(message);
        emit pushGuiStatus( "STATION DATA - " +  message);

    }
    else if(!exportTriggered)
    {
        //parse lsainverse output and fill dialog gui
        parseToGui(inverseData);
        StationInverseMap.insert(std::map<QString,QStringList>::value_type(fromToLabel,inverseData));
    }

}

void StationDataDialog::parseToGui(QStringList inverseData)
{
    bool readingFrom = false;
    bool readingTo = false;
    bool readingExtracted = false;

    foreach(QString line, inverseData)
    {
        if(line.isEmpty() || line.contains("lsainverse") || line.toUpper().contains("WARNING"))
        {
            continue;
        }
        else if(line == "EXTRACTED MEASUREMENTS (value, (standard deviation)):")
        {   readingTo = false;
            readingExtracted = true;
            continue;
        }

        QStringList dataline = line.split((": "));
        QString measurementLabel  = dataline[0];
        QString measurementStr = dataline[1];

        if(measurementLabel == "FROM STATION"){ readingFrom = true; }
        if(measurementLabel == "TO STATION") { readingFrom = false; readingTo = true; }


        if(readingFrom)
        {
            QStringList dataStr = measurementStr.split(" ");
            QString measurementValue = dataStr[0];
            int numItems = dataStr.size();

            if(measurementLabel  == "Geodetic Latitude" && numItems == 4)
            {
                if((dataStr[1].toInt() < 0) || (dataStr[1].contains("-0")))//always report positive latitude
                {
                    //convert from +N latitude to +S latitude
                    dataStr[1] = QString::number(-dataStr[1].toInt());
                    dataStr[0] = QString("S");
                }
                ui->fromLatD->setText(dataStr[1]);
                ui->fromLatM->setText(dataStr[2]);
                ui->fromLatS->setText(dataStr[3]);
                ui->fromLatDir->setText(dataStr[0]);
            }
            else if(measurementLabel  == "Geodetic Longitude" && numItems == 4)
            {
                ui->fromLonD->setText(dataStr[1]);
                ui->fromLonM->setText(dataStr[2]);
                ui->fromLonS->setText(dataStr[3]);
                ui->fromLonDir->setText(dataStr[0]);
            }
            else if(measurementLabel  == "Ellipsoidal Height" && numItems == 1)
            {
                ui->fromEllipHt->setText(measurementValue);
            }
            else if(measurementLabel  == "Orthometric Height" && numItems == 1)
            {
                ui->fromOrthoHt->setText(measurementValue);
            }
            else if(measurementLabel  == "Undulation" && numItems == 1)
            {
                ui->fromUnd->setText(measurementValue);
            }
            else if(measurementLabel  == "N Deflection" && numItems == 1)
            {
                ui->fromNSDefl->setText(measurementValue);
            }
            else if(measurementLabel  == "E Deflection" && numItems == 1)
            {
                ui->fromEWDefl->setText(measurementValue);
            }
        }
        if(readingTo)
        {
            QStringList dataStr = measurementStr.split(" ");
            QString measurementValue = dataStr[0];
            int numItems = dataStr.size();

            if(measurementLabel  == "Geodetic Latitude" && numItems == 4)
            {
                if((dataStr[1].toInt() < 0) || (dataStr[1].contains("-0")))//always report positive latitude
                {
                    //convert from +N latitude to +S latitude
                    dataStr[1] = QString::number(-dataStr[1].toInt());
                    dataStr[0] = QString("S");
                }
                ui->toLatD->setText(dataStr[1]);
                ui->toLatM->setText(dataStr[2]);
                ui->toLatS->setText(dataStr[3]);
                ui->toLatDir->setText(dataStr[0]);
            }
            else if(measurementLabel  == "Geodetic Longitude" && numItems == 4)
            {
                ui->toLonD->setText(dataStr[1]);
                ui->toLonM->setText(dataStr[2]);
                ui->toLonS->setText(dataStr[3]);
                ui->toLonDir->setText(dataStr[0]);
            }
            else if(measurementLabel  == "Ellipsoidal Height" && numItems == 1)
            {
                ui->toEllipHt->setText(measurementValue);
            }
            else if(measurementLabel  == "Orthometric Height" && numItems == 1)
            {
                ui->toOrthoHt->setText(measurementValue);
            }
            else if(measurementLabel  == "Undulation" && numItems == 1)
            {
                ui->toUnd->setText(measurementValue);
            }
            else if(measurementLabel  == "N Deflection" && numItems == 1)
            {
                ui->toNSDefl->setText(measurementValue);
            }
            else if(measurementLabel  == "E Deflection" && numItems == 1)
            {
                ui->toEWDefl->setText(measurementValue);
            }
        }
        if(readingExtracted)
        {
            QStringList dataStr = measurementStr.simplified().split(" ");
            QString measurementValue = dataStr[0];
            QString uncertaintyValue = dataStr[1];
            int numItems = dataStr.size();

            if( measurementLabel  == "Slant Distance" && numItems == 2)
            {
                ui->slantDistance->setText(measurementValue);
                ui->uncrSlantDistance->setText(uncertaintyValue);
            }
            else if( measurementLabel  == "Ellipsoidal Distance" && numItems == 2)
            {
                ui->geodesicDistance->setText(measurementValue);
                ui->uncrGeoDistance->setText(uncertaintyValue);
            }
            else if( measurementLabel  == "Horizontal Distance" && numItems == 2)
            {
                ui->horizontalDistance->setText(measurementValue);
                ui->uncrHorizontalDistance->setText(uncertaintyValue);
            }
            else if( measurementLabel  == "Azimuth [N]" && numItems == 4)
            {
                ui->azimD->setText(dataStr[0]);
                ui->azimM->setText(dataStr[1]);
                ui->azimS->setText(dataStr[2]);
                // Fix to bug issue256
                if(dataStr[3] == "-nanind")
                {
                   ui->uncrAzimuth->setText("N/A");
                }
                else
                {
                   ui->uncrAzimuth->setText(dataStr[3]);
                }
            }
            else if( measurementLabel  == "Vertical Angle" && numItems == 4)
            {
                ui->vangD->setText(dataStr[0]);
                ui->vangM->setText(dataStr[1]);
                ui->vangS->setText(dataStr[2]);
                ui->uncrVerticalAngle->setText(dataStr[3]);
            }
            else if( measurementLabel  == "Geodetic Height Difference" && numItems == 2)
            {
                ui->heightDifference->setText(measurementValue);
                ui->uncrHeightDifference->setText(uncertaintyValue);
            }
            else if( measurementLabel  == "Geocentric dX dY dZ" && numItems == 3)
            {
                ui->DX->setText(dataStr[0]);
                ui->DY->setText(dataStr[1]);
                ui->DZ->setText(dataStr[2]);
            }
            else if( measurementLabel == "Covariance Matrix" && numItems == 6)
            {
                ui->covarianceXX->setText(dataStr[0]);
                ui->covarianceXY->setText(dataStr[1]);
                ui->covarianceXZ->setText(dataStr[2]);
                ui->covarianceYY->setText(dataStr[3]);
                ui->covarianceYZ->setText(dataStr[4]);
                ui->covarianceZZ->setText(dataStr[5]);
            }
        }
    }
}

void StationDataDialog::clear()
{
    //clear combo box, put dash back in
    ui->comboBoxFrom->clear();
    ui->comboBoxTo->clear();

    //clear the label text
    ui->labelExportPath->clear();

    //call clearOutput
    clearOutput();

    fromToStations_Changed = false;

    //clear cached list of stations
    lsainverse.clear();
    stationList.clear();

    //internal tracking strings
    oldH5File.clear();
    oldFromStation.clear();
    oldToStation.clear();

    fromStation.clear();
    toStation.clear();

    // clear the Station Data Dialog View
    LSAH5File *h5file = NULL;
    QString hdf5FileName = "";
    bool apv = false;
    bool westLon = false;

    this->setup(h5file, hdf5FileName, apv, westLon);
}

void StationDataDialog::clearOutput()
{
    ui->fromLatD->clear();
    ui->fromLatM->clear();
    ui->fromLatS->clear();
    ui->fromLatDir->clear();
    ui->fromLonD->clear();
    ui->fromLonM->clear();
    ui->fromLonS->clear();
    ui->fromLonDir->clear();
    ui->fromEllipHt->clear();
    ui->fromOrthoHt->clear();
    ui->fromUnd->clear();
    ui->fromNSDefl->clear();
    ui->fromEWDefl->clear();
    ui->toLatD->clear();
    ui->toLatM->clear();
    ui->toLatS->clear();
    ui->toLatDir->clear();
    ui->toLonD->clear();
    ui->toLonM->clear();
    ui->toLonS->clear();
    ui->toLonDir->clear();
    ui->toEllipHt->clear();
    ui->toOrthoHt->clear();
    ui->toUnd->clear();
    ui->toNSDefl->clear();
    ui->toEWDefl->clear();
    ui->slantDistance->clear();
    ui->uncrSlantDistance->clear();
    ui->geodesicDistance->clear();
    ui->uncrGeoDistance->clear();
    ui->horizontalDistance->clear();
    ui->uncrHorizontalDistance->clear();
    ui->azimD->clear();
    ui->azimM->clear();
    ui->azimS->clear();
    ui->uncrAzimuth->clear();
    ui->vangD->clear();
    ui->vangM->clear();
    ui->vangS->clear();
    ui->uncrVerticalAngle->clear();
    ui->heightDifference->clear();
    ui->uncrHeightDifference->clear();
    ui->DX->clear();
    ui->DY->clear();
    ui->DZ->clear();
    ui->covarianceXX->clear();
    ui->covarianceXY->clear();
    ui->covarianceXZ->clear();
    ui->covarianceYY->clear();
    ui->covarianceYZ->clear();
    ui->covarianceZZ->clear();
}

// Gets the base file name.
// If a user defined inverse is utilized then stick with that file, otherwise default
// to h5 file.
QString StationDataDialog::giveFileBase() const
{
    QFileInfo chosenFile;

    // outFileRoot non-empty if set by user. Should only be possible
    // via Export Results As button

    !userDefInvPath.isEmpty() ? chosenFile.setFile(userDefInvPath) : chosenFile.setFile(h5FilePath);
    return  chosenFile.baseName();
}

std::string StationDataDialog::giveFileBaseString() const
{
    return giveFileBase().toStdString();
}

// Gets the base file path.
// If a user defined inverse is utilized then stick with that file, otherwise default
// to h5 file.
QString StationDataDialog::giveFileAbsPath() const
{
    QFileInfo chosenFile;

    // outFileRoot non-empty if set by user. Should only be possible
    // via Export Results As button

    !userDefInvPath.isEmpty() ? chosenFile.setFile(userDefInvPath) : chosenFile.setFile(h5FilePath);
    return  chosenFile.absolutePath();
}

std::string StationDataDialog::giveFileAbsPathString() const
{
    return giveFileAbsPath().toStdString();
}

void StationDataDialog::closeEvent(QCloseEvent *event)
{
    emit closing();
    event->accept();
}

void StationDataDialog::keyPressEvent(QKeyEvent *e) {
    if(e->key() == Qt::Key_Escape)
    {
        if(this->isVisible())
            this->close();
    }

    // Modified from this->hasFocus()
    else if(e->key() == Qt::Key_Delete && this->ui->stationPairsTable->hasFocus())
    {
        handleDeleteAction();
    }
}

void StationDataDialog::resizeEvent(QResizeEvent *event)
{
    if(initialized)
        saveWidgetSettings();

    QDialog::resizeEvent(event);
}

void StationDataDialog::moveEvent(QMoveEvent *event)
{
    if(initialized)
        saveWidgetSettings();

    QDialog::moveEvent(event);
}

void StationDataDialog::onCustomContextMenu(const QPoint &provPoint)
{
    QModelIndex index = ui->stationPairsTable->indexAt(provPoint);

    if(index.isValid())
    {
        rightClickMenu->exec(ui->stationPairsTable->mapToGlobal(provPoint));
    }

    rightClickMenu->close();
}

void StationDataDialog::handleTableStationPairsResized(int columnNumber, int oldWidth, int newWidth)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    switch(columnNumber)
    {
        case 0:
            settings.setValue(QSETTINGS_PAIRS_COLUMN_WIDTH_0, newWidth);
            break;
        case 1:
            settings.setValue(QSETTINGS_PAIRS_COLUMN_WIDTH_1, newWidth);
            break;
        default:
            break;
    }

    return;
}

void StationDataDialog::checkBtnAddStatus()
{
    bool enable = true;

    // Covers case where one of stations is not in HDF5 file
    // or from == to
    if(!validStations(fromStation, toStation))
    {
        enable = false;
    }

    ui->btnPairAdd->setEnabled(enable);
}

void StationDataDialog::setTableColumnWidths()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    int width0 = settings.value(QSETTINGS_PAIRS_COLUMN_WIDTH_0).toInt();
    int width1 = settings.value(QSETTINGS_PAIRS_COLUMN_WIDTH_1).toInt();

    int minWidth = (ui->stationPairsTable->width()) / 10;

    if(width0 < minWidth)
    {
        width0 = minWidth;
    }

    if(width1 < minWidth)
    {
        width1 = minWidth;
    }

    ui->stationPairsTable->setColumnWidth(0, width0);
    ui->stationPairsTable->setColumnWidth(1, width1);
}
