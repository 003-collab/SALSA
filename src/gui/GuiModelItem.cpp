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
#include "GuiModelItem.hpp"
#include <QStringList>


GuiModelItem::GuiModelItem(const QVector<QVariant> &data, GuiModelItem *parent)
{
    parentItem = parent;
    itemData = data;
}

GuiModelItem::~GuiModelItem()
{
    qDeleteAll(childItems);
}

GuiModelItem *GuiModelItem::child(int number)
{
    return childItems.value(number);
}

int GuiModelItem::childCount() const
{
    return childItems.count();
}

int GuiModelItem::childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<GuiModelItem*>(this));

    return 0;
}

int GuiModelItem::columnCount() const
{
    return itemData.count();
}

QVariant GuiModelItem::data(int column) const
{
    return itemData.value(column);
}

bool GuiModelItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childItems.size() )
        return false;

    for (int row = 0; row < count; ++row)
    {
        QVector<QVariant> data(columns);
        GuiModelItem *item = new GuiModelItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool GuiModelItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.insert(position, QVariant());

    foreach (GuiModelItem *child, childItems)
        child->insertColumns(position, columns);

    return true;
}

GuiModelItem *GuiModelItem::parent()
{
    return parentItem;
}

bool GuiModelItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

bool GuiModelItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.remove(position);

    foreach (GuiModelItem *child, childItems)
        child->removeColumns(position, columns);

    return true;
}

bool GuiModelItem::setData(int column, const QVariant &value)
{
    Q_ASSERT(0 <= column);
    Q_ASSERT(column <= itemData.size());

    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

bool GuiModelItem::isActive(GuiModelItem *rootItem)
{
    if (this == rootItem)
    {
        // The root include item in the project is always active
        return true;
    }

    LSARecord* lsaRecord = getLSARecord();
    if (lsaRecord == NULL)
    {
        return true;
    }

    return parentItem->isActive(rootItem) && !lsaRecord->isCommented;
}

bool GuiModelItem::isDescendantOfItem(GuiModelItem *ancestorItem)
{
    // base cases
    if      (parentItem == NULL) return false;
    else if (parentItem == ancestorItem) return true;

    // otherwise recurse on the parent
    return (parentItem->isDescendantOfItem(ancestorItem) );
}

QList<LSARecord*> GuiModelItem::getChildRecords()
{
    QList<LSARecord*> childRecords;
    foreach(GuiModelItem *item, childItems)
        childRecords.push_back(item->getLSARecord());

    return childRecords;
}


