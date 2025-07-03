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
#include "AdjustedPositions.hpp"
#include "ui_AdjustedPositions.h"

const QString AdjustedPositions::QSETTINGS_ADJPTS_GEOMETRY = "QSETTINGS_ADJPTS_GEOMETRY";
const QString AdjustedPositions::QSETTINGS_ADJPTS_POSITION = "QSETTINGS_ADJPTS_POSITION";
const QString AdjustedPositions::QSETTINGS_ADJPTS_SIZE = "QSETTINGS_ADJPTS_SIZE";
const QString AdjustedPositions::QSETTINGS_ADJPTS_MAXIMIZED = "QSETTINGS_ADJPTS_MAXIMIZED";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_0 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_0";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_1 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_1";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_2 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_2";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_3 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_3";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_4 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_4";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_5 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_5";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_6 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_6";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_7 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_7";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_8 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_8";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_9 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_9";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_10 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_10";
const QString AdjustedPositions::QSETTINGS_ADJPTS_COLUMN_WIDTH_11 = "QSETTINGS_ADJPTS_COLUMN_WIDTH_11";
const int AdjustedPositions::NUM_TABLE_COLUMNS=12;//| Label | Lat | Lon | Eht | Oht | Sig N(m) | Sig E(m) | Sig U(m) | AdjN(m) | AdjE(m) | AdjU(m) | |Adj(m)| |

AdjustedPositions::AdjustedPositions(GuiModel *guiModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdjustedPositions)
{
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    pointsModel = NULL;
    suppressTableSelectionChanges = false;
    setGuiModel(guiModel);

    initialized = false;//this must come before setupUi
    ui->setupUi(this);

    addToProjectFixed = new QAction(ui->tableViewPoints);
    addToProjectFixed->setText(QString("Add to project as Fixed"));
    ui->tableViewPoints->addAction(addToProjectFixed);
    connect(addToProjectFixed, SIGNAL(triggered()), this, SLOT(handleAddToProjectFixed()));

    addToProjectFloat = new QAction(ui->tableViewPoints);
    addToProjectFloat->setText(QString("Add to project as Float"));
    ui->tableViewPoints->addAction(addToProjectFloat);
    connect(addToProjectFloat, SIGNAL(triggered()), this, SLOT(handleAddToProjectFloat()));

    addToProjectFloat->setVisible(false);
    addToProjectFixed->setVisible(false);

    ui->labelLegend->setText(QString("<b>Subject to least squares adjustment</b>, <font color=blue><b>Pass-through (fixed, unused, or derived)</b></font>"));

    loadAndApplyDockWidgetSettings();
    initialized = true;
}


AdjustedPositions::~AdjustedPositions()
{
    if(pointsModel != NULL)
        delete pointsModel;

    delete ui;
}

void AdjustedPositions::closeEvent(QCloseEvent *event)
{
    emit closing();
    event->accept();
}

void AdjustedPositions::keyPressEvent(QKeyEvent *e) {
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
    else
    {
        if(this->isVisible())
            this->close();
    }
}

