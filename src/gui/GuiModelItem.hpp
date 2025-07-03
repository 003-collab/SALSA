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
#ifndef LSAITEM_H
#define LSAITEM_H

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <QList>
#include <QVariant>
#include <QVector>
#include <QPersistentModelIndex>
#include <LSARecord.hpp>

// Struct for capturing the necessary index/parent->root chain in undo/redo commands.
// Since index can be created by calling GuiModel->index(row, column, parentIndex), and the rootIndex never changes,
// this chain is a way to update the current index in the case it and/or its parent was deleted
//
// The goal is to create a chain vector of the items described below
// that maps an index to the root as follows:
// ___________________________
// | [In, Rn, Cn, RootIndex] | (RootIndex NEVER changes)
// |         .               |
// |         .               |
// |         .               |
// | [I3, R3, C3, Parent3  ] | (I3 = Parent 2)
// | [I2, R2, C2, Parent2  ] | (I2 = Parent 1)
// | [I1, R1, C1, Parent1  ] | (I1 is the index we ultimately want to validate)
// ---------------------------
//
// Thus, we are able to reverse solve for the parents starting with In all the way down
// Each Index has its own unique vector structure as described above
struct IndexRowColumnParentChainItem
{
  QPersistentModelIndex index = QPersistentModelIndex();
  int row = 0;
  int col = 0;
  QPersistentModelIndex parentIndex = QPersistentModelIndex();
};

/// Class GuiModelItem represents one item in the tree representation of the Gui model.
/// Each GuiModelItem object is one tree node in the GuiModel.
///
class GuiModelItem
{
public:

    // Constructors & Destructuor
    /// Constructor given data and parent
    /// @param data QVariant data for item to contain
    /// @param parent the parent item for this item
    explicit GuiModelItem(const QVector<QVariant> &data, GuiModelItem *parent = NULL);

    /// Destructor
    ~GuiModelItem();

    // Data members
    /// Get the number of children this item has
    /// @return the number of children
    int childCount() const;

    /// Get the number of columns for this item
    /// @return the number of columns
    int columnCount() const;

    /// Child number is the index of this object in its parent's QList of child items
    /// @return the child number
    int childNumber() const;

    /// Get this item's parent
    /// @return the parent item
    GuiModelItem *parent();

    /// Get the child as position number
    /// @param number index position into childItems
    /// @return the child item for number
    GuiModelItem *child(int number);

    /// Get the data stored in one of this item's columns
    /// @param column, the desired column number
    /// @return QVariant containing the column's data
    QVariant data(int column) const;

    /// Insert a new child for this object
    /// @param position index in childItems where new children will be inserted
    /// @param count number of new children to be inserted
    /// @param columns number of columns present in each newly created child
    /// @return true is position is a valid position to insert
    bool insertChildren(int position, int count, int columns);

    /// Insert additional columns for this object. Insert the same columns for
    /// all children of this object.
    /// @param position the position to insert the columns
    /// @param columns the number of columns to insert
    /// @return true is position is a valid position to insert
    bool insertColumns(int position, int columns);

    /// Delete children of this object
    /// @param position to begin deleting
    /// @param number of children to delete
    /// @return true is position is valid
    bool removeChildren(int position, int count);

    /// Delete columns in this object
    /// @param position to begin deleting
    /// @param number of columns to delete
    /// @return true is position is valid
    bool removeColumns(int position, int columns);

    /// Set data for a given column in this object
    /// @param column the column to set the data
    /// @param value the data value to set
    /// @return true if column is a valid column
    bool setData(int column, const QVariant &value);

    /// Set the LSARecord* for this item
    /// @param recordPtr pointer to the LSARecord for this item
    void setLSARecord(LSARecord* recordPtr) { lsaRecord = recordPtr; lsaType = recordPtr->getRecType(); }

    /// Get the LSARecord* for this item
    /// @return pointer to the LSARecord for this item
    LSARecord* getLSARecord() const { return lsaRecord; }

    /// Get the LSAType for this item's LSARecord
    /// @return LSAType of the LSARecord
    LSAType getLSAType() { return lsaType; }    

    /// Get the 'active' state for this item, used to track if the row in the tree view is gray
    /// @param rootItem pointer to the root item in the GuiModel tree
    /// @return true if the item is not commented and its parent is active
    bool isActive(GuiModelItem *rootItem);

    /// Check if this item is a descendent of ancestorItem
    /// @param parentItem the possible ancestor item
    /// @return true if this item is a descendant of ancestorItem
    bool isDescendantOfItem(GuiModelItem *ancestorItem);

    QList<LSARecord*> getChildRecords();

private:
    QList<GuiModelItem*> childItems;  ///< list of children for this item
    QVector<QVariant>   itemData;     ///< QVector of item data for this item
    GuiModelItem         *parentItem; ///< the parent item for this item
    LSAType             lsaType;      ///< type of the LSARecord for this item
    LSARecord           *lsaRecord;   ///< LSARecord for this item
};

#endif // LSAITEM_H
