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
#include "FindWidget.hpp"
#include "ui_FindWidget.h"

#include <algorithm>

FindWidget::FindWidget(QWidget *parent) : QWidget(parent),
    ui(new Ui::FindWidget)
{
    ui->setupUi(this);

    initializeListOrder();
    initializeMultiTermBoxes();

    handleBoxChanged("All Records");

    connect(ui->buttonAll,         SIGNAL(clicked()),                   this, SLOT( findAll()) );
    connect(ui->buttonNext,        SIGNAL(clicked()),                   this, SLOT( findNext()) );
    connect(ui->buttonPrevious,    SIGNAL(clicked()),                   this, SLOT( findPrevious()) );
    connect(ui->buttonNextWarning, SIGNAL(clicked()),                   this, SLOT( findNextWarning()) );
    connect(ui->buttonClose,       SIGNAL(clicked()),                   this, SLOT( hideWidget()) );
    connect(ui->lineFilter,        SIGNAL(textChanged(QString)),        this, SLOT( handleFilterChanged(QString)) );
    connect(ui->chkExactMatchFilter, SIGNAL(clicked(bool)),             this, SLOT( handleFilterExactChanged(bool)) );
    connect(ui->filterComboBox,    SIGNAL(currentTextChanged(QString)), this, SLOT( handleBoxChanged(QString) ) );
    connect(ui->anyAllFilterComboBox,    SIGNAL(currentTextChanged(QString)), this, SLOT( handleMultiTermBoxChanged(QString) ) );

    setFocusProxy(ui->lineSearchString);

    ui->buttonClose->setStyleSheet("QToolButton {border: none} ");


}

FindWidget::~FindWidget()
{
    delete ui;
}

void FindWidget::setFilterText(QString value)
{
    ui->lineFilter->setText(value);
}

void FindWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (ui->lineSearchString->hasFocus() || ui->buttonNext->hasFocus() )
        {
            findNext();
        }
        else if (ui->buttonPrevious->hasFocus())
        {
            findPrevious();
        }
        else if (ui->buttonNextWarning->hasFocus())
        {
            findNextWarning();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if ( findWidgetHasFocus() )
        {
            this->hide();
            ui->lineFilter->clear();
        }
    }

    return;
}

void FindWidget::findAll()
{
    if (!guiModel) return;
    // Check that the seach string is not empty
    QString searchString = ui->lineSearchString->text();
    if (searchString.isEmpty()) return;

    QModelIndex currentIndex = guiModel->getNextIndex();
    QModelIndexList foundRecords;


    while(currentIndex.isValid())
    {
        LSARecord* lsaRecord = guiModel->getLSARecord(currentIndex);
        QString lsaString = QString::fromStdString(lsaRecord->getSingleLSAString());

        if ( stringIsMatch(lsaString, searchString) && !guiModel->isRecordHidden(currentIndex) )
        {
            foundRecords << currentIndex;
        }

        currentIndex = guiModel->getNextIndex(currentIndex);
    }

    if (foundRecords.size() > 0)
    {
        guiModel->select(foundRecords);
    }

    return;
}

QModelIndex FindWidget::getCurrentSingleSelection()
{
    // See if there is one record selected
    QModelIndex selectedIndex = guiModel->getSingleSelectedRecord();

    // If we didn't find one and only one selected record, try the current index
    if (!selectedIndex.isValid())
    {
        selectedIndex = guiModel->selectionModel->currentIndex();
    }

    // If the current index isn't valid, fall back to the project record
    if (!selectedIndex.isValid())
    {
        QModelIndex projectIndex = guiModel->index(0,0).child(0,0);
        selectedIndex = projectIndex;
    }

    return selectedIndex;
}

void FindWidget::verifyFilters(const std::vector<LSAType> &filters)
{
    for(const auto &filter : filters)
    {
        if(filter == LSAType::Unknown || filter >= LSAType::count)
        {
            QString warningMessage = "Warning - unknown filter type of value " + QString::number(filter.getLSAtype());
            emit pushGuiWarning(warningMessage);
        }
    }
}