void AdjustedPositions::updateView(const LSAH5File& hdf5File, double ellipseScaleFactor, GuiModel *gooeymodel, bool usingWestLon, bool includeUnused)
{
    // Disable exporting until a selection is made
    addToProjectFloat->setVisible(false);
    addToProjectFixed->setVisible(false);

    int numAdjustedPoints = 0, numFixedPoints = 0, numDerivedPoints = 0;

    points.clear();
    initial_points.clear();
    for(int i=0; i<hdf5File.llh->getSize(); i++)
    {
        points.push_back(hdf5File.llh->getDataPoint(i));
        initial_points.push_back(getInitialLlhPoint(hdf5File, i));
    }

    std::vector<adjustedNEUPos> deltaNEUs = getAdjustments();

    setGuiModel(gooeymodel);

    //update relevant labels
    ui->labelGeoidFileValue->setText(QString::fromStdString(hdf5File.projCfg->getDataPoint().geoFileName));
    ui->labelPercentConfidenceValue->setText(QString::fromStdString(hdf5File.projCfg->getDataPoint().confidence));

    setupTable();

    //update the table contents
    //| Label | Lat | Lon | Eht | Oht | Sig N(m) | Sig E(m) | Sig U(m) | AdjN(m) | AdjE(m) | AdjU(m) | |Adj(m)| |
    for(int i=0;i<points.size();i++)
    {
        //honor user's choice on the config panel to include unused points or not
        if(!points.at(i).utilized && !includeUnused)
            continue;

        double latDecDeg = points[i].lat;
        double lonDecDeg = points[i].lon;
        std::stringstream latSS;
        std::stringstream lonSS;
        QString latString;
        QString lonString;

        // Convert latDecDeg to DMS
        if(latDecDeg < 0.0)
        {
            if(::fabs(latDecDeg)<lsa::ZERO_BOUND)
                latDecDeg = 0.0;
            else
            {
                latDecDeg = -latDecDeg;//convert from north lat to south lat
            }
            latSS << "S ";
        }
        else
        {
            latSS << "N ";
        }
        bool latDMSIsNeg;
        int dmsLatDegrees = 0;
        int dmsLatMinutes = 0;
        double dmsLatSeconds = 0.0;
        degToDMS(latDecDeg, latDMSIsNeg, dmsLatDegrees, dmsLatMinutes, dmsLatSeconds);
        if(dmsLatSeconds > 59.99999) dmsLatSeconds = 59.99999;//fix to Bug #1301
        latSS << (latDMSIsNeg ?  "-" : "") << dmsLatDegrees << " " << dmsLatMinutes << " " <<
                 std::fixed << std::setprecision(getAngularPositionPrecisionSOA()) << dmsLatSeconds;
        latString = QString::fromStdString(latSS.str());

        // Convert lonDecDeg to DMS
        if(usingWestLon)
        {
            if(::fabs(lonDecDeg)<lsa::ZERO_BOUND)//fix to Bug #1301
                lonDecDeg = 0.0;
            else
            {
                lonDecDeg = 360.0 - lonDecDeg;//convert from east lon to west lon
                if(lonDecDeg > 360.0) lonDecDeg -= 360.0;
            }

            // Convert lonDecDeg to DMS
            bool lonDMSIsNeg;
            int dmsLonDegrees = 0;
            int dmsLonMinutes = 0;
            double dmsLonSeconds = 0.0;
            degToDMS(lonDecDeg, lonDMSIsNeg, dmsLonDegrees, dmsLonMinutes, dmsLonSeconds);
            if(dmsLonSeconds > 59.99999) dmsLonSeconds = 59.99999;//fix to Bug #1301
            lonSS << "W " << (lonDMSIsNeg ?  "-" : "") << dmsLonDegrees << " " << dmsLonMinutes << " " <<
                     std::fixed << std::setprecision(getAngularPositionPrecisionSOA()) << dmsLonSeconds;
            lonString = QString::fromStdString(lonSS.str());
        }
        else
        {
            // Convert lonDecDeg to DMS
            bool lonDMSIsNeg;
            int dmsLonDegrees = 0;
            int dmsLonMinutes = 0;
            double dmsLonSeconds = 0.0;
            degToDMS(lonDecDeg, lonDMSIsNeg, dmsLonDegrees, dmsLonMinutes, dmsLonSeconds);
            lonSS << "E " << (lonDMSIsNeg ?  "-" : "") << dmsLonDegrees << " " << dmsLonMinutes << " " << std::fixed << std::setprecision(getAngularPositionPrecisionSOA()) << dmsLonSeconds;
            lonString = QString::fromStdString(lonSS.str());
        }

        QStandardItem *label = new QStandardItem(QString::fromStdString(points[i].label));
        QStandardItem *lat = new QStandardItem(latString);
        QStandardItem *lon = new QStandardItem(lonString);
        QStandardItem *eHt = new QStandardItem();
        QStandardItem *oHt = new QStandardItem();
        QStandardItem *sigN = new QStandardItem();
        QStandardItem *sigE = new QStandardItem();
        QStandardItem *sigU = new QStandardItem();
        QStandardItem *AdjN = new QStandardItem();
        QStandardItem *AdjE = new QStandardItem();
        QStandardItem *AdjU = new QStandardItem();
        QStandardItem *AdjMag = new QStandardItem();

        //assign values to the items
        eHt->setData(points[i].ht, Qt::DisplayRole);
        oHt->setData(points[i].oht, Qt::DisplayRole);
        sigN->setData(points[i].sigN*ellipseScaleFactor, Qt::DisplayRole);
        sigE->setData(points[i].sigE*ellipseScaleFactor, Qt::DisplayRole);
        sigU->setData(points[i].sigU*ellipseScaleFactor, Qt::DisplayRole);
        AdjN->setData(deltaNEUs.at(i).n, Qt::DisplayRole);
        AdjE->setData(deltaNEUs.at(i).e, Qt::DisplayRole);
        AdjU->setData(deltaNEUs.at(i).u, Qt::DisplayRole);
        AdjMag->setData(deltaNEUs.at(i).magnitude, Qt::DisplayRole);

        //do right alignment for numbers
        eHt->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        oHt->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        sigN->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        sigE->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        sigU->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        AdjN->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        AdjE->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        AdjU->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);
        AdjMag->setData(Qt::AlignRight + Qt::AlignVCenter,Qt::TextAlignmentRole);

        if(points.at(i).utilized)
        {
            if(points.at(i).sourceType == DERIVED_MEAN || points.at(i).sourceType == DERIVED_ENUO)//derived points are blue
            {
                label->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                lat->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                lon->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                eHt->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                oHt->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                sigN->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                sigE->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                sigU->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjN->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjE->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjU->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjMag->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                numDerivedPoints++;
            }
            else if(points.at(i).fixedType == LSAFixedState::FIXED)//fixed points are blue
            {
                label->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                lat->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                lon->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                eHt->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                oHt->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                sigN->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                sigE->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                sigU->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjN->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjE->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjU->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                AdjMag->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
                numFixedPoints++;
            }
            else//adjusted points are black
            {
                numAdjustedPoints++;
            }
        }
        else//unused points are blue
        {
            label->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            lat->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            lon->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            eHt->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            oHt->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            sigN->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            sigE->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            sigU->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            AdjN->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            AdjE->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            AdjU->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            AdjMag->setData(QVariant(QColor(Qt::blue)),Qt::ForegroundRole);
            numFixedPoints++;
        }

        QList<QStandardItem*> newRow;
        newRow << label << lat << lon << eHt << oHt << sigN << sigE << sigU << AdjN << AdjE << AdjU << AdjMag;
        pointsModel->appendRow(newRow);
    }

    //perform string formatting on the numerical items
    ui->tableViewPoints->setItemDelegateForColumn(3,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));
    ui->tableViewPoints->setItemDelegateForColumn(4,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));
    ui->tableViewPoints->setItemDelegateForColumn(5,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));
    ui->tableViewPoints->setItemDelegateForColumn(6,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));
    ui->tableViewPoints->setItemDelegateForColumn(7,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));
    ui->tableViewPoints->setItemDelegateForColumn(8,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));
    ui->tableViewPoints->setItemDelegateForColumn(9,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));
    ui->tableViewPoints->setItemDelegateForColumn(10,new NumberFormatDelegate(this,getLinearPositionPrecisionMeters()));

    //initial display sorted by label
    ui->tableViewPoints->sortByColumn(0,Qt::AscendingOrder);
    ui->tableViewPoints->horizontalHeader()->setSortIndicator(0,Qt::AscendingOrder);
    //resizeEvent(NULL);//added because issue with TopResid table (see above)
    setTableColumnWidths();

    //update relevant label - Number of Points: Adjusted(36) + Fixed/Unused(3) + Derived(2) = 41
    QString numPointsValue = QString("Adjusted(") + QString::number(numAdjustedPoints) + QString(") + ") +
                             QString("Fixed/Unused(") + QString::number(numFixedPoints) +  QString(") + ") +
                             QString("Derived(") + QString::number(numDerivedPoints) + QString(") = ") +
                             QString::number(numAdjustedPoints + numFixedPoints + numDerivedPoints);
    ui->labelNumPointsValue->setText(numPointsValue);
}

