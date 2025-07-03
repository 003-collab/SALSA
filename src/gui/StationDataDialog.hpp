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
#ifndef STATIONDATADIALOG_HPP
#define STATIONDATADIALOG_HPP

#include <vector>
#include <string>

#include <QDialog>
#include <QCloseEvent>
#include <QProcess>
#include <QDir>
#include <QStandardItemModel>
#include <QStringList>
#include <QPair>
#include <QList>
#include <QAction>
#include <QMenu>
#include <QItemSelection>

#include <GuiModel.hpp>
#include "lsah5.hpp"
#include "lsainvpairdata.h"

namespace Ui {
class StationDataDialog;
}

class StationDataDialog : public QDialog
{
    Q_OBJECT

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests

public:
    explicit StationDataDialog(QWidget *parent = 0);
    ~StationDataDialog();

    Ui::StationDataDialog* getUi() { return ui; }

    void setup(LSAH5File *h5file, QString hdf5FileName, bool apv, bool westLon);

    void setupComboBoxes(LSAH5File *h5file, QString hdf5FileName, bool apv, bool westLon);
    void setupPairTable(const QString &projectDir, const QString &projectName);
    void setupPairTable(const QString &projectFile);
    void setNewFromTo(QString fromLabel, QString toLabel);
    void runLSAInverse(bool exportTriggered, bool clearOutputFile = false);
    void updateOutput(bool exportTriggered = false, bool clearOutputFile = false);
    void parseToGui(QStringList inverseData);
    bool validStations(QString from, QString to);
    void clear();
    void clearOutput();

    void setInvFileName(const QString &baseName) {userDefInvPath = baseName;}

    QString giveFileBase() const;
    std::string giveFileBaseString() const;

    QString giveFileAbsPath() const;
    std::string giveFileAbsPathString() const;

    const QString QSETTINGS_PAIRS_COLUMN_WIDTH_0 = "QSETTINGS_PAIRS_COLUMN_WIDTH_0";
    const QString QSETTINGS_PAIRS_COLUMN_WIDTH_1 = "QSETTINGS_PAIRS_COLUMN_WIDTH_1";
    const QString QSETTINGS_SDD_GEOMETRY = "QSETTINGS_SDD_GEOMETRY";

    std::map<QString, QStringList> StationInverseMap;

private slots:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *e);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);

    void comboBoxChanged()
    {
        if(!suppressOutputUpdates)
        {
            fromToStations_Changed = true;
            handleStationsChanged();
            fromToStations_Changed = false;
        }
    }
    void btnSwapClicked()
    {
        fromToStations_Changed = true;
        handleSwapBtnClicked();
        fromToStations_Changed = false;
    }
    void btnExportClicked()
    {
        handleExportBtnClicked();
    }

    void btnExportAsClicked(const QString &provName = "")
    {
        auto fileName = provName;
        // If a filename isn't provided, have the user pick via the File Browser.
        if(fileName.isEmpty() || fileName.isNull())
        {
            fileName = utilizeFileBrowser();
        }

        // If it's still empty, then they probably pressed cancel
        if(fileName.isEmpty() || fileName.isNull())
        {
            return;
        }
        else
        {
            setInvFileName(fileName);
        }

        handleExportBtnClicked();
        setInvFileName("");
    }

    void handleAddBtnClicked();

    void handleDeleteAction();

    void handleSelectionChanged(const QItemSelection &updatedSelection, const QItemSelection &oldSelection);

    void onCustomContextMenu(const QPoint &provPoint);

    void handleTableStationPairsResized(int columnNumber, int oldWidth, int newWidth);

signals:
    void pushWarning(QString msg);        ///< Push a 'warning' formatted string to the status window
    void pushError(QString msg);          ///< Push a 'error' formatted string to the status window
    void pushStatus(QString msg);         ///< Push a 'error' formatted string to the status window

    void closing();

private:
    Ui::StationDataDialog *ui;

    void checkBtnAddStatus();

    const int NUM_PAIR_COLUMNS = 2;

    void setTableColumnWidths();

    QString lsainverse;
    QStringList stationList;

    bool initialized;

    bool scaleByAPV;
    bool usingWestLon;
    QString h5FilePath;

    bool fromToStations_Changed;
    bool suppressOutputUpdates;
    void handleStationsChanged();
    void handleSwapBtnClicked();
    void handleExportBtnClicked();   

    QStandardItemModel * invPairsModel = nullptr;
    QMenu * rightClickMenu = nullptr;
    QAction * actionDelete = nullptr;
    LSAInvPairData allStationPairs;

    QString oldH5File;
    QString oldFromStation;
    QString oldToStation;

    QString fromStation;
    QString toStation;


    /* Will be utilized in helping name the output csv and inv files.
    Equivalent to full inv output file name (if set by user).
    Should be empty except for when user utilizes Export As button */

    QString userDefInvPath;

    void pushGuiWarning(QString message) { emit pushWarning(message); }
    void pushGuiError(QString message) { emit pushError(message); }
    void pushGuiStatus(QString message) { emit pushStatus(message); }

    bool eventFilter(QObject *object, QEvent *event);

    QString utilizeFileBrowser();
    void validateAndAddPair(const QString &fromText, const QString &toText, bool fromAddButton = false);
    void deletePair(int deleteIndex);
    void saveWidgetSettings();
};

#endif // STATIONDATADIALOG_HPP
