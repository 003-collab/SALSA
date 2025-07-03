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
#include "HistogramDialog.hpp"
#include "ui_histogramdialog.h"

#include <QFileDialog>

//TODO - Statistical summary

//TODO - Grouping by include file


const QString HistogramDialog::QSETTINGS_HISTOGRAM_SPLITTER_SIZE_0 = "QSETTINGS_HISTOGRAM_SPLITTER_SIZE_0";
const QString HistogramDialog::QSETTINGS_HISTOGRAM_SPLITTER_SIZE_1 = "QSETTINGS_HISTOGRAM_SPLITTER_SIZE_1";
const QString HistogramDialog::QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_0 = "QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_0";
const QString HistogramDialog::QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_1 = "QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_1";
const QString HistogramDialog::QSETTINGS_HISTOGRAM_GEOMETRY = "QSETTINGS_HISTOGRAM_GEOMETRY";
const QString HistogramDialog::QSETTINGS_HISTOGRAM_MAXIMIZED = "QSETTINGS_HISTOGRAM_MAXIMIZED";
const QString HistogramDialog::QSETTINGS_HISTOGRAM_POSITION = "QSETTINGS_HISTOGRAM_POSITION";
const QString HistogramDialog::QSETTINGS_HISTOGRAM_SIZE = "QSETTINGS_HISTOGRAM_SIZE";
const double HistogramDialog::DEFAULT_HIST_WIDTH = 6.0;
const int HistogramDialog::DEFAULT_NUM_BINS = 10;

HistogramDialog::HistogramDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::HistogramDialog)
{
    initialized = false;
    ui->setupUi(this);

    connect(ui->pushButton_Close,     SIGNAL(clicked()),         this, SLOT(onCloseButtonPressed())    );
    connect(ui->pushButton_Save,      SIGNAL(clicked()),         this, SLOT(onSaveButtonPressed())     );
    connect(ui->pushButton_ResetView, SIGNAL(clicked()),         this, SLOT(onResetViewButtonPressed()));
    connect(ui->spinBox_NumBins,      SIGNAL(valueChanged(int)), this, SLOT(onNumBinsChanged())        );
    connect(ui->checkBox_AZIM,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_DIST,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_DXYZ,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_HANG,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_HDIF,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_HDIR,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_PPP,         SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_VANG,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_ZANG,        SIGNAL(toggled(bool)),     this, SLOT(onIncludedMeasChanged())   );
    connect(ui->checkBox_ShowGrid,    SIGNAL(toggled(bool)),     this, SLOT(onShowGridChanged(bool))   );

    connect(ui->treeFilesToPlot,      SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(onIncludedFileChanged(QTreeWidgetItem*, int)) );

    connect(ui->splitter,                  SIGNAL(splitterMoved(int,int)),      this, SLOT(handleSplitterMoved(int,int)) );
    connect(ui->treeFilesToPlot->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(handleTreeViewColumnWidthChange(int,int,int)) );

    numBins = DEFAULT_NUM_BINS;

    // Make bin list widgets look like they are read only
//    ui->listFilesInBin->setEnabled(false);
    QPalette palette = ui->listFilesInBin->palette();
    QColor color = palette.color( QPalette::Disabled, QPalette::Base );
    palette.setColor( QPalette::Normal, QPalette::Base, color );
    ui->listFilesInBin->setPalette( palette );

    ui->listFilesInBin->setStyleSheet("background-color: lightGray;");

    // Set splitter positions
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    QList<int> splitterSizes;
    splitterSizes << settings.value(QSETTINGS_HISTOGRAM_SPLITTER_SIZE_0,350).toInt();
    splitterSizes << settings.value(QSETTINGS_HISTOGRAM_SPLITTER_SIZE_1,100).toInt();
    ui->splitter->setSizes(splitterSizes);

    // TreeView sizes
    int columnWidth0 = settings.value(QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_0, 200).toInt();
    int columnWidth1 = settings.value(QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_1, 50).toInt();
    ui->treeFilesToPlot->setColumnWidth(0, columnWidth0);
    ui->treeFilesToPlot->setColumnWidth(1, columnWidth1);

    //restore previous position and size
    loadAndApplyWidgetSettings();

    QString title("Measurement Standard Residuals");

    ui->qwtPlot_Histogram->setTitle(title);
    ui->qwtPlot_Histogram->setAxisTitle(QwtPlot::xBottom, "Standard Residuals");
    ui->qwtPlot_Histogram->setAxisTitle(QwtPlot::yLeft, "Counts");
    ui->qwtPlot_Histogram->setCanvasBackground(Qt::white);
    ui->qwtPlot_Histogram->setMouseTracking(true);
    ui->qwtPlot_Histogram->canvas()->setMouseTracking(true);
    ui->qwtPlot_Histogram->canvas()->installEventFilter(this);

    histPlot = NULL;
    panner = NULL;
    magnifier = NULL;
    grid = NULL;
    guiModel = NULL;
    histogramHasData = false;
    initialized = true;
}

void HistogramDialog::setupHistogram(const LSAH5File& h5File, GuiModel *inputGuiModel, const QString projectFile)
{
    //Bug 1443 & 1524
    if(inputGuiModel == NULL || projectFile == "")
        return;

    std::vector<measurementsExternal> measurements;
    for (int i = 0; i < h5File.measurements->getSize(); ++i)
    {
        measurements.push_back(h5File.measurements->getDataPoint(i));
    }

    if(measurements.size()>0)
    {
        histogramHasData = true;
    }

    projMeas = measurements;
    guiModel = inputGuiModel;
    projFile = projectFile;
//    regenerateModelData(); was seeing some ghosting of prior histograms when kept open with multiple adjustments
    tagToIndexMap = guiModel->inputOutputMap;

    populateParentIncludeMap();//must come before findMinMaxStdResiduals() -- Bug #1239
    findMaxMinStdResiduals();
    synchFileTreeToMap();

    if(panner != NULL)
    {
        delete panner;
        panner = NULL;
    }

    if(magnifier != NULL)
    {
        delete magnifier;
        magnifier = NULL;
    }

    //assign plot panner
    panner = new QwtPlotPanner(ui->qwtPlot_Histogram->canvas());
    //this prevents vertical panning, but also prevents the axis rescaling when zooming. :(
    //panner->setAxisEnabled(ui->qwtPlot_Histogram->yLeft,false);

    // Create and assign magnifier if not alreay done.
    magnifier = new QwtPlotMagnifier(ui->qwtPlot_Histogram->canvas());
    // Magnifier scaling is defined by wheelFactor^(turnedRadians/120) in QwtMagnifier::widgetWheelEvent (QwtPlotMagnifier parent)
    // The default wheelFactor of the QwtMagnifier is .9, so make 1.1 to invert the zoom behaviour.
    magnifier->setWheelFactor(1.1);

    if(populateBinsFromHDF5File())
    {
        setSigmaLabels();
        setMaxMinNumBins();
        resetView();//Fix to Bugs 1428, 1437
    }

    disableFiltersForAbsentMeasurements();    
}