llhExternal AdjustedPositions::getInitialLlhPoint(const LSAH5File& hdf5File, int pointsIndex)
{
    std::string pointLabel = hdf5File.llh->getDataPoint(pointsIndex).label;
    if((pointsIndex < hdf5File.initialLlh->getSize()) &&
       (pointLabel == hdf5File.initialLlh->getDataPoint(pointsIndex).label))//most likely
    {
        return hdf5File.initialLlh->getDataPoint(pointsIndex);
    }
    else
    {
        //search initial llh entries for the corresponding point
        for(int i=0; i<hdf5File.initialLlh->getSize(); i++)
        {
            if(pointLabel == hdf5File.initialLlh->getDataPoint(i).label)
                return hdf5File.initialLlh->getDataPoint(i);
        }
        //no match found, must be derived point. Want zeros for adjN, adjE, adjU.
        return hdf5File.llh->getDataPoint(pointsIndex);
    }
}

std::vector<adjustedNEUPos> AdjustedPositions::getAdjustments()
{
    std::vector<adjustedNEUPos> deltaNEUs;
    deltaNEUs.clear();

    for(int i=0; i<points.size(); i++)
    {
        gnsstk::Position initialLLHPosition(initial_points[i].lat, initial_points[i].lon, initial_points[i].ht,gnsstk::Position::Geodetic);
        gnsstk::Position finalLLHPosition(points[i].lat,points[i].lon,points[i].ht,gnsstk::Position::Geodetic);
        gnsstk::Position initialXYZPosition = initialLLHPosition.asECEF();
        gnsstk::Position finalXYZPosition = finalLLHPosition.asECEF();
        gnsstk::Vector<double> deltaXYZ(3);
        deltaXYZ(0) = finalXYZPosition.X() - initialXYZPosition.X();
        deltaXYZ(1) = finalXYZPosition.Y() - initialXYZPosition.Y();
        deltaXYZ(2) = finalXYZPosition.Z() - initialXYZPosition.Z();
        gnsstk::Matrix<double> rot = northEastUpGeodetic(finalXYZPosition); //ECEF2ENU
        gnsstk::Vector<double> deltaNEU = rot * deltaXYZ;
        adjustedNEUPos adjNEU;
        adjNEU.n = deltaNEU(0);
        adjNEU.e = deltaNEU(1);
        adjNEU.u = deltaNEU(2);
        adjNEU.magnitude = sqrt(deltaXYZ(0)*deltaXYZ(0) + deltaXYZ(1)*deltaXYZ(1) + deltaXYZ(2)*deltaXYZ(2));
        deltaNEUs.push_back(adjNEU);
    }

    return deltaNEUs;
}

