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
#ifndef HISTOGRAMDIALOG_HPP
#define HISTOGRAMDIALOG_HPP

#include <QDialog>
#include <QMenu>
#include <QMouseEvent>
#include <lsah5.hpp>
#include <expandpath.hpp>
#include <LSARecord.hpp>
#include <GuiModel.hpp>
#include <QListWidgetItem>
#include <QSettings>
#include <QTreeWidgetItem>
#include <QPainter>
#include <qwt_plot_histogram.h> //histograms
#include <qwt_painter.h> //used by QwtPlotStackedHistogram
#include <qwt_plot_panner.h> //panning
#include <qwt_plot_magnifier.h> //zooming
#include <qwt_plot_renderer.h> //saving
#include <qwt_plot_grid.h> //grid
#include <qwt_column_symbol.h> // rectangles
#include <functional> //to use doubles as map key
#include <assert.h>
//#include <qwt_plot_picker.h> //mouse click event
//#include <qwt_picker_machine.h> //mouse click event
//#include <qwt_event_pattern.h> //mouse click event

namespace Ui {
class HistogramDialog;
}

class QwtPlotStackedHistogram : public QwtPlotHistogram {

public:
    //member data
    std::vector<double> binWalls;
    std::map<std::string, std::map<int,int> >countsPerBinPerMeasType;
    std::vector<std::string> measTypes;

    /// constructor
    QwtPlotStackedHistogram(std::vector<double> histogramBinWalls, std::map<std::string, std::map<int,int> > histogramCountsPerBinPerMeasType)
       : QwtPlotHistogram(),
         binWalls(histogramBinWalls), countsPerBinPerMeasType(histogramCountsPerBinPerMeasType)
       { measTypes.push_back("AZIM"); measTypes.push_back("DIST"); measTypes.push_back("DXYZ"); measTypes.push_back("HANG"); measTypes.push_back("HDIF");
         measTypes.push_back("HDIR"); measTypes.push_back("PPP"); measTypes.push_back("VANG"); measTypes.push_back("ZANG");}