void HistogramDialog::clearHistogram()
{
    //This clears the plot
    resetView(true);

    ui->treeFilesToPlot->clear();
    ui->listFilesInBin->clear();

    ui->spinBox_NumBins->setValue(DEFAULT_NUM_BINS);

    //Bug 1443
    projMeas.clear();
    tagToIndexMap.clear();
    parentIncludeMap.clear();
    parentSigmaMap.clear();
    guiModel = NULL;
    projFile = QString("");

    histogramHasData = false;

    ui->checkBox_AZIM->setEnabled(false);
    ui->checkBox_DIST->setEnabled(false);
    ui->checkBox_DXYZ->setEnabled(false);
    ui->checkBox_HANG->setEnabled(false);
    ui->checkBox_HDIF->setEnabled(false);
    ui->checkBox_HDIR->setEnabled(false);
    ui->checkBox_PPP->setEnabled(false);
    ui->checkBox_VANG->setEnabled(false);
    ui->checkBox_ZANG->setEnabled(false);

    ui->checkBox_AZIM->setChecked(true);
    ui->checkBox_DIST->setChecked(true);
    ui->checkBox_DXYZ->setChecked(true);
    ui->checkBox_HANG->setChecked(true);
    ui->checkBox_HDIF->setChecked(true);
    ui->checkBox_HDIR->setChecked(true);
    ui->checkBox_PPP->setChecked(true);
    ui->checkBox_VANG->setChecked(true);
    ui->checkBox_ZANG->setChecked(true);

    ui->checkBox_AZIM->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_DIST->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_DXYZ->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_HANG->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_HDIF->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_HDIR->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_PPP->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_VANG->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_ZANG->setStyleSheet("color: gray; background-color: light gray;");

    ui->label_AZIMsigma->setText(QString("--"));
    ui->label_DISTsigma->setText(QString("--"));
    ui->label_DXYZsigma->setText(QString("--"));
    ui->label_HANGsigma->setText(QString("--"));
    ui->label_HDIFsigma->setText(QString("--"));
    ui->label_HDIRsigma->setText(QString("--"));
    ui->label_PPPsigma->setText(QString("--"));
    ui->label_VANGsigma->setText(QString("--"));
    ui->label_ZANGsigma->setText(QString("--"));

    ui->label_AZIMsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_DISTsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_DXYZsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_HANGsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_HDIFsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_HDIRsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_PPPsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_VANGsigma->setStyleSheet("color: gray; background-color: light gray;");
    ui->label_ZANGsigma->setStyleSheet("color: gray; background-color: light gray;");
}

HistogramDialog::~HistogramDialog()
{
    if(histPlot != NULL) delete histPlot;
    if(panner != NULL) delete panner;
    if(magnifier != NULL) delete magnifier;
    if(grid != NULL) delete grid;

    delete ui;
}

void HistogramDialog::onCloseButtonPressed()
{
    this->close();
}

void HistogramDialog::populateParentIncludeMap()
{
    //Fix to Bug #1419
    parentIncludeMap.clear();

    for(int i=0; i<projMeas.size(); i++)
    {
        // Get the index for the measurement and its parent include
        std::string tag = projMeas[i].tag;
        QModelIndex measurementIndex = guiModel->getIndexFromTag(tag);
        QModelIndex parentIndex = measurementIndex.parent();

        while (parentIndex.isValid())
        {
            LSARecord *parentRecord = guiModel->getLSARecord(parentIndex);
            if (parentRecord->getRecType() == LSAType::INCLUDE)
            {
                parentIncludeMap.insert(parentIndex, Qt::Checked);
            }
            parentIndex = parentIndex.parent();
        }
    }
}

void HistogramDialog::populateParentSigmaMap()
{
    // Sum up the standard residual for every member in each include
    QMap<QModelIndex, double> meanMap;
    QMap<QModelIndex, int>    countMap;
    for(int i=0; i<projMeas.size(); i++)
    {
        if(isIncludedMeasurement(projMeas[i].measType) && isChildOfActiveInclude(projMeas[i].tag))//apply filters
        {
            // Get the index for the measurement and its parent include
            std::string tag = projMeas[i].tag;
            QModelIndex measurementIndex = guiModel->getIndexFromTag(tag);
            QModelIndex parentIndex = measurementIndex.parent();

            double standardResidual = projMeas[i].stdResid;

            // Add the standard deviation for each record to all of its parent record mean values
            while (parentIndex.isValid())
            {
                LSARecord *parentRecord = guiModel->getLSARecord(parentIndex);
                if (parentRecord->getRecType() == LSAType::INCLUDE)
                {
                    double previousResidualSum = meanMap.take(parentIndex);
                    double newResidualSum = previousResidualSum + standardResidual;
                    meanMap.insert(parentIndex, newResidualSum);

                    double previousCount = countMap.take(parentIndex);
                    double newCount = previousCount + 1;
                    countMap.insert(parentIndex, newCount);
                }
                parentIndex = parentIndex.parent();
            }
        }
    }

    // convert the sums to means
    QModelIndexList indexes = meanMap.keys();
    foreach (QModelIndex index, indexes)
    {
        double sum = meanMap.take(index);
        int counts = countMap.value(index);
        double meanValue = sum/(double)counts;
        meanMap.insert(index, meanValue);
    }

    // calculate the sum of the square of the differences for file
    QMap<QModelIndex, double> sigmaMap;
    for(int i=0; i<projMeas.size(); i++)
    {
        if(isIncludedMeasurement(projMeas[i].measType) && isChildOfActiveInclude(projMeas[i].tag))//apply filters
        {
            // Get the index for the measurement and its parent include
            std::string tag = projMeas[i].tag;
            QModelIndex measurementIndex = guiModel->getIndexFromTag(tag);
            QModelIndex parentIndex = measurementIndex.parent();

            double standardResidual = projMeas[i].stdResid;

            // Update each parent
            while (parentIndex.isValid())
            {
                LSARecord *parentRecord = guiModel->getLSARecord(parentIndex);
                if (parentRecord->getRecType() == LSAType::INCLUDE)
                {
                    double previousSigma = sigmaMap.take(parentIndex);
                    double meanValue = meanMap.value(parentIndex);
                    double newSigma = previousSigma + pow(meanValue - standardResidual, 2);
                    sigmaMap.insert(parentIndex, newSigma);
                }
                parentIndex = parentIndex.parent();
            }
        }
    }

    // convert the sums to std dev and insert into parentSigmaMap
    parentSigmaMap.clear();
    QModelIndexList indexList = parentIncludeMap.keys();
    foreach (QModelIndex index, indexList)
    {
        double variance = sigmaMap.value(index);
        int counts = countMap.value(index);
        if (counts > 1)
        {
            double sigmaValue = sqrt(variance/(counts-1));
            int numSigmaDigits = 2;
            QString sigmaString = QString::number(sigmaValue, 'f', numSigmaDigits);
            parentSigmaMap.insert(index, sigmaString);
        }
    }
}

