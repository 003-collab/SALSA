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
#include <QtWidgets>



// gnsstk
#include "Exception.hpp"

#include "GuiModelItem.hpp"
#include "GuiModel.hpp"
#include "LSASupportedToVersion.hpp"
#include <ostream>

#include <algorithm>

const std::string GuiModel::CHILD_WARNING_MESSAGE = "Child record has warning.";
const std::string GuiModel::INVALID_COVARIANCE_MESSAGE = "Invalid covariance.";
const std::string GuiModel::INVALID_SCALING_MESSAGE = "Invalid net scale value.";
const std::string GuiModel::INVALID_SIGMA_MESSAGE = "Net measurement sigma must be > 0.";
const std::string GuiModel::INVALID_POSG_POLE_MESSAGE = "Latitude must be in bounds (" + std::to_string(lsa::MIN_LATITUDE_BOUNDARY) + ", "
        + std::to_string(lsa::MAX_LATITUDE_BOUNDARY) + ").";
const std::string GuiModel::INVALID_POSC_POLE_MESSAGE = "Having x and y coordinates near zero yields pole singularity.";
const std::string GuiModel::INITIAL_COORDS_FILE_NAME = std::string("initial_coordinates.lsa");
const QString GuiModel::QSETTINGS_SHOWCOMMENTS = "QSETTINGS_SHOWCOMMENTS";
const QString GuiModel::QSETTINGS_VISIBLE_RECORDEDITOR = "QSETTINGS_VISIBLE_RECORDEDITOR";
const QString GuiModel::QSETTINGS_VISIBLE_MAP = "QSETTINGS_VISIBLE_MAP";
const QString GuiModel::QSETTINGS_VISIBLE_ADJUSTEDPOSITIONS = "QSETTINGS_VISIBLE_ADJUSTEDPOSITIONS";
const QString GuiModel::QSETTINGS_VISIBLE_STATIONDATA = "QSETTINGS_VISIBLE_STATIONDATA";
const QString GuiModel::QSETTINGS_VISIBLE_INITIALPOSITIONS = "QSETTINGS_VISIBLE_INITIALPOSITIONS";
const QString GuiModel::QSETTINGS_VISIBLE_HISTOGRAM = "QSETTINGS_VISIBLE_HISTOGRAM";

GuiModel::GuiModel() : solutionNumDOF(0), suppressGuiModelUpdates(false), selectionModel(NULL), confidenceInterval(""), filterString("")
{
    rootItem = new GuiModelItem(QVector<QVariant>());
    lsaFile = new LSAFile(true);

    pointMap = &lsaFile->pointMap;
    hghtMap  = &lsaFile->hghtMap;
    uncrMap  = &lsaFile->uncrMap;
    vscaMap  = &lsaFile->vscaMap;
    dgrpMap  = &lsaFile->dgrpMap;

}

GuiModel::GuiModel(std::string lsaFileName, QObject *parent) : solutionNumDOF(0), confidenceInterval(""), filterString(""),
    suppressGuiModelUpdates(false), selectionModel(NULL), QAbstractItemModel(parent)
{
    lsaFile = new LSAFile(true);
    lsaFile->read(lsaFileName);

    m_stack = new QUndoStack();
    m_stack->setUndoLimit(30);

    pointMap     = &lsaFile->pointMap;
    hghtMap      = &lsaFile->hghtMap;
    uncrMap      = &lsaFile->uncrMap;
    vscaMap      = &lsaFile->vscaMap;
    dgrpMap      = &lsaFile->dgrpMap;

    // Setup the model data to mirror contents of lsaFile
    setupModelData();

    validateModelAndRegenerateMaps();
    setInitialCoordsIndex();

}

GuiModel::~GuiModel()
{
    delete lsaFile;
    delete rootItem;
}

void GuiModel::save()
{
    validateModelAndRegenerateMaps();
    lsaFile->save();
}

// Added to fix bug 582
void GuiModel::saveBranch(QModelIndex includeIndex)
{
    validateModelAndRegenerateMaps();
    LSAInclude *lsaInclude = static_cast<LSAInclude*>(getLSARecord(includeIndex));
    lsaFile->saveLSAIncludeRecord(lsaInclude);
    clearDescendantIncludeIsModified(includeIndex);
}

void GuiModel::clearDescendantIncludeIsModified(QModelIndex includeIndex)
{
    if(!includeIndex.isValid()) return;

    QModelIndex currentIndex = includeIndex;
    LSAInclude *parentInclude = static_cast<LSAInclude*>(getLSARecord(includeIndex));
    parentInclude->setIsModified(false);

    while((currentIndex = getNextDescendant(includeIndex,currentIndex)).isValid())
    {
        LSARecord *lsaRecord = getLSARecord(currentIndex);
        if(lsaRecord->getRecType() == LSAType::INCLUDE)
        {
            LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
            if(lsaInclude->getIsModified())
                lsaInclude->setIsModified(false);
        }
    }
}

QModelIndex GuiModel::descendantIncludeIsModified(QModelIndex includeIndex)
{
    QModelIndex currentIndex = includeIndex;

    LSAInclude *parentInclude = static_cast<LSAInclude*>(getLSARecord(currentIndex));
    if(!parentInclude->getIsModified())
    {
        while((currentIndex = getNextDescendant(includeIndex,currentIndex)).isValid())
        {
            LSARecord *lsaRecord = getLSARecord(currentIndex);
            if(lsaRecord->getRecType() == LSAType::INCLUDE)
            {
                LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
                if(lsaInclude->getIsModified() && !lsaInclude->isAutogenerated)
                    break;
            }
        }
    }

    return currentIndex;
}

void GuiModel::setParentIncludeModified(QModelIndex modifiedChildIndex)
{
    if(isRootInclude(modifiedChildIndex))
    {
        static_cast<LSAInclude*>(getLSARecord(modifiedChildIndex))->setIsModified(true);
    }
    else
    {
        //DEBUG
        LSARecord *lsaRecord = getLSARecord(modifiedChildIndex);
        static_cast<LSAInclude*>(getLSARecord(parent(modifiedChildIndex)))->setIsModified(true);
    }
}
int GuiModel::columnCount(const QModelIndex & /* parent */) const
{
    return rootItem->columnCount();
}

QVariant GuiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    GuiModelItem *item = getItem(index);
    int column = index.column();
    LSARecord* lsaRecord = item->getLSARecord();

    switch(role)
    {
    case Qt::DisplayRole:
    {
        QString retVal = item->data(column).toString();

        if (index == this->index(0,0) && column == 0)
        {
            // Data for the PROJECT row at the top of the tree view is a special case.
            // The filename is likely longer because we display the full path.  As a result,
            // we append the filename to the contents of the first column and make it span the entire row.
            int row = index.row();
            int column = index.column() + 1;
            QModelIndex parent = index.parent();
            GuiModelItem *nextItem = getItem(this->index(row, column, parent));

            retVal += " " + nextItem->data(column).toString();
            retVal.replace("INCLUDE","PROJECT");

            // If there is a project-wide VSCA, list it
            std::string projectVSCALabel = lsaRecord->getModifierLabel(MODKEY_VSCA);
            if ( !projectVSCALabel.empty() )
            {
                QString scaleFactorString = QString::fromStdString(gnsstk::StringUtils::asString(lsaFile->getVSCAValue(lsaRecord),4) );
                retVal += " Scale: " + scaleFactorString;
            }
        }

        return retVal;

        break;
    }
    case Qt::CheckStateRole:
        if (column == 0 && lsaRecord->getRecType() != LSAType::COMMENT && index != this->index(0,0))
        {
            int checkState = lsaRecord->isCommented ? Qt::Unchecked : Qt::Checked;
            return checkState;
        }
        break;
    case Qt::ForegroundRole:
        if (!isActive(index))
        {
            return QColor(Qt::gray);
        }
        else if (lsaRecord->getRecType() == LSAType::COMMENT)
        {
            LSAComment *lsaComment = static_cast<LSAComment*>(lsaRecord);
            std::string contents = lsaComment->lineContents;

            if (lsaComment->hasParseWarnings())
            {
                // Parse warnings are orange in the tree view
                QColor darkOrange(255,140,0);
                return darkOrange;
            }
            else
            {
                // regular comments are blue in the tree view
                return QColor(Qt::blue);
            }
        }
        break;
    case Qt::DecorationRole:
        if (column == 0 && !lsaRecord->warningMessages_generated.empty() && !lsaRecord->isCommented)
        {
            return QIcon(":/guiIcons/alertIcon.png");
        }
        else if (column == 0 && lsaRecord->isAutogenerated && !lsaRecord->isCommented)
        {
            return QIcon(":/guiIcons/autogen.png");
        }
        break;
    case Qt::ToolTipRole:
        if (!lsaRecord->warningMessages_generated.empty()&& !lsaRecord->isCommented )
        {
            std::set<std::string> warnings = lsaRecord->warningMessages_generated;
            std::ostringstream oss;
            for (std::set<std::string>::iterator iter = warnings.begin(); iter != warnings.end(); ++iter)
            {
                oss << *iter << '\n';
            }

            std::string warningMessage = oss.str();
            warningMessage = warningMessage.substr(0, warningMessage.size() - 1); // trim trailing '\n'
            return QString::fromStdString(warningMessage);
        }
        else if (lsaRecord->isAutogenerated && !lsaRecord->isCommented)
        {
            return QString("Auto-generated during least squares adjustment.");
        }
        break;

    }

    return QVariant();
}

LSARecord* GuiModel::getLSARecord(const QModelIndex &index) const
{
    if (!index.isValid())
        return NULL;

     GuiModelItem *item = getItem(index);

    return item->getLSARecord();
}

// NOTE: this method can be expensive.  Try to use getLSARecord(index) instead.
QModelIndex GuiModel::getIndexForRecord(LSARecord* lsaRecord) const
{
    auto iter = m_LSARecordToQModelIndexMap.find(lsaRecord);
    if(iter != m_LSARecordToQModelIndexMap.end())
    {
        if(iter->second.isValid())
        {
            return iter->second;
        }
    }

    QModelIndex index = getNextIndex();

    while(index.isValid() && getLSARecord(index) != lsaRecord)
    {
        index = getNextIndex(index);
    }

    return index;
}

Qt::ItemFlags GuiModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    Qt::ItemFlags retVal;
    int column = index.column();
    if (column == 0)
    {
        retVal = Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | QAbstractItemModel::flags(index);
    }
    else
    {
        retVal = Qt::ItemIsSelectable | QAbstractItemModel::flags(index);
    }

    return retVal | Qt::ItemIsDropEnabled;
}

GuiModelItem *GuiModel::getItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        GuiModelItem *item = static_cast<GuiModelItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

QVariant GuiModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return rootItem->data(section);
    }

    return QVariant();
}

QModelIndex GuiModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    GuiModelItem *parentItem = getItem(parent);

    GuiModelItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

bool GuiModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool GuiModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    GuiModelItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

QModelIndex GuiModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    GuiModelItem *childItem = getItem(index);
    GuiModelItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool GuiModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool GuiModel::removeRows(int position, int numRows, const QModelIndex &parent)
{
    GuiModelItem *parentItem = getItem(parent);
    bool success = true;

    if(numRows>0)
    {
        beginRemoveRows(parent, position, position + numRows - 1);
        success = parentItem->removeChildren(position, numRows);
        endRemoveRows();
    }

    return success;
}

