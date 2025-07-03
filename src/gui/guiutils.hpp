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
#ifndef GUIUTILS_HPP
#define GUIUTILS_HPP

#include <QStyledItemDelegate>

//necessary to be able to specify precision in a QTableView item (i.e. treat as QString)
//while also being able to sort on the item (i.e. treat as a double)
class NumberFormatDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NumberFormatDelegate(QObject *parent=0, int digits=3):QStyledItemDelegate(parent) {precision = digits;};
    virtual QString displayText(const QVariant &value, const QLocale &locale) const {return locale.toString(value.toDouble(),'f',precision);};
private:
    int precision;
};

class StringFormatDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
   explicit StringFormatDelegate(QObject *parent):QStyledItemDelegate(parent){};
    virtual QString displayText(const QVariant &value) const {return value.toString();};
private:
    int precision;
};

#endif // GUIUTILS_HPP