void FindWidget::initializeListOrder()
{
    QStringList orderedList;
    std::vector<LSAType> tmpFiltList;

    std::function<bool (std::vector<LSAType>, LSARecord *)> funcAppropriate = &isAppropriateType;
    std::function<bool (std::vector<LSAType>, LSARecord *)> funcOrthometric = &isOrthometric;
    std::function<bool (std::vector<LSAType>, LSARecord *)> funcEnabled = [this](std::vector<LSAType>, LSARecord * record)
    {
        return recordIsEnabled(guiModel, record);
    };

    QStandardItemModel * FilterBoxModel = new QStandardItemModel();

    // Filter types for type ALL
    for(int typeVal = LSAType::COMMENT; typeVal < LSAType::count; typeVal++)
    {
        tmpFiltList.push_back(typeVal);
    }

    performAssignment("All Records", "All Records", FilterBoxModel, tmpFiltList, funcAppropriate);

    // Next few sections will just be defined for mostly individual record types
    performAssignment("Include", "Include", FilterBoxModel, funcAppropriate, LSAType::INCLUDE);
    performAssignment("Comment", "Comment", FilterBoxModel, funcAppropriate, LSAType::COMMENT);

    tmpFiltList.push_back(LSAType::POSG);
    tmpFiltList.push_back(LSAType::POSC);
    performAssignment("POS*", "POS*", FilterBoxModel, tmpFiltList, funcAppropriate);

    performAssignment("DIST", "DIST", FilterBoxModel, funcAppropriate, LSAType::DIST);
    performAssignment("DXYZ", "DXYZ", FilterBoxModel, funcAppropriate, LSAType::DXYZ);
    performAssignment("HANG", "HANG", FilterBoxModel, funcAppropriate, LSAType::HANG);
    performAssignment("AZIM", "AZIM", FilterBoxModel, funcAppropriate, LSAType::AZIM);
    performAssignment("VANG", "VANG", FilterBoxModel, funcAppropriate, LSAType::VANG);
    performAssignment("ZANG", "ZANG", FilterBoxModel, funcAppropriate, LSAType::ZANG);
    performAssignment("HDIF", "HDIF", FilterBoxModel, funcAppropriate, LSAType::HDIF);

    tmpFiltList.push_back(LSAType::HDIR);
    tmpFiltList.push_back(LSAType::DGRP);
    performAssignment("HDIR/DGRP", "HDIR/DGRP", FilterBoxModel, tmpFiltList, funcAppropriate);

    performAssignment("HGHT", "HGHT", FilterBoxModel, funcAppropriate, LSAType::HGHT);
    performAssignment("VSCA", "VSCA", FilterBoxModel, funcAppropriate, LSAType::VSCA);
    performAssignment("UNCR", "UNCR", FilterBoxModel, funcAppropriate, LSAType::UNCR);
    performAssignment("MEAN", "MEAN", FilterBoxModel, funcAppropriate, LSAType::MEAN);
    performAssignment("ENUO", "ENUO", FilterBoxModel, funcAppropriate, LSAType::ENUO);

    performAssignment("Orthometric", "POSG with orthometric heights", FilterBoxModel, funcOrthometric, LSAType::POSG);

    tmpFiltList.push_back(LSAType::POSG);
    tmpFiltList.push_back(LSAType::POSC);
    tmpFiltList.push_back(LSAType::DXYZ);
    performAssignment("GPS/GNSS", "GPS/GNSS {POSC; POSG; DXYZ}",
                      FilterBoxModel, tmpFiltList, funcAppropriate);

    tmpFiltList.push_back(LSAType::DIST);
    tmpFiltList.push_back(LSAType::HANG);
    tmpFiltList.push_back(LSAType::AZIM);
    tmpFiltList.push_back(LSAType::VANG);
    tmpFiltList.push_back(LSAType::ZANG);
    tmpFiltList.push_back(LSAType::HDIF);
    tmpFiltList.push_back(LSAType::HDIR);
    tmpFiltList.push_back(LSAType::DGRP);
    performAssignment("Conventional", "Conventional {DIST; HANG; AZIM;\nVANG; ZANG; HDIF; HDIR; DGRP}",
                      FilterBoxModel, tmpFiltList, funcAppropriate);

    tmpFiltList.push_back(LSAType::MEAN);
    tmpFiltList.push_back(LSAType::ENUO);
    performAssignment("Derived", "Derived {MEAN; ENUO}",
                      FilterBoxModel, tmpFiltList, funcAppropriate);

    tmpFiltList.push_back(LSAType::HGHT);
    tmpFiltList.push_back(LSAType::VSCA);
    tmpFiltList.push_back(LSAType::UNCR);
    performAssignment("Modifiers", "Modifiers {HGHT; VSCA; UNCR}",
                      FilterBoxModel, tmpFiltList, funcAppropriate);

    // Put something in the Type list or it doesn't work quite right
    tmpFiltList.push_back(LSAType::HGHT);
    performAssignment("Enabled", "Enabled records",
                      FilterBoxModel, tmpFiltList, funcEnabled);

    ui->filterComboBox->setModel(FilterBoxModel);
}

