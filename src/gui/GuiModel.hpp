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
#ifndef LSAMODEL_H
#define LSAMODEL_H

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <sstream>
#include <iostream>
#include <fstream>
#include <functional>

#include <LSAFile.hpp>
#include <FinalAdjustedPosition.hpp>
#include <LSARecord.hpp>
#include <lsabinary.hpp>
#include <ChangeValueCommand.hpp>
#include <InsertRecordsCommand.hpp>
#include <RemoveRecordsCommand.hpp>

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QModelIndex>
#include <QVariant>
#include <QPersistentModelIndex>
#include <memory>
#include <QUndoStack>

#include <DebugTimer.hpp>

// Note for test harnesses. Reloading a project (explicitly or implicitly) will
// require a reinitialization a GuiModel or index gathered from a gui model
// In other words don't make a root index, reload a project and expect the original
// root index to work.

class GuiModelItem;

/// Class GuiModel contains all of the data that widgets and view in the MainWindow need to display.
/// The MainWindow stores all information about the adjustment network in this model.  MainWindow
/// slots responding to signals from child widgets will first update this model then call
/// MainWindow::updateWidgets to update the GUI.
/// Note: this class is largely defined by its parent class QAbstractItemModel.  Refer to the Qt
/// documentation for QAbstractItemModel for more detail.
class GuiModel : public QAbstractItemModel
{
    Q_OBJECT

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests
    friend class ChangeValueCommand; ///< Needed to access GuiModel methods
    friend class RemoveRecordsCommand;
    friend class InsertRecordsCommand;

signals:
    void pushWarning(QString msg);        ///< Push a 'warning' formatted string to the status window
    void pushError(QString msg);          ///< Push a 'error' formatted string to the status window
    void pushStatus(QString msg);         ///< Push a 'status' formatted string to the status window
    void signalSuppressGuiUpdates(bool);
    /// Push the message that something has been dropped into the treeView to the MainWindow, bc that's where all of the
    /// code to handle file conversion, new records, etc. is located.
    void signalDataDroppedOnTree(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

public:
    QUndoStack *undoStack() const {return m_stack;};
    void undo();
    void redo();

    // Refresh the gui model after completing an undo/redo action
    void refreshAfterUndoRedo(QPersistentModelIndex undoRedoIndex);

    // Index/parent->root chain for main record in undo/redo
    QVector<IndexRowColumnParentChainItem> buildSingleIndexChain(QPersistentModelIndex changeIndex);
    void refreshSingleIndexChain(QVector<IndexRowColumnParentChainItem>& parentChain);

    // Multiple-indice index/parent->root chain for additional selected records in change value and/or remove records undo/redo commands.
    QVector<QVector<IndexRowColumnParentChainItem>> buildMultipleIndicesChain(QList<QPersistentModelIndex> selectedIndices);
    void refreshMultipleIndicesChain(QVector<QVector<IndexRowColumnParentChainItem>>& multipleIndicesChain);

    /// Create a new undo/redo object for edits to an existing record
    /// @param index Index that is being modified
    /// @param selectedIndices Complete list of selected records (multi-select) which allows selection of all edited records
    /// @param oldValue LSA formatted string of the record before the edit
    /// @param newValue LSA formatted string of the record after edits
    /// @param isDerivativeChange Determines if the record being edited is a resultant change of another edit (e.g. update references in the treeView)
    void pushChangeValue(const QPersistentModelIndex& index,
                         const QList<QPersistentModelIndex>& selectedIndices,
                         std::string oldValue,
                         std::string newValue,
                         bool isDerivativeChange);

    static const std::string CHILD_WARNING_MESSAGE;
    static const std::string INVALID_COVARIANCE_MESSAGE;
    static const std::string INVALID_SCALING_MESSAGE;
    static const std::string INVALID_SIGMA_MESSAGE;
    static const std::string INVALID_POSG_POLE_MESSAGE;
    static const std::string INVALID_POSC_POLE_MESSAGE;
    static const std::string INITIAL_COORDS_FILE_NAME;
    static const QString QSETTINGS_SHOWCOMMENTS;
    static const QString QSETTINGS_VISIBLE_RECORDEDITOR;
    static const QString QSETTINGS_VISIBLE_MAP;
    static const QString QSETTINGS_VISIBLE_ADJUSTEDPOSITIONS;
    static const QString QSETTINGS_VISIBLE_STATIONDATA;
    static const QString QSETTINGS_VISIBLE_INITIALPOSITIONS;
    static const QString QSETTINGS_VISIBLE_HISTOGRAM;

    // TODO: make this private and move setters in MainWindow into this class
    std::map<int, QModelIndex> inputOutputMap; ///<maps tags in .dat and .h5 to records in the guiModel


    /// Default constructor
    GuiModel();

    /// Constructor given a lsaFileName and parent object
    /// @param lsaFileName name of the lsa file to open
    /// @param QObject parent for the model
    GuiModel(std::string lsaFileName, QObject *parent = 0);

    /// Destructor
    ~GuiModel();

    void setSelectionModel(QItemSelectionModel *inputModel) { selectionModel = inputModel; }
    // Begin QAbstractItemInterface

    /// Get data for the model item at index
    /// @param index index for an item in the model
    /// @param QtRole for this data
    /// @return QVariant containing the data
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    /// Get data for the root model item that contains header data
    /// @param section column of data to get
    /// @param orientation
    /// @param role the QtRole for the data
    /// @return QVariant containing the data
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    /// Get the QModelIndex corresponding to the row (child num) and column of an item
    /// @param row the row (child) number of the item
    /// @param the column number of the item
    /// @param parent the parent of the item
    /// @return QModelIndex for the indicated item
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    /// Get the QModelIndex for a child item's parent
    /// @param index the child item
    /// @return QModelIndex for the child's parent
    QModelIndex parent(const QModelIndex& index) const;

    /// Get the number of rows (children) for an item
    /// @param parent the item in question
    /// @return the number of rows
    int rowCount(const QModelIndex& parent = QModelIndex() ) const;

    /// Get the number of columns for an item
    /// @param parent the item in question
    /// @return the number of columns
    int columnCount(const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    /// Set the data on a model item
    /// @param index QModelIndex for the item to set
    /// @param value QVarient containing the data value to set
    /// @param role QtRole for the data
    /// @return true on success
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    /// Utility function to set data on multiple items. Exists to make sure dataChanged signal
    /// is called once instead of calling it for each individual QModelIndex. Note, do not
    /// call if indexes do not share same parent index
    /// @param List of indexes for which to set items
    /// @param value QVariant containing data value to set
    /// @param role QtRole for data
    void setMultipleData(const QList<QPersistentModelIndex>& indexList, const QVariant& value, int role = Qt::EditRole);

    /// Set header data contained in the root item of the tree
    /// @param section
    /// @param orientation
    /// @param value to set
    /// @param role QtRole to set
    /// @return true on success
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole);

    /// Insert columns for an item
    /// @param parent parent of the item to set
    /// @param position position of the item to set
    /// @param columns number of columns to insert
    /// @return true on success
    bool insertColumns(int position, int columns, const QModelIndex& parent = QModelIndex() );

    /// Remove columns from an item
    /// @param parent parent of the item to modify
    /// @param position position of the first column to remove
    /// @param columns number of columns to insert
    /// @return true on success
    bool removeColumns(int position, int columns, const QModelIndex& parent = QModelIndex() );

    /// Insert rows in an item
    /// @param parent parent of the item to modify
    /// @param position position of the first row to insert
    /// @param rows number of rows to insert
    /// @return true on success
    bool insertRows(int position, int rows, const QModelIndex& parent = QModelIndex() );

    /// Remove rows from an item
    /// @param parent parent of the item to modify
    /// @param position position of the first row to remove
    /// @param rows number of rows to remove
    /// @return true on success
    bool removeRows(int position, int rows, const QModelIndex& parent = QModelIndex() );

    // End QAbstractItemInterface


    /// Insert a new record into the GuiModel directly after the item indicated by index
    /// @param index new item will be inserted as the first sibling after index
    /// @param recordType LSAType of the new record
    /// @param includePath (only used when inserting an INCLUDE file) path to the new file to insert
    /// @return true on succesful insert

    QModelIndex insertSiblingAfter(LSAType recordType, std::string line = "", QModelIndex index = QModelIndex());
    QModelIndex insertFirstChild  (LSAType recordType, std::string line = "", QModelIndex parentIndex = QModelIndex(), bool isAutogenerated = false);
    QModelIndex insertLastChild   (LSAType recordType, std::string line = "", QModelIndex parentIndex = QModelIndex(), bool isAutogenerated = false);
    void        insertFromStreamAfter(std::stringstream& ss, QModelIndex parent = QModelIndex(), int row = -1);
    //This was added as a fix to Bug #1088 - Currently only works if all records are of same type.  But, currently only used to insert block of POSGs for autogen include.

    QModelIndexList insertNewRecords(QModelIndex parentIndex, int rowPosition, QVector<LSARecord*> lsaRecordList, bool isUndoRedoAction = false);
    QModelIndexList insertNewRecords(QModelIndex parentIndex, int rowPosition, QList<LSAType> recordTypes, QList<std::string> recordStrings, QList<bool> isAutogeneratedList, QList<bool> isCommentedList, bool isUndoRedoAction = false);
    QModelIndexList insertRecordsPost(QList<LSARecord *> newRecords, QModelIndex parentIdx, int rowPosition, QList<bool> isAutogeneratedList);

    /// remove a record from the GuiModel
    /// @param removeIndex index of record to remove
    /// @return index of previous record in the tree
    QModelIndex removeRecord(QModelIndex removeIndex, bool deleteReferences = false, bool moving_initial_coord = false, bool isUndoRedoAction = false, QVector<QVector<IndexRowColumnParentChainItem>> removedIndicesTree = QVector<QVector<IndexRowColumnParentChainItem>>());
    void removeMembersOfDirGroup(std::string dirGroupLabel);
    bool removeSingleMemberOfDirGroup(std::string dirGroupLabel);
    bool isRecordHidden(const QModelIndex& index);
    // Save
    /// Save the contents of the lsaModel to lsaFiles on disk.  Filenames and paths are determined
    /// by the filenames and paths stored in individual LSAInclude records
    void save();
    /// Save the descendants of an lsaInclude to lsaFiles on disk.
    void saveBranch(QModelIndex includeIndex);
    void clearDescendantIncludeIsModified(QModelIndex includeIndex);
    /// Returns the QModelIndex of a descendant include with the isModified flag set to true
    /// If there are no modified include descendants, an invalid QModelIndex is returned
    QModelIndex descendantIncludeIsModified(QModelIndex includeIndex);
    void setParentIncludeModified(QModelIndex modifiedChildIndex);
    bool projectIsLoaded();
    bool projectIsModified();
    bool getIsRootModified();
    void setIsRootModified(bool isMod);

    /// Accessor for the list of records the LSAFile reader was unable to parse.
    /// @return vector<std::string> containing every lsaFile line not successfully parsed
    std::vector<std::string> getBadRecords() { return lsaFile->getErrorsAndWarnings(); }
    QStringList getIncludedLSAFiles();

    QList<LSARecord*> getChildRecords(QModelIndex index);

    /// Get the RecordIndex for the currently selected record
    /// @return QModelIndex for the selected record
    QModelIndexList getSelectedRows() const;
    
    // getSelectedRows calls selectedRows(), which is actaully quite expensive when called over 20
    // times when handling the selection from Histogram. This is used only when reacting after the selection has changed.
    void enableSelectedRowsCache(bool bUse);

    QModelIndex getSingleSelectedRecord();
    QModelIndex getLastSelectedRecord();
    void selectReferencingRecords();

    void select(QModelIndex index);
    void select(QItemSelection selection) { select(selection.indexes()); }
    void select(QModelIndexList list, bool sort = false);

    /// Attempts to select the last child of the root include
    /// If the root  include has no children, the root include is selected and this method returns false
    /// @return true if the last child of the root include is selected, false if there are no children and root include is selected
    bool selectLastChildOfRootInclude();

    /// Get the LSARecord* for the item at QModelIndex
    /// @param index QModelIndex specifying an item
    /// @return lsaRecord for the item
    LSARecord* getLSARecord(const QModelIndex &index) const;
    LSAFile* getLSAFile() { return lsaFile; }

    /// This method is potentially expensive.  Try to use getLSARecord(index) instead.
    /// @param the lsaRecord to search for
    /// @return the QModelIndex corresponding to lsaRecord
    QModelIndex getIndexForRecord(LSARecord* lsaRecord) const;

    bool includeRowIsSelected();

    int getNewProjectCommentRow();

    QModelIndex getIndexFromPointLabel(std::string pointName);

    ///looks in inputOutputMap for ID extracted from meas string to find corresponding LSARecord pointer
    QModelIndex getIndexFromTag(std::string meas);

    /// Get the next item in the tree in this order:
    /// 1) CurrentItem's first child
    /// 2) CurrentItem's next sibling
    /// 3) Recurse up the tree looking for the next available sibling of a parent
    /// @param currentItem QModelIndex for a given item
    /// @return QModelIndex for the "next" item in the tree as defined above
    QModelIndex getNextIndex(QModelIndex currentIndex = QModelIndex() ) const;
    QModelIndex getPreviousIndex(QModelIndex currentIndex = QModelIndex() );
    QModelIndex getLastIndex(QModelIndex parentIndex);
    QModelIndex getNextDescendant(QModelIndex parentIndex, QModelIndex currentIndex);
    QModelIndex getLastDescendant(QModelIndex parentIndex);

    /// Returns true if currentIndex is a descendant of parentIndex
    bool isDescendant(QModelIndex parentIndex, QModelIndex currentIndex);

    /// Get the previous item in the tree in this order:
    /// 1) currentItem's previous sibling
    /// 2) item's parent
    /// @param currentItem QModelIndex for a given item
    /// @return QModelIndex for the "previous" item in the tree as defined above
    QModelIndex getPreviousParentIndex(QModelIndex currentIndex);

    std::string getProjectDirectory() const { return lsaFile->getProjectDirectory(); }

    bool rootIncludeIsSelected() { return selectionModel->isSelected( index(0,0) ); }
    bool indexIsSelected(QModelIndex index) { return selectionModel->isSelected(index); }
    bool isRootInclude(QModelIndex input) const { return index(0,0) == input; }
    bool hasAutogeneratedRow();

    bool isAutogenIndex(const QModelIndex &recordIndex) const;
    bool selectionIsAllAutogen() const;

    /// Returns true if any parent and one of its children are selected
    bool parentAndChildSelected();

    void synchSelectedItemsToRecords();

    LSARecordMap*           getPoints()       { return &lsaFile->pointMap; }

    std::vector<LSARecord*> getMeasurements() { return lsaFile->measurements; }

    LSAHGHTMap*     getHeightMap()     { return hghtMap; }
    LSAHGHTMap      getFullHeightMap();//includes disabled HGHTs
    LSAUNCRMap*      getUncrMap()      { return uncrMap; }
    LSAUNCRMap      getFullUncrMap();//includes disabled UNCRs
    LSAVSCAMap*        getVarScalingMap() { return vscaMap; }
    LSAVSCAMap        getFullVarScalingMap();//includes disabled VSCAs
    LSADirGroupMap*   getDirGroupMap()   { return dgrpMap; }
    LSADirGroupMap  getFullDirGroupMap();//includes disabled DGRPs
    QPersistentModelIndex getAutogenIndex() {return autogenIndex;}
    QPersistentModelIndex getInitialCoordsIndex() {return initialCoordsIndex;}

    QItemSelectionModel *selectionModel; ///< pointer to selection model that tracks selected records

    std::string filterString;
    bool filterExact = false;
    bool bMatchAllFilter = false; // match ANY or ALL

    std::vector<FinalAdjustedPosition> outputPositions;
    int solutionNumDOF;
    QString confidenceInterval;

    bool itemIsComment(QModelIndex index);
    bool itemIsBlankComment(QModelIndex index);
    bool isMemberOfDirGroup(QModelIndex index, std::string dirGroupLabel);

    void renamePositionLabel(std::string oldLabel, std::string newLabel);
    void renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel);

