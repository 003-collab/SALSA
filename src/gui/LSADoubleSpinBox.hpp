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

#ifndef LSADOUBLESPINBOX_H
#define LSADOUBLESPINBOX_H

#include <QDoubleSpinBox>

class LSADoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit LSADoubleSpinBox( QWidget* parent = nullptr ) : QDoubleSpinBox(parent) {}
    virtual double valueFromText(const QString &text) const;

protected:
    QValidator::State validate(QString &text, int &pos) const;

};

#endif // LSADOUBLESPINBOX_H