int GuiModel::rowCount(const QModelIndex &parent) const
{
    GuiModelItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

QModelIndex GuiModel::insertSiblingAfter(LSAType recordType, std::string line, QModelIndex index)
{
    // If we weren't passed an index, use the currently selected index
    if (!index.isValid())
    {
        index = getSingleSelectedRecord();
    }

    // Don't attempt to insert if index is stil invalid
    if (!index.isValid())
    {
        return QModelIndex();
    }

    // Insert the new row into the GuiModel
    QModelIndex parentIndex = parent(index);
    int rowPosition = index.row()+1;         // row index where we will insert the new record

    return insertNewRecord(parentIndex, rowPosition, recordType, line);
}

QModelIndex GuiModel::insertFirstChild(LSAType recordType, std::string line, QModelIndex parentIndex, bool isAutogenerated)
{
    // If we weren't passed an index, use the currently selected index
    if (!parentIndex.isValid())
    {
        parentIndex = getSingleSelectedRecord();
    }

    // Don't attempt to insert if index is stil invalid
    if (!parentIndex.isValid())
    {
        return QModelIndex();
    }

    int rowPosition = 0;
    return insertNewRecord(parentIndex, rowPosition, recordType, line, isAutogenerated);
}

QModelIndex GuiModel::insertLastChild(LSAType recordType, std::string line, QModelIndex parentIndex, bool isAutogenerated)
{
    DebugTimer timer(false);
    // If we weren't passed an index, use the currently selected index
    if (!parentIndex.isValid())
    {
        parentIndex = getSingleSelectedRecord();
    }

    // Don't attempt to insert if index is stil invalid
    if (!parentIndex.isValid())
    {
        return QModelIndex();
    }

    int numChildren = getItem(parentIndex)->childCount();
    int rowPosition = numChildren;

    return insertNewRecord(parentIndex, rowPosition, recordType, line, isAutogenerated);
}

void GuiModel::insertFromStreamAfter(std::stringstream& ss, QModelIndex index, int row)
{
    LSARecord* parentRecord = getLSARecord(index);

    // Qt freaks out if we give it a row < 0.  If this is the root and we have row of -1
    // make it the last row in the tree. This case happens when the users drops outside of the tree
    if(index == this->index(0,0) && row == -1)
    {
        LSAInclude* rootInclude = static_cast<LSAInclude*>(parentRecord);
        row = rootInclude->childRecords.size();
    }

    // Get the appropriate LSAInclude, so that we put it in the right include internally
    // pareseLSAStream defaults to putting it in the rootInclude if we pass in nullptr
    LSAInclude *lsaInclude = nullptr;
    if(parentRecord->getRecType() == LSAType::INCLUDE)
    {
        lsaInclude = static_cast<LSAInclude*>(parentRecord);
    }

    // Get a copy of the badRecords cache on the lsaFile
    int numOldBadRecords = lsaFile->getErrorsAndWarnings().size();

    QList<LSARecord*> newRecords;
    lsaFile->parseLSAStream(ss, lsaInclude, true, row, &newRecords);

    // Push any new parse errors to the gui
    std::vector<std::string> newBadRecords = lsaFile->getErrorsAndWarnings();
    int numNewBadRecords = newBadRecords.size();
    if (numNewBadRecords > numOldBadRecords)
    {
        for (size_t i = numOldBadRecords; i < numNewBadRecords; ++i)
        {
            pushGuiWarning(QString::fromStdString(newBadRecords[i]) );
        }
    }

    insertRecordsPost(newRecords, index, row, {} );
}

QModelIndexList GuiModel::insertNewRecords(QModelIndex parentIndex, int rowPosition, QVector<LSARecord*> lsaRecordList, bool isUndoRedoAction)
{
    DebugTimer timer(false);
    QList<LSAType> recordTypes;
    QList<std::string> recordStrings;
    QList<bool> isAutogeneratedList;
    QList<bool> isCommentedList;

    foreach(LSARecord* lsaRecord, lsaRecordList)
    {
        recordTypes.append(lsaRecord->getRecType());
        isAutogeneratedList.append(lsaRecord->isAutogenerated);
        isCommentedList.append(lsaRecord->isCommented);
        std::string recordString = lsaRecord->getSingleLSAString();
        if(lsaRecord->getRecType() == LSAType::INCLUDE)
        {
            recordString.erase(0, recordString.find(" ") + 1); // Remove "(#)--include "
        }
        recordStrings.append(recordString);
    }
    LOG_TIME(timer, "Prepare to insert");

    QModelIndexList indexList = insertNewRecords(parentIndex, rowPosition, recordTypes, recordStrings, isAutogeneratedList, isCommentedList, isUndoRedoAction);

    LOG_TIME(timer, "Insert the records");

    return indexList;

}

//This was added as a fix to Bug #1088 - Currently only works if all records are of same type.  But, currently only used to insert block of POSGs for autogen include.
QModelIndexList GuiModel::insertNewRecords(QModelIndex parentIndex, int rowPosition, QList<LSAType> recordTypes, QList<std::string> recordStrings, QList<bool> isAutogeneratedList, QList<bool> isCommentedList, bool isUndoRedoAction)
{
    DebugTimer timer(false);

    if (!parentIndex.isValid())
        parentIndex = getSingleSelectedRecord();

    // Don't attempt to insert if index is stil invalid
    if (!parentIndex.isValid())
        return QModelIndexList();

    int numRecordsToInsert = recordStrings.size();

    // Get a copy of the badRecords cache on the lsaFile
    int numOldBadRecords = lsaFile->getErrorsAndWarnings().size();

    // Get the LSAInclude for parentIndex
    GuiModelItem* parentItem = getItem(parentIndex);
    bool parentIsActive = isActive(parentIndex);
    LSARecord *parentRecord = parentItem->getLSARecord();
    LSAInclude* parentInclude = static_cast<LSAInclude*>(parentRecord);
    LOG_TIME(timer, "Prepare to insert");

    //loop over record of the block to be inserted
    QList<LSARecord*> lsaRecordList;
    for(int i=0; i<numRecordsToInsert;i++)
    {
        std::string line = recordStrings.at(i);
        LSAType recordType = recordTypes.at(i);
        bool isCommented = isCommentedList.at(i);
        bool isAutogenerated = isAutogeneratedList.at(i);
        int row = rowPosition + i;
        LSARecord  *newLSARecord = NULL;

        // Insert the new record
        if (recordType == LSAType::COMMENT)
        {
            std::string newCommentContents;
            if(line.empty())
                newCommentContents = "#New comment";
            else
                newCommentContents = line;

            newLSARecord = lsaFile->insertNewRecord(recordType, newCommentContents, parentInclude, isCommented, parentIsActive, row);
        }
        else if(line.empty())
            newLSARecord = lsaFile->insertNewRecord(recordType, line, parentInclude, isCommented, parentIsActive, row);
        else
            newLSARecord = lsaFile->insertNewRecord(recordType, line, parentInclude, isCommented, parentIsActive, row, isAutogenerated);

        if (newLSARecord == NULL)
            continue;
        else
            lsaRecordList.append(newLSARecord);
        //LOG_TIME(timer,"insert one record in lsa tree");
    }
    LOG_TIME(timer, "After inserting records in lsa tree.");

    // Push any new parse errors to the gui
    std::vector<std::string> newBadRecords = lsaFile->getErrorsAndWarnings();
    int numNewBadRecords = newBadRecords.size();
    if (numNewBadRecords > numOldBadRecords)
    {
        for (size_t i = numOldBadRecords; i < numNewBadRecords; ++i)
        {
            pushGuiWarning(QString::fromStdString(newBadRecords[i]) );
        }
    }
    LOG_TIME(timer,"Push parse warnings to gui");

    QModelIndexList insertedRecords = insertRecordsPost(lsaRecordList, parentIndex, rowPosition, isAutogeneratedList);

    // This check is avoid creating a new undo/redo object when this method is called by an undo/redo object
    if(!isUndoRedoAction)
    {
        QList<QPersistentModelIndex> insertedIndices;
        for(auto& index : insertedRecords)
        {
            insertedIndices.append(index);
        }
        m_stack->push(new InsertRecordsCommand(parentIndex, insertedIndices, rowPosition, recordTypes, recordStrings, isAutogeneratedList, isCommentedList, this));
    }

    return insertedRecords;
}

QModelIndexList GuiModel::insertRecordsPost(QList<LSARecord *> newRecords, QModelIndex parentIdx, int rowPosition, QList<bool> isAutogeneratedList)
{

    GuiModelItem* parentItem = getItem(parentIdx);
    LSARecord *parentRecord = parentItem->getLSARecord();
    LSAInclude* parentInclude = static_cast<LSAInclude*>(parentRecord);
    QModelIndexList newlyInsertedRecords;
    if(newRecords.size()>0)
    {
        parentInclude->setIsModified(true);

        // Create the corresponding new records in the GuiModel
        beginInsertRows(parentIdx, rowPosition, rowPosition + newRecords.size() - 1);

        {
            int insertCounter = 0;
            foreach(LSARecord* lsaRecord, newRecords)
            {

                int currentRowNum = rowPosition + insertCounter;
                ++insertCounter;

                int numRowsToInsert = 1;
                int numColumnsToInsert = lsaRecord->getNumDisplayColumns();
                parentItem->insertChildren(currentRowNum, numRowsToInsert, numColumnsToInsert);

                // Set the LSARecord on the new item
                parentItem->child(currentRowNum)->setLSARecord(lsaRecord); // set the pointer to the lsaRecord

                // Synch the newly inserted gui item to it's lsaRecord
                bool oldState = suppressGuiUpdates(true);
                for (int col = 0; col < numColumnsToInsert; ++col)
                {
                    QModelIndex childIndex = index(currentRowNum, col, parentIdx);
                    synchItemToRecord(childIndex);
                    if (col == 0)
                    {
                        newlyInsertedRecords << childIndex;
                    }
                }
                suppressGuiUpdates(oldState);

                //LOG_TIME(timer,"Insert one record into gui model");
            }
        }
        endInsertRows();// QAbstractItemModel method that must be called after insert

        // if we inserted a new INCLUDE update the GuiModel tree with any newly included child records
        for(int i=0;i<newRecords.size();i++)
        {
            int row = rowPosition + i;
            LSARecord *lsaRecord = newRecords.at(i);

            if (lsaRecord->getRecType() == LSAType::INCLUDE)
            {
                QModelIndex newIncludeIndex = index(row,0, parentIdx);
                double newIncludeVSCAFactor = getNetVSCAFactor(newIncludeIndex);
                setupModelDataRecurse(newIncludeIndex, parentItem->child(row), newIncludeVSCAFactor);

                cleanUpVSCAToParentTags(newIncludeIndex);
            }
        }

        // Select the last newly inserted records
        bool isAutogenerated = false;
        foreach (bool itemIsAutogen, isAutogeneratedList)
        {
            if (itemIsAutogen)
            {
                isAutogenerated = true;
                break;
            }
        }

        if (!isAutogenerated && newlyInsertedRecords.size() > 0)
        {
            select(newlyInsertedRecords, true);
        }
    }
    return newlyInsertedRecords;
}

QModelIndex GuiModel::insertNewRecord(QModelIndex parentIndex, int rowPosition, LSAType recordType, std::string line, bool isAutogenerated, bool isCommented, bool isUndoRedoAction)
{
    QList<LSAType> recordTypes;
    QList<std::string> recordStrings;
    QList<bool> isAutogeneratedList;
    QList<bool> isCommentedList;

    recordTypes.append(recordType);
    recordStrings.append(line);
    isAutogeneratedList.append(isAutogenerated);
    isCommentedList.append(isCommented);

    QModelIndexList insertedRecords = insertNewRecords(parentIndex, rowPosition, recordTypes, recordStrings, isAutogeneratedList, isCommentedList, isUndoRedoAction);

    if (insertedRecords.isEmpty())
        return QModelIndex();

    return insertedRecords.first();
}

void GuiModel::cleanUpVSCAToParentTags(QModelIndex parentIndex)
{
    DebugTimer timer(false);

    LSARecord* parentRecord = getLSARecord(parentIndex);
    if (parentRecord->getRecType() != LSAType::INCLUDE)
    {
        return;
    }

    LSAInclude *parentInclude = static_cast<LSAInclude*>(parentRecord);
    std::list<LSARecord*> *children = &parentInclude->childRecords;
    LOG_TIME(timer, "Get Children");

    bool parentTagFound = false;
    for (std::list<LSARecord*>::iterator iter = children->begin(); iter != children->end(); ++iter)
    {
        LSARecord *lsaRecord = *iter;
        if (lsaRecord->getRecType() == LSAType::VSCA)
        {
            LOG_TIME(timer, "child found");
            LSAVarScaling* lsaVSCA = static_cast<LSAVarScaling*>(lsaRecord);
            if (lsaVSCA->isAppliedToParent)
            {
                Q_ASSERT(!parentTagFound); // if parentTagFound == true here, then there are multiple VSCA records applied to the same parent

                if (!parentTagFound) // only apply the first record found
                {
                    std::string vscaLabel = lsaVSCA->getLabel();
                    parentInclude->modifiers.setVSCAData(vscaLabel);

                    // Remove the to parent tag unless we are working with the root include
                    if (parentIndex != this->index(0,0))
                    {
                        lsaVSCA->isAppliedToParent = false;
                    }

                    parentTagFound = true;
                }
                else
                {
                    Q_ASSERT(false);  // Duplicate VSCA toParent tags found
                    lsaVSCA->isAppliedToParent = false;
                }

            }
            LOG_TIME(timer, "child processed");
        }
    }
    LOG_TIME(timer,"Check for vsca children");
}

//return index of the record before the one deleted
QModelIndex GuiModel::removeRecord(QModelIndex removeIndex, bool deleteReferences, bool moving_initial_coords, bool isUndoRedoAction, QVector<QVector<IndexRowColumnParentChainItem>> removedIndicesTree)
{
    // This check is avoid creating a new undo/redo object when this method is called by an undo/redo object
    if(!isUndoRedoAction)
    {
        m_stack->push(new RemoveRecordsCommand(removeIndex, deleteReferences, moving_initial_coords, removedIndicesTree, this));
    }

    // If we are deleting a direction group, save the label to use when deleting member directions
    std::string labelToRemove  = "";

    LSARecord* lsaRecord = getLSARecord(removeIndex);
    LSAType currentType = lsaRecord->getRecType();

    if (currentType == LSAType::DGRP)
    {
        LSADirGroup *lsaDirGroup = static_cast<LSADirGroup*>(lsaRecord);
        labelToRemove = lsaDirGroup->label;
    }
    else if (currentType.isModifier())
    {
        labelToRemove = lsaRecord->getLabel();
    }

    // Identify a visible record to select after deletion(s) are complete
    //prevent current and selected index getting out of synch (Bug #894)
    int rowPosition = removeIndex.row();
    QModelIndex indexToSelect = getPreviousParentIndex(removeIndex);
    while(indexToSelect.isValid() && isRecordHidden(indexToSelect))
    {
        QModelIndex previousIndex = indexToSelect;
        indexToSelect = getPreviousParentIndex(previousIndex);
    }
    if(!indexToSelect.isValid())
        indexToSelect = index(0,0);//select the root node

    // Save a pointer to the previousRecord so we can select it later
    QPersistentModelIndex persistentIndex(indexToSelect);

    if(!removeIndex.isValid())
    {
        return QModelIndex(persistentIndex);
    }

    m_LSARecordToQModelIndexMap.erase(lsaRecord);

    // Get the parent record
    QModelIndex parentIndex = parent(removeIndex);
    LSAInclude *parentInclude = static_cast<LSAInclude*>( getLSARecord(parentIndex) );

    // Don't allow removal of the first include record at the top of the tree.
    // rootItem is the first GuiModelItem in the tree.  It contains the treeview header info.
    // The first (root) include record is the first child of the rootItem.
    GuiModelItem* firstInclude = rootItem->child(0);
    if (getItem(removeIndex) == firstInclude)
    {
        return QModelIndex(persistentIndex);
    }

    //fix to Bug #1372
    bool removeIsAutogenerated = false;
    if(currentType == LSAType::INCLUDE)
    {
        LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
        if(lsaInclude->isAutogenerated)
            removeIsAutogenerated = true;
    }
    if(!removeIsAutogenerated)
        parentInclude->setIsModified(true);

    //remove children of removed record
    if(currentType == LSAType::INCLUDE)
    {
        // Remove the children in the lsa tree
        int numChildren = getItem(removeIndex)->childCount();
        LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
        for(int row=numChildren-1;row>=0;row--)
        {
            lsaFile->removeRecord(lsaInclude, row);
        }

        // Remove the children in the gui model
        removeRows(0,numChildren,removeIndex);

        //Fix to Bug #964 and Issue #1236
        if((lsaInclude->getLSAPath() == INITIAL_COORDS_FILE_NAME) && !moving_initial_coords)
        {
            std::string fileName(INITIAL_COORDS_FILE_NAME);
            include_lsapath(getProjectDirectory(),fileName);
            QFileInfo initCoordInfo(QString::fromStdString(fileName));
            if(initCoordInfo.exists())
            {
                QFile initialCoordFile(QString::fromStdString(fileName));
                initialCoordFile.remove();
            }
        }
    }

    // Delete the LSARecord corresponding to index
    lsaFile->removeRecord(parentInclude, rowPosition);

    // Remove the GuiModelItem at index
    int numRowsToRemove = 1;
    removeRows(rowPosition,numRowsToRemove,parentIndex);

    // Delete any HDIR records that were members of the DGRP just deleted
    if (deleteReferences)
    {
        if (currentType == LSAType::DGRP)
            removeMembersOfDirGroup(labelToRemove);
        else if (currentType.isModifier())
            renameModifierLabel(currentType, labelToRemove, "");
        else
            Q_ASSERT(false); // we should never reach this point
    }

    if (!suppressGuiModelUpdates && persistentIndex.isValid())
    {
        select(persistentIndex);
        selectionModel->setCurrentIndex(persistentIndex,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    // If we deleted an INCLUDE or VSCA record, resynch the gui model to refresh scale values in the tree view
    if (currentType == LSAType::INCLUDE || currentType == LSAType::VSCA)
    {
        synchGuiModelToLSATree();
    }

    //TODO - This is the place to add Undo/Redo tracking

    return QModelIndex(persistentIndex);
}

int GuiModel::getNumReferencesToModifier(LSAType lsaType, std::string label)
{
    QModelIndex currentIndex = getNextIndex();
    int numReferences = 0;

    while(currentIndex.isValid())
    {
        if (currentIndex.column() == 0)
        {
            LSARecord* lsaRecord = getLSARecord(currentIndex);
            if (lsaRecord->referencesModifier(lsaType, label))
            {
                ++numReferences;
            }
        }

        currentIndex = getNextIndex(currentIndex);
    }

    return numReferences;
}

bool GuiModel::hasDuplicateTypeAndLabel(QModelIndex index)
{
    if (!index.isValid()) return false;
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();
    std::string label = lsaRecord->getLabel();

    int numRecords = 0;

    QModelIndex currentIndex = getNextIndex();

    while(currentIndex.isValid())
    {
        if (currentIndex.column() == 0)
        {
            LSARecord* lsaRecord = getLSARecord(currentIndex);
            if (lsaRecord->getLabel() == label && lsaRecord->getRecType() == lsaType)
            {
                numRecords++;
            }
        }

        currentIndex = getNextIndex(currentIndex);
    }

    // If we have more than one record with the same type and record, we have a duplicate
    return numRecords > 1;
}

void GuiModel::setFilterLogic(const std::vector<LSAType> &inputTypes, const std::function<bool (std::vector<LSAType>, LSARecord *)> &logicalFunc)
{
    filterTypes = inputTypes;
    filterLogic = logicalFunc;
}

void GuiModel::removeMembersOfDirGroup(std::string dirGroupLabel)
{
    bool recordFound = true;

    while(recordFound)
    {
        recordFound = removeSingleMemberOfDirGroup(dirGroupLabel);
    }

    return;
}

bool GuiModel::removeSingleMemberOfDirGroup(std::string dirGroupLabel)
{
    bool recordDeleted = false;

    QModelIndex index = getNextIndex();
    while (!recordDeleted && index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        if (lsaRecord->getRecType() == LSAType::HDIR)
        {
            LSAHDir* lsaHDir = static_cast<LSAHDir*>(lsaRecord);
            if (lsaHDir->dirGroupLabel == dirGroupLabel)
            {
                removeRecord(index);
                recordDeleted = true;
                break; // we just deleted index, so calling getNextIndex(index) isn't a good idea
            }
        }
        index = getNextIndex(index);
    }

    return recordDeleted;
}

QStringList splitIntoMultiTermSearch(QString searchString)
{
    QStringList searchTerms;
    // Crawl along the string to get the quoted search terms.
    int leftQuoteLoc = searchString.indexOf("\"");
    if(leftQuoteLoc >= 0)
    {
        int afterLeftQuote = leftQuoteLoc + 1;
        int quoteLoc = searchString.indexOf("\"", afterLeftQuote);
        while(quoteLoc > 0)
        {
            QString quoted = searchString.mid(afterLeftQuote, quoteLoc - afterLeftQuote);
            searchTerms.push_back(quoted);
            searchString = searchString.remove(leftQuoteLoc, (quoteLoc - leftQuoteLoc) + 1);
            leftQuoteLoc = quoteLoc;
            quoteLoc = searchString.indexOf("\"", afterLeftQuote);
        }
    }
    searchTerms.append(searchString.split(" ", Qt::SkipEmptyParts));
    return searchTerms;
}

// Static utility method for finding searchString in lsaString (e.g. from a row in the tree view). bExact = false will search like substr.
// bMatchAll = true will only return if all search terms are matched. if bMatchAll = false, will return if any of the search terms are matched.
bool GuiModel::findInString(const QString& lsaString, const QString& searchString, bool bExact /*= false*/, bool bMatchAll /*= false*/)
{
    if(searchString.isEmpty())
    {
        return true;
    }
    QStringList searchTerms = splitIntoMultiTermSearch(searchString);
    bool bMatch = false;
    for(const auto& term : qAsConst(searchTerms))
    {
        QRegExp searchRegex(bExact ? "\\b" % term % "\\b" : term);
        searchRegex.setCaseSensitivity(Qt::CaseInsensitive);
        bMatch = lsaString.contains(searchRegex);
        if(bMatchAll && !bMatch)
        {
            return false;
        }
        else if (!bMatchAll && bMatch)
        {
            return true;
        }
    }
    return bMatch;
}

bool GuiModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    return true;
}

bool GuiModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parentIndex)
{
    emit signalDataDroppedOnTree(data, action, row, column, parentIndex);

    return true;
}

bool GuiModel::isRecordHidden(const QModelIndex& index)
{
    if (!index.isValid())
        return false;

    LSARecord *lsaRecord = getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();

    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    bool showCommentsIsChecked = qsettings.value( QSETTINGS_SHOWCOMMENTS, true).toBool();

    // Hide all blank comments, show other comments if the show comment option is checked
    bool recordIsHidden = false;

    // Apply filtering
    std::string lsaString = lsaRecord->getSingleLSAString();

    // If there's actaully a string to look for and it's not in the line...
    if (!filterString.empty() && !findInString(QString::fromStdString(lsaString), QString::fromStdString(filterString), filterExact, bMatchAllFilter))
    {
        recordIsHidden = true;
        // Fix for issue #234
        // Make sure to show HDIRs who's DGRPs are shown after filter is applied.
        if(lsaType == LSAType::HDIR)
        {
            LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);
            // Get the DGRP's LSARecord
            std::string dgrpLabel = lsaHDir->dirGroupLabel;
            LSADirGroup *dirG = getDirGroup(dgrpLabel);
            LSARecord *dgrpRecord = static_cast<LSARecord*>(dirG);
            // Dont hide the HDIR if the DGRP would be visible
            recordIsHidden = isRecordHidden(getIndexForRecord(dgrpRecord));
        }
    }
    if(filterLogic)
    {
        bool filterBoxVisible = filterLogic(filterTypes, lsaRecord);
        if(filterTypes.size() != 0 && !filterBoxVisible)
            recordIsHidden = true;
    }

    if (lsaType == LSAType::COMMENT)
    {
        LSAComment* lsaComment = static_cast<LSAComment*>(lsaRecord);
        if (lsaComment->lineContents.empty())
        {
            // Always hide blank comments
            recordIsHidden = true;
        }
        else if (!showCommentsIsChecked)
        {
            // Hide all comments if Preferences->ShowComments is not checked
            recordIsHidden = true;
        }
    }

    if(indexesToCut.contains(index))
    {
        recordIsHidden = true;
    }

    return recordIsHidden;
}

