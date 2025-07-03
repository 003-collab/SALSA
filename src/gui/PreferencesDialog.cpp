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
#include "PreferencesDialog.hpp"
#include "ui_PreferencesDialog.h"

#include <QSettings>

PreferencesDialog::PreferencesDialog(QWidget *parent) : ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    setupControls();

    connect(ui->buttonBoxConfigDialog, SIGNAL(accepted()),    this, SLOT(handleOKButtonPressed()) );

}

void PreferencesDialog::setupControls()
{
  QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

  bool allowMapDisable = qsettings.value( lsa::QSETTINGS_ALLOW_AUTO_DISABLE_MAP, true).toBool();
  ui->checkBoxAllowMapDisable->setChecked(allowMapDisable);
  bool warnPrecUpgrade = qsettings.value(lsa::QSETTINGS_WARN_PREC_UPGRADE, true).toBool();
  ui->checkBoxPrecWarning->setChecked(warnPrecUpgrade);
  int CLITimeoutSeconds = qsettings.value(lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, lsa::DEFAULT_CLI_TIMEOUT_SECONDS).toInt();
  ui->spinBoxCLITimeoutSeconds->setValue(CLITimeoutSeconds);

  return;

}

void PreferencesDialog::handleOKButtonPressed()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    qsettings.setValue( lsa::QSETTINGS_ALLOW_AUTO_DISABLE_MAP, ui->checkBoxAllowMapDisable->isChecked() );
    qsettings.setValue( lsa::QSETTINGS_WARN_PREC_UPGRADE, ui->checkBoxPrecWarning->isChecked() );
    qsettings.setValue( lsa::QSETTINGS_CLI_TIMEOUT_SECONDS, ui->spinBoxCLITimeoutSeconds->value() );

    return;
}