void FindWidget::initializeMultiTermBoxes()
{
    QStandardItemModel * matchTypeModel = new QStandardItemModel();

    QStandardItem * addedItem = new QStandardItem();
    addedItem->setData("ANY", Qt::DisplayRole);
    addedItem->setData("Records must match at least one search term.", Qt::ToolTipRole);
    matchTypeModel->appendRow(addedItem);

    addedItem = new QStandardItem();
    addedItem->setData("ALL", Qt::DisplayRole);
    addedItem->setData("Records must match all search terms.", Qt::ToolTipRole);
    matchTypeModel->appendRow(addedItem);

    ui->anyAllFilterComboBox->setModel(matchTypeModel);
    ui->anyAllSearchComboBox->setModel(matchTypeModel);
}

void FindWidget::performAssignment(const QString &comboBoxLabel, const QString &toolTipLabel, QStandardItemModel *filterBoxModel, std::vector<LSAType> &filterTypes,
                                   const std::function<bool (std::vector<LSAType>, LSARecord *)> &logicalFunc, bool autoClear)
{
    QStandardItem * addedItem = new QStandardItem();
    addedItem->setData(comboBoxLabel, Qt::DisplayRole);
    addedItem->setData(toolTipLabel, Qt::ToolTipRole);
    filterBoxModel->appendRow(addedItem);

    labelToFiltersMap[comboBoxLabel] = filterTypes;
    if(autoClear)
    {
        filterTypes.clear();
    }
    labelToFuncMap[comboBoxLabel] = logicalFunc;
}

void FindWidget::performAssignment(const QString &comboBoxLabel, const QString &toolTipLabel, QStandardItemModel *filterBoxModel,
                                   const std::function<bool (std::vector<LSAType>, LSARecord *)> &logicalFunc, int typeVal)
{
    std::vector<LSAType> tmpFiltList;
    tmpFiltList.push_back(typeVal);
    performAssignment(comboBoxLabel, toolTipLabel, filterBoxModel, tmpFiltList, logicalFunc, false);
}

void FindWidget::findNext()
{
    if (!guiModel) return;

    // Find the index for the currently selected record
    QModelIndex originalIndex = getCurrentSingleSelection();
    if ( !originalIndex.isValid() ) return;

    // Check that the seach string is not empty
    QString searchString = ui->lineSearchString->text();
    if (searchString.isEmpty()) return;

    // Find the next record that contains the search string
    bool nextRecordFound = false;
    QModelIndex currentIndex = guiModel->getNextIndex(originalIndex);
    if (!currentIndex.isValid())
    {
        currentIndex = guiModel->index(0,0);
    }
    while (currentIndex.isValid() && currentIndex != originalIndex)
    {
        LSARecord* lsaRecord = guiModel->getLSARecord(currentIndex);
        QString lsaString = QString::fromStdString(lsaRecord->getSingleLSAString());

        if ( stringIsMatch(lsaString, searchString) && !guiModel->isRecordHidden(currentIndex) )
        {
            nextRecordFound = true;
            break;
        }

        currentIndex = guiModel->getNextIndex(currentIndex);
        if (!currentIndex.isValid())
        {
            currentIndex = guiModel->index(0,0);
        }
    }

    ui->buttonNext->setFocus();

    // If we found a record, select it
    if (nextRecordFound)
    {
        guiModel->select(currentIndex);
    }

    return;
}