void AdjustedPositions::setupTable()
{
    if(pointsModel != NULL)
    {
        delete pointsModel;
        pointsModel = NULL;
    }

    pointsModel = new QStandardItemModel(0,NUM_TABLE_COLUMNS,this);
    ui->tableViewPoints->setModel(pointsModel);

    setTableColumnWidths();
    addTableHeaders();
    ui->tableViewPoints->verticalHeader()->setVisible(false);//hide row numbers

    // Connect signals for resizing the table columns
    connect(ui->tableViewPoints->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(handleTableViewPointsColumnResized(int,int,int)) );

    // Connect signals to do stuff upon selection
    connect(ui->tableViewPoints->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(handleTableViewPointsSelectionChanged(QItemSelection, QItemSelection)));
}

void AdjustedPositions::addTableHeaders()
{
//| Label | Lat | Lon | Eht | Oht | Sig N(m) | Sig E(m) | Sig U(m) | AdjN(m) | AdjE(m) | AdjU(m) |
    pointsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Label")));
    pointsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Lat")));
    pointsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Lon")));
    pointsModel->setHorizontalHeaderItem(3, new QStandardItem(QString("EHt(m)")));
    pointsModel->setHorizontalHeaderItem(4, new QStandardItem(QString("OHt(m)")));
    pointsModel->setHorizontalHeaderItem(5, new QStandardItem(QString("Sig N(m)")));
    pointsModel->setHorizontalHeaderItem(6, new QStandardItem(QString("Sig E(m)")));
    pointsModel->setHorizontalHeaderItem(7, new QStandardItem(QString("Sig U(m)")));
    pointsModel->setHorizontalHeaderItem(8, new QStandardItem(QString("AdjN(m)")));
    pointsModel->setHorizontalHeaderItem(9, new QStandardItem(QString("AdjE(m)")));
    pointsModel->setHorizontalHeaderItem(10, new QStandardItem(QString("AdjU(m)")));
    pointsModel->setHorizontalHeaderItem(11, new QStandardItem(QString("|Adj|(m)")));
}

