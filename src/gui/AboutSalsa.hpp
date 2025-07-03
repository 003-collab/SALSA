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
#ifndef ABOUTSALSA_HPP
#define ABOUTSALSA_HPP

#include <QDialog>
#include <QLabel>

namespace Ui {
class AboutSalsa;
}

class AboutSalsaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutSalsaDialog(QWidget *parent = 0);
    ~AboutSalsaDialog();

private:
    Ui::AboutSalsa *ui;
    void setLabelText(const QString &labelText, QLabel *labelToSet);
};

#endif // ABOUTSALSA_HPP