bool GuiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //DebugTimer timer(false);

    //LOG_TIME(timer, "Beginning of GuiModel::setData");
    if (!index.isValid())
    {
        return false;
    }


    GuiModelItem *item = getItem(index);
    int column = index.column();
    //LOG_TIME(timer, "After getItem in GuiModel::setData");

    switch(role)
    {
    case Qt::EditRole:
        item->setData(column, value);
        break;
    case Qt::CheckStateRole:
        if (column == 0)
        {
           LSARecord *lsaRecord = item->getLSARecord();
           LSAType lsaType = lsaRecord->getRecType();
           std::string label = lsaRecord->getLabel();
           std::string oldValue, newValue;
           QList<QPersistentModelIndex> selectedIndices;
           selectedIndices.append(index);

           if (value == Qt::Checked)
           {
               oldValue = lsaRecord->getSingleLSAString();
               lsaRecord->isCommented = false;
               newValue = lsaRecord->getSingleLSAString();
               pushChangeValue(index, selectedIndices, oldValue, newValue, false);
//               modelIsSaved = false;
               setParentIncludeModified(index);

               if (lsaType == LSAType::VSCA)
               {
                   // insert the newly enabled vsca into the varMap
                   QModelIndex index = getIndexForRecord(lsaRecord);
                   if (isActive(index))
                   {
                       LSAVarScaling* lsaVarPtr = static_cast<LSAVarScaling*>(lsaRecord);
                       vscaMap->insert(std::pair<std::string, LSAVarScaling*>(label, lsaVarPtr));
                   }
               }

           }
           else if (value == Qt::Unchecked)
           {
               if (lsaType != LSAType::COMMENT)
               {
                   oldValue = lsaRecord->getSingleLSAString();
                   lsaRecord->isCommented = true;
                   newValue = lsaRecord->getSingleLSAString();
                   pushChangeValue(index, selectedIndices, oldValue,newValue, false);
//                   modelIsSaved = false;
                   setParentIncludeModified(index);
               }

               if (lsaType == LSAType::VSCA)
               {
                   // remove the newly disabled vsca from the varMap;
                   LSAVSCAMap::iterator iter = vscaMap->find(label);
                   if (iter != vscaMap->end() ) vscaMap->erase(iter);
               }
           }
        }
        break;
    }
    //LOG_TIME(timer, "After switch statement in GuiModel::setData");

    if (!suppressGuiModelUpdates)
    {
        QVector<int> roles;
        roles << role;
        emit dataChanged(index, index, roles);
        //LOG_TIME(timer, "After emission of dataChanged signal");
    }

    return true;
}

void GuiModel::setMultipleData(const QList<QPersistentModelIndex> &indexList, const QVariant &value, int role)
{
    bool oldSuppressState = suppressGuiModelUpdates;
    suppressGuiModelUpdates = true;
    foreach(QModelIndex index, indexList)
    {
        setData(index, value, role);
        setParentIncludeModified(index);
    }

    suppressGuiModelUpdates = oldSuppressState;
    if(!suppressGuiModelUpdates)
    {
        QVector<int> roles(indexList.length(), role);
        emit dataChanged(indexList[0], indexList[indexList.length() - 1], roles);
    }
}

void GuiModel::setCheckStateData(const QModelIndex &index, const QVariant &value)
{
    GuiModelItem *item = getItem(index);
    LSARecord *lsaRecord = item->getLSARecord();
    LSAType lsaType = lsaRecord->getRecType();
    std::string label = lsaRecord->getLabel();

    if (value == Qt::Checked)
    {
        lsaRecord->isCommented = false;

        if (lsaType == LSAType::VSCA)
        {
            // insert the newly enabled vsca into the varMap
            QModelIndex index = getIndexForRecord(lsaRecord);
            if (isActive(index))
            {
                LSAVarScaling* lsaVarPtr = static_cast<LSAVarScaling*>(lsaRecord);
                vscaMap->insert(std::pair<std::string, LSAVarScaling*>(label, lsaVarPtr));
            }
        }

    }
    else if (value == Qt::Unchecked)
    {
        lsaRecord->isCommented = true;

        if (lsaType == LSAType::VSCA)
        {
            // remove the newly disabled vsca from the varMap;
            LSAVSCAMap::iterator iter = vscaMap->find(label);
            if (iter != vscaMap->end() ) vscaMap->erase(iter);
        }
    }
}

bool GuiModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

bool GuiModel::itemIsComment(QModelIndex index)
{
    if (!index.isValid())
    {
        return false;
    }

    LSAType lsaType = getLSARecord(index)->getRecType();
    return lsaType == LSAType::COMMENT;
}

bool GuiModel::itemIsBlankComment(QModelIndex index)
{
    if (!index.isValid())
    {
        return false;
    }
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();

    if (lsaType == LSAType::COMMENT)
    {
        LSAComment* lsaComment = static_cast<LSAComment*>(lsaRecord);
        if (lsaComment->lineContents.empty())
        {
            return true;
        }
    }

    return false;
}

bool GuiModel::parentAndChildSelected()
{
    bool parentAndChildSelected = false;
    QModelIndexList selectedRows = getSelectedRows();

    // For every selected include, check if one of its children are selected
    foreach (QModelIndex parentIndex, selectedRows)
    {
        if (!parentIndex.isValid()) continue;
        if (getLSARecord(parentIndex)->getRecType() != LSAType::INCLUDE) continue;

        GuiModelItem *parentItem = getItem(parentIndex);
        // We found a selected include, so see if one of its children is in the list of selected rows
        foreach (QModelIndex childIndex, selectedRows)
        {
            if (childIndex == parentIndex) continue;
            GuiModelItem *childItem = getItem(childIndex);

            if ( childItem->isDescendantOfItem(parentItem) )
            {
                parentAndChildSelected= true;
                break;
            }
        }
    }

    return parentAndChildSelected;
}

void GuiModel::synchSelectedItemsToRecords()
{
    QModelIndexList indexes = selectionModel->selection().indexes();

    bool oldState = suppressGuiUpdates(true);
    {
        foreach (QModelIndex index, indexes)
        {
            synchItemToRecord(index);
        }
    }
    suppressGuiUpdates(oldState);

    if ( !indexes.empty() )
    {
        emit dataChanged( indexes.first(), indexes.last() );
    }

    return;

}


void GuiModel::setupModelData()
{
    // Create the first tree item that contains the headers for the tree view
    QVector<QVariant> headers;
    headers << "Record" << "Summary" << "Scale" << "Details" << "Modifiers";
    rootItem = new GuiModelItem(headers);

    LSARecord *rootRecord = lsaFile->getRootRecord();
    LSAInclude *rootIncludeRecord = static_cast<LSAInclude*>(rootRecord);
    std::string filename = rootIncludeRecord->getLSAPath();

    int numCols = rootRecord->getNumDisplayColumns();
    // Create the first LSAInclude tree item to contain the root lsa file
    rootItem->insertChildren(0, 1, numCols);                     // insert the rootIncludeRecord
    GuiModelItem* rootIncludeRec = rootItem->child(0);
    rootIncludeRec->setData(0, QString::fromStdString("PROJECT   " + filename));
    rootIncludeRec->setLSARecord(lsaFile->getRootRecord() );     // set the pointer to the root lsaRecord

    // Setup all the children
    double rootIncludeVSCAFactor = 1.0;
    QModelIndex rootIndex = getNextIndex();
    setupModelDataRecurse(rootIndex, rootIncludeRec, rootIncludeVSCAFactor);

    // Apply a vscaToParent to the root include if needed
    cleanUpVSCAToParentTags(this->index(0,0));

    return;
}

void GuiModel::setupModelDataRecurse(QModelIndex parentIndex, GuiModelItem *parentItem, double groupVSCAFactor)
{
    DebugTimer timer(false);

    bool oldState = suppressGuiUpdates(true);
    {
        //get the child records
        GuiModelItem* parent = getItem(parentIndex);
        LSAInclude *parentInclude = static_cast<LSAInclude*>(parent->getLSARecord());
        std::list<LSARecord*> &childLSARecords = parentInclude->childRecords;

        int numRows = 1;// always insert one child at a time

        int numChildren = childLSARecords.size();

        int newChildNumber;
        GuiModelItem* newChild;

        // Iterate through all the children for this parent
        if (numChildren > 0)
        {
            beginInsertRows(parentIndex,0,numChildren-1);
            LOG_TIME(timer, "begin insert");
            {
                for (std::list<LSARecord*>::iterator iter = childLSARecords.begin(); iter != childLSARecords.end(); ++iter )
                {
                    // Get the current child and its type
                    LSARecord* currentLSARecord = *iter;
                    int numCols = currentLSARecord->getNumDisplayColumns();

                    // Insert the new child
                    newChildNumber = parentItem->childCount();
                    {
                        parentItem->insertChildren(newChildNumber, numRows, numCols);
                        LOG_TIME(timer, "Insert GuiModelItem");

                         // Populate values on the new child
                        newChild = parentItem->child(parentItem->childCount() - 1);
                        newChild->setLSARecord(currentLSARecord); // set the pointer to the lsaRecord
                        for (int i = 0; i < numCols; ++i)
                        {
                            if (i == 2)
                            {
                                QVariant vscaData = QString::fromStdString(gnsstk::StringUtils::asString(groupVSCAFactor * getVSCAValue(currentLSARecord), 3));
                                newChild->setData(i, vscaData);
                            }
                            else
                            {
                                newChild->setData(i, QString::fromStdString(currentLSARecord->getUIColumnData(i)));
                            }
                        }

                        // If the currentLSARecord is a LSAInclude, add its child data to the tree
                        if (currentLSARecord->getRecType() == LSAType::INCLUDE)
                        {
                          double childIncludeGroupVSCAFactor = groupVSCAFactor * lsaFile->getVSCAValue(currentLSARecord);
                          QModelIndex childIndex = index(newChildNumber, 0, parentIndex);
                          setupModelDataRecurse(childIndex, newChild, childIncludeGroupVSCAFactor);
                        }

                        LOG_TIME(timer, "Set values on GuiModelItem");
                    }
                    LOG_TIME(timer, "end insert");
                }
            }
            endInsertRows();
        }

//        // Clean up any vscaToParent tags on children
//        int numChildren = parentItem->childCount();
//        for (int row = 0; row < numChildren; ++row)
//        {
//            QModelIndex index = this->index(row, 0, parentIndex);
//            cleanUpVSCAToParentTags(index);
//        }
    }
    suppressGuiUpdates(oldState);
}

void GuiModel::synchGuiModelToLSATree()
{
    DebugTimer timer(false);
    bool oldState = suppressGuiUpdates(true);
    QModelIndex index = getNextIndex();
    LOG_TIME(timer, "Start of GuiModel::synchGuiModelToLSATree main loop");
    while(index.isValid())
    {
        int row = index.row();
        int numCols = getLSARecord(index)->getNumDisplayColumns();
        for (int col = 0; col < numCols; ++col)
        {
            synchItemToRecord(this->index(row,col,index.parent()));
        }
        index = getNextIndex(index);
    }
    LOG_TIME(timer, "Start of GuiModel::synchGuiModelToLSATree main loop");
    suppressGuiUpdates(oldState);
}

void GuiModel::synchItemToRecord(QModelIndex index)
{

    Q_ASSERT(index.isValid());
    if (!index.isValid()) return;

    LSARecord *lsaRecord = getLSARecord(index);

    if(index.column() == 0)
    {
        m_LSARecordToQModelIndexMap[lsaRecord] = QPersistentModelIndex(index);
    }

    Q_ASSERT(lsaRecord != NULL );
    if (lsaRecord == NULL) return;

    QVariant newData;
    if (2 == index.column() )
    {
        LSAModifier *modifier = lsaRecord->getModifiers();
        if (modifier != NULL && modifier->getVSCASupport() )
        {
            newData = QString::fromStdString( gnsstk::StringUtils::asString(getNetVSCAFactor(index),3));
        }
    }
    else
    {
        LSAType lsaType = lsaRecord->getRecType();
        if (lsaType == LSAType::HDIR && index.column() == 1)
        {
            // For the Summary column of a HDIR record, we need to look up the FROM station for the parent DGRP
            // NOTE: This overrides LSAHDir::getUIName(). Necessary because LSAHDir references another record.
            // Code that requires referencing another record is normally in LSAFile (e.g. convertToDAT()).
            LSAHDir *hdir = static_cast<LSAHDir*>(lsaRecord);
            std::string atStation = lsaFile->getParentDGRPFromStation(hdir);
            newData = QString::fromStdString(atStation + ", " + hdir->toLabel);
        }
        else
        {
            newData = QString::fromStdString(lsaRecord->getUIColumnData(index.column()));
        }
    }

    // Note: the selection model can return multiple indexes for rows that span multiple columns
    // Don't attempt to setData for columns that don't exist in the model
    if (index.column() < lsaRecord->getNumDisplayColumns())
    {
        setData(index, newData, Qt::EditRole );
    }

    return;
}