bool HistogramDialog::populateBinsFromHDF5File()
{
    if(projMeas.size() < 1 || !histogramHasData)
        return false;

    //define the bins
    double binWidth = std::fabs(stdResMax - stdResMin)/(double)numBins;
    if(binWidth < 1.0e-9)//Fix to Bug #1119
    {
        stdResMax = DEFAULT_HIST_WIDTH/2.0;
        stdResMin = -DEFAULT_HIST_WIDTH/2.0;
        binWidth = (stdResMax - stdResMin)/(double)numBins;
    }
    binWalls.clear();
    for(int i=0; i<=numBins; i++)
    {
        binWalls.push_back(stdResMin + (double)i*binWidth);
    }

    //initialize the bins
    countsPerBin.clear();
    for(int i=0; i<numBins; i++)
        countsPerBin.push_back(0);
    IDsInBin.clear();
    for(int i=0; i<numBins; i++)
        IDsInBin.insert(std::pair<int, QStringList >(i,QStringList()));

    //fill the bins
    for(int i=0; i<projMeas.size(); i++)
    {
        if(isIncludedMeasurement(projMeas[i].measType) &&
                isChildOfActiveInclude(projMeas[i].tag))
        {
            int binNum = InBinNum(projMeas[i].stdResid);
            if(binNum >= 0 && binNum < numBins)
            {
                countsPerBin[binNum] += 1;
                IDsInBin[binNum].append(QString::fromStdString(projMeas[i].tag));
            }
        }
    }

    DetermineCountsPerBinPerMeasType();

    return true;
}

int HistogramDialog::InBinNum(double resid)
{
    for(int i=0; i<numBins; i++)
    {
        if(i==0)//stdResMin will be on wall boundary
        {
            if(resid >= (binWalls[i]-lsa::ZERO_BOUND) && resid < binWalls[i+1])//added ZERO_BOUND to fix Bug #1239
                return i;
        }
        else if(i<numBins-1)
        {
            if(resid >= binWalls[i] && resid < binWalls[i+1])
                return i;
        }
        else
        {
            //added ZERO_BOUND to fix Bug #1239
            if(resid >= binWalls[i] && resid <= (binWalls[i+1]+lsa::ZERO_BOUND))//stdResMax will be on wall boundary
                return i;
            else
            {
//                assert(resid >= binWalls[i] && resid <= (binWalls[i+1]+lsa::ZERO_BOUND));
                return -1;//shouldn't happen
            }
        }
    }
//    assert(1==0);
    return -1;//shouldn't happen
}

void HistogramDialog::findMaxMinStdResiduals()
{
    int ctr=0;

    stdResMax = -99.9;
    stdResMin = 99.9;
    stdResMean = 0.0;

    if(projMeas.size() < 1)
        return;

    for(int i=0; i<projMeas.size(); i++)
    {
        if(isIncludedMeasurement(projMeas[i].measType) &&
                isChildOfActiveInclude(projMeas[i].tag))
        {
            if(projMeas[i].stdResid > stdResMax)
                stdResMax = projMeas[i].stdResid;
            if(projMeas[i].stdResid < stdResMin)
                stdResMin = projMeas[i].stdResid;
            stdResMean += projMeas[i].stdResid;
            ctr++;
        }
    }
    stdResMean = stdResMean/(double)ctr;
    numIncludedMeas = ctr;
}

