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
#ifndef CONFIGDIALOG_HPP
#define CONFIGDIALOG_HPP

#include <QDialog>
#include "LSAGuiConstants.hpp"
#include "ConverterDialog.hpp"

namespace Ui
{
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests

public:
    explicit ConfigDialog(QString projectConfigFile, QWidget *parent = 0);
    ~ConfigDialog();
    Ui::ConfigDialog* getUi() { return ui; }
    void clickOK() { return onOKButtonPressed();}
    ConverterDialog      *converterDialog;

private:
    Ui::ConfigDialog *ui;
    QString configFile;

    void setupControls();
    QStringList findInstalledGeoidFiles();
private slots:
    void onOKButtonPressed();
    void onConverterConfigButtonPressed();
};
#endif // CONFIGDIALOG_HPP