    unsigned int hashTree();

    // TODO: Make this private once we are able to funnel all insert, delete and edit operations through the gui model
    void validateModelAndRegenerateMaps();
    void validateUNCRUnits(LSARecord *lsaRecord);
    void checkForMissingAndDisabledReferences(QModelIndex index,
                                             const std::set<std::string>& disabledPositions, const std::set<std::string>& disabledHeights,
                                             const std::set<std::string>& disabledUncrs,
                                             const std::set<std::string>& disabledVarScales, const std::set<std::string>& disabledDirGroups);
    void checkForMeasurementsInvolvingDerivedPoints(QModelIndex index, const std::list<std::string>& derivedPoints);
    void validateDerivedPointReferences(QModelIndexList derivedPointsIndexes);
    void setWarningsOnAncestors(QModelIndex index);

    void clearRootIncludeUIWarnings() { lsaFile->clearRootIncludeUIWarnings() ;}

    QModelIndex insertAutogeneratedInclude();
    QModelIndex getIndexFromAutogenPointLabel(std::string pointName);
    void convertAutogeneratedToNormalPoint(QModelIndex POSGIndex, bool printDebug = false);
    void addAutogenPointsToProject(const QModelIndexList &modelIndexes);
    void addAdjustedPointsToProject(QList<std::string> POSGstrings, bool checkDuplicates);
    bool initialCoordsLacksComment(std::string initialCoordsFileName);
    void setInitialCoordsIndex();
    bool removeLegacyAutogenPointsFile();