    //overload the drawColumn method
    virtual void drawColumn( QPainter *painter, const QwtColumnRect &rect, const QwtIntervalSample &sample ) const
    {
        Q_UNUSED( sample );

        //determine bin number from sample
        int binNum = -1;
        for(int i=0; i<binWalls.size()-1; i++)
        {
            if(sample.interval.minValue() == binWalls[i] && sample.interval.maxValue()==binWalls[i+1])
            {
                binNum = i;
                break;
            }
        }
        if(binNum < 0) return;

        //determine counts per measurement type in that bin
        //redefine rect and brush for each meas type
        int countBottom, countTop;
        for(int i=0; i<measTypes.size(); i++)
        {
            if(measTypes[i] == std::string("AZIM"))
            {
                countBottom = 0;
                countTop = countsPerBinPerMeasType.at(std::string("AZIM")).at(binNum);
                painter->setBrush(QBrush(QColor("black"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("DIST"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("DIST")).at(binNum);
                painter->setBrush(QBrush(QColor("green"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("DXYZ"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("DXYZ")).at(binNum);
                painter->setBrush(QBrush(QColor("blue"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("HANG"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("HANG")).at(binNum);
                painter->setBrush(QBrush(QColor("red"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("HDIF"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("HDIF")).at(binNum);
                painter->setBrush(QBrush(QColor("yellow"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("HDIR"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("HDIR")).at(binNum);
                painter->setBrush(QBrush(QColor("orange"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("PPP"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("PPP")).at(binNum);
                painter->setBrush(QBrush(QColor("gray"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("VANG"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("VANG")).at(binNum);
                painter->setBrush(QBrush(QColor("cyan"),Qt::SolidPattern));
            }
            else if(measTypes[i] == std::string("ZANG"))
            {
                countBottom = countTop;
                countTop += countsPerBinPerMeasType.at(std::string("ZANG")).at(binNum);
                painter->setBrush(QBrush(QColor("magenta"),Qt::SolidPattern));
            }

            //convert from countBottom and countTop to rect
            QRectF r;
            if(sample.value > 0)
            {
                QwtColumnRect myrect = rect;
                QwtInterval myvinterval;
                mapCountsToRectVInterval(sample.value,rect.vInterval.maxValue(),rect.vInterval.minValue(),countTop,countBottom,myvinterval);
                myrect.vInterval = myvinterval;
                r = myrect.toRect();
            }
            else
                r = rect.toRect();
/* this was in the original drawColumn method, but it causes Windows build errors.  Commenting out.
            if ( QwtPainter::roundingAlignment( painter ) )
            {
                r.setLeft( qRound( r.left() ) );
                r.setRight( qRound( r.right() ) );
                r.setTop( qRound( r.top() ) );
                r.setBottom( qRound( r.bottom() ) );
            }
*/
            QwtPainter::drawRect( painter, r );
        }
    }

    void mapCountsToRectVInterval(int totalCount, double vTotalMax, double vTotalMin, int countTop, int countBottom, QwtInterval &myvinterval) const
    {
        double slope = (vTotalMax-vTotalMin)/(double)totalCount;
        double intercept = vTotalMin;
        double vMin = slope*countBottom + intercept;
        double vMax = slope*countTop + intercept;
        QwtInterval tmpInterval(vMin,vMax);
        myvinterval = tmpInterval;
    }

    void refreshHistogramModelData(std::vector<double> newBinWalls, std::map<std::string, std::map<int,int> > newCountsPerBinPerMeasType)
    {
        binWalls.clear();
        binWalls = newBinWalls;

        countsPerBinPerMeasType.clear();
        countsPerBinPerMeasType = newCountsPerBinPerMeasType;
    }
};

// sub-class of QListWidgetItem with a separate index for the parentIndex
class HistogramTreeItem : public QTreeWidgetItem
{
public:
    HistogramTreeItem(QStringList labelList, QModelIndex parentIncludeIndex)
        : includeIndex(parentIncludeIndex), QTreeWidgetItem(labelList) { }

    HistogramTreeItem(HistogramTreeItem* parentItem, QStringList labelList, QModelIndex parentIncludeIndex)
        : includeIndex(parentIncludeIndex), QTreeWidgetItem(parentItem, labelList) {}

    QModelIndex includeIndex;
};

class HistogramDialog : public QDialog
{
    Q_OBJECT

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests

public:
    explicit HistogramDialog(QWidget *parent = 0);
    ~HistogramDialog();
    void setupHistogram(const LSAH5File& h5File, GuiModel *inputGuiModel, const QString projectFile);
    void clearHistogram();

private slots:
    void handleSplitterMoved(int position, int index);
    void handleTreeViewColumnWidthChange(int columnNumber, int oldWidth, int newWidth);
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *e);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);


private:

    static const QString QSETTINGS_HISTOGRAM_SPLITTER_SIZE_0;
    static const QString QSETTINGS_HISTOGRAM_SPLITTER_SIZE_1;
    static const QString QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_0;
    static const QString QSETTINGS_HISTOGRAM_TREE_COLUMN_WIDTH_1;
    static const QString QSETTINGS_HISTOGRAM_GEOMETRY;
    static const QString QSETTINGS_HISTOGRAM_MAXIMIZED;
    static const QString QSETTINGS_HISTOGRAM_POSITION;
    static const QString QSETTINGS_HISTOGRAM_SIZE;
    static const double DEFAULT_HIST_WIDTH;
    static const int DEFAULT_NUM_BINS;

    Ui::HistogramDialog *ui;
    std::vector<measurementsExternal> projMeas;
    std::map<int,QModelIndex> tagToIndexMap;
    QMap<QModelIndex, Qt::CheckState> parentIncludeMap;
    QMap<QModelIndex, QString> parentSigmaMap;
    GuiModel *guiModel;
    QString projFile;
    bool initialized;
    int numBins;
    int lastBinNum; //last bin moused over
    std::string lastMeasType; //last block moused over
    double stdResMax;
    double stdResMin;
    double stdResMean;
    int numIncludedMeas;
    std::vector<double> binWalls;
    std::map<std::string, std::map<int,int> >countsPerBinPerMeasType;
    std::vector<int> countsPerBin;
    std::map< int, QStringList > IDsInBin;
    QwtPlotStackedHistogram *histPlot;
    QwtPlotPanner *panner;
    QwtPlotMagnifier *magnifier;
    QwtPlotGrid *grid;
//    QwtPlotPicker *picker;
    bool histogramHasData;

    void populateParentIncludeMap();
    void populateParentSigmaMap();
    bool populateBinsFromHDF5File();//returns true on success, false on failure
    void DetermineCountsPerBinPerMeasType();
    void findMaxMinStdResiduals();
    void redrawHistogram(bool clear=false);
    int InBinNum(double resid);
    bool isIncludedMeasurement(std::string measType);
    bool isChildOfActiveInclude(std::string tag);
    int findMaxBinCount();
    QModelIndex getIndexFromID(std::string ID);
    QStringList getIDsInBlock(int binNum, std::string measType);
    std::string getMeasTypeAtBarHeight(int binNum, double float_count);
    void addMeasTypesToLists(QModelIndexList indexesInBin);

    void synchFileTreeToMap();
    void addChildRecordsToItemList(QList<QTreeWidgetItem *> &itemList, HistogramTreeItem *parentItem, QModelIndex parentIndex);
    bool includeIsActive(QModelIndex index);

    void setMaxMinNumBins();
    void setSigmaLabels();
    void disableFiltersForAbsentMeasurements();
    void resetView(bool clear=false);
    void loadAndApplyWidgetSettings();
    void saveWidgetSettings();

    void checkAllChildren(QModelIndex parent, Qt::CheckState bCheck);
    void updateParentCheckbox(QModelIndex child);

private slots:
    void onCloseButtonPressed();
    bool onSaveButtonPressed();
    void onResetViewButtonPressed();
    void onNumBinsChanged();
    void onIncludedMeasChanged();
    void onShowGridChanged(bool checked);

    void onIncludedFileChanged(QTreeWidgetItem *item, int column);
    bool eventFilter(QObject *object, QEvent *event);

signals:
    void closing();
};

#endif // HISTOGRAMDIALOG_HPP