void AdjustedPositions::setTableColumnWidths()
{
    // Set column widths to previous values
    ui->tableViewPoints->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    int Width0 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_0, 75).toInt();
    int Width1 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_1, 75).toInt();
    int Width2 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_2, 75).toInt();
    int Width3 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_3, 75).toInt();
    int Width4 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_4, 75).toInt();
    int Width5 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_5, 75).toInt();
    int Width6 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_6, 75).toInt();
    int Width7 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_7, 75).toInt();
    int Width8 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_8, 75).toInt();
    int Width9 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_9, 75).toInt();
    int Width10 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_10, 75).toInt();
    int Width11 = settings.value(QSETTINGS_ADJPTS_COLUMN_WIDTH_11, 75).toInt();

    ui->tableViewPoints->setColumnWidth(0, Width0);
    ui->tableViewPoints->setColumnWidth(1, Width1);
    ui->tableViewPoints->setColumnWidth(2, Width2);
    ui->tableViewPoints->setColumnWidth(3, Width3);
    ui->tableViewPoints->setColumnWidth(4, Width4);
    ui->tableViewPoints->setColumnWidth(5, Width5);
    ui->tableViewPoints->setColumnWidth(6, Width6);
    ui->tableViewPoints->setColumnWidth(7, Width7);
    ui->tableViewPoints->setColumnWidth(8, Width8);
    ui->tableViewPoints->setColumnWidth(9, Width9);
    ui->tableViewPoints->setColumnWidth(10, Width10);
    ui->tableViewPoints->setColumnWidth(11, Width11);
}

void AdjustedPositions::handleTableViewPointsColumnResized(int columnNumber, int oldWidth, int newWidth)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    switch (columnNumber)
    {
    case 0:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_0, newWidth);
        break;
    case 1:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_1, newWidth);
        break;
    case 2:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_2, newWidth);
        break;
    case 3:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_3, newWidth);
        break;
    case 4:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_4, newWidth);
        break;
    case 5:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_5, newWidth);
        break;
    case 6:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_6, newWidth);
        break;
    case 7:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_7, newWidth);
        break;
    case 8:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_8, newWidth);
        break;
    case 9:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_9, newWidth);
        break;
    case 10:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_10, newWidth);
        break;
    case 11:
        settings.setValue(QSETTINGS_ADJPTS_COLUMN_WIDTH_11, newWidth);
        break;
    default:
        break;
    }
}