bool HistogramDialog::isIncludedMeasurement(std::string measType)
{
    if( (measType == std::string("AZIM") && ui->checkBox_AZIM->isChecked()) ||
        (measType == std::string("DIST") && ui->checkBox_DIST->isChecked()) ||
        (measType == std::string("DXYZ.X") && ui->checkBox_DXYZ->isChecked()) ||
        (measType == std::string("DXYZ.Y") && ui->checkBox_DXYZ->isChecked()) ||
        (measType == std::string("DXYZ.Z") && ui->checkBox_DXYZ->isChecked()) ||
        (measType == std::string("HANG") && ui->checkBox_HANG->isChecked()) ||
        (measType == std::string("HDIF") && ui->checkBox_HDIF->isChecked()) ||
        (measType == std::string("HDIR") && ui->checkBox_HDIR->isChecked()) ||
        (( (measType == std::string("PPP.X")) || (measType == std::string("PPP.Y")) || (measType == std::string("PPP.Y")) ) && ui->checkBox_PPP->isChecked()) ||
        (measType == std::string("VANG") && ui->checkBox_VANG->isChecked()) ||
        (measType == std::string("ZANG") && ui->checkBox_ZANG->isChecked()) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool HistogramDialog::isChildOfActiveInclude(std::string tag)
{
    QModelIndex index = guiModel->getIndexFromTag(tag);
    QModelIndex parentIndex = index.parent();
    bool isActive =  includeIsActive(parentIndex);

    return isActive;
}

void HistogramDialog::redrawHistogram(bool clear)
{
    lastBinNum = -1;//previous bar moused over
    lastMeasType = std::string(""); //previous block moused over

    //fill histogram data
    QVector<QwtIntervalSample> data;
    data.clear();
    if(!clear && histogramHasData)
    {
        for(int i=0; i<numBins; i++)
        {
            data.append(QwtIntervalSample(countsPerBin[i],QwtInterval(binWalls[i],binWalls[i+1])));
        }
    }

    if(histPlot != NULL)
    {
        histPlot->detach();
        delete histPlot;
        histPlot = NULL;
    }

    histPlot = new QwtPlotStackedHistogram(binWalls,countsPerBinPerMeasType);
    histPlot->setBrush(QBrush(QColor("blue"),Qt::SolidPattern));
    histPlot->attach(ui->qwtPlot_Histogram);

    histPlot->setSamples(data);
    ui->qwtPlot_Histogram->replot();
}

void HistogramDialog::onNumBinsChanged()
{
    //Fix to Bug #1443
    if(!histogramHasData || guiModel == NULL)
        return;

    numBins = ui->spinBox_NumBins->value();
    findMaxMinStdResiduals();
    if(populateBinsFromHDF5File())
    {
        setSigmaLabels();
        setMaxMinNumBins();
        redrawHistogram();
    }
}

void HistogramDialog::onIncludedMeasChanged()
{
    //Fix to Bug #1443
    if(!histogramHasData || guiModel == NULL)
        return;

    if(populateBinsFromHDF5File())
    {
        setSigmaLabels();
        synchFileTreeToMap();
        setMaxMinNumBins();
        redrawHistogram();
    }
}

void HistogramDialog::onIncludedFileChanged(QTreeWidgetItem* item, int column)
{
    //Fix to Bug #1443
    if(!histogramHasData || guiModel == NULL)
        return;

    HistogramTreeItem *listItem = static_cast<HistogramTreeItem*>(item);
    Qt::CheckState isChecked = listItem->checkState(0);
    QString selectedLabel = listItem->text(0);

    QModelIndex fileIndex = listItem->includeIndex;    
    //Fix to Bug #1419
    guiModel->select(fileIndex);

    parentIncludeMap.remove(fileIndex);
    parentIncludeMap.insert(fileIndex, isChecked);

    checkAllChildren(fileIndex, isChecked);
    updateParentCheckbox(fileIndex);

    synchFileTreeToMap();

    findMaxMinStdResiduals();

    if(populateBinsFromHDF5File())
    {
        setSigmaLabels();
        setMaxMinNumBins();
        redrawHistogram();
    }

    // Select the previously selected treeWidgetItem
    QTreeWidgetItemIterator it(ui->treeFilesToPlot);
    while (*it)
    {
        if ((*it)->text(0) == selectedLabel)
        {
          (*it)->setSelected(true);
          break;
        }
        ++it;
    }

}

bool HistogramDialog::onSaveButtonPressed()
{
    //Fix to Bug #1443
    if(!histogramHasData|| guiModel == NULL)
    {
        QMessageBox::critical(this,QString("Unable to save image"),QString("The histogram contains no data"));
        return false;
    }

    QString svgExt=".svg", pdfExt=".pdf", pngExt=".png";
    QString caption = "Select file name and type for histogram image file";
    QString filter = "SVG files (*.svg)";
    QString projectPath = QString::fromStdString(guiModel->getProjectDirectory());
    QString selectedFilter = "SVG files (*.svg);; PDF files (*.pdf);; PNG file (*.png)";
    QString filename = QFileDialog::getSaveFileName(this, caption, projectPath, selectedFilter, &filter);

    if(!filename.isEmpty())
    {
        if (filename.contains(svgExt))
        {
            filename.remove(svgExt);
        }
        else if (filename.contains(pdfExt))
        {
            filename.remove(pdfExt);
        }
        else if (filename.contains(pngExt))
        {
            filename.remove(pngExt);
        }

        if (filter.contains(svgExt))
        {
            filename += svgExt;
        }
        else if (filter.contains(pdfExt))
        {
            filename += pdfExt;
        }
        else if (filter.contains(pngExt))
        {
            filename += pngExt;
        }

        QwtPlotRenderer renderer;
        ui->qwtPlot_Histogram->replot();
        renderer.renderDocument(ui->qwtPlot_Histogram, filename, QSizeF(200, 100));
    }
    return true;
}

//Shows measurement types within a bin and highlights records in the tree by mousing over a bin on the histogram.
bool HistogramDialog::eventFilter(QObject *object, QEvent *event)
{
    //Fix to Bug #1443
    if(!histogramHasData || guiModel == NULL)
        return false;

    bool didSomething = false;

    if(object == ui->qwtPlot_Histogram->canvas())
    {
        if(event->type() == QEvent::MouseButtonPress)//Fix to Bug #1419
        {
            QMouseEvent *mouseEvent =  (QMouseEvent *) event;
            if(mouseEvent->button() == Qt::LeftButton)//Fix to Bug #1419
            {
                double stdRes = ui->qwtPlot_Histogram->invTransform(QwtPlot::xBottom, mouseEvent->x());
                double height = ui->qwtPlot_Histogram->invTransform(QwtPlot::yLeft, mouseEvent->y());
                if(stdRes <= stdResMax && stdRes >= stdResMin)
                {
                    int binNum = InBinNum(stdRes);
                    if(height > 0 && height <= countsPerBin[binNum])//only respond if within a bar
                    {
                        std::string measType = getMeasTypeAtBarHeight(binNum,height);
                        if(binNum != lastBinNum || measType != lastMeasType)//only respond if changed blocks
                        {
                            lastBinNum = binNum;
                            lastMeasType = measType;
                            QStringList IDs = getIDsInBlock(binNum,measType);
                            QModelIndexList indexesToSelect;

                            indexesToSelect.clear();
                            foreach(QString ID, IDs)
                            {
                                indexesToSelect.append(getIndexFromID(ID.toStdString()));
                                didSomething = true;
                            }
                            guiModel->select(indexesToSelect);
                            addMeasTypesToLists(indexesToSelect);
                        }
                    }
                }
            }
        }
    }
    return didSomething;
}

int HistogramDialog::findMaxBinCount()
{
    int maxCount = 0;

    for(int i=0; i<countsPerBin.size(); i++)
    {
        if(countsPerBin[i] > maxCount)
            maxCount = countsPerBin[i];
    }

    return maxCount;
}

void HistogramDialog::onResetViewButtonPressed()
{
    //Fix to Bug #1443
    if(!histogramHasData || guiModel == NULL)
        return;

    resetView();
}

void HistogramDialog::resetView(bool clear)
{
    //this sets the axes back from wherever they were with panning and zooming
    ui->qwtPlot_Histogram->setAxisScale(QwtPlot::xBottom,stdResMin,stdResMax);
    ui->qwtPlot_Histogram->setAxisScale(QwtPlot::yLeft,0,findMaxBinCount());

    //this redraws the plot
    redrawHistogram(clear);

    //this adds the grid, if on
    onShowGridChanged(ui->checkBox_ShowGrid->isChecked());
}

QModelIndex HistogramDialog::getIndexFromID(std::string ID)
{
    unsigned int DatIndex = 0;
    DatIndex = atoi(ID.c_str());

    std::map<int,QModelIndex>::iterator it;

    it = tagToIndexMap.find(DatIndex);

    if(it != tagToIndexMap.end())//standard measurement
        return it->second;
    else
        return QModelIndex();
}

void HistogramDialog::synchFileTreeToMap()
{
    populateParentSigmaMap();

    // Clear the list control
    ui->treeFilesToPlot->clear();

    // Get the root include
    QModelIndex rootIndex = guiModel->getNextIndex();
    LSARecord *rootRecord = guiModel->getLSARecord(rootIndex);
    LSAInclude *rootInclude = static_cast<LSAInclude*>(rootRecord);

    // Create the list of items to add to the tree
    QList<QTreeWidgetItem*> itemList;

    // Add the top level project record to itemList
    QStringList labels;
    QString projectName = QFileInfo(QString::fromStdString(rootInclude->getLSAPath())).fileName();
    QString sigmaValue = parentSigmaMap.value(rootIndex, "--");
    labels << projectName << sigmaValue;
    HistogramTreeItem *newItem = new HistogramTreeItem(labels, rootIndex);
    Qt::CheckState checkState = parentIncludeMap.value(rootIndex);
    newItem->setCheckState(0,checkState);
    itemList.append((QTreeWidgetItem*)newItem);

    // Add child records to the item list
    addChildRecordsToItemList(itemList, newItem, rootIndex);

    // Insert the list of records into the tree
    ui->treeFilesToPlot->insertTopLevelItems(0,itemList);

    // Expand every record in the tree
    ui->treeFilesToPlot->expandAll();

    // Set column widths
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    int columnWidth0 = settings.value(QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_0, 200).toInt();
    int columnWidth1 = settings.value(QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_1, 50).toInt();
    ui->treeFilesToPlot->setColumnWidth(0, columnWidth0);
    ui->treeFilesToPlot->setColumnWidth(1, columnWidth1);
}

void HistogramDialog::addChildRecordsToItemList(QList<QTreeWidgetItem*> &itemList, HistogramTreeItem *parentItem, QModelIndex parentIndex)
{
    QList<LSARecord*> childRecords = guiModel->getChildRecords(parentIndex);

    foreach(LSARecord *lsaRecord, childRecords)
    {
        if (lsaRecord->getRecType() == LSAType::INCLUDE)
        {
            LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);

            // Get the index for the row to add to the tree view
            QModelIndex childIndex = guiModel->getIndexForRecord(lsaRecord);
            auto it = parentIncludeMap.find(childIndex);

            if(it != parentIncludeMap.end())//Fix to Bug #1419
            {
                // Get the text for the row to add to the tree view
                QStringList labels;
                QString lsaPath = QString::fromStdString(lsaInclude->getLSAPath());
                QString sigmaValue = parentSigmaMap.value(childIndex, "--");
                labels << lsaPath << sigmaValue;

                // Create the item
                HistogramTreeItem *newItem = new HistogramTreeItem(parentItem, labels, childIndex);

                // TODO: make changes here
                // Set the checkstate on the item
                Qt::CheckState checkState = parentIncludeMap.value(childIndex);
                newItem->setCheckState( 0, checkState );

                // Set the text color
//                QColor textColor = includeIsActive(childIndex) ? Qt::black : Qt::gray;
//                newItem->setTextColor(0,textColor);

                // Add the item to the list
                itemList.append((QTreeWidgetItem*)newItem);

                // Recursively add all the item's (include) children to the list
                addChildRecordsToItemList(itemList, newItem, childIndex);
            }
        }
    }
}

bool HistogramDialog::includeIsActive(QModelIndex index)
{
    // If we are passed an invalid index, return false
    if (!index.isValid())
        return false;

    // If the item is not checked, return false
    bool itemIsChecked = parentIncludeMap.value(index, Qt::Unchecked);
    if (!itemIsChecked)
        return false;

    // If the item has no valid parent, its active state equal to its check state
    QModelIndex parentIndex = index.parent();
    if (!parentIndex.isValid())
        return itemIsChecked;

    // Item is checked and has an active parent, so recurse up the tree
    return includeIsActive(parentIndex);
}

void HistogramDialog::addMeasTypesToLists(QModelIndexList indexesInBin)
{
    // Add to listFiles
    QStringList lsaPaths;
    foreach(QModelIndex index, indexesInBin)
    {
        QModelIndex parentIndex = index.parent();
        if (!parentIndex.isValid())
            continue;
        LSARecord* lsaRecord = guiModel->getLSARecord(parentIndex);
        LSAInclude* lsaInclude = static_cast<LSAInclude*>(lsaRecord);
        QString lsaPath = QString::fromStdString(lsaInclude->getLSAPath());

        if(lsaPaths.contains(lsaPath))
            continue;
        else
            lsaPaths.append(lsaPath);
    }

    ui->listFilesInBin->clear();
    foreach(QString lsaPath, lsaPaths)
    {
        ui->listFilesInBin->addItem(lsaPath);
    }
}

void HistogramDialog::onShowGridChanged(bool checked)
{
    //Fix to Bug #1443
    if(!histogramHasData || guiModel == NULL)
        return;

    if(grid != NULL)
    {
        grid->detach();
        delete grid;
        grid = NULL;
    }

    if(checked)
    {
        grid = new QwtPlotGrid();
        grid->attach( ui->qwtPlot_Histogram );
        ui->qwtPlot_Histogram->replot();
    }
    else
    {
        ui->qwtPlot_Histogram->replot();
    }
}

void HistogramDialog::setMaxMinNumBins()
{
    //find the ideal number of bins using the Freedman-Diaconis rule
    double firstQuartile = (stdResMin + stdResMean)/2.0;
    double thirdQuartile = (stdResMean + stdResMax)/2.0;
    double IQR = thirdQuartile - firstQuartile;
    double binSize = 2.0*IQR/pow((double)numIncludedMeas,(1.0/3.0));
    int maxNumBins = (int)(stdResMax - stdResMin)/binSize;
    int minNumBins = (int)((double)maxNumBins/10.0);

    if(maxNumBins > 100)
    {
        //set the range of the spin box
        ui->spinBox_NumBins->setMinimum(minNumBins);
        ui->spinBox_NumBins->setMaximum(maxNumBins);
        if(ui->spinBox_NumBins->value() < ui->spinBox_NumBins->minimum())
            ui->spinBox_NumBins->setValue(ui->spinBox_NumBins->minimum());
        if(numBins < ui->spinBox_NumBins->minimum())
            numBins = ui->spinBox_NumBins->value();//issue calling this AFTER populatBinsFromHDF5File?
    }
}

void HistogramDialog::setSigmaLabels()
{
    double AZIMmean = 0.0, DISTmean = 0.0, DXYZmean = 0.0, HANGmean = 0.0, HDIFmean = 0.0, HDIRmean = 0.0, PPPmean = 0.0, VANGmean = 0.0, ZANGmean = 0.0;
    double AZIMsigma = 0.0, DISTsigma = 0.0, DXYZsigma = 0.0, HANGsigma = 0.0, HDIFsigma = 0.0, HDIRsigma = 0.0, PPPsigma = 0.0, VANGsigma = 0.0, ZANGsigma = 0.0;
    int AZIMctr = 0, DISTctr = 0, DXYZctr = 0, HANGctr = 0, HDIFctr = 0, HDIRctr = 0, PPPctr = 0, VANGctr = 0, ZANGctr = 0;

    //find the mean stdRes for each measurement type
    for(int i=0; i<projMeas.size(); i++)
    {
        if(isChildOfActiveInclude(projMeas[i].tag))
        {
            if(projMeas[i].measType == std::string("AZIM"))
            {
                AZIMmean += projMeas[i].stdResid;
                AZIMctr++;
            }
            else if(projMeas[i].measType == std::string("DIST"))
            {
                DISTmean += projMeas[i].stdResid;
                DISTctr++;
            }
            else if(projMeas[i].measType == std::string("DXYZ.X") || projMeas[i].measType == std::string("DXYZ.Y") || projMeas[i].measType == std::string("DXYZ.Z"))
            {
                DXYZmean += projMeas[i].stdResid;
                DXYZctr++;
            }
            else if(projMeas[i].measType == std::string("HANG"))
            {
                HANGmean += projMeas[i].stdResid;
                HANGctr++;
            }
            else if(projMeas[i].measType == std::string("HDIF"))
            {
                HDIFmean += projMeas[i].stdResid;
                HDIFctr++;
            }
            else if(projMeas[i].measType == std::string("HDIR"))
            {
                HDIRmean += projMeas[i].stdResid;
                HDIRctr++;
            }
            else if(projMeas[i].measType == std::string("PPP.X") || projMeas[i].measType == std::string("PPP.Y") || projMeas[i].measType == std::string("PPP.Z"))
            {
                PPPmean += projMeas[i].stdResid;
                PPPctr++;
            }
            else if(projMeas[i].measType == std::string("VANG"))
            {
                VANGmean += projMeas[i].stdResid;
                VANGctr++;
            }
            else if(projMeas[i].measType == std::string("ZANG"))
            {
                ZANGmean += projMeas[i].stdResid;
                ZANGctr++;
            }
        }
    }
    if(AZIMctr>0) AZIMmean = AZIMmean/(double)AZIMctr;
    if(DISTctr>0) DISTmean = DISTmean/(double)DISTctr;
    if(DXYZctr>0) DXYZmean = DXYZmean/(double)DXYZctr;
    if(HANGctr>0) HANGmean = HANGmean/(double)HANGctr;
    if(HDIFctr>0) HDIFmean = HDIFmean/(double)HDIFctr;
    if(HDIRctr>0) HDIRmean = HDIRmean/(double)HDIRctr;
    if(PPPctr>0) PPPmean = PPPmean/(double)PPPctr;
    if(VANGctr>0) VANGmean = VANGmean/(double)VANGctr;
    if(ZANGctr>0) ZANGmean = ZANGmean/(double)ZANGctr;


    //find the sum of the square of the differences for each measurement type
    for(int i=0; i<projMeas.size(); i++)
    {
        if(isChildOfActiveInclude(projMeas[i].tag))
        {
            if(projMeas[i].measType == std::string("AZIM"))
            {
                AZIMsigma += pow(AZIMmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("DIST"))
            {
                DISTsigma += pow(DISTmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("DXYZ.X") || projMeas[i].measType == std::string("DXYZ.Y") || projMeas[i].measType == std::string("DXYZ.Z"))
            {
                DXYZsigma += pow(DXYZmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("HANG"))
            {
                HANGsigma += pow(HANGmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("HDIF"))
            {
                HDIFsigma += pow(HDIFmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("HDIR"))
            {
                HDIRsigma += pow(HDIRmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("PPP.X") || projMeas[i].measType == std::string("PPP.Y") || projMeas[i].measType == std::string("PPP.Z"))
            {
                PPPsigma += pow(PPPmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("VANG"))
            {
                VANGsigma += pow(VANGmean - projMeas[i].stdResid,2.0);
            }
            else if(projMeas[i].measType == std::string("ZANG"))
            {
                ZANGsigma += pow(ZANGmean - projMeas[i].stdResid,2.0);
            }
        }
    }
    if(AZIMctr>1) AZIMsigma = std::sqrt(AZIMsigma/(double)(AZIMctr - 1));
    if(DISTctr>1) DISTsigma = std::sqrt(DISTsigma/(double)(DISTctr - 1));
    if(DXYZctr>1) DXYZsigma = std::sqrt(DXYZsigma/(double)(DXYZctr - 1));
    if(HANGctr>1) HANGsigma = std::sqrt(HANGsigma/(double)(HANGctr - 1));
    if(HDIFctr>1) HDIFsigma = std::sqrt(HDIFsigma/(double)(HDIFctr - 1));
    if(HDIRctr>1) HDIRsigma = std::sqrt(HDIRsigma/(double)(HDIRctr - 1));
    if(PPPctr>1)  PPPsigma  = std::sqrt(PPPsigma/ (double)(PPPctr - 1));
    if(VANGctr>1) VANGsigma = std::sqrt(VANGsigma/(double)(VANGctr - 1));
    if(ZANGctr>1) ZANGsigma = std::sqrt(ZANGsigma/(double)(ZANGctr - 1));

    //update the labels
    if(AZIMctr>0) ui->label_AZIMsigma->setText(QString::number(AZIMsigma,'f',2));
    else
    {
        ui->label_AZIMsigma->setText(QString("--"));
        ui->label_AZIMsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(DISTctr>0) ui->label_DISTsigma->setText(QString::number(DISTsigma,'f',2));
    else
    {
        ui->label_DISTsigma->setText(QString("--"));
        ui->label_DISTsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(DXYZctr>0) ui->label_DXYZsigma->setText(QString::number(DXYZsigma,'f',2));
    else
    {
        ui->label_DXYZsigma->setText(QString("--"));
        ui->label_DXYZsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(HANGctr>0) ui->label_HANGsigma->setText(QString::number(HANGsigma,'f',2));
    else
    {
        ui->label_HANGsigma->setText(QString("--"));
        ui->label_HANGsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(HDIFctr>0) ui->label_HDIFsigma->setText(QString::number(HDIFsigma,'f',2));
    else
    {
        ui->label_HDIFsigma->setText(QString("--"));
        ui->label_HDIFsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(HDIRctr>0) ui->label_HDIRsigma->setText(QString::number(HDIRsigma,'f',2));
    else
    {
        ui->label_HDIRsigma->setText(QString("--"));
        ui->label_HDIRsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(PPPctr>0) ui->label_PPPsigma->setText(QString::number(PPPsigma,'f',2));
    else
    {
        ui->label_PPPsigma->setText(QString("--"));
        ui->label_PPPsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(VANGctr>0) ui->label_VANGsigma->setText(QString::number(VANGsigma,'f',2));
    else
    {
        ui->label_VANGsigma->setText(QString("--"));
        ui->label_VANGsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
    if(ZANGctr>0) ui->label_ZANGsigma->setText(QString::number(ZANGsigma,'f',2));
    else
    {
        ui->label_ZANGsigma->setText(QString("--"));
        ui->label_ZANGsigma->setStyleSheet("color: gray; background-color: light gray;");
    }
}

void HistogramDialog::disableFiltersForAbsentMeasurements()
{
    ui->checkBox_AZIM->setEnabled(false);
    ui->checkBox_DIST->setEnabled(false);
    ui->checkBox_DXYZ->setEnabled(false);
    ui->checkBox_HANG->setEnabled(false);
    ui->checkBox_HDIF->setEnabled(false);
    ui->checkBox_HDIR->setEnabled(false);
    ui->checkBox_PPP->setEnabled(false);
    ui->checkBox_VANG->setEnabled(false);
    ui->checkBox_ZANG->setEnabled(false);

    ui->checkBox_AZIM->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_DIST->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_DXYZ->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_HANG->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_HDIF->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_HDIR->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_PPP->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_VANG->setStyleSheet("color: gray; background-color: light gray;");
    ui->checkBox_ZANG->setStyleSheet("color: gray; background-color: light gray;");

    for(int i=0; i<projMeas.size(); i++)
    {
        if(projMeas[i].measType == std::string("AZIM"))
        {
            ui->checkBox_AZIM->setEnabled(true);
            ui->checkBox_AZIM->setStyleSheet("color: gray; background-color: black;");
        }
        else if(projMeas[i].measType == std::string("DIST"))
        {
            ui->checkBox_DIST->setEnabled(true);
            ui->checkBox_DIST->setStyleSheet("color: black; background-color: green;");
        }
        else if(projMeas[i].measType == std::string("DXYZ.X") || projMeas[i].measType == std::string("DXYZ.Y") || projMeas[i].measType == std::string("DXYZ.Z"))
        {
            ui->checkBox_DXYZ->setEnabled(true);
            ui->checkBox_DXYZ->setStyleSheet("color: black; background-color: blue;");
        }
        else if(projMeas[i].measType == std::string("HANG"))
        {
            ui->checkBox_HANG->setEnabled(true);
            ui->checkBox_HANG->setStyleSheet("color: black; background-color: red;");
        }
        else if(projMeas[i].measType == std::string("HDIF"))
        {
            ui->checkBox_HDIF->setEnabled(true);
            ui->checkBox_HDIF->setStyleSheet("color: black; background-color: yellow;");
        }
        else if(projMeas[i].measType == std::string("HDIR"))
        {
            ui->checkBox_HDIR->setEnabled(true);
            ui->checkBox_HDIR->setStyleSheet("color: black; background-color: orange;");
        }
        else if(projMeas[i].measType == std::string("PPP.X") || projMeas[i].measType == std::string("PPP.Y") || projMeas[i].measType == std::string("PPP.Z"))
        {
            ui->checkBox_PPP->setEnabled(true);
            ui->checkBox_PPP->setStyleSheet("color: black; background-color: gray;");
        }
        else if(projMeas[i].measType == std::string("VANG"))
        {
            ui->checkBox_VANG->setEnabled(true);
            ui->checkBox_VANG->setStyleSheet("color: black; background-color: cyan;");
        }
        else if(projMeas[i].measType == std::string("ZANG"))
        {
            ui->checkBox_ZANG->setEnabled(true);
            ui->checkBox_ZANG->setStyleSheet("color: black; background-color: magenta;");
        }
    }
}

void HistogramDialog::DetermineCountsPerBinPerMeasType()
{
    std::map<int,int> binCount;
    binCount.clear();
    for(int i=0;i<numBins;i++)
        binCount.insert(std::pair<int,int>(i,0));

    countsPerBinPerMeasType.clear();
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("AZIM"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("DIST"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("DXYZ"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("HANG"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("HDIF"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("HDIR"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("PPP"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("VANG"),binCount));
    countsPerBinPerMeasType.insert(std::pair<std::string,std::map<int,int> >(std::string("ZANG"),binCount));

    for(int j=0; j<projMeas.size(); j++)
    {
        if(isIncludedMeasurement(projMeas[j].measType) &&
                isChildOfActiveInclude(projMeas[j].tag))
        {
            int bin = InBinNum(projMeas[j].stdResid);
            if(projMeas[j].measType == std::string("DXYZ.X") || projMeas[j].measType == std::string("DXYZ.Y") ||
                    projMeas[j].measType == std::string("DXYZ.Z") )
            {
                countsPerBinPerMeasType[std::string("DXYZ")][bin] += 1;//single dxyz measurement gets counted 3 times
            }
            else if(projMeas[j].measType == std::string("PPP.X") || projMeas[j].measType == std::string("PPP.Y") ||
                    projMeas[j].measType == std::string("PPP.Z") )
            {
                countsPerBinPerMeasType[std::string("PPP")][bin] += 1;//single PPP measurement gets counted 3 times
            }
            else
            {
                countsPerBinPerMeasType[projMeas[j].measType][bin] += 1;
            }
        }
    }
}

void HistogramDialog::handleSplitterMoved(int position, int index)
{
    QList<int> splitterSizes = ui->splitter->sizes();

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue(QSETTINGS_HISTOGRAM_SPLITTER_SIZE_0, splitterSizes.at(0));
    settings.setValue(QSETTINGS_HISTOGRAM_SPLITTER_SIZE_1, splitterSizes.at(1));

    return;
}

void HistogramDialog::handleTreeViewColumnWidthChange(int columnNumber, int oldWidth, int newWidth)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    switch (columnNumber)
    {
    case 0:
        settings.setValue(QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_0, newWidth);
        break;
    case 1:
        settings.setValue(QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_1, newWidth);
        break;
    default:
        break;
    }
    return;
}

QStringList HistogramDialog::getIDsInBlock(int binNum, std::string measType)
{
    QStringList retList;

    //loop over all measurements
    for(int i=0; i<projMeas.size(); i++)
    {
        //filter on meas types
        if( ((projMeas[i].measType == std::string("DXYZ.X") || projMeas[i].measType == std::string("DXYZ.Y") || projMeas[i].measType == std::string("DXYZ.Z")) && measType == std::string("DXYZ")) ||
            ((projMeas[i].measType == std::string("PPP.X")  || projMeas[i].measType == std::string("PPP.Y")  || projMeas[i].measType == std::string("PPP.Z"))  && measType == std::string("PPP"))  ||
            (projMeas[i].measType == measType) )
        {
            //filter on included meas types and included files
            if(isIncludedMeasurement(projMeas[i].measType) && isChildOfActiveInclude(projMeas[i].tag))
            {
                //filter on bin number
                if(InBinNum(projMeas[i].stdResid) == binNum)
                {
                    if(!retList.contains(QString::fromStdString(projMeas[i].tag)))//don't triple-count DXYZ and PPP measurements
                        retList.append(QString::fromStdString(projMeas[i].tag));
                }
            }
        }
    }

    return retList;
}

std::string HistogramDialog::getMeasTypeAtBarHeight(int binNum, double float_count)
{
    int countBottom, countTop;
    std::vector<std::string> measTypes = histPlot->measTypes;
    std::string retType("");

    for(int i=0; i<measTypes.size(); i++)
    {
        if(measTypes[i] == std::string("AZIM"))
        {
            countBottom = 0;
            countTop = countsPerBinPerMeasType.at(std::string("AZIM")).at(binNum);
        }
        else if(measTypes[i] == std::string("DIST"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("DIST")).at(binNum);
        }
        else if(measTypes[i] == std::string("DXYZ"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("DXYZ")).at(binNum);
        }
        else if(measTypes[i] == std::string("HANG"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("HANG")).at(binNum);
        }
        else if(measTypes[i] == std::string("HDIF"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("HDIF")).at(binNum);
        }
        else if(measTypes[i] == std::string("HDIR"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("HDIR")).at(binNum);
        }
        else if(measTypes[i] == std::string("PPP"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("PPP")).at(binNum);
        }
        else if(measTypes[i] == std::string("VANG"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("VANG")).at(binNum);
        }
        else if(measTypes[i] == std::string("ZANG"))
        {
            countBottom = countTop;
            countTop += countsPerBinPerMeasType.at(std::string("ZANG")).at(binNum);
        }

        //check if we are in the block for the current meas type
        if(float_count > 0.0 &&
           countBottom != countTop &&
           (double)countBottom <= float_count &&
            float_count <= (double)countTop)//problematic at block bounaries
        {
            retType = measTypes[i];
            break;
        }
    }

    return retType;
}
void HistogramDialog::closeEvent(QCloseEvent *event)
{
    emit closing();
    event->accept();
}

void HistogramDialog::keyPressEvent(QKeyEvent *e) {
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
    else
    {
        if(this->isVisible())
            this->close();
    }
}

void HistogramDialog::resizeEvent(QResizeEvent *event)
{
    if(initialized)
        saveWidgetSettings();

    QDialog::resizeEvent(event);
}

void HistogramDialog::moveEvent(QMoveEvent *event)
{
    if(initialized)
        saveWidgetSettings();

    QDialog::moveEvent(event);
}

void HistogramDialog::saveWidgetSettings()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    qsettings.setValue( QSETTINGS_HISTOGRAM_GEOMETRY, saveGeometry() );
    qsettings.setValue( QSETTINGS_HISTOGRAM_MAXIMIZED, isMaximized() );
    if ( !isMaximized() )
    {
        qsettings.setValue( QSETTINGS_HISTOGRAM_POSITION, pos() );
        QSize debugSize = size();
        qsettings.setValue( QSETTINGS_HISTOGRAM_SIZE, size() );
    }

}

void HistogramDialog::checkAllChildren(QModelIndex parent, Qt::CheckState bCheck)
{
    QList<LSARecord*> childRecords = guiModel->getChildRecords(parent);

    foreach(LSARecord *lsaRecord, childRecords)
    {
        QModelIndex childIndex = guiModel->getIndexForRecord(lsaRecord);
        if(childIndex.isValid())
        {
            parentIncludeMap[childIndex] = bCheck;
            if(lsaRecord->getRecType() == LSAType::INCLUDE)
            {
                checkAllChildren(childIndex, bCheck);
            }
        }
    }
}

void HistogramDialog::updateParentCheckbox(QModelIndex child)
{
    QModelIndex parent = child.parent();
    if(parent.isValid())
    {
        QList<LSARecord*> childRecords = guiModel->getChildRecords(parent);

        bool bFoundUnchecked = false;
        bool bFoundChecked = false;
        bool bFoundPartChecked = false;
        foreach(LSARecord *lsaRecord, childRecords)
        {
            if(lsaRecord->getRecType() != LSAType::INCLUDE)
                continue;

            QModelIndex childIndex = guiModel->getIndexForRecord(lsaRecord);
            if(childIndex.isValid())
            {
                Qt::CheckState state = parentIncludeMap.value(childIndex, Qt::Checked);
                if(state == Qt::Checked)
                {
                    bFoundChecked = true;
                }
                else if(state == Qt::Unchecked)
                {
                    bFoundUnchecked = true;
                }
                else
                {
                    bFoundPartChecked = true;
                }
            }
        }
        if(!bFoundChecked && !bFoundPartChecked)
        {
            parentIncludeMap[parent] = Qt::Unchecked;
        }
        else if(!bFoundUnchecked && !bFoundPartChecked)
        {
            parentIncludeMap[parent] = Qt::Checked;
        }
        else
        {
            parentIncludeMap[parent] = Qt::PartiallyChecked;
        }
        updateParentCheckbox(parent);
    }
}

void HistogramDialog::loadAndApplyWidgetSettings()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    restoreGeometry(qsettings.value( QSETTINGS_HISTOGRAM_GEOMETRY, saveGeometry() ).toByteArray());
    move(qsettings.value( QSETTINGS_HISTOGRAM_POSITION, pos() ).toPoint());
    QSize debugSize = qsettings.value( QSETTINGS_HISTOGRAM_SIZE, size() ).toSize();
    resize(debugSize);
    if ( qsettings.value( QSETTINGS_HISTOGRAM_MAXIMIZED, isMaximized() ).toBool() )
    {
        showMaximized();
    }

    return;
}