void FindWidget::findPrevious()
{
    if (!guiModel) return;

    // Find the index for the currently selected record
    QModelIndex originalIndex = getCurrentSingleSelection();

    // If the project index is selected, start at the end of the project
    QModelIndex rootIndex = guiModel->index(0,0);

    // Check that the seach string is not empty
    QString searchString = ui->lineSearchString->text();
    if (searchString.isEmpty()) return;

    // Find the previous record that contains the search string
    bool previousRecordFound = false;
    QModelIndex currentIndex;
    if (originalIndex == rootIndex)
    {
        currentIndex = guiModel->getPreviousIndex();
    }
    else
    {
        currentIndex = guiModel->getPreviousIndex(originalIndex);
    }
    while (currentIndex.isValid() && currentIndex != originalIndex)
    {
        LSARecord* lsaRecord = guiModel->getLSARecord(currentIndex);
        QString lsaString = QString::fromStdString(lsaRecord->getSingleLSAString());

        if ( stringIsMatch(lsaString, searchString) && !guiModel->isRecordHidden(currentIndex) )
        {
            previousRecordFound = true;
            break;
        }

        currentIndex = guiModel->getPreviousIndex(currentIndex);
        if (!currentIndex.isValid())
        {
            // If we are at the top of the tree, wrap back down to the bottom
            currentIndex = guiModel->getPreviousIndex();
        }
    }

    ui->buttonPrevious->setFocus();

    // If we found a record, select it
    if (previousRecordFound)
    {
        guiModel->select(currentIndex);
    }

    return;
}

void FindWidget::findNextWarning(QModelIndex originalIndex)
{
    if (!guiModel) return;

    // If originalIndex is not valid, set it to the current single selected record
    if ( !originalIndex.isValid() ) originalIndex = getCurrentSingleSelection();

    // Ensure originalIndex col is zero
    if (originalIndex.isValid() && originalIndex.column() > 0)
    {
        int rowNum = originalIndex.row();
        int colNum = 0;
        QModelIndex parentIndex = originalIndex.parent();
        originalIndex = guiModel->index(rowNum, colNum, parentIndex);
    }

    // If originalIndex is still not valid, do nothing
    if ( !originalIndex.isValid() ) return;

    // Find the next record that contains the search string
    bool nextRecordFound = false;
    QModelIndex currentIndex = guiModel->getNextIndex(originalIndex);
    while (currentIndex.isValid() && currentIndex != originalIndex)
    {
        LSARecord* lsaRecord = guiModel->getLSARecord(currentIndex);

        if ( (lsaRecord->warningMessages_persistent.size() > 0 ||
            lsaRecord->warningMessages_generated.size() > 0) &&
             !guiModel->isRecordHidden(currentIndex) )
        {
            nextRecordFound = true;
            break;
        }

        currentIndex = guiModel->getNextIndex(currentIndex);
        if (!currentIndex.isValid())
        {
            // wrap the search from the bottom back to the top of the tree
            currentIndex = guiModel->index(0,0);
        }
    }

    this->show(); // show the find widget in the MainWindow
    ui->buttonNextWarning->setFocus();

    // If we found a record, select it
    if (nextRecordFound)
    {
        guiModel->select(currentIndex);
    }

    return;
}

void FindWidget::hideWidget()
{
    ui->lineFilter->clear();
    ui->filterComboBox->setCurrentIndex(0);
    // i#1431 say we're closing after we clear the filter text, since
    // changing the filter text triggers filtering callbacks in GuiModel.
    emit closing(this->priorFocusWidget);
    this->hide();
}

