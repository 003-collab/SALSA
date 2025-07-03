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
#ifndef ADJUSTEDPOSITIONS_HPP
#define ADJUSTEDPOSITIONS_HPP

//Qt
#include <QDialog>
#include <QCloseEvent> //to map closing widget to Main Window View menu
#include <QStandardItemModel>
#include <QSettings> //to allow table column widths to persist
#include <QAction> //context menu in table
#include <QFileInfo> //for creating initial_coordinates.lsa

// LSA
#include <GuiModel.hpp>
#include <lsah5.hpp>
#include <LSAConstants.hpp>
#include <guiutils.hpp>
#include <lsaUtils.hpp>

namespace Ui {
class AdjustedPositions;
}

struct adjustedNEUPos {
    double n;
    double e;
    double u;
    double magnitude;
};

class AdjustedPositions : public QDialog
{
    Q_OBJECT

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests

public:
    explicit AdjustedPositions(GuiModel *guiModel, QWidget *parent = 0);
    ~AdjustedPositions();
    Ui::AdjustedPositions* getUi() { return ui; }
    void updateView(const LSAH5File& hdf5File, double ellipseScaleFactor, GuiModel *gooeymodel, bool usingWestLon, bool includeUnused);
    void updateSelection(QModelIndexList pointIndexes);
    void setGuiModel(GuiModel *gooeymodel);
    void clear();
    QStandardItemModel* getPointsModel() { return pointsModel; }
    QString getGeoid();
    llhExternal getPoint(QString label);

private slots:
    void keyPressEvent(QKeyEvent *e);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);
    void handleTableViewPointsSelectionChanged(QItemSelection, QItemSelection);
    void handleTableViewPointsColumnResized(int,int,int);
    void handleAddToProjectFixed();
    void handleAddToProjectFloat();

signals:
    void closing();


private:
    static const int NUM_TABLE_COLUMNS;
    static const QString QSETTINGS_ADJPTS_GEOMETRY;
    static const QString QSETTINGS_ADJPTS_POSITION;
    static const QString QSETTINGS_ADJPTS_SIZE;
    static const QString QSETTINGS_ADJPTS_MAXIMIZED;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_0;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_1;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_2;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_3;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_4;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_5;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_6;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_7;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_8;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_9;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_10;
    static const QString QSETTINGS_ADJPTS_COLUMN_WIDTH_11;
    bool initialized;

    Ui::AdjustedPositions *ui;
    GuiModel *guiModel;//not currently used, but here for the future model-view implementation
    std::vector<llhExternal> points;
    std::vector<llhExternal> initial_points;
    QStandardItemModel *pointsModel;
    QAction            *addToProjectFixed;
    QAction            *addToProjectFloat;
    bool suppressTableSelectionChanges;

    void setupTable();
    void setTableColumnWidths();
    void addTableHeaders();
    std::vector<adjustedNEUPos> getAdjustments();
    llhExternal getInitialLlhPoint(const LSAH5File& hdf5File, int pointsIndex);
    void saveDockWidgetSettings();
    void loadAndApplyDockWidgetSettings();
    void handleAddToProject(bool addAsFloat=false);
};

#endif // ADJUSTEDPOSITIONS_HPP