    void clearOutputPositions() {outputPositions.clear();}
    void setOutputPositions(std::vector<FinalAdjustedPosition> newValues) {for(int i=0;i<newValues.size();i++) outputPositions.push_back(newValues.at(i));}

    void synchItemToRecord(QModelIndex index); ///< synch a single GuiModelItem to its LSARecord
    void synchGuiModelToLSATree();             ///< synch every item in the GuiModel to its LSARecord

    void changeProjectDirectory(QString newDir, QString oldDir); ///< Update all of the LSARecord file paths to the new directory

    bool isActive(QModelIndex index) const;    ///< Returns true if the item and all of its ancestors are not commented
    double getNetVSCAFactor(QModelIndex index) const;
    bool hasValidScaling(QModelIndex index) const;
    bool hasValidMeasurementSigma(QModelIndex index) const;
    bool isNonPolePosition(QModelIndex index) const;

    void moveRecord(bool up);

    QModelIndexList getRecordsReferencingPosition(const std::string& label);
    bool recordReferencesPosition(LSARecord *lsaRecord, const std::string& positionLabel);
    QModelIndexList getRecordsReferencingDirGroup(const std::string& label);
    QModelIndexList getRecordsReferencingModifier(LSAType modifierType, const std::string& label);
    QModelIndexList getRecordsReferencingModifier(LSAType modifierType, const std::string& label, QModelIndex parentIncludeIndex);
    QModelIndexList getRecordsReferencingDirGroup(const std::string& label, QModelIndex parentIncludeIndex);