void AdjustedPositions::handleTableViewPointsSelectionChanged(QItemSelection, QItemSelection)
{
    if(!suppressTableSelectionChanges)
    {
        QModelIndexList selectedList = ui->tableViewPoints->selectionModel()->selectedRows();
        // Enable adding points to project
        bool selectionMade = selectedList.size() > 0 ? true : false;
        addToProjectFloat->setVisible(selectionMade);
        addToProjectFixed->setVisible(selectionMade);

        QModelIndexList treeViewIndices;
        for(int i=0; i<selectedList.count(); i++)
        {
            QModelIndex index = selectedList.at(i);
            int row = index.row();
            std::string label = index.sibling(row,0).data().toString().toStdString();
            if(label.empty())
                continue;//no point in the row
            QModelIndex currentIndex = guiModel->getIndexFromPointLabel(label);
            if(currentIndex.isValid())
                treeViewIndices << currentIndex;
        }

        if(treeViewIndices.size()>0)
        {
            guiModel->select(treeViewIndices);
        }
    }
}

void AdjustedPositions::updateSelection(QModelIndexList pointIndexes)
{
    if(pointsModel == NULL) return;

    suppressTableSelectionChanges=true;
    {
        //select the rows
        bool first_selection = true;
        int numRows = ui->tableViewPoints->model()->rowCount();
        int row;
        foreach(QModelIndex index, pointIndexes)
        {
            QString pointLabel = QString::fromStdString(guiModel->getLSARecord(index)->getLabel());
            if(pointLabel.isEmpty())
                continue;
            for(row=0;row<numRows;row++)
            {
                QString label = ui->tableViewPoints->model()->data(ui->tableViewPoints->model()->index(row,0)).toString();
                if(label.isEmpty())
                    continue;
                if(pointLabel == label)
                {
                    QModelIndex topLeft = ui->tableViewPoints->model()->index(row,0);
                    QModelIndex bottomRight = ui->tableViewPoints->model()->index(row,NUM_TABLE_COLUMNS-1);
                    QItemSelection selection(topLeft,bottomRight);
                    if(first_selection)
                    {
                        ui->tableViewPoints->selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
                        first_selection = false;
                    }
                    else
                    {
                        ui->tableViewPoints->selectionModel()->select(selection,QItemSelectionModel::Select);
                    }
                    break;
                }
            }
        }

        //scroll to item in the table
        QModelIndexList selectedRows = ui->tableViewPoints->selectionModel()->selectedIndexes();
        if(selectedRows.size()>0)
        {
            QModelIndex topIndex = selectedRows.at(0);
            int topRow = topIndex.row();
            foreach (QModelIndex index, selectedRows)
            {
                if(index.row()>topRow)
                {
                    topIndex = index;
                    topRow = topIndex.row();
                }
            }
            QWidget *focusWidget = QApplication::focusWidget();
            if(focusWidget != dynamic_cast<QWidget*>(ui->tableViewPoints))
            {
                ui->tableViewPoints->scrollTo(topIndex,QAbstractItemView::PositionAtCenter);
            }

            ui->tableViewPoints->selectionModel()->setCurrentIndex(selectedRows.first(), QItemSelectionModel::Select);
        }
    }
    suppressTableSelectionChanges=false;
}

void AdjustedPositions::handleAddToProjectFixed()
{
    handleAddToProject(false);
}

void AdjustedPositions::handleAddToProjectFloat()
{
    handleAddToProject(true);
}