void GuiModel::changeProjectDirectory(QString newDir, QString oldDir)
{
    QModelIndex index = getNextIndex();

    while(index.isValid())
    {
        LSARecord *lsaRecord = getLSARecord(index);
        if (!lsaRecord) return;
        if(lsaRecord->getRecType() == LSAType::INCLUDE)
        {

            LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);

            if(QFileInfo(QString::fromStdString(lsaInclude->getLSAPath())).isRelative() || index == this->index(0,0))
            {
                QString absPath = QString::fromStdString(lsaInclude->getAbsolutePath());
                if (absPath.contains(oldDir))
                {
                    QStringList split = absPath.split(oldDir);
                    lsaInclude->setAbsolutePath(newDir.toStdString() + split.at(1).toStdString());
                }
            }
        }

        index = getNextIndex(index);
    }
}

QModelIndex GuiModel::getNextIndex(QModelIndex currentIndex ) const
{
    auto root = QModelIndex();
    if ( currentIndex == root )
    {
        // return the QModelIndex for the root item
        return index(0,0);
    }

    int currentRow = currentIndex.row();

    if ( currentIndex.child(0,0) != root )
    {
        // The item at currentIndex has a child, so return the QModelIndex for the first child
        return currentIndex.child(0,0);
    }
    else if (currentIndex.sibling(currentRow + 1,0) != root )
    {
        // The item at currentIndex has a sibling, so return the QModelIndex for the next sibling
        return currentIndex.sibling(currentRow + 1,0);
    }

    // There is no child or sibling, so recurse up the tree looking for a parent with a next sibling
    return findNextSiblingOfParent(currentIndex);
}

QModelIndex GuiModel::getNextDescendant(QModelIndex parentIndex, QModelIndex currentIndex)
{
    //This method is intended to be called iteratively, where the same parentIndex is passed each time,
    //and the iteration stops when an invalid index is returned

    QModelIndex nextIndex;

    if ( !currentIndex.isValid() || currentIndex == parentIndex)//start with call to getNextDescendant(parentIndex)
        nextIndex = getNextIndex(parentIndex);//first child index of the parentIndex include record
    else
        nextIndex = getNextIndex(currentIndex);//next sibling or child of the currentIndex

    if( isDescendant(parentIndex, nextIndex))
        return nextIndex;

    return QModelIndex();

}

// returns true if currentIndex is a descendant of parentIndex
bool GuiModel::isDescendant(QModelIndex parentIndex, QModelIndex currentIndex)
{
    if (!currentIndex.isValid())
        return false;

    if (parentIndex == currentIndex.parent())
        return true; // We found the parent and it matches the parent we are looking for
    else if (!parentIndex.isValid())
        return false; // We recursed to the top of the tree and did not find the parent we were looking for

    // Recurse up the tree looking for parentIndex
    return isDescendant(parentIndex, currentIndex.parent());
}

QModelIndex GuiModel::getLastDescendant(QModelIndex parentIndex)
{
    if (!parentIndex.isValid()) return QModelIndex();

    int numChildren = getItem(parentIndex)->childCount();

    if (numChildren > 0)
    {
        QModelIndex lastChildIndex = parentIndex.child(numChildren-1, 0);
        return getLastDescendant(lastChildIndex);
    }

    return parentIndex;
}

QModelIndex GuiModel::getPreviousIndex(QModelIndex currentIndex)
{
    // If current index is invalid, return the last child of the root index
    if (!currentIndex.isValid())
    {
        return getLastDescendant(this->index(0,0));
    }

    // If currentIndex is the root index, return an invalid QModelIndex
    if (currentIndex == this->index(0,0)) return QModelIndex();


    QModelIndex previousIndex;
    int currentRow = currentIndex.row();
    if (currentRow > 0)
    {
        // if current row is greater than zero, then a sibling must exist
        // so we need to return the sibling or it's last descendant
        currentRow--;
        QModelIndex siblingIndex = currentIndex.sibling(currentRow,0);
        previousIndex = getLastDescendant(siblingIndex); // returns siblingIndex if no children present
    }
    else if (currentRow == 0)
    {
        // if currentRow is zero, then return the parent
        previousIndex = parent(currentIndex);
    }

    return previousIndex;
}

QModelIndex GuiModel::getPreviousParentIndex(QModelIndex currentIndex)
{
    QModelIndex previousIndex;

    if (!currentIndex.isValid())
    {
        return QModelIndex();
    }

    QModelIndex parentIndex = parent(currentIndex);
    int currentRow = currentIndex.row();
    if (currentRow > 0)
    {
        // if current row is greater than zero, then a sibling must exist
        currentRow--;
        previousIndex =  currentIndex.sibling(currentRow,0);
    }
    else
    {
        // if currentRow is zero, then return the parent
        previousIndex = parentIndex;
    }

    return previousIndex;

}
bool GuiModel::isMemberOfDirGroup(QModelIndex index, std::string dirGroupLabel)
{
    bool isMember = false;

    // check if previousIndex is a HDIR that is a member of dirGroupLabel
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();

    if (lsaType == LSAType::HDIR)
    {
        LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);
        if (lsaHDir->dirGroupLabel == dirGroupLabel)
        {
            isMember = true;
        }
    }

    return isMember;
}

bool GuiModel::isActive(QModelIndex index) const
{
    if ( !index.isValid() ) return false;

    GuiModelItem *item = getItem(index);

    return item->isActive(rootItem);
}

double GuiModel::getNetVSCAFactor(QModelIndex index) const
{
    if (!index.isValid())
    {
        return 1.0;
    }

    LSARecord* lsaRecord = getLSARecord(index);
    double recordVSCAFactor = lsaFile->getVSCAValue(lsaRecord);

    // If this is the root include or invalid, just return the record's VSCA factor
    if (isRootInclude(index) || !index.isValid())
    {
        return recordVSCAFactor;
    }

    // We have a parent, so recurse up the tree to get the product of all ancestors' VSCA factors
    return recordVSCAFactor * getNetVSCAFactor(index.parent());
}

bool GuiModel::hasValidScaling(QModelIndex index) const
{
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType type = lsaRecord->getRecType();

    if(lsaRecord->supportsModifier(MODKEY_VSCA))
    {
        double netScale = getNetVSCAFactor(index);
        if ((netScale >= lsa::MIN_VARIANCE_SCALE) &&
            (netScale <= lsa::MAX_VARIANCE_SCALE))
            return true;
        else
            return false;
    }
    else if(type == LSAType::VSCA)
    {
        LSAVarScaling *lsaVarScaling = static_cast<LSAVarScaling*>(lsaRecord);
        double scale = lsaVarScaling->varFactor;
        if ((scale >= lsa::MIN_VARIANCE_SCALE) &&
            (scale <= lsa::MAX_VARIANCE_SCALE))
            return true;
        else
            return false;
    }
    else
    {
        return true;
    }
}

bool GuiModel::hasValidMeasurementSigma(QModelIndex index) const
{
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType type = lsaRecord->getRecType();
    if(type.isMeasurement() && type != LSAType::DGRP)
    {
        bool netSigmaCheck = lsaFile->isValidSigma(lsaRecord);

        return  netSigmaCheck;
    }
    else
        return true;
}

bool GuiModel::isNonPolePosition(QModelIndex index) const
{
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType type = lsaRecord->getRecType();
    if(type.isPosition())
    {
        if(type==LSAType::POSG)
        {
            LSAPosG* posgRec = static_cast<LSAPosG *>(lsaRecord);
            double standLat = posgRec->giveStandardLat();
            if(standLat > lsa::MAX_LATITUDE_BOUNDARY || standLat < lsa::MIN_LATITUDE_BOUNDARY)
            {
                return false;
            }

            else
            {
                return true;
            }
        }

        // POSC record
        else
        {
            LSAPosC* poscRec = static_cast<LSAPosC *>(lsaRecord);
            if(std::abs(poscRec->x) < lsa::EPSILON && std::abs(poscRec->y) < lsa::EPSILON)
            {
                return false;
            }

            else
            {
                return true;
            }
        }
    }

    return true;
}

QModelIndex GuiModel::findNextSiblingOfParent(QModelIndex currentIndex) const
{
    QModelIndex parentIndex = currentIndex.parent();

    if (parentIndex == QModelIndex() )
    {
        // we are at the top of the tree and can't recurse farther
        return QModelIndex();
    }

    int parentRow = parentIndex.row();
    if ( parentIndex.sibling(parentRow+1,0) != QModelIndex() )
    {
        // We found a sibling of a parent, so return it
        return parentIndex.sibling(parentRow+1,0);
    }

    return findNextSiblingOfParent(parentIndex);
}

QModelIndex GuiModel::getSingleSelectedRecord()
{
    QModelIndexList indexList = getSelectedRows();

    //check if more than one index is selected
    if (indexList.size() != 1 )
    {
        return QModelIndex();
    }

    return indexList.at(0);
}

QModelIndex GuiModel::getLastSelectedRecord()
{
    QModelIndexList indexList = getSelectedRows();

    //check for invalid selection list
    if (indexList.size() <= 0 )
    {
        return QModelIndex();
    }

    return indexList.at(indexList.size()-1);
}

QStringList GuiModel::getIncludedLSAFiles()
{
    QStringList filenames;
    std::set<std::string> includedFiles = lsaFile->getIncludedLSAFiles();
    for (std::set<std::string>::iterator iter = includedFiles.begin(); iter != includedFiles.end(); ++iter )
    {
        filenames << QString::fromStdString(*iter);
    }

    return filenames;
}

QList<LSARecord*> GuiModel::getChildRecords(QModelIndex index)
{
    GuiModelItem *item = getItem(index);
    return item->getChildRecords();
}

QModelIndexList GuiModel::getSelectedRows() const
{
    if (selectionModel != NULL && selectionModel->hasSelection())
    {
        if(m_selectedRowsCache)
            return *m_selectedRowsCache;
        else
            return selectionModel->selectedRows();
    }

    return QModelIndexList();
}

void GuiModel::enableSelectedRowsCache(bool bUse)
{
    if(bUse && selectionModel != NULL)
    {
        m_selectedRowsCache.reset(new QModelIndexList(selectionModel->selectedRows()));
    }
    else
    {
        m_selectedRowsCache.release();
    }
}

void GuiModel::select(QModelIndex index)
{
    QModelIndexList paramList;
    paramList << index;
    select(paramList);
}

void GuiModel::select(QModelIndexList list, bool sort)
{
    DebugTimer timer(false);

    if (list.empty() || list.size() == 0) return;

    QItemSelection selection;
    if(sort)
    {
        std::sort(list.begin(), list.end());

        QModelIndex beginIndex, tmpIndex, nextIndex;

        bool firstRange = true;

        // Add rows to selections ranges based on if they are contiguous and have the same parent
        for(int index = 0; index < list.size() - 1; index++)
        {
            if(firstRange)
            {
                beginIndex = list[index];
                tmpIndex = list[index];
                firstRange = false;
            }

            int tmpRow = tmpIndex.row();
            nextIndex = list[index + 1];
            if((tmpRow + 1) == nextIndex.row() && tmpIndex.parent() == nextIndex.parent())
            {
                tmpIndex = nextIndex;
            }
            else
            {
                selection << QItemSelectionRange(beginIndex, tmpIndex);
                beginIndex = nextIndex;
                tmpIndex = nextIndex;
            }
        }

        selection << QItemSelectionRange(beginIndex, tmpIndex);
    }
    else
    {
        foreach(QModelIndex index, list)
        {
            selection << QItemSelectionRange(index);
        }
    }

    LOG_TIME(timer, "Create QItemSelection");

    // Set the current index to the first index of the selection
    QModelIndex firstIndex = list.first();
    selectionModel->setCurrentIndex(firstIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    LOG_TIME(timer, "setCurrentIndex");
    selectionModel->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);

    LOG_TIME(timer, "selectionModel->select");
}

bool GuiModel::selectLastChildOfRootInclude()
{
    QModelIndex rootIndex = index(0,0);
    int numChildren = getItem(rootIndex)->childCount();

    if (numChildren == 0)
    {
        select(rootIndex);
        return false;
    }
    else
    {
        QModelIndex lastChild = index(numChildren-1, 0, rootIndex);
        select(lastChild);
    }

    return true;
}

void GuiModel::selectReferencingRecords()
{
    // Get the currently selected record
    QModelIndex selectedIndex = getSingleSelectedRecord();

    // If more or less than one record is selected, do nothing
    if ( !selectedIndex.isValid() )
    {
        return;
    }

    LSARecord *selectedRecord = getLSARecord(selectedIndex);
    if (selectedRecord == NULL)
    {
        return;
    }

    LSAType selectedType = selectedRecord->getRecType();
    std::string label = selectedRecord->getLabel();

    QModelIndexList indexList;

    if ( selectedType.isPosition() )
    {
        indexList = getRecordsReferencingPosition(label);
    }
    else if ( selectedType.isModifier() )
    {
        indexList = getRecordsReferencingModifier(selectedType, label);
    }
    else if (selectedType == LSAType::DGRP)
    {
        indexList = getRecordsReferencingDirGroup(label);
    }

    if ( !indexList.empty() )
    {
        select(indexList);
    }


}

QModelIndexList GuiModel::getRecordsReferencingPosition(const std::string& label)
{
    QModelIndexList indexList;

    QModelIndex index = getNextIndex();
    while( index.isValid() )
    {
        LSARecord *lsaRecord = getLSARecord(index);
        if (recordReferencesPosition(lsaRecord, label))
        {
            indexList.push_back(index);
        }

        index = getNextIndex(index);
    }

    return indexList;
}

bool GuiModel::recordReferencesPosition(LSARecord *lsaRecord, const std::string& positionLabel)
{
    LSAType lsaType = lsaRecord->getRecType();
    if (lsaType == LSAType::HDIR)
    {
        // Check the To Position
        bool referencesToPosition = lsaRecord->referencesPosition(positionLabel);

        // Check the From Position
        LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);
        std::string dgrpLabel = lsaHDir->dirGroupLabel;

        LSADirGroup *lsaDirGroup = getDirGroup(dgrpLabel);
        bool referencesFromPosition = false;
        if (lsaDirGroup != NULL)
        {
            referencesFromPosition = lsaDirGroup->referencesPosition(positionLabel);
        }

        return referencesToPosition || referencesFromPosition;
    }

    return lsaRecord->referencesPosition(positionLabel);
}

QModelIndexList GuiModel::getRecordsReferencingDirGroup(const std::string& label)
{
    QModelIndexList indexList;

    QModelIndex index = getNextIndex();
    while( index.isValid() )
    {
        LSARecord *lsaRecord = getLSARecord(index);
        if (lsaRecord->getRecType() == LSAType::HDIR)
        {
            LSAHDir * lsaHDir = static_cast<LSAHDir*>(lsaRecord);
            if (lsaHDir->dirGroupLabel == label)
            {
                indexList.push_back(index);
            }
        }

        index = getNextIndex(index);
    }

    return indexList;
}

QModelIndexList GuiModel::getRecordsReferencingDirGroup(const std::string& label, QModelIndex parentIncludeIndex)
{
    QModelIndexList indexList;
    QModelIndex index;

    if(!parentIncludeIndex.isValid())//search entire tree
        indexList = getRecordsReferencingDirGroup(label);
    else//search a single branch of the tree
    {
        index = parentIncludeIndex;
        while((index = getNextDescendant(parentIncludeIndex,index)).isValid())
        {
            LSARecord *lsaRecord = getLSARecord(index);
            if(lsaRecord->getRecType() == LSAType::HDIR)
            {
                LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);
                if(lsaHDir->dirGroupLabel == label)
                    indexList.push_back(index);
            }
        }
    }

    return indexList;
}

QModelIndexList GuiModel::getRecordsReferencingModifier(LSAType modifierType, const std::string& label)
{
    QModelIndexList indexList;

    QModelIndex index = getNextIndex();
    while( index.isValid() )
    {
        LSARecord *lsaRecord = getLSARecord(index);
        if ( lsaRecord->referencesModifier(modifierType, label) )
        {
            indexList.push_back(index);
        }

        index = getNextIndex(index);
    }

    return indexList;
}