    std::string getProjectFilename() const;

    bool hasChildrenWithWarnings(QModelIndex parentIncludeIndex);
    std::vector<std::string> findNonPosDuplicateLabelWarnings(QModelIndex parentIncludeIndex);

    // TODO: find a better way to do this.  This is a pretty ugly hack.
    bool suppressGuiModelUpdates; ///< used to prevent the GuiModel::dataChanged(...) signal from being sent

    bool suppressGuiUpdates(bool value);

    double getVSCAValue(LSARecord *lsaRecord) const { return lsaFile->getVSCAValue(lsaRecord); }
    LSAUncertainty* getUNCRRecord(LSARecord* lsaRecord) const { return lsaFile->getUNCRRecord(lsaRecord);}

    void getHeightDataByLabel(std::string label, double & value, std::string & units, double & sigma, std::string sigmaUnits)
        { lsaFile->getHeightDataByLabel(label, value, units, sigma, sigmaUnits); }

    bool getHeightFromData(LSARecord *lsaRecord, std::string &label, double & value, std::string & units, double & sigma, std::string & sigmaUnits, bool & isNumeric) const
        { return lsaFile->getHeightFromData(lsaRecord, label, value, units, sigma, sigmaUnits, isNumeric); }
    bool getHeightToData(LSARecord *lsaRecord, std::string &label, double & value, std::string & units, double & sigma, std::string & sigmaUnits, bool  & isNumeric) const
        { return lsaFile->getHeightToData(lsaRecord, label, value, units, sigma, sigmaUnits, isNumeric); }
    bool getRefractData(LSARecord *lsaRecord, double & value) const
        { return lsaFile->getRefractData(lsaRecord, value); }