void AdjustedPositions::handleAddToProject(bool addAsFloat)
{
    if(points.size() == 0)
        return;

    //get the selected rows
    QModelIndexList selectedList = ui->tableViewPoints->selectionModel()->selectedRows();

    if(selectedList.count() == 0)//fix to Bug #1305
        return;

    bool checkDuplicates = false; // Look for exception in loop below to see if at least one
    // point is not an autogenerated point.
    QList<std::string> POSGstrings;
    for(int i=0; i<selectedList.count(); i++)
    {
        QModelIndex index = selectedList.at(i);
        int row = index.row();
        std::string label = index.sibling(row,0).data().toString().toStdString();
        std::string latDMS = index.sibling(row,1).data().toString().toStdString();
        std::string lonDMS = index.sibling(row,2).data().toString().toStdString();
        double height = index.sibling(row,3).data().toDouble();

        QStringList latitude = QString::fromStdString(latDMS).split(" ");
        std::string latD = latitude[1].toStdString();
        std::string latM = latitude[2].toStdString();
        std::string latS = latitude[3].toStdString();
        QStringList longitude = QString::fromStdString(lonDMS).split(" ");
        std::string lonD = longitude[1].toStdString();
        std::string lonM = longitude[2].toStdString();
        std::string lonS = longitude[3].toStdString();
        std::string lonDir = longitude[0].toStdString();

        std::stringstream POSGstrm;

        if(addAsFloat)
        {
            POSGstrm << "POSG " << addQuotes(label) << " Flt "
                                << latD << " " << latM << " " << latS << " N "
                                << lonD << " " << lonM << " " << lonS << " " << lonDir << " "
                                << std::fixed << std::setprecision(getLinearPositionPrecisionMeters()) << height << " m";
        }
        else
        {
            POSGstrm << "POSG " << addQuotes(label) << " Fix "
                                << latD << " " << latM << " " << latS << " N "
                                << lonD << " " << lonM << " " << lonS << " " << lonDir << " "
                                << std::fixed << std::setprecision(getLinearPositionPrecisionMeters()) << height << " m";
        }

        std::string POSGString = POSGstrm.str();

        POSGstrings.append(POSGString);
        if(guiModel->getIndexFromAutogenPointLabel(label) == QModelIndex())
        {
            checkDuplicates = true;
        }
    }
    //insert those into the GUI
    guiModel->addAdjustedPointsToProject(POSGstrings, checkDuplicates);
}

void AdjustedPositions::setGuiModel(GuiModel *gooeymodel)
{
    guiModel = gooeymodel;
}

void AdjustedPositions::clear()
{
    //update relevant labels
    ui->labelGeoidFileValue->setText("");
    ui->labelPercentConfidenceValue->setText("");
    ui->labelNumPointsValue->setText("");

    setupTable();

    points.clear();
    initial_points.clear();
}

void AdjustedPositions::resizeEvent(QResizeEvent *event)
{
    if(initialized)
        saveDockWidgetSettings();

    QDialog::resizeEvent(event);
}

void AdjustedPositions::moveEvent(QMoveEvent *event)
{
    if(initialized)
        saveDockWidgetSettings();

    QDialog::moveEvent(event);
}

void AdjustedPositions::saveDockWidgetSettings()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    qsettings.setValue( QSETTINGS_ADJPTS_GEOMETRY, saveGeometry() );
    qsettings.setValue( QSETTINGS_ADJPTS_MAXIMIZED, isMaximized() );
    if ( !isMaximized() )
    {
        qsettings.setValue( QSETTINGS_ADJPTS_POSITION, pos() );
        QSize debugSize = size();
        qsettings.setValue( QSETTINGS_ADJPTS_SIZE, size() );
    }

}

void AdjustedPositions::loadAndApplyDockWidgetSettings()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    restoreGeometry(qsettings.value( QSETTINGS_ADJPTS_GEOMETRY, saveGeometry() ).toByteArray());
    move(qsettings.value( QSETTINGS_ADJPTS_POSITION, pos() ).toPoint());
    QSize debugSize = qsettings.value( QSETTINGS_ADJPTS_SIZE, size() ).toSize();
    resize(debugSize);
    if ( qsettings.value( QSETTINGS_ADJPTS_MAXIMIZED, isMaximized() ).toBool() )
    {
        showMaximized();
    }

    return;
}

QString AdjustedPositions::getGeoid()
{
    QFileInfo file(ui->labelGeoidFileValue->text());

    return file.fileName();
}

llhExternal AdjustedPositions::getPoint(QString labelToFind)
{
    for (int i = 0; i < points.size(); ++i)
    {
        if (points[i].label == labelToFind.toStdString())
        {
            return points[i];
        }
    }
    llhExternal dummy;
    return dummy;
}