QModelIndexList GuiModel::getRecordsReferencingModifier(LSAType modifierType, const std::string& label, QModelIndex parentIncludeIndex)
{
    QModelIndexList indexList;
    QModelIndex index;

    if(!parentIncludeIndex.isValid())//search entire tree
        indexList = getRecordsReferencingModifier(modifierType, label);
    else//search a single branch of the tree
    {
        index = parentIncludeIndex;
        while((index = getNextDescendant(parentIncludeIndex,index)).isValid())
        {
            LSARecord *lsaRecord = getLSARecord(index);
            if ( lsaRecord->referencesModifier(modifierType, label) )
                indexList.push_back(index);
        }
    }

    return indexList;
}

std::string GuiModel::getProjectFilename() const
{
    LSARecord *rootRecord = rootItem->child(0)->getLSARecord();
    if (!rootRecord) return "";

    LSAInclude *rootInclude = static_cast<LSAInclude*>(rootRecord);

    std::string projectDirectory = getProjectDirectory();
    std::string projectFilename = rootInclude->getAbsolutePath();

    return projectFilename;
}

void GuiModel::renamePositionLabel(std::string oldLabel, std::string newLabel)
{
    // delete the map entry with the old label and insert one with the new label
    LSARecordMap::iterator it;
    it = pointMap->find(oldLabel);
    if (it != pointMap->end())
    {
        // if we found a map element with the old label, delete it
        LSARecord* lsaRecord = it->second;
        pointMap->erase(it);

        // insert a new map element with the new label
        if ( !newLabel.empty() )
        {
            pointMap->insert(std::pair<std::string, LSARecord*>(newLabel, lsaRecord));
        }
    }

    // change all references to oldLabel to newLabel in the tree
    QModelIndex index = getNextIndex();
    while (index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        std::string oldValue = lsaRecord->getSingleLSAString();
        lsaRecord->renamePosition(oldLabel, newLabel);
        std::string newValue = lsaRecord->getSingleLSAString();
        setParentIncludeModified(index);//fix to Bug #1341

        if(oldValue != newValue)
        {
            QList<QPersistentModelIndex> selectedIndices;
            selectedIndices.append(index);
            pushChangeValue(index, selectedIndices, oldValue, newValue, true);
        }

        index = getNextIndex(index);
    }

    // resynch the tree
    synchGuiModelToLSATree();

    return;
}

void GuiModel::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    if      ( lsaType == LSAType::HGHT ) renameHGHTLabel(oldLabel, newLabel);
    else if ( lsaType == LSAType::UNCR ) renameUNCRLabel(oldLabel, newLabel);
    else if ( lsaType == LSAType::VSCA ) renameVSCALabel(oldLabel, newLabel);
    else if ( lsaType == LSAType::DGRP ) renameDGRPLabel(oldLabel, newLabel);

    // resynch the tree
    synchGuiModelToLSATree();
}

unsigned int GuiModel::hashTree()
{
    unsigned int hashcode = 0;
    try
    {
        hashcode = lsaFile->hashDatFile();
    }
    catch(gnsstk::Exception& e)
    {
        pushError(QString("Salsa encountered an exception.  Please send this message to the development team: "));
        std::ostringstream oss;
        oss << e;

        pushError(QString::fromStdString(oss.str()));
    }

    return hashcode;
}

void GuiModel::renameHGHTLabel(std::string oldLabel, std::string newLabel)
{

    // delete the map entry with the old label and insert one with the new label
    LSAHGHTMap *heightMap = getHeightMap();
    LSAHGHTMap::iterator it;
    it = heightMap->find(oldLabel);
    if (it != heightMap->end())
    {
        // if we found a map element with the old label, delete it
        LSAHeight* lsaHeight = it->second;
        heightMap->erase(it);

        // insert a new map element with the new label
        if ( !newLabel.empty() )
        {
            heightMap->insert(std::pair<std::string, LSAHeight*>(newLabel, lsaHeight));
        }
    }

    // change all references to oldLabel height to newLabel heights
    renameModifierLabelInTree(LSAType::HGHT, oldLabel, newLabel);

    return;
}

void GuiModel::renameUNCRLabel(std::string oldLabel, std::string newLabel)
{
    // delete the map entry with the old label and insert one with the new label
    LSAUNCRMap *uncrMap = getUncrMap();
    LSAUNCRMap::iterator it;
    it = uncrMap->find(oldLabel);
    if (it != uncrMap->end())
    {
        LSAUncertainty* lsaUncr = it->second;
        uncrMap->erase(it);

        if (!newLabel.empty())
        {
            // insert a new map element with the new label
            uncrMap->insert(std::pair<std::string, LSAUncertainty*>(newLabel, lsaUncr));
        }
    }

    // change all references to oldLabel height to newLabel heights
    renameModifierLabelInTree(LSAType::UNCR, oldLabel, newLabel);

    return;
}

void GuiModel::renameVSCALabel(std::string oldLabel, std::string newLabel)
{
    // delete the map entry with the old label and insert one with the new label
        LSAVSCAMap::iterator it;
    LSAVSCAMap *varScalingMap = getVarScalingMap();
    it = varScalingMap->find(oldLabel);
    if (it != varScalingMap->end())
    {
        LSAVarScaling* lsaVarScaling = it->second;
        varScalingMap->erase(it);

        // insert a new map element with the new label
        if (!newLabel.empty())
        {
            // If the VSCA record is active, add it to the map
            LSARecord *lsaRecord = static_cast<LSARecord*>(lsaVarScaling);
            QModelIndex index = getIndexForRecord(lsaRecord);
            if (isActive(index))
            {
                varScalingMap->insert(std::pair<std::string, LSAVarScaling*>(newLabel, lsaVarScaling));
            }
        }
    }

    // change all references to oldLabel height to newLabel heights
    renameModifierLabelInTree(LSAType::VSCA, oldLabel, newLabel);

    return;
}

void GuiModel::renameDGRPLabel(std::string oldLabel, std::string newLabel)
{
    // Cache a pointer to the dgrp
    LSADirGroup* dgrp = getDirGroup(oldLabel);

    // delete the map element with the old label
    lsaFile->removeDGRPLabel(oldLabel);

    // insert a new map element with the new label
    if (!newLabel.empty())
    {
        lsaFile->addDGRPpair(newLabel, dgrp);
    }

    // change all references to oldLabel height to newLabel heights
    renameDGRPLabelInTree(oldLabel, newLabel);

    return;
}

LSAVSCAMap GuiModel::getFullVarScalingMap()
{
    LSAVSCAMap retMap;

    QModelIndex index = getNextIndex();
    while (index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        if(lsaRecord->getRecType() == LSAType::VSCA)
        {
            LSAVarScaling* vsca = static_cast<LSAVarScaling*>(lsaRecord);
            std::string label = vsca->getLabel();
            retMap.insert(std::pair<std::string, LSAVarScaling*>(label, vsca));
        }

        index = getNextIndex(index);
    }

    return retMap;
}

LSAUNCRMap GuiModel::getFullUncrMap()
{
    LSAUNCRMap retMap;

    QModelIndex index = getNextIndex();
    while (index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        if(lsaRecord->getRecType() == LSAType::UNCR)
        {
            LSAUncertainty* uncr = static_cast<LSAUncertainty*>(lsaRecord);
            std::string label = uncr->getLabel();
            retMap.insert(std::pair<std::string, LSAUncertainty*>(label, uncr));
        }

        index = getNextIndex(index);
    }

    return retMap;
}

LSAHGHTMap GuiModel::getFullHeightMap()
{
    LSAHGHTMap retMap;

    QModelIndex index = getNextIndex();
    while (index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        if(lsaRecord->getRecType() == LSAType::HGHT)
        {
            LSAHeight* hght = static_cast<LSAHeight*>(lsaRecord);
            std::string label = hght->getLabel();
            retMap.insert(std::pair<std::string, LSAHeight*>(label, hght));
        }

        index = getNextIndex(index);
    }

    return retMap;
}

LSADirGroupMap GuiModel::getFullDirGroupMap()
{
    LSADirGroupMap retMap;

    QModelIndex index = getNextIndex();
    while (index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        if(lsaRecord->getRecType() == LSAType::DGRP)
        {
            LSADirGroup* dgrp = static_cast<LSADirGroup*>(lsaRecord);
            std::string label = dgrp->getLabel();
            retMap.insert(std::pair<std::string, LSADirGroup*>(label, dgrp));
        }

        index = getNextIndex(index);
    }

    return retMap;
}

LSADirGroup* GuiModel::getDirGroup(std::string label)
{
    LSADirGroup *lsaDirGroup = NULL;

    LSADirGroupMap::iterator it = lsaFile->dgrpMapComplete.find(label);
    if (it != lsaFile->dgrpMapComplete.end())
    {
        lsaDirGroup = it->second;
    }

    return lsaDirGroup;
}

void GuiModel::renameModifierLabelInTree(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    QModelIndex index = getNextIndex();

    while (index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        std::string oldValue = lsaRecord->getSingleLSAString();
        lsaRecord->renameModifierLabel(lsaType, oldLabel, newLabel);
        std::string newValue = lsaRecord->getSingleLSAString();
        if(oldValue != newValue)
        {
            QList<QPersistentModelIndex> selectedIndices;
            selectedIndices.append(index);
            pushChangeValue(index, selectedIndices, oldValue, newValue, true);
        }

        index = getNextIndex(index);

    }

    return;
}

void GuiModel::renameDGRPLabelInTree(std::string oldLabel, std::string newLabel)
{
    QModelIndex index = getNextIndex();

    while (index.isValid())
    {
        LSARecord* lsaRecord = getLSARecord(index);
        if (lsaRecord->getRecType() == LSAType::HDIR)
        {
            LSAHDir* lsaHDir = static_cast<LSAHDir*>(lsaRecord);
            std::string oldValue = lsaRecord->getSingleLSAString();
            if ( lsaHDir->dirGroupLabel == oldLabel )
            {
                lsaHDir->dirGroupLabel = newLabel;
            }
            std::string newValue = lsaRecord->getSingleLSAString();
            QList<QPersistentModelIndex> selectedIndices;
            selectedIndices.append(index);
            pushChangeValue(index, selectedIndices, oldValue, newValue, true);
        }

        index = getNextIndex(index);

    }

    synchGuiModelToLSATree();

    return;
}

void GuiModel::createInitialCoordsInclude()
{
    if(!initialCoordsIndex.isValid())
    {
        // insert the include record into the tree
        QModelIndex rootIndex = this->index(0,0);
        initialCoordsIndex = insertFirstChild(LSAType::INCLUDE, INITIAL_COORDS_FILE_NAME, rootIndex, false);

        // clear the erroneous 'file not found' warning
        LSAInclude* lsaInclude = static_cast<LSAInclude*>(getLSARecord(initialCoordsIndex));
        lsaInclude->warningMessages_generated.clear();

        // Insert a creation comment if the file does not exist from a previous run
        std::string fileName(INITIAL_COORDS_FILE_NAME);
        include_lsapath(getProjectDirectory(),fileName);
        QFileInfo initCoord(QString::fromStdString(fileName));
        if(!initCoord.exists() || initialCoordsLacksComment(fileName))
        {
            QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            std::string line("# New .lsa file created " + dateTime.toStdString());
            insertNewRecord(initialCoordsIndex, 0, LSAType::COMMENT, line);
        }
    }
}

void GuiModel::validateModelAndRegenerateMaps()
{
    // clear the maps
    pointMap->clear();
    hghtMap->clear();
    uncrMap->clear();
    vscaMap->clear();
    dgrpMap->clear();

    std::set<std::string> allPositionsIncludingAutogenerated;
    std::list<std::string> derivedPoints;
    QModelIndexList derivedPointsIndexes;

    std::set<std::string> disabledPositions;
    std::set<std::string> disabledHeights;
    std::set<std::string> disabledUncrs;
    std::set<std::string> disabledVarScales;
    std::set<std::string> disabledDirGroups;
    std::vector<LSAUncertainty*> activeUNCRRecords;
    activeUNCRRecords.clear();

    // traverse the tree a first time to rebuild the maps and find any duplicate records
    QModelIndex index = getNextIndex();
    while (index.isValid())
    {
        LSARecord *lsaRecord = getLSARecord(index);
        LSAType lsaType = lsaRecord -> getRecType();

        // clear warning messages
        lsaRecord->warningMessages_generated.clear();

        std::string label = lsaRecord->getLabel();
        bool indexIsActive = isActive(index);
        bool parentIsActive = isActive(index.parent());
        // insert values into maps
        LSARecord *originalRec = NULL;
        if (indexIsActive && !lsaRecord->hasParseWarnings())
        {
            // try to insert the new label into the appropriate map
            // Note: if the label is already in the map, insertMapValue will return a LSARecord*
            // corresponding the LSARecord already in the map
            originalRec = lsaFile->insertMapValue(lsaType, label, lsaRecord, lsaRecord->isCommented, parentIsActive);

            // Add all referenced positions to the list of referenced positions
            std::vector<std::string> referencedPositions = lsaRecord->getReferencedPositions();
            for (size_t i = 0; i < referencedPositions.size(); ++i)
            {
                allPositionsIncludingAutogenerated.insert(referencedPositions.at(i));
            }
            if(lsaRecord->getRecType().isPostProcessed())
            {
                derivedPoints.push_back(lsaRecord->getLabel());
                derivedPointsIndexes.append(index);
            }

            //warn of duplicate UNCR labels only if values are distinct
            //NOTE: If 19 records are the same, and 1 is different, ALL 20 get flagged as duplicates.
            if(lsaType == LSAType::UNCR)
            {
                std::vector<LSAUncertainty*>::iterator it;
                for(it=activeUNCRRecords.begin();it!=activeUNCRRecords.end();it++)
                {
                    std::string storedLabel = (*it)->getLabel();
                    if(label == storedLabel)//duplicate label
                    {
                        std::string thisLSAString = lsaRecord->getSingleLSAString();
                        std::string storedLSAString = (*it)->getSingleLSAString();
                        if(thisLSAString != storedLSAString)//distinct values
                        {
                            std::ostringstream oss;
                            oss << "Duplicate " << lsaType.asString() << " record: " << label;
                            lsaRecord->warningMessages_generated.insert( oss.str() );
                            (*it)->warningMessages_generated.insert( oss.str() );
                        }
                    }
                }
                activeUNCRRecords.push_back(static_cast<LSAUncertainty*>(lsaRecord));
            }
        }

        // Validate that the lsaRecord has the correct units of uncertainty, warning if not
        validateUNCRUnits(lsaRecord);

        // If we have a duplicate record, set the warning message on both records
        if (originalRec != NULL)
        {
            std::ostringstream oss;
            oss << "Duplicate " << lsaType.asString() << " record: " << label;

            //don't warn the user of duplicate UNCRs (already warning above if distinct values)
            if(lsaType != LSAType::UNCR)
            {
                // set the value on the second record
                lsaRecord->warningMessages_generated.insert( oss.str() );

                // set the value on the first record
                originalRec->warningMessages_generated.insert( oss.str() );
            }
        }

        // also build up disabled vectors
        if (lsaType == LSAType::POSC && !indexIsActive )
        {
            disabledPositions.insert(label);
        }
        else if (lsaType == LSAType::POSG && !indexIsActive )
        {
            disabledPositions.insert(label);
        }
        else if (lsaType == LSAType::HGHT && !indexIsActive )
        {
            disabledHeights.insert(label);
        }
        else if (lsaType == LSAType::UNCR && !indexIsActive )
        {
            disabledUncrs.insert(label);
        }
        else if (lsaType == LSAType::VSCA && !indexIsActive )
        {
            disabledVarScales.insert(label);
        }
        else if (lsaType == LSAType::DGRP && !indexIsActive )
        {
            disabledDirGroups.insert(label);
        }

        index = getNextIndex(index);
    }

    //warn user of circular derived point references
    validateDerivedPointReferences(derivedPointsIndexes);

    // traverse the tree a second time to identify references to missing or disabled records
    index = getNextIndex();
    while (index.isValid())
    {
        LSARecord *lsaRecord = getLSARecord(index);
        LSAType lsaType = lsaRecord->getRecType();
        if (isActive(index))
        {
            checkForMissingAndDisabledReferences(index,
                                                 disabledPositions, disabledHeights, disabledUncrs,
                                                 disabledVarScales, disabledDirGroups);

            checkForMeasurementsInvolvingDerivedPoints(index,derivedPoints);

            // Check for invalid covariance
            if ( !lsaRecord->hasValidCovariance())
            {
                lsaRecord->warningMessages_generated.insert(INVALID_COVARIANCE_MESSAGE);
            }

            //Check for invalid vscale
            if ( !hasValidScaling(index) )
            {
                lsaRecord->warningMessages_generated.insert(INVALID_SCALING_MESSAGE);
            }

            //Check for invalid sigma
            if( !hasValidMeasurementSigma(index))
            {
                lsaRecord->warningMessages_generated.insert(INVALID_SIGMA_MESSAGE);
            }

            // Check to see if position is near the poles
            if( !isNonPolePosition(index) )
            {
                if(lsaType == LSAType::POSG)
                {
                    lsaRecord->warningMessages_generated.insert(INVALID_POSG_POLE_MESSAGE);
                }

                else
                {
                    lsaRecord->warningMessages_generated.insert(INVALID_POSC_POLE_MESSAGE);
                }
            }

            // Validate derived positions
            if (lsaType == LSAType::MEAN)
            {
                LSAMean* lsaMean = static_cast<LSAMean*>(lsaRecord);
                lsaMean->validatePositions(allPositionsIncludingAutogenerated,derivedPoints);
            }
            else if (lsaType == LSAType::ENUO)
            {
                LSAEnuo* lsaEnuo = static_cast<LSAEnuo*>(lsaRecord);
                lsaEnuo->validatePositions(allPositionsIncludingAutogenerated,derivedPoints);
            }

            //propagate warnings up to parents
            if (!lsaRecord->warningMessages_generated.empty() || !lsaRecord->warningMessages_persistent.empty())
            {
                setWarningsOnAncestors(index);
            }
        }

        // Add any persistent warning messages to the set of generated messages so they will be displayed in ui
        lsaRecord->warningMessages_generated.insert(lsaRecord->warningMessages_persistent.begin(), lsaRecord->warningMessages_persistent.end());


        index = getNextIndex(index);
    }

    return;
}