    std::string getParentDGRPFromStation(LSAHDir *lsaHDir) const { return lsaFile->getParentDGRPFromStation(lsaHDir); }
    int getNumReferencesToModifier(LSAType lsaType, std::string label);
    bool hasDuplicateTypeAndLabel(QModelIndex index);

    bool isApplicationGui() const { return lsaFile->isApplicationGui(); }

    bool isFileVersionCurrent(std::string salsaVersion) {return lsaFile->isFileVersionCurrent(salsaVersion);}

    void setFilterLogic(const std::vector<LSAType> &inputTypes, const std::function<bool (std::vector<LSAType>, LSARecord *)> &logicalFunc);
    
    /// Static utility method for finding searchString in lsaString from a row in the tree view. bExact = false will search like substr.
    /// @param lsaString string to be searched
    /// @param searchString string to find in lsaString
    /// @param bExact whether to search as exact word or as substring (default false->substring)
    static bool findInString(const QString& lsaString, const QString& searchString, bool bExact = false, bool bMatchAll = false);

    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const;

    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    /// Manually update a record in the undo/redo loop
    /// @param index Index that is being updated
    /// @param lsaString LSA formatted string that will update the record
    bool updateRecord(QPersistentModelIndex& index, std::string lsaString);

    LSARecord* getPointWithLabel(std::string label);

private:

    LSAFile             *lsaFile;         ///< pointer to LSA file parser object currently used by the model
    GuiModelItem        *rootItem;        ///< pointer to the root item at the top of the model tree

    QPersistentModelIndex autogenIndex;
    QPersistentModelIndex initialCoordsIndex;
    QList<QPersistentModelIndex> indexesToCut;

    LSARecordMap   *pointMap;
    LSAHGHTMap     *hghtMap;
    LSAUNCRMap     *uncrMap;
    LSAVSCAMap     *vscaMap;
    LSADirGroupMap *dgrpMap;

    std::function<bool (std::vector<LSAType>, LSARecord *)> filterLogic;
    std::vector<LSAType> filterTypes;

    QMessageBox *questionDialog;
    QUndoStack *m_stack;

    ///< used to forcibly reset item selection after enabling/disabling multi-selected records
    ///< see GuiModel::handleSelectionChanged(...) for usage

    void mergeAprioriValuesIntoLSATree();

    void emitDataChanged(const QModelIndex& index,
                         const QVector<int>& roles = QVector<int>())
    {
        emit dataChanged(index, index, roles);
    }

    void setupModelData();

    /// Set up the model tree to mirror the contents of an LSAInclude tree
    /// This method is called by constructor after the lsa file parsers loads the lsa file tree.
    /// @param parentIndex index of Include node
    /// @param parentItem
    void setupModelDataRecurse(QModelIndex parentIndex, GuiModelItem *parentItem, double groupVSCAFactor);

    /// Get the GuiModelItem specified by a QModelIndex
    /// @param index QModelIndex specifying an item
    GuiModelItem *getItem(const QModelIndex &index) const;

    void setCheckStateData(const QModelIndex &index, const QVariant &value);

    /// Recursive helper method used by GuiModel::getNextIndex(...)
    /// Recurse up the tree looking for a parent with a sibling
    /// @param currentIndex index where the search should begin
    /// @return index of a sibling of a parent, QModelIndex() if no sibling is found
    QModelIndex findNextSiblingOfParent(QModelIndex currentIndex) const;

    void parseIncludeFilename(std::string line, std::string &newFile, std::vector<std::string> &modifierStrings);

    QModelIndex insertNewRecord(QModelIndex parentIndex, int rowPosition, LSAType recordType,
                                std::string filename, bool isAutogenerated = false,
                                bool isCommented = false, bool isUndoRedoAction = false);
    void cleanUpVSCAToParentTags(QModelIndex parentIndex);

    LSADirGroup* getDirGroup(std::string label);

    void renameHGHTLabel(std::string oldLabel, std::string newLabel);
    void renameUNCRLabel(std::string oldLabel, std::string newLabel);
    void renameVSCALabel(std::string oldLabel, std::string newLabel);
    void renameDGRPLabel(std::string oldLabel, std::string newLabel);

    void renameModifierLabelInTree(LSAType lsaType, std::string oldLabel, std::string newLabel);
    void renameDGRPLabelInTree(std::string oldLabel, std::string newLabel);
    void createInitialCoordsInclude();

    void pushGuiStatus(QString message) { emit pushStatus(message); }
    void pushGuiWarning(QString message) { emit pushWarning(message); }
    void pushGuiError(QString message) { emit pushError(message); }
    
    std::unique_ptr<QModelIndexList> m_selectedRowsCache;

    std::map<LSARecord*, QPersistentModelIndex> m_LSARecordToQModelIndexMap;
};

#endif // LSAMODEL_H
