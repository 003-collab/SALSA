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
#ifndef FINDDIALOG_HPP
#define FINDDIALOG_HPP

#include <QDialog>
#include <QKeyEvent>
#include <QStandardItemModel>

#include <vector>
#include <map>
#include <functional>

#include <GuiModel.hpp>
#include <LSAType.hpp>
#include <LSARecord.hpp>

bool isAppropriateType(std::vector<LSAType>, LSARecord *currentRecord);
bool isOrthometric(std::vector<LSAType>, LSARecord *currentRecord);
bool recordIsEnabled(const GuiModel* guiModel, LSARecord *currentRecord);


namespace Ui {
class FindWidget;
}

class FindWidget : public QWidget
{
    Q_OBJECT

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests

public:

    explicit FindWidget(QWidget *parent = 0);
    ~FindWidget();

    void setGuiModel(GuiModel *guiModel) { this->guiModel = guiModel; }
    void setFilterText(QString value);
    bool findWidgetButtonClicked() const;
    void setPriorWidget(QWidget *priorWidget) { this->priorFocusWidget = priorWidget; }

signals:
    // Both signals are hooked in connect statements in MainWindow to propagate changes to tree view
    void filterStringChanged(QString, bool, bool);
    void boxValueChanged(std::vector<LSAType>, std::function<bool (std::vector<LSAType>, LSARecord *)>);
    void matchTypeChanged(bool);

    void pushWarning(QString msg);
    void closing(QWidget *);

public slots:
    void findAll();
    void findNext();
    void findPrevious();
    void findNextWarning() { findNextWarning(QModelIndex()); }
    void findNextWarning(QModelIndex originalIndex);
    void hideWidget();
    void handleFilterChanged(QString newString);
    void handleFilterExactChanged(bool filterExact);
    void clearControls();

    // Sets value of currentLabel. If different from past label will also change the filters
    // and emit a boxValueChanged signal
    void handleBoxChanged(QString comboBoxVal);
    void handleMultiTermBoxChanged(QString comboBoxVal);

private:
    Ui::FindWidget *ui;
    GuiModel       *guiModel;
    QWidget        *priorFocusWidget;

    void keyPressEvent(QKeyEvent* event);
    bool findWidgetHasFocus() const;
    bool stringIsMatch(QString lsaString, QString searchString);
    QModelIndex getCurrentSingleSelection();

    void setCurrentLabel(const QString &label) {currentLabel = label;}
    void setPastLabel(const QString &label) {pastLabel = label;}
    void verifyFilters(const std::vector<LSAType> &filters);

    void initializeListOrder();
    void initializeMultiTermBoxes();

    // Two methods below exist to allow an easy way to assign data to labelToFiltersMap while preserving order that
    // map was filled via the labelList parameter which is passed by reference.
    void performAssignment(const QString &comboBoxLabel, const QString &toolTipLabel, QStandardItemModel * filterBoxModel, std::vector<LSAType> &filterTypes,
                           const std::function<bool (std::vector<LSAType>, LSARecord *)> &logicalFunc, bool autoClear=true);
    void performAssignment(const QString &comboBoxLabel, const QString &toolTipLabel, QStandardItemModel * filterBoxModel,
                           const std::function<bool (std::vector<LSAType>, LSARecord *)> &logicalFunc, int typeVal);

    void pushGuiWarning(QString warning) { emit pushWarning(warning);}

    QString pastLabel = "ALL";
    QString currentLabel;
    std::map<QString, std::vector<LSAType>> labelToFiltersMap;

    std::map<QString, std::function<bool (std::vector<LSAType>, LSARecord *)>> labelToFuncMap;
};

#endif // FINDDIALOG_HPP