void GuiModel::validateUNCRUnits(LSARecord *lsaRecord)
{
    //get lsaType
    LSAType lsaType = lsaRecord->getRecType();

    //check if UNCR has additional sigma

    if(lsaRecord->getModifiers() == NULL)
        return;

    bool hasUNCR = lsaRecord->getModifiers()->hasUNCRCorrection();
    std::string uncrLabel = lsaRecord->getModifiers()->getUncrLabel();

    if(hasUNCR && !uncrLabel.empty())
    {
        // Get the uncertainty model
        if ( uncrMap->find(uncrLabel) != uncrMap->end() )
        {
            LSAUncertainty *uncr = uncrMap->find(uncrLabel)->second;

            // Get the units for additional sigma
            std::string sigmaUnits = uncr->sigmaUnits;
            gnsstk::StringUtils::upperCase(sigmaUnits);

            if ( lsaType == LSAType::DXYZ || lsaType == LSAType::DIST || lsaType == LSAType::HDIF)
            {
                 if(!isLinearUnit(sigmaUnits) && uncr->Sigma > lsa::ZERO_BOUND)
                {
                    std::ostringstream oss;
                    oss << "UNCR " << uncrLabel << " units of " << sigmaUnits << " cannot be applied to " << lsaType.asString();
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }

            }
            if ( lsaType == LSAType::HANG || lsaType == LSAType::VANG || lsaType == LSAType::ZANG ||
                 lsaType == LSAType::AZIM || lsaType == LSAType::DGRP)
            {
                if(!isAngularUnit(sigmaUnits) && uncr->Sigma > lsa::ZERO_BOUND)
                {
                    std::ostringstream oss;
                    oss << "UNCR " << uncrLabel << " units of " << sigmaUnits << " cannot be applied to " << lsaType.asString();
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
            }
        }

    }

}

void GuiModel::checkForMissingAndDisabledReferences(QModelIndex index,
                                                    const std::set<std::string>& disabledPositions,
                                                    const std::set<std::string>& disabledHeights,
                                                    const std::set<std::string>& disabledUncrs,
                                                    const std::set<std::string>& disabledVarScales,
                                                    const std::set<std::string>& disabledDirGroups)
{
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();
    LSAModifier *modifiers = LSAFile::getModifiersForLSARecord(lsaRecord);

    if (modifiers != NULL)
    {
        std::string heightFromLabel = modifiers->getHeightFromLabel();
        std::string heightToLabel   = modifiers->getHeightToLabel();
        std::string uncrLabel      = modifiers->getUncrLabel();
        std::string vscaLabel       = modifiers->getVSCALabel();

        // validate heightFrom
        if (!heightFromLabel.empty() && !modifiers->fromHeightIsNumeric() )
        {
            // Check for label referencing missing modifier
            LSAHGHTMap::iterator iterFrom = hghtMap->find(heightFromLabel);
            if (iterFrom == hghtMap->end())
            {
                if (disabledHeights.find(heightFromLabel) == disabledHeights.end())
                {
                    // label is in neither the map NOR the disabled list, so it is missing
                    std::ostringstream oss;
                    oss << "Missing HGHT FROM: " << heightFromLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
                else
                {
                    // label is not in map but IS in disabled list, so it is disabled
                    std::ostringstream oss;
                    oss << "Disabled HGHT FROM: " << heightFromLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
            }
        }

        // validate heightTo
        if (!heightToLabel.empty() && !modifiers->toHeightIsNumeric() )
        {
            // Check for label referencing missing modifier
            LSAHGHTMap::iterator iterTo = hghtMap->find(heightToLabel);
            if (iterTo == hghtMap->end())
            {
                if (disabledHeights.find(heightToLabel) == disabledHeights.end())
                {
                    std::ostringstream oss;
                    // label is in neither the map NOR the disabled list, so it is missing
                    oss << "Missing HGHT TO: " << heightToLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
                else
                {
                    std::ostringstream oss;
                    // label is not in map but IS in disabled list, so it is disabled
                    oss << "Disabled HGHT TO: " << heightToLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
            }
        }

        // validate sigmas
        if (!uncrLabel.empty() && !modifiers->hasNumericVSCA() )
        {
            // Check for label referencing missing modifier
            LSAUNCRMap::iterator iterUncr = uncrMap->find(uncrLabel);
            if (iterUncr == uncrMap->end())
            {
                if (disabledUncrs.find(uncrLabel) == disabledUncrs.end())
                {
                    std::ostringstream oss;
                    // label is in neither the map NOR the disabled list, so it is missing
                    oss << "Missing UNCR: " << uncrLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
                else
                {
                    std::ostringstream oss;
                    // label is not in map but IS in disabled list, so it is disabled
                    oss << "Disabled UNCR: " << uncrLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
            }
        }

        // validate vsca
        if (!vscaLabel.empty())
        {
            // Check for label referencing missing modifier
            LSAVSCAMap::iterator iterVSCA = vscaMap->find(vscaLabel);
            if (iterVSCA == vscaMap->end())
            {
                if (disabledVarScales.find(vscaLabel) == disabledVarScales.end())
                {
                    std::ostringstream oss;
                    // label is in neither the map NOR the disabled list, so it is missing
                    oss << "Missing VSCA: " << vscaLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
                else
                {
                    std::ostringstream oss;
                    // label is not in map but IS in disabled list, so it is disabled
                    oss << "Disabled VSCA: " << vscaLabel;
                    lsaRecord->warningMessages_generated.insert( oss.str() );
                }
            }
        }

    }

    // Look for reference to missing or disabled DIRGROUP
    if (lsaType == LSAType::HDIR)
    {
        LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);
        std::string dirGroupLabel = lsaHDir->dirGroupLabel;

        // Check for label referencing missing modifier
        LSADirGroupMap::iterator iterDGRP = dgrpMap->find(dirGroupLabel);
        if (iterDGRP == dgrpMap->end())
        {
            if (disabledDirGroups.find(dirGroupLabel) == disabledDirGroups.end())
            {
                std::ostringstream oss;
                // label is in neither the map NOR the disabled list, so it is missing
                oss << "Missing DGRP: " << dirGroupLabel;
                lsaRecord->warningMessages_generated.insert( oss.str() );
            }
            else
            {
                std::ostringstream oss;
                // label is not in map but IS in disabled list, so it is disabled
                oss << "Disabled DGRP: " << dirGroupLabel;
                lsaRecord->warningMessages_generated.insert( oss.str() );
            }
        }
    }

    if (!lsaRecord->warningMessages_generated.empty() )
    {
        setWarningsOnAncestors(index);
    }

    return;
}

void GuiModel::checkForMeasurementsInvolvingDerivedPoints(QModelIndex index, const std::list<std::string>& derivedPoints)
{
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType type = lsaRecord->getRecType();
    std::ostringstream oss;
    std::string referenced("");
    bool referencesDerived = false;

    if(!type.isMeasurement()) return;//only checking measurements

    if(type == LSAType::AZIM)
    {
        LSAAzimuth *lsaAzimuth = static_cast<LSAAzimuth*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaAzimuth->From) != derivedPoints.end())
        {
            referenced = lsaAzimuth->From;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaAzimuth->To) != derivedPoints.end())
        {
            referenced = lsaAzimuth->To;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::DGRP)
    {
        LSADirGroup *lsaDirgroup = static_cast<LSADirGroup*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaDirgroup->fromLabel) != derivedPoints.end())
        {
            referenced = lsaDirgroup->fromLabel;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::DIST)
    {
        LSADist *lsaDist = static_cast<LSADist*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaDist->From) != derivedPoints.end())
        {
            referenced = lsaDist->From;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaDist->To) != derivedPoints.end())
        {
            referenced = lsaDist->To;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::DXYZ)
    {
        LSADelta *lsaDelta = static_cast<LSADelta*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaDelta->From) != derivedPoints.end())
        {
            referenced = lsaDelta->From;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaDelta->To) != derivedPoints.end())
        {
            referenced = lsaDelta->To;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::HANG)
    {
        LSAHAngle *lsaHangle = static_cast<LSAHAngle*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaHangle->At) != derivedPoints.end())
        {
            referenced = lsaHangle->At;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaHangle->From) != derivedPoints.end())
        {
            referenced = lsaHangle->From;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaHangle->To) != derivedPoints.end())
        {
            referenced = lsaHangle->To;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::HDIF)
    {
        LSAHeightDiff *lsaHDiff = static_cast<LSAHeightDiff*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaHDiff->From) != derivedPoints.end())
        {
            referenced = lsaHDiff->From;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaHDiff->To) != derivedPoints.end())
        {
            referenced = lsaHDiff->To;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::HDIR)
    {
        LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaHDir->toLabel) != derivedPoints.end())
        {
            referenced = lsaHDir->toLabel;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::VANG)
    {
        LSAVAngle *lsaVangle = static_cast<LSAVAngle*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaVangle->From) != derivedPoints.end())
        {
            referenced = lsaVangle->From;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaVangle->To) != derivedPoints.end())
        {
            referenced = lsaVangle->To;
            referencesDerived = true;
        }
    }
    else if(type == LSAType::ZANG)
    {
        LSAZAngle *lsaZangle = static_cast<LSAZAngle*>(lsaRecord);
        if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaZangle->From) != derivedPoints.end())
        {
            referenced = lsaZangle->From;
            referencesDerived = true;
        }
        else if(std::find(derivedPoints.begin(), derivedPoints.end(), lsaZangle->To) != derivedPoints.end())
        {
            referenced = lsaZangle->To;
            referencesDerived = true;
        }
    }

    if(referencesDerived)
    {
        std::ostringstream oss;
        oss << "Measurement may not reference a derived point " << referenced;
        lsaRecord->warningMessages_generated.insert( oss.str() );
        setWarningsOnAncestors(index);
    }
}

//NOTE: This code largely duplicates findValidDerivedDependencies() in lsapost.cpp
//Modifications to the algorithm here may constitute code changes there.
void GuiModel::validateDerivedPointReferences(QModelIndexList derivedPointsIndexes)
{
    //create a set of derived point labels
    std::set<std::string> derivedPointLabels;
    LSAType type;
    LSARecord *lsaRecord;
    std::string label("");
    for(int i=0; i<derivedPointsIndexes.size(); i++)
    {
        lsaRecord = getLSARecord(derivedPointsIndexes.at(i));
        type = lsaRecord->getRecType();
        if(type == LSAType::MEAN)
        {
            LSAMean *lsaMean = static_cast<LSAMean*>(lsaRecord);
            label = lsaMean->getLabel();
        }
        else if(type == LSAType::ENUO)
        {
            LSAEnuo *lsaEnuo = static_cast<LSAEnuo*>(lsaRecord);
            label = lsaEnuo->getLabel();
        }
        derivedPointLabels.insert(label);
    }

    //map all derived point dependencies on other derived points
    std::map<std::string,std::set<std::string> > depMap;
    for(int i=0; i<derivedPointsIndexes.size(); i++)
    {
        std::string label("");
        std::set<std::string> deps;
        deps.clear();
        lsaRecord = getLSARecord(derivedPointsIndexes.at(i));
        type = lsaRecord->getRecType();
        if(type == LSAType::MEAN)
        {
            LSAMean *lsaMean = static_cast<LSAMean*>(lsaRecord);
            label = lsaMean->getLabel();
            for(int j=0;j<lsaMean->points.size();j++)
            {
                if(derivedPointLabels.find(lsaMean->points.at(j)) != derivedPointLabels.end())
                    deps.insert(lsaMean->points.at(j));
            }
        }
        else if(type == LSAType::ENUO)
        {
            LSAEnuo *lsaEnuo = static_cast<LSAEnuo*>(lsaRecord);
            label = lsaEnuo->getLabel();
            if(derivedPointLabels.find(lsaEnuo->From) != derivedPointLabels.end())
                deps.insert(lsaEnuo->From);
        }

        depMap.insert(std::pair<std::string,std::set<std::string> >(label,deps));
    }

    //remove derived points that don't depend upon other derived points from consideration
    std::vector<std::string> indepList;
    std::map<std::string,std::set<std::string> >::iterator iter;
    for(iter=depMap.begin(); iter!=depMap.end(); iter++)
    {
        if(iter->second.size() == 0)
            indepList.push_back(iter->first);
    }
    for(int i=0;i<indepList.size();i++)
        depMap.erase(indepList.at(i));

    //iterate until size of indepList is unchanged (done or circular dependency)
    int lastIndepListSize = indepList.size();
    while(true)
    {
        //add dependent derived points
        for(iter=depMap.begin(); iter!=depMap.end(); iter++)
        {
            std::set<std::string> depSet = iter->second;
            std::set<std::string>::iterator it2;
            bool dependent = false;
            for(it2=depSet.begin(); it2!=depSet.end(); it2++)
            {
                if(depMap.find(*it2) != depMap.end())
                {
                    dependent = true;
                    break;
                }
            }
            if(!dependent)
            {
                indepList.push_back(iter->first);
            }
        }
        for(int i=0;i<indepList.size();i++)
            depMap.erase(indepList.at(i));
        if(indepList.size() == lastIndepListSize)
            break;
        else
            lastIndepListSize = indepList.size();
    }

    //warn of circular dependencies
    if(indepList.size() < derivedPointLabels.size())
    {
        for(iter=depMap.begin(); iter!=depMap.end(); iter++)
        {
            std::set<std::string> deps = iter->second;
            std::set<std::string>::iterator it2;
            std::string depList("");
            for(it2=deps.begin();it2!=deps.end();it2++)
                depList += (" " + *it2 + ",");
            depList.erase(depList.size()-1,1);
            std::string message = "Derived position circular dependency. " + iter->first +
                    " depends on" + depList + ".";
            QModelIndex badIndex = getIndexFromPointLabel(iter->first);
            LSARecord *badRecord = getLSARecord(badIndex);
            badRecord->warningMessages_generated.insert( message );
            setWarningsOnAncestors(badIndex);
        }
    }
}

void GuiModel::setWarningsOnAncestors(QModelIndex childIndex)
{
    QModelIndex parentIndex = childIndex.parent();

    if (parentIndex.isValid())
    {
        LSARecord *parentRecord = getLSARecord(parentIndex);
        LSARecord *childRecord = getLSARecord(childIndex);

        if ( !childRecord->warningMessages_generated.empty() || !childRecord->warningMessages_persistent.empty())
        {
            parentRecord->warningMessages_generated.insert(CHILD_WARNING_MESSAGE);
        }
        setWarningsOnAncestors(parentIndex);
    }

    return;
}