void FindWidget::handleFilterChanged(QString newString)
{
    emit filterStringChanged(newString, ui->chkExactMatchFilter->isChecked(), ui->anyAllFilterComboBox->currentText() == "ALL");
}

void FindWidget::handleFilterExactChanged(bool filterExact)
{
    emit filterStringChanged(ui->lineFilter->text(), filterExact, ui->anyAllFilterComboBox->currentText() == "ALL");
}

bool FindWidget::findWidgetHasFocus() const
{
    bool childHasFocus = this->hasFocus()                 ||
                         ui->lineFilter->hasFocus()       ||
                         ui->lineSearchString->hasFocus() ||
                         ui->chkExactMatch->hasFocus()    ||
                         findWidgetButtonClicked();

    return childHasFocus;
}

bool FindWidget::findWidgetButtonClicked() const
{
    bool childHasFocus = ui->buttonAll->hasFocus()        ||
                         ui->buttonClose->hasFocus()      ||
                         ui->buttonNext->hasFocus()       ||
                         ui->buttonPrevious->hasFocus()   ||
                         ui->buttonNextWarning->hasFocus();

    return childHasFocus;
}

void FindWidget::clearControls()
{
    ui->lineFilter->clear();
    ui->lineSearchString->clear();
}

void FindWidget::handleBoxChanged(QString comboBoxVal)
{
    setCurrentLabel(comboBoxVal);
    if(currentLabel == pastLabel)
    {
        return;
    }

    std::map<QString, std::vector<LSAType>>::const_iterator labelFiltersMap_iter = labelToFiltersMap.find(currentLabel);
    if(labelFiltersMap_iter == labelToFiltersMap.end())
    {
        QString warningMsg = "Warning - unable to find filters for " + currentLabel + ". Will revert back to type All Records.";
        pushGuiWarning(warningMsg);
        handleBoxChanged("All Records");
        return;
    }

    auto labelFuncMap_iter = labelToFuncMap.find(currentLabel);
    if(labelFuncMap_iter == labelToFuncMap.end())
    {
        QString warningMsg = "Warning - unable to find decision logic for " + currentLabel;
        warningMsg += ". Will revert back to All Records.";
        pushGuiWarning(warningMsg);
        handleBoxChanged("All Records");
        return;
    }

    verifyFilters(labelFiltersMap_iter->second);
    emit boxValueChanged(labelFiltersMap_iter->second, labelFuncMap_iter->second);
    setPastLabel(currentLabel);
}

void FindWidget::handleMultiTermBoxChanged(QString comboBoxVal)
{
    emit matchTypeChanged(comboBoxVal == "ALL");
}

bool FindWidget::stringIsMatch(QString lsaString, QString searchString)
{
    return GuiModel::findInString(lsaString,
                                  searchString,
                                  ui->chkExactMatch->isChecked(),
                                  ui->anyAllSearchComboBox->currentText() == "ALL");
}

bool isAppropriateType(std::vector<LSAType> filterTypes, LSARecord *currentRecord)
{
    std::vector<LSAType>::const_iterator searchIter = std::find(filterTypes.begin(), filterTypes.end(),
                                                                currentRecord->getRecType());
    if(searchIter == filterTypes.end())
    {
        return false;
    }

    else
    {
        return true;
    }
}

bool isOrthometric(std::vector<LSAType> filterTypes, LSARecord *currentRecord)
{
    bool isOrthometric = false;
    if(currentRecord->getRecType() == LSAType::Types::POSG)
    {
        LSAPosG * posgRec = static_cast<LSAPosG *>(currentRecord);
        if(!posgRec->heightIsEllipsoidal)
        {
            isOrthometric = true;
        }
    }

    return isOrthometric;
}

bool recordIsEnabled(const GuiModel* guiModel, LSARecord *currentRecord)
{
    if (guiModel == nullptr)
    {
        return false;
    }
    return guiModel->isActive(guiModel->getIndexForRecord(currentRecord));
}