bool GuiModel::hasChildrenWithWarnings(QModelIndex parentIncludeIndex)
{
    LSARecord *lsaRecord = getLSARecord(parentIncludeIndex);
    LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
    std::list<LSARecord*> &childRecords = lsaInclude->childRecords;
    std::list<LSARecord*>::iterator iter;
    bool childHasWarning = false;

    for (iter = childRecords.begin(); iter != childRecords.end(); ++iter )
    {
        LSARecord *lsaRecord = *iter;
        if(!lsaRecord->warningMessages_generated.empty())
        {
            childHasWarning = true;
            break;
        }
    }

    return childHasWarning;
}

QModelIndex GuiModel::insertAutogeneratedInclude()
{
    if(!autogenIndex.isValid())
    {
        // Insert the new records
        QModelIndex rootIndex = this->index(0,0);
        bool isAutogenerated = true;
        QModelIndex newIndex;
        int newProjectCommentRow = getNewProjectCommentRow();//Fix to Bug #1025
        if(newProjectCommentRow < 0)//comment has been deleted from the project
            newIndex = insertFirstChild(LSAType::INCLUDE, "Auto-Generated Initial Coordinates", rootIndex, isAutogenerated);
        else
            newIndex = insertNewRecord(rootIndex,newProjectCommentRow+1,LSAType::INCLUDE, "Auto-Generated Initial Coordinates", isAutogenerated);

        std::string commentString("#Temporary POSG records auto-generated by the solver.  These will not be passed to the solver.");
        insertFirstChild( LSAType::COMMENT, commentString, newIndex, isAutogenerated); // HACK: isAutogenerated is true here to prevent a change of selection

        autogenIndex = QPersistentModelIndex(newIndex);
    }

    return QModelIndex(autogenIndex);
}

void GuiModel::addAdjustedPointsToProject(QList<std::string> POSGstrings, bool checkDuplicates)
{
    QList<LSAType> recordTypes;
    QList<bool> isAutogenList;
    QList<bool> isCommentedList;

    for(int i=0; i<POSGstrings.size(); i++)
    {
        recordTypes.append(LSAType::POSG);
        isAutogenList.append(false);
        isCommentedList.append(false);
    }

    //check for duplicate points prior to insertion
    bool duplicatesFound = false;
    QList<QPersistentModelIndex> duplicateList;
    for(int i=0;i<POSGstrings.size();i++)
    {
        std::string label = gnsstk::StringUtils::splitWithDoubleQuotes(POSGstrings.at(i),' ').at(1);
        QModelIndex index = QModelIndex();
        index = getNextIndex(index);
        while(index.isValid())
        {
            LSARecord *lsaRecord = getLSARecord(index);
            LSAType type = lsaRecord->getRecType();
            if(type.isPosition() || type.isPostProcessed())
            {
                if(lsaRecord->getLabel() == label)
                {
                    duplicatesFound = true;
                    duplicateList.append(QPersistentModelIndex(index));
                    break;
                }
            }
            index = getNextIndex(index);
        }
    }

    //ask user how to handle duplicates
    if(duplicatesFound && checkDuplicates)
    {
        QString message = QString("Add selected point(s) and disable POSG records with duplicate labels?");

        QMessageBox queryBox(QMessageBox::Question,"Warning - duplicate records found.",message,
                             QMessageBox::Cancel | QMessageBox::Ok,
                             NULL, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        int buttonClicked = queryBox.exec();//blocks
        switch (buttonClicked)
        {
            case QMessageBox::Cancel:
                return;
            case QMessageBox::Ok:
                break;
            default:
                return;
        }
    }

    // Create initial_coordinates.lsa include record if it doesn't already exist
    createInitialCoordsInclude();

    setMultipleData(duplicateList, Qt::Unchecked, Qt::CheckStateRole);

    insertNewRecords(initialCoordsIndex, 1, recordTypes, POSGstrings, isAutogenList, isCommentedList);

    // Refresh the gui model and UI once after all changes are made
    synchGuiModelToLSATree();
}

bool GuiModel::initialCoordsLacksComment(std::string initialCoordsFileName)
{
    std::ifstream istrm;
    istrm.open(initialCoordsFileName.c_str(), std::ios::in);
    std::string line;
    bool hasComment = false;
    while(1)
    {
        line = std::string("");
        std::getline(istrm,line);
        // TD need better handling of read failures : return -x
        if((istrm.eof() || !istrm.good()) && line.empty()) break;

        gnsstk::StringUtils::stripTrailing(line,"\n");
        gnsstk::StringUtils::stripTrailing(line,"\r");
        gnsstk::StringUtils::stripTrailing(line," ");

        if(line.find("# New .lsa file created") != std::string::npos)
        {
            hasComment = true;
            break;
        }
    }
    istrm.close();

    return !hasComment;
}

void GuiModel::convertAutogeneratedToNormalPoint(QModelIndex POSGIndex, bool printDebug)
{
    undoStack()->beginMacro("Converting autogens to normal points.");
    DebugTimer timer(printDebug);
    LOG_TIME(timer, "Beginning of GuiModel::convertAutogeneratedToNormalPoint");
    LSAPosG *lsaPoint = static_cast<LSAPosG*>(getLSARecord(POSGIndex));
    std::string lsaString = lsaPoint->getSingleLSAString();
    bool isCommented = lsaPoint->isCommented;

    LOG_TIME(timer, "Record casting GuiModel::convertAutogeneratedToNormalPoint");
    //remove this record from the autogenerated include file
    removeRecord(POSGIndex);
    LOG_TIME(timer, "Autogen removal in GuiModel::convertAutogeneratedToNormalPoint");

    //Remove the autogenerated points file, if it is empty
    if(autogenIndex.isValid())
    {
        LSAInclude *lsaInclude = static_cast<LSAInclude*>(getLSARecord(autogenIndex));
        if(lsaInclude->childRecords.size()==0 ||
           (lsaInclude->childRecords.size()==1 && lsaInclude->getChildRecord(0)->getRecType()==LSAType::COMMENT))
        {
            removeRecord(autogenIndex);
        }
    }

    LOG_TIME(timer, "Check to remove autogen file in GuiModel::convertAutogeneratedToNormalPoint");
    // Create initial_coordinates.lsa include record if it doesn't already exist
    if(!initialCoordsIndex.isValid())
    {
        // insert the include record into the tree
        QModelIndex rootIndex = this->index(0,0);
        initialCoordsIndex = insertFirstChild(LSAType::INCLUDE, INITIAL_COORDS_FILE_NAME, rootIndex, false);

        // clear the erroneous 'file not found' warning
        LSAInclude* lsaInclude = static_cast<LSAInclude*>(getLSARecord(initialCoordsIndex));
        lsaInclude->warningMessages_generated.clear();

        // Insert a creation comment if the file does not exist from a previous run
        std::string fileName(INITIAL_COORDS_FILE_NAME);
        include_lsapath(getProjectDirectory(),fileName);
        QFileInfo initCoord(QString::fromStdString(fileName));
        if(!initCoord.exists())
        {
            QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            std::string line("# New .lsa file created " + dateTime.toStdString());
            insertNewRecord(initialCoordsIndex, 0, LSAType::COMMENT, line);
        }
    }

    LOG_TIME(timer, "Check to add initial coordinates file in GuiModel::convertAutogeneratedToNormalPoint");

    //insert the POSG record that is being moved
    QModelIndex newPOSGIndex = insertLastChild(LSAType::POSG,lsaString,initialCoordsIndex,false);
    LOG_TIME(timer, "Insertion of POSG record being moved in GuiModel::convertAutogeneratedToNormalPoint");
    if(isCommented)
    {
        LSARecord *newPOSGRecord = getLSARecord(newPOSGIndex);
        newPOSGRecord->isCommented = true;
        synchItemToRecord(newPOSGIndex);
    }
    LOG_TIME(timer, "Performing last sync in GuiModel::convertAutogeneratedToNormalPoint");
    undoStack()->endMacro();
}

void GuiModel::addAutogenPointsToProject(const QModelIndexList &modelIndexes)
{
    undoStack()->beginMacro("Adding autogen points.");
    int modelIndexSize = modelIndexes.size();
    QList<QPersistentModelIndex> duplicateList;
    QList<LSAType> recordTypes;
    QList<bool> isAutogenList;
    QList<bool> isCommentedList;
    QList<std::string> POSGstrings;

    for(int indexVal = 0; indexVal < modelIndexSize; indexVal++)
    {
        LSARecord *lsaRecord = getLSARecord(modelIndexes[indexVal]);

        // Case: clicked Add to Project on the Auto-Gen include
        if(lsaRecord->getRecType() == LSAType::INCLUDE)
        {
            LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
            std::list<LSARecord*> &childRecords = lsaInclude->childRecords;
            std::list<LSARecord*>::iterator iter;

            for (iter = childRecords.begin(); iter != childRecords.end(); ++iter )
            {
                LSARecord *lsaRecord = *iter;
                // Check to make sure it is actually a POSG and not a comment
                if(lsaRecord->getRecType()==LSAType::POSG)
                {
                    duplicateList << QPersistentModelIndex(modelIndexes[indexVal]);
                    recordTypes << LSAType::POSG;
                    isAutogenList << false;
                    isCommentedList << lsaRecord->isCommented;
                    POSGstrings << lsaRecord->getSingleLSAString();
                }
            }
        }
        else // Case: Clicked Add to Project on a single or multiple auto-gen point(s) (can only be POSGs)
        {
            duplicateList << QPersistentModelIndex(modelIndexes[indexVal]);
            recordTypes << LSAType::POSG;
            isAutogenList << false;
            isCommentedList << lsaRecord->isCommented;
            POSGstrings << lsaRecord->getSingleLSAString();
        } // if(lsaType==INCLUDE)
    } // for(indexVal)

    createInitialCoordsInclude();
    setMultipleData(duplicateList, Qt::Unchecked, Qt::CheckStateRole);
    insertNewRecords(initialCoordsIndex, 1, recordTypes, POSGstrings, isAutogenList, isCommentedList);
    synchGuiModelToLSATree();
    undoStack()->endMacro();
}

bool GuiModel::suppressGuiUpdates(bool value)
{
    bool oldState = suppressGuiModelUpdates;
    suppressGuiModelUpdates = value;

    emit signalSuppressGuiUpdates(value);

    return oldState;
}

void GuiModel::moveRecord(bool up)
{
    undoStack()->beginMacro("Moving records.");
    QModelIndex index = getSingleSelectedRecord();
    QModelIndex parentIndex = parent(index);
    QModelIndex siblingIndex;
    LSAInclude *parentInclude = static_cast<LSAInclude*>(getLSARecord(parentIndex));
    int num_children = parentInclude->childRecords.size();
    int newRow;

    if(up)
    {
        if(index.row() == 0)//can't move up out of current include (yet)
        {
            undoStack()->endMacro();
            return;
        }
        newRow = index.row()-1;
        siblingIndex = parentIndex.child(newRow,0);
        while(itemIsBlankComment(siblingIndex) && newRow>=0)
        {
            newRow -= 1;//skip over hidden blank lines in the tree
            siblingIndex = parentIndex.child(newRow,0);
        }
        if(newRow < 0)//can't move up out of current include (yet)
        {
            undoStack()->endMacro();
            return;
        }
    }
    else
    {
        if(index.row() == num_children-1)//can't move down out of current include (yet)
        {
            undoStack()->endMacro();
            return;
        }
        newRow = index.row()+1;//skip over hidden blank lines in the tree
        siblingIndex = parentIndex.child(newRow,0);
        while(itemIsBlankComment(siblingIndex) && newRow<num_children)
        {
            newRow += 1;
            siblingIndex = parentIndex.child(newRow,0);
        }
        if(newRow == num_children)//can't move down out of current include (yet)
        {
            undoStack()->endMacro();
            return;
        }
    }

    LSARecord * lsaRecord = getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();
    bool isDisabled = lsaRecord->isCommented;
    std::string lsaString = lsaRecord->getSingleLSAString();
    std::string newFile;
    std::vector<std::string> modifierStrings;
    if(lsaType == LSAType::INCLUDE)
    {
        parseIncludeFilename(lsaString, newFile, modifierStrings);
        lsaString = newFile;
        LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
        lsaFile->saveLSAIncludeRecord(lsaInclude);//save the state of edited/disabled/enabled child records - Bug 917
    }

    removeRecord(index, false, true);

    QModelIndex newIndex = insertNewRecord(parentIndex, newRow, lsaType, lsaString, false, isDisabled);
    Q_ASSERT(newIndex.isValid()); // If this assert fails, it means lsaString is likely invalid.  Take a look at how it is formed.
    if (!newIndex.isValid())
    {
        undoStack()->endMacro();
        return; // This prevents a crash, but it causes the moved record to be deleted instead of moved
    }

    if(isDisabled)//Bug 917
        getLSARecord(newIndex)->isCommented = true;

    if((lsaType == LSAType::INCLUDE) && newIndex.isValid())
    {
        LSAInclude* newInclude = static_cast<LSAInclude*>(getLSARecord(newIndex));
        newInclude->parseModifiers(modifierStrings);
        synchItemToRecord(newIndex);
        emit dataChanged(newIndex,newIndex);//to get tree view scaling to update
    }
    undoStack()->endMacro();
}

std::vector<std::string> GuiModel::findNonPosDuplicateLabelWarnings(QModelIndex parentIncludeIndex)
{
    std::vector<std::string> returnVector;
    QModelIndex currentIndex = parentIncludeIndex;

    //look for duplicate warning messages within this branch
    returnVector.clear();
    while((currentIndex = getNextDescendant(parentIncludeIndex,currentIndex)).isValid())
    {
        LSARecord *lsaRecord = getLSARecord(currentIndex);
        LSAType type = lsaRecord->getRecType();
        std::set<std::string> warnings = lsaRecord->warningMessages_generated;
        for (std::set<std::string>::iterator iter = warnings.begin(); iter != warnings.end(); ++iter)
        {
            std::string message = *iter;
            if ( ( message.find("Duplicate") != std::string::npos ) &&
                 (type != LSAType::POSC) && //ignore duplicate warnings on position labels -- don't autocorrect
                 (type != LSAType::POSG))
            {
                returnVector.push_back(message);
            }
        }
    }

    return returnVector;
}

QModelIndex GuiModel::getIndexFromPointLabel(std::string pointName)
{
    //find the corresponding element in the tree
    QModelIndex currentIndex = getNextIndex();
    while ( currentIndex.isValid() )
    {

        if (currentIndex.column() == 0)
        {
            LSARecord * lsaRecord = getLSARecord(currentIndex);
            LSAType currentType = lsaRecord->getRecType();
            if((currentType==LSAType::POSG) && isActive(currentIndex))
            {
                LSAPosG *lsaPOSG(static_cast<LSAPosG *>(lsaRecord));
                if(lsaPOSG->label==pointName)
                    break;
            }
            else if((currentType==LSAType::POSC) && isActive(currentIndex))
            {
                LSAPosC *lsaPOSC(static_cast<LSAPosC *>(lsaRecord));
                if(lsaPOSC->label==pointName)
                    break;
            }
            else if((currentType==LSAType::MEAN) && isActive(currentIndex))
            {
                LSAMean *lsaMean(static_cast<LSAMean *>(lsaRecord));
                if(lsaMean->label==pointName)
                    break;
            }
            else if((currentType==LSAType::ENUO) && isActive(currentIndex))
            {
                LSAEnuo *lsaEnuo(static_cast<LSAEnuo *>(lsaRecord));
                if(lsaEnuo->getLabel()==pointName)
                    break;
            }
        }
        currentIndex = getNextIndex(currentIndex);
    }

    return currentIndex;//caller must check for validity
}

//Takes part of the 1st column of the measurement residuals section of the .bin
//file, extracts the record index added to .dat records on output, and matches
//it up with the corresponding LSARecord pointer valud in the inputOutputMap
QModelIndex GuiModel::getIndexFromTag(std::string ID)
{
    unsigned int tableDatIndex = atoi(ID.c_str());

    std::map<int,QModelIndex>::iterator it;
    it = inputOutputMap.find(tableDatIndex);
    if(it != inputOutputMap.end())
        return it->second;
    else
        return QModelIndex();
}

//TODO - Determine if we still need this after reverting to non-persistent autogen points
QModelIndex GuiModel::getIndexFromAutogenPointLabel(std::string pointName)
{
    int numChildren = getItem(autogenIndex)->childCount();
    QModelIndex childIndex;
    for(int row=0;row<numChildren;row++)
    {
        childIndex = index(row, 0, autogenIndex);
        if(isActive(childIndex))
        {
            LSARecord *lsaRecord = getLSARecord(childIndex);
            if(lsaRecord->getRecType() == LSAType::POSG)
            {
                LSAPosG *lsaPoint = static_cast<LSAPosG*>(lsaRecord);
                if(lsaPoint->label == pointName)
                    return childIndex;
            }
        }
    }

    return QModelIndex();
}

//LSAFile::parseIncludeFilename should duplicate this!
void GuiModel::parseIncludeFilename(std::string line, std::string &newFile, std::vector<std::string> &modifierStrings)
{
    int n;
    std::vector<std::string> F;

    std::string str(line);                // copy const line
    gnsstk::StringUtils::stripTrailing(str,"\n");
    gnsstk::StringUtils::stripTrailing(str,"\r");
    gnsstk::StringUtils::stripTrailing(str," ");
    gnsstk::StringUtils::stripLeading(str," ");

    // cut out the text notes so that it isn't considered as a modifier.
    extractXMLFromString(str, RecordTags::TextNotes, true);
    // split into fields
    F = gnsstk::StringUtils::splitWithDoubleQuotes(str,' ');
    n = F.size();

    if (n < 2)
    {
        return;
    }

    newFile = F[1];

    if (n >2)
    {
        for (size_t i = 2; i < n; ++i )
        {
            modifierStrings.push_back(F.at(i));
        }
    }

    return;

}
/* In v1.0.0 the autogenerated points INCLUDE was made into a persistent
 * file, AutogeneratedPoints.lsa, where users could edit points and change
 * the POSG records to lose the AUTOGEN tag, but the file would remain.  Users
 * found this confusion, so in v1.2.0 we reverted to pre-v1.0.0 release behavior.
 * This method handles projects that may have been created between 1.0.0 and 1.2.0.
 * Returns true if a legacy file was found, false otherwise.
 */
bool GuiModel::removeLegacyAutogenPointsFile()
{
    bool legacyFileFound = false;
    std::list<LSAPosG*> nonAUTOGENPoints;
    nonAUTOGENPoints.clear();

    //search the project for an INCLUDE record name AutogeneratedPoints.lsa
    QModelIndex currentIndex = getNextIndex();
    while(currentIndex.isValid())
    {
        LSARecord *lsaRecord = getLSARecord(currentIndex);
        if(lsaRecord->getRecType() == LSAType::INCLUDE)
        {
            LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
            std::string includeFileName = lsaInclude->getLSAPath();
            if(includeFileName.find("AutogeneratedPoints.lsa") != std::string::npos)
            {
                //look for non-AUTOGEN points
                std::list<LSARecord*> *children = &lsaInclude->childRecords;
                for (std::list<LSARecord*>::iterator iter = children->begin(); iter != children->end(); ++iter)
                {
                    LSARecord *childRecord = *iter;
                    if (childRecord->getRecType() == LSAType::POSG)
                    {
                        LSAPosG *lsaPoint = static_cast<LSAPosG*>(childRecord);
                        //if non-AUTOGEN points are found, put them into initial_coordinates.lsa
                        if(!lsaPoint->isAutogenerated)
                            nonAUTOGENPoints.push_back(lsaPoint);
                    }
                }

                legacyFileFound = true;
                //remove AutogeneratedPoints.lsa record from the project
                removeRecord(currentIndex, false,false, true); // This should NOT be undo-able
                //remove the reference to the file in the project
                save();
                //remove AutogeneratedPoints.lsa from disk
                QFile legacyFile(QString::fromStdString(lsaInclude->getLSAPath()));
                legacyFile.remove();
                //stop searching the project
                break;
            }//end of if on includefile = AutogeneratedPoints.lsa
        }//end of if on type==INCLUDE
        currentIndex = getNextIndex(currentIndex);
    }//end of while

    //move non-AUTOGEN points into the intial_coordinates.lsa file
    if(legacyFileFound && nonAUTOGENPoints.size()>0)
    {
        // Create initial_coordinates.lsa include record if it doesn't already exist
        if(!initialCoordsIndex.isValid())
        {
            // insert the include record into the tree
            std::string fileName(INITIAL_COORDS_FILE_NAME);
            include_lsapath(getProjectDirectory(),fileName);

            QModelIndex rootIndex = this->index(0,0);
            initialCoordsIndex = insertFirstChild(LSAType::INCLUDE, INITIAL_COORDS_FILE_NAME, rootIndex, false);

            // clear the erroneous 'file not found' warning
            LSAInclude* lsaInclude = static_cast<LSAInclude*>(getLSARecord(initialCoordsIndex));
            lsaInclude->warningMessages_generated.clear();

            // Insert a creation comment if the file does not exist from a previous run
            QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            std::string line("# New .lsa file created " + dateTime.toStdString());
            insertNewRecord(initialCoordsIndex, 0, LSAType::COMMENT, line, false, false, true); // This should NOT be undo-able
        }

        //insert the non-AUTOGEN points
        for (std::list<LSAPosG*>::iterator iter = nonAUTOGENPoints.begin(); iter != nonAUTOGENPoints.end(); ++iter)
        {
            LSAPosG *lsaPoint = *iter;
            std::string lsaString = lsaPoint->getSingleLSAString();
            bool isCommented = lsaPoint->isCommented;

            //insert the non-AUTOGEN POSG record
            QModelIndex newPOSGIndex = insertLastChild(LSAType::POSG,lsaString,initialCoordsIndex,false);
            if(isCommented)
            {
                LSARecord *newPOSGRecord = getLSARecord(newPOSGIndex);
                newPOSGRecord->isCommented = true;
                synchItemToRecord(newPOSGIndex);
            }
        }
        //add initial_coordinates.lsa INCLUDE record to the project on disk
        save();
    }

    return legacyFileFound;
}

//only gets the saved state of the root include.  Does not know if child includes have modified children
bool GuiModel::getIsRootModified()
{
    return static_cast<LSAInclude*>(getLSARecord(getNextIndex()))->getIsModified();
}

//only sets the saved state of the root include.
void GuiModel::setIsRootModified(bool isMod)
{
    static_cast<LSAInclude*>(getLSARecord(getNextIndex()))->setIsModified(isMod);
}

bool GuiModel::projectIsLoaded()
{
    return rootItem->childCount() > 0;
}

bool GuiModel::projectIsModified()
{
    QModelIndex rootIndex = index(0,0);
    if (rootIndex.isValid() && !descendantIncludeIsModified(rootIndex).isValid())
        return false;
    else
        return true;
}

//returns true if anything in the selection is an INCLUDE
bool GuiModel::includeRowIsSelected()
{
    QModelIndexList selectedRows = getSelectedRows();

    if(selectedRows.size() > 0)
    {
       foreach(QModelIndex index, selectedRows)
       {
           LSARecord* lsaRecord = getLSARecord(index);
           LSAType recType = lsaRecord->getRecType();

           if(recType == LSAType::INCLUDE)
           {
               return true;
           }
       }
    }
    return false;
}

bool GuiModel::hasAutogeneratedRow()
{
    QModelIndexList selectedRows = getSelectedRows();

    if(selectedRows.size() > 0)
    {
       foreach(QModelIndex index, selectedRows)
       {
           if(isAutogenIndex(index))
               return true;

           // If a record's ancestor is an autogenerated record, it too must be one
           auto parent = index.parent();
           while(parent.isValid())
           {
               if(isAutogenIndex(parent))
               {
                   return true;
               }
               parent = parent.parent();
           }
       }
    }
    return false;

}

bool GuiModel::isAutogenIndex(const QModelIndex &recordIndex) const
{
    if(!recordIndex.isValid())
        return false;

    LSARecord* lsaRecord = getLSARecord(recordIndex);
    return lsaRecord->isAutogenerated;
}

bool GuiModel::selectionIsAllAutogen() const
{
    QModelIndexList selectedRows = getSelectedRows();
    if(selectedRows.empty())
        return false;

    bool allAutogen = true;
    int counter = 0;
    while(counter < selectedRows.size() && allAutogen)
    {
        allAutogen = isAutogenIndex(selectedRows[counter]);
        counter++;
    }

    return allAutogen;
}

int GuiModel::getNewProjectCommentRow()
{
    QModelIndex rootIndex = getNextIndex();
    std::list<LSARecord*> *children = &static_cast<LSAInclude*>(getLSARecord(rootIndex))->childRecords;
    int row=-1;
    int ctr=0;

    for (std::list<LSARecord*>::iterator iter = children->begin(); iter != children->end(); ++iter)
    {
        LSARecord *childRecord = *iter;
        if(childRecord->getRecType() == LSAType::COMMENT)
        {
            std::string match_string = static_cast<LSAComment*>(childRecord)->lineContents.substr(0,19);
            if(match_string == std::string("New project created") ||
               match_string == std::string("iobconverter, Ver. "))
            {
                row = ctr;
                break;
            }
        }
        ctr++;
    }

    return row;
}

void GuiModel::setInitialCoordsIndex()
{
    initialCoordsIndex = QPersistentModelIndex();

    //check for the initial coords file
    QModelIndex index = QModelIndex();

    index = getNextIndex(index);
    while(index.isValid())
    {
        LSARecord *lsaRecord = getLSARecord(index);
        if(lsaRecord->getRecType() == LSAType::INCLUDE)
        {
            LSAInclude *lsaInclude = static_cast<LSAInclude*>(lsaRecord);
            if(lsaInclude->getLSAPath() == INITIAL_COORDS_FILE_NAME)
            {
                initialCoordsIndex = QPersistentModelIndex(index);
                break;
            }
        }
        index = getNextIndex(index);
    }
}

/**
 * @brief GuiModel::undo Performs an undo action if possible
 */
void GuiModel::undo()
{
    if (undoStack()->canUndo())
    {
        undoStack()->undo();
    }
}

/**
 * @brief GuiModel::redo Performs a redo action if possible
 */
void GuiModel::redo()
{
    if (undoStack()->canRedo())
    {
        undoStack()->redo();
    }
}

/**
 * @brief GuiModel::pushChangeValue Create a new undo/redo object for edits to an existing record
 */
void GuiModel::pushChangeValue(const QPersistentModelIndex& index, const QList<QPersistentModelIndex>& selectedIndices, std::string oldValue, std::string newValue, bool isDerivativeChange)
{
    m_stack->push(new ChangeValueCommand(index, selectedIndices, oldValue, newValue, isDerivativeChange, this));
}

/**
 * @brief GuiModel::updateRecord Manually updates the record. Called for undo/redo
 */
bool GuiModel::updateRecord(QPersistentModelIndex& index, std::string lsaString)
{
    LSARecord *lsaRecord = getLSARecord(index);
    LSAType type = lsaRecord->getRecType();

    bool result = false;
    if(type != LSAType::COMMENT)
    {
        if(lsaString.at(0) == '#')
        {
            lsaRecord->isCommented = true;
        }
        else
        {
            lsaRecord->isCommented = false;
        }
    }


    lsaRecord->fromString(lsaString);

    result = true;

    return result;
}

// Like getIndexFromPointLabel but it doesn't care if the records are enabled
LSARecord *GuiModel::getPointWithLabel(std::string label)
{
    //find the corresponding element in the tree
    QModelIndex currentIndex = getNextIndex();
    LSARecord * lsaRecord = nullptr;
    while ( currentIndex.isValid() )
    {

        if (currentIndex.column() == 0)
        {
            lsaRecord = getLSARecord(currentIndex);
            LSAType currentType = lsaRecord->getRecType();
            if((currentType==LSAType::POSG))
            {
                LSAPosG *lsaPOSG(static_cast<LSAPosG *>(lsaRecord));
                if(lsaPOSG->label==label)
                    break;
            }
            else if((currentType==LSAType::POSC))
            {
                LSAPosC *lsaPOSC(static_cast<LSAPosC *>(lsaRecord));
                if(lsaPOSC->label==label)
                    break;
            }
            else if((currentType==LSAType::MEAN))
            {
                LSAMean *lsaMean(static_cast<LSAMean *>(lsaRecord));
                if(lsaMean->label==label)
                    break;
            }
            else if((currentType==LSAType::ENUO))
            {
                LSAEnuo *lsaEnuo(static_cast<LSAEnuo *>(lsaRecord));
                if(lsaEnuo->getLabel()==label)
                    break;
            }
        }
        lsaRecord = nullptr;
        currentIndex = getNextIndex(currentIndex);
    }

    return lsaRecord;//caller must check for validity
}

// Refresh the index/parent->root chain. Necessary to enable undo/redo after a parent is deleted
QVector<IndexRowColumnParentChainItem> GuiModel::buildSingleIndexChain(QPersistentModelIndex modifiedIndex)
{
    QVector<IndexRowColumnParentChainItem> parentChain;

    // Necessary check since when inserting records parentTree[0].index could be the root index.
    // Thus, trying to create a parent off of it would segfault.
    if(!isRootInclude(modifiedIndex))
    {
        // Add first item
        IndexRowColumnParentChainItem item;
        item.index = modifiedIndex;
        item.row = item.index.row();
        item.col = item.index.column();
        item.parentIndex = item.index.parent();
        parentChain.append(item);

        // Fill out the rest of the tree
        int counter = 0;
        while(!isRootInclude(parentChain[counter].parentIndex))
        {
            IndexRowColumnParentChainItem newItem;
            newItem.index = parentChain[counter].parentIndex;
            newItem.row = newItem.index.row();
            newItem.col = newItem.index.column();
            newItem.parentIndex = newItem.index.parent();
            parentChain.append(newItem);
            counter++;
        }
    }
    else
    {
        IndexRowColumnParentChainItem item;
        item.index = modifiedIndex;
        item.col = 0;
        item. row = 0;
        parentChain.append(item);
    }

    return parentChain;
}

// Refresh the index/parent->root chain. Necessary to enable undo/redo after a parent is deleted
void GuiModel::refreshSingleIndexChain(QVector<IndexRowColumnParentChainItem>& parentChain)
{
    // Necessary check since when inserting records parentTree[0].index could be the root index.
    // Thus, trying to create a parent off of it would segfault.
    if(!isRootInclude(parentChain[0].index))
    {
        for(int i = parentChain.size()-1; i > 0; i--)
        {
            parentChain[i].index = index(parentChain[i].row, parentChain[i].col, parentChain[i].parentIndex);
            parentChain[i-1].parentIndex = parentChain[i].index;
        }
        parentChain[0].index = index(parentChain[0].row, parentChain[0].col, parentChain[0].parentIndex);
    }
}

// Build the multi-indice index/parent->root chain. Necessary to enable multi-select/focus after inserting/removing records
QVector<QVector<IndexRowColumnParentChainItem>> GuiModel::buildMultipleIndicesChain(QList<QPersistentModelIndex> selectedIndices)
{
    QVector<QVector<IndexRowColumnParentChainItem>> multipleIndicesChain;
    for(auto& selectedIndex : selectedIndices)
    {
        QVector<IndexRowColumnParentChainItem> vectorItem = buildSingleIndexChain(selectedIndex);
        multipleIndicesChain.append(vectorItem);
    }

    return multipleIndicesChain;
}

// Refresh the multi-Indice index/parent->root chain
void GuiModel::refreshMultipleIndicesChain(QVector<QVector<IndexRowColumnParentChainItem>>& multipleIndicesChain)
{
    for(auto& vectorItem : multipleIndicesChain)
    {
        refreshSingleIndexChain(vectorItem);
    }
}

// Sync the updated GuiModel and push any signals after an undo/redo action
void GuiModel::refreshAfterUndoRedo(QPersistentModelIndex undoRedoIndex)
{
    setIsRootModified(true);

    QVector<int> roles;
    roles.append(Qt::EditRole);
    synchGuiModelToLSATree();
    emitDataChanged(undoRedoIndex, roles);
    validateModelAndRegenerateMaps();
}
