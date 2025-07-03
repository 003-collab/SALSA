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
#include "ConverterDialog.hpp"
#include "ui_ConverterDialog.h"
#include "lsaUtils.hpp"//addQuotes
#include <StringUtils.hpp>
#include <QMessageBox>
#include <fstream>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include "LSASupportedToVersion.hpp"

const int ConverterDialog::TRIPOD = 0;
const int ConverterDialog::VERT_COLLIM = 1;
const int ConverterDialog::FORCED_CENTER = 2;
const int ConverterDialog::RANGE_POLE = 3;
const int ConverterDialog::SETUP_CUSTOM = 4;
const int ConverterDialog::DIGITAL_PRECISE = 0;
const int ConverterDialog::DIGITAL_STANDARD = 1;
const int ConverterDialog::SPIRIT_STANDARD = 2;
const int ConverterDialog::SOURCE_DEFAULT = 0;
const int ConverterDialog::SOURCE_INST = 1;
const int ConverterDialog::SOURCE_CUSTOM = 2;
const int ConverterDialog::INST = -1;
const int ConverterDialog::INSTR_HGHT_MEASURE_DEFAULT = 0;
const int ConverterDialog::INSTR_HGHT_MEASURE_ROD = 1;
const int ConverterDialog::INSTR_HGHT_MEASURE_TAPE = 2;
const int ConverterDialog::INSTR_HGHT_MEASURE_CUSTOM =3;
const QString ConverterDialog::LABEL_GPS_STATIC = QString("UNCR#1_DXYZ_STATIC");
const QString ConverterDialog::LABEL_GPS_KINEMATIC = QString("UNCR#1_DXYZ_KINEMATIC");
const QString ConverterDialog::LABEL_DIFF_LEVELS = QString("UNCR#1_HDIF");
const QString ConverterDialog::LABEL_TOTSTA_DIST_IR = QString("UNCR#1_DIST_IR");
const QString ConverterDialog::LABEL_TOTSTA_DIST_RED = QString("UNCR#1_DIST_RED");
const QString ConverterDialog::LABEL_TOTSTA_DIST_NON = QString("UNCR#1_DIST_NOEDM");
const QString ConverterDialog::LABEL_TOTSTA_HORZ_DIR = QString("UNCR#1_HDIR");
const QString ConverterDialog::LABEL_TOTSTA_HORZ_ANG = QString("UNCR#1_HANG");
const QString ConverterDialog::LABEL_TOTSTA_VERT = QString("UNCR#1_ZANG");
const QString ConverterDialog::LABEL_TOTSTA_DE = QString("UNCR#1_DIFFELEV");
const QString ConverterDialog::LABEL_TOTSTA_INSTR_HGHT = QString("UNCR#1_INSTR_HGHT");


ConverterDialog::ConverterDialog(QString converterConfigFile, QWidget *parent) :
    configFile(converterConfigFile), QDialog(parent), ui(new Ui::ConverterDialog)
{
    ui->setupUi(this);

    //buttons
    connect(ui->pushButtonRestoreFactoryDefaults, SIGNAL(clicked()),                this, SLOT(onRestoreFactoryDefaultsButtonPressed()));
    connect(ui->pushButtonSaveAsUserDefaults,     SIGNAL(clicked()),                this, SLOT(onSaveAsUserDefaultsButtonPressed()));
    connect(ui->pushButtonCancel,                 SIGNAL(clicked()),                this, SLOT(onCancelButtonPressed()));
    connect(ui->pushButtonSaveAsProjectConfig,    SIGNAL(clicked()),                this, SLOT(onSaveAsProjectConfigButtonPressed()));
    //comboboxes
    connect(ui->comboBoxStaticGPSFRSetup,         SIGNAL(currentIndexChanged(int)), this, SLOT(onStaticGPSFRChanged(int)));
    connect(ui->comboBoxKinGPSFRSetup,            SIGNAL(currentIndexChanged(int)), this, SLOT(onKinGPSFRSetupChanged(int)));
    connect(ui->comboBoxDiffLevelsFRSetup,        SIGNAL(currentIndexChanged(int)), this, SLOT(onDiffLevelsFRSetupChanged(int)));
    connect(ui->comboBoxTSFRSetup,                SIGNAL(currentIndexChanged(int)), this, SLOT(onTSFRSetupChanged(int)));
    connect(ui->comboBoxStaticGPSTOSetup,         SIGNAL(currentIndexChanged(int)), this, SLOT(onStaticGPSTOSetupChanged(int)));
    connect(ui->comboBoxKinGPSTOSetup,            SIGNAL(currentIndexChanged(int)), this, SLOT(onKinGPSTOSetupChanged(int)));
    connect(ui->comboBoxDiffLevelsTOSetup,        SIGNAL(currentIndexChanged(int)), this, SLOT(onDiffLevelsTOSetupChanged(int)));
    connect(ui->comboBoxTSTOSetup,                SIGNAL(currentIndexChanged(int)), this, SLOT(onTSTOSetupChanged(int)));
    connect(ui->comboBoxDiffLevelsMeasType,       SIGNAL(currentIndexChanged(int)), this, SLOT(onDiffLevelsMeasTypeChanged(int)));
    connect(ui->comboBoxDiffLevelsSigmaSource,    SIGNAL(currentIndexChanged(int)), this, SLOT(onDiffLevelsSigmaSourceChanged(int)));
    connect(ui->comboBoxTSDistancesSigmaSource,   SIGNAL(currentIndexChanged(int)), this, SLOT(onTSDistancesSigmaSourceChanged(int)));
    connect(ui->comboBoxTSHorzAnglesSigmaSource,  SIGNAL(currentIndexChanged(int)), this, SLOT(onTSHorzAnglesSigmaSourceChanged(int)));
    connect(ui->comboBoxTSVertAnglesSigmaSource,  SIGNAL(currentIndexChanged(int)), this, SLOT(onTSVertAnglesSigmaSourceChanged(int)));
    connect(ui->comboBoxTSElevDiffsSigmaSource,   SIGNAL(currentIndexChanged(int)), this, SLOT(onTSElevDiffsSigmaSourceChanged(int)));
    connect(ui->comboBoxTSInstrHghtSigmaSource,   SIGNAL(currentIndexChanged(int)), this, SLOT(onTSInstrHghtSigmaSourceChanged(int)));


    setupControls();
}

ConverterDialog::~ConverterDialog()
{
    delete ui;
}

void ConverterDialog::setupControls()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //Check to see if a converter config file has been saved in the project.  If so, use it to configure the panel
    if(readConverterConfigSettingsFile())
        return;
    else//if not, configure the panel using the last settings saved by the user
    {
        //Setup FROM
        ui->comboBoxStaticGPSFRSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_GPSSTATIC_FROMC, TRIPOD).toInt());
        ui->comboBoxKinGPSFRSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_GPSKINEMATIC_FROMC, TRIPOD).toInt());
        ui->comboBoxDiffLevelsFRSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_LEVELS_FROMC, TRIPOD).toInt());
        ui->comboBoxTSFRSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_TOTSTA_FROMC, TRIPOD).toInt());
        //Setup TO
        ui->comboBoxStaticGPSTOSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_GPSSTATIC_TOC, TRIPOD).toInt());
        ui->comboBoxKinGPSTOSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_GPSKINEMATIC_TOC, RANGE_POLE).toInt());
        ui->comboBoxDiffLevelsTOSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_LEVELS_TOC, TRIPOD).toInt());
        ui->comboBoxTSTOSetup->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_TOTSTA_TOC, TRIPOD).toInt());
        //Measurement Type
        ui->comboBoxDiffLevelsMeasType->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_LEVELS_MEASTYPE, DIGITAL_PRECISE).toInt());
        //Sigma Source
        ui->comboBoxDiffLevelsSigmaSource->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_LEVELS_SIGMASOURCE, SOURCE_DEFAULT).toInt());
        ui->comboBoxTSDistancesSigmaSource->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_TOTSTA_DIST_SIGMASOURCE, SOURCE_DEFAULT).toInt());
        ui->comboBoxTSHorzAnglesSigmaSource->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_TOTSTA_HORZ_SIGMASOURCE, SOURCE_DEFAULT).toInt());
        ui->comboBoxTSVertAnglesSigmaSource->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_TOTSTA_VERT_SIGMASOURCE, SOURCE_DEFAULT).toInt());
        ui->comboBoxTSElevDiffsSigmaSource->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_TOTSTA_ELEVDIFF_SIGMASOURCE, SOURCE_DEFAULT).toInt());
        ui->comboBoxTSInstrHghtSigmaSource->setCurrentIndex(qsettings.value( lsa::QSETTINGS_COMBO_TOTSTA_INSTR_HGHT_SIGMASOURCE, INSTR_HGHT_MEASURE_DEFAULT).toInt());

        //line edit labels
        ui->lineEditStaticGPSLabel->setText(LABEL_GPS_STATIC);
        ui->lineEditKinGPSLabel->setText(LABEL_GPS_KINEMATIC);
        ui->lineEditDiffLevelsLabel->setText(LABEL_DIFF_LEVELS);
        ui->lineEditTSDistIRLabel->setText(LABEL_TOTSTA_DIST_IR);
        ui->lineEditTSDistRedLabel->setText(LABEL_TOTSTA_DIST_RED);
        ui->lineEditTSDistNoEDMLabel->setText(LABEL_TOTSTA_DIST_NON);
        ui->lineEditTSHorzAngleLabel->setText(LABEL_TOTSTA_HORZ_DIR);
        ui->lineEditTSVertAnglesLabel->setText(LABEL_TOTSTA_VERT);
        ui->lineEditTSElevDiffsLabel->setText(LABEL_TOTSTA_DE);
        //double spin boxes
        ui->doubleSpinBoxStaticGPSPPM->setValue(qsettings.value( lsa::QSETTINGS_GPSSTATIC_PPM, 0.0).toDouble());
        ui->doubleSpinBoxKinGPSPPM->setValue(qsettings.value( lsa::QSETTINGS_GPSKINEMATIC_PPM, 0.0).toDouble());
        ui->doubleSpinBoxDiffLevelsPPM->setValue(qsettings.value( lsa::QSETTINGS_LEVELS_PPM, 0.0).toDouble());
        ui->doubleSpinBoxTSDistIRPPM->setValue(qsettings.value( lsa::QSETTINGS_DISTIR_PPM, 0.0).toDouble());
        ui->doubleSpinBoxTSDistRedPPM->setValue(qsettings.value( lsa::QSETTINGS_DISTRED_PPM, 0.0).toDouble());
        ui->doubleSpinBoxTSDistNonPPM->setValue(qsettings.value( lsa::QSETTINGS_DISTNON_PPM, 0.0).toDouble());
        //double spinbox values corresponding to custom combobox settings
        //FROM
        if(ui->comboBoxStaticGPSFRSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxStaticGPSFRCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_GPSSTATIC_FROMC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        if(ui->comboBoxKinGPSFRSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxKinGPSFRCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_GPSKIMEMATIC_FROMC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        if(ui->comboBoxDiffLevelsFRSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxDiffLevelsFRCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_LEVELS_FROMC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        if(ui->comboBoxTSFRSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxTSDistancesFRCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_FROMC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
            ui->doubleSpinBoxTSHorzAnglesFRCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_HORZ_FROMC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
            ui->doubleSpinBoxTSVertAnglesFRCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_VERT_FROMC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
            ui->doubleSpinBoxTSElevDiffsFRCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_FROMC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        //TO
        if(ui->comboBoxStaticGPSTOSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxStaticGPSTOCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_GPSSTATIC_TOC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        if(ui->comboBoxKinGPSTOSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxKinGPSTOCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_GPSKIMEMATIC_TOC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        if(ui->comboBoxDiffLevelsTOSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxDiffLevelsTOCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_LEVELS_TOC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        if(ui->comboBoxTSTOSetup->currentIndex() == SETUP_CUSTOM)
        {
            ui->doubleSpinBoxTSDistancesTOCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_TOC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
            ui->doubleSpinBoxTSHorzAnglesTOCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_HORZ_TOC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
            ui->doubleSpinBoxTSVertAnglesTOCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_VERT_TOC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
            ui->doubleSpinBoxTSElevDiffsTOCent->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_TOC_M,
                                                                           lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M).toDouble());
        }
        //SIGMA
        if(ui->comboBoxDiffLevelsSigmaSource->currentIndex() == SOURCE_CUSTOM)
        {
            ui->doubleSpinBoxDiffLevelsSigma->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_LEVELS_SIGMA_M,
                                                                           lsa::DEFAULT_SIGMA_DIGITALLEVELS_PRECISERODS_M).toDouble());
        }
        if(ui->comboBoxTSDistancesSigmaSource->currentIndex() == SOURCE_CUSTOM)
        {
            ui->doubleSpinBoxTSDistIRSigma->setValue(qsettings.value(lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_IR_SIGMA_M,
                                                                     lsa::DEFAULT_SIGMA_TOTALSTATION_DISTANCE_INFRARED_M).toDouble());
            ui->doubleSpinBoxTSDistRedSigma->setValue(qsettings.value(lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_RED_SIGMA_M,
                                                                      lsa::DEFAULT_SIGMA_TOTALSTATION_DISTANCE_REDLASER_M).toDouble());
            ui->doubleSpinBoxTSDistNoEDMSigma->setValue(qsettings.value(lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_NOEDM_SIGMA_M,
                                                                        lsa::DEFAULT_SIGMA_TOTALSTATION_DISTANCE_UNKNOWNPRISM_M).toDouble());
        }
        if(ui->comboBoxTSHorzAnglesSigmaSource->currentIndex() == SOURCE_CUSTOM)
        {
            ui->doubleSpinBoxTSHorzAnglesSigma->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_HORZ_SIGMA_SOA,
                                                                           lsa::DEFAULT_SIGMA_TOTALSTATION_HORZANGLES_SOA).toDouble());
        }
        if(ui->comboBoxTSVertAnglesSigmaSource->currentIndex() == SOURCE_CUSTOM)
        {
            ui->doubleSpinBoxTSVertAnglesSigma->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_VERT_SIGMA_SOA,
                                                                           lsa::DEFAULT_SIGMA_TOTALSTATION_VERTANGLES_SOA).toDouble());
        }
        else if(ui->comboBoxTSElevDiffsSigmaSource->currentIndex() == SOURCE_CUSTOM)
        {
            ui->doubleSpinBoxTSElevDiffsSigma->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_SIGMA_M,
                                                                           lsa::DEFAULT_SIGMA_TOTALSTATION_ELEVDIFFS_M).toDouble());
        }
        else if(ui->comboBoxTSInstrHghtSigmaSource->currentIndex() == SOURCE_CUSTOM)
        {
            ui->doubleSpinBoxTSInstrHghtSigma->setValue(qsettings.value( lsa::QSETTINGS_CUSTOM_TOTSTA_INSTR_HGHT_SIGMA_M,
                                                                           lsa::DEFAULT_SIGMA_TOTALSTATION_INSTR_HGHT_M).toDouble());
        }
    }
}

void ConverterDialog::onSaveAsUserDefaultsButtonPressed()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    int index=-1;
    try
    {
        //Save the combo box settings and custom values entered
        index = ui->comboBoxStaticGPSFRSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_GPSSTATIC_FROMC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_GPSSTATIC_FROMC_M, ui->doubleSpinBoxStaticGPSFRCent->value());
        }

        index = ui->comboBoxKinGPSFRSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_GPSKINEMATIC_FROMC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_GPSKIMEMATIC_FROMC_M, ui->doubleSpinBoxKinGPSFRCent->value() );
        }

        index =  ui->comboBoxDiffLevelsFRSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_LEVELS_FROMC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_LEVELS_FROMC_M, ui->doubleSpinBoxDiffLevelsFRCent->value() );
        }

        index = ui->comboBoxStaticGPSTOSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_GPSSTATIC_TOC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_GPSSTATIC_TOC_M, ui->doubleSpinBoxStaticGPSTOCent->value() );
        }

        index = ui->comboBoxKinGPSTOSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_GPSKINEMATIC_TOC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_GPSKIMEMATIC_TOC_M, ui->doubleSpinBoxKinGPSTOCent->value() );
        }

        index = ui->comboBoxDiffLevelsTOSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_LEVELS_TOC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_LEVELS_TOC_M, ui->doubleSpinBoxDiffLevelsTOCent->value() );
        }

        index = ui->comboBoxTSFRSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_TOTSTA_FROMC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_FROMC_M, ui->doubleSpinBoxTSDistancesFRCent->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_HORZ_FROMC_M, ui->doubleSpinBoxTSHorzAnglesFRCent->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_VERT_FROMC_M, ui->doubleSpinBoxTSVertAnglesFRCent->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_FROMC_M, ui->doubleSpinBoxTSElevDiffsFRCent->value() );
        }

        index = ui->comboBoxTSTOSetup->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_TOTSTA_TOC, index);
        if(index == SETUP_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_TOC_M, ui->doubleSpinBoxTSDistancesTOCent->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_HORZ_TOC_M, ui->doubleSpinBoxTSHorzAnglesTOCent->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_VERT_TOC_M, ui->doubleSpinBoxTSVertAnglesTOCent->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_TOC_M, ui->doubleSpinBoxTSElevDiffsTOCent->value() );
        }

        index = ui->comboBoxDiffLevelsMeasType->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_LEVELS_MEASTYPE, index);

        index = ui->comboBoxDiffLevelsSigmaSource->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_LEVELS_SIGMASOURCE, index);
        if(index == SOURCE_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_LEVELS_SIGMA_M, ui->doubleSpinBoxDiffLevelsSigma->value() );
        }

        index = ui->comboBoxTSDistancesSigmaSource->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_TOTSTA_DIST_SIGMASOURCE, index);
        if(index == SOURCE_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_IR_SIGMA_M, ui->doubleSpinBoxTSDistIRSigma->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_RED_SIGMA_M, ui->doubleSpinBoxTSDistRedSigma->value() );
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_DIST_NOEDM_SIGMA_M, ui->doubleSpinBoxTSDistNoEDMSigma->value() );
        }

        index = ui->comboBoxTSHorzAnglesSigmaSource->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_TOTSTA_HORZ_SIGMASOURCE, index);
        if(index == SOURCE_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_HORZ_SIGMA_SOA, ui->doubleSpinBoxTSHorzAnglesSigma->value() );
        }

        index = ui->comboBoxTSVertAnglesSigmaSource->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_TOTSTA_VERT_SIGMASOURCE, index);
        if(index == SOURCE_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_VERT_SIGMA_SOA, ui->doubleSpinBoxTSVertAnglesSigma->value() );
        }

        index = ui->comboBoxTSElevDiffsSigmaSource->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_TOTSTA_ELEVDIFF_SIGMASOURCE, index);
        if(index == SOURCE_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_SIGMA_M, ui->doubleSpinBoxTSElevDiffsSigma->value() );
        }

        index = ui->comboBoxTSInstrHghtSigmaSource->currentIndex();
        qsettings.setValue( lsa::QSETTINGS_COMBO_TOTSTA_INSTR_HGHT_SIGMASOURCE, index);
        if(index == SOURCE_CUSTOM)
        {
            qsettings.setValue( lsa::QSETTINGS_CUSTOM_TOTSTA_INSTR_HGHT_SIGMA_M, ui->doubleSpinBoxTSInstrHghtSigma->value() );
        }

        //PPM double spin boxes
        qsettings.setValue( lsa::QSETTINGS_GPSKINEMATIC_PPM, ui->doubleSpinBoxKinGPSPPM->value() );
        qsettings.setValue( lsa::QSETTINGS_LEVELS_PPM, ui->doubleSpinBoxDiffLevelsPPM->value() );
        qsettings.setValue( lsa::QSETTINGS_GPSSTATIC_PPM, ui->doubleSpinBoxStaticGPSPPM->value() );
        qsettings.setValue( lsa::QSETTINGS_DISTIR_PPM, ui->doubleSpinBoxTSDistIRPPM->value() );
        qsettings.setValue( lsa::QSETTINGS_DISTRED_PPM, ui->doubleSpinBoxTSDistRedPPM->value() );
        qsettings.setValue( lsa::QSETTINGS_DISTNON_PPM, ui->doubleSpinBoxTSDistNonPPM->value() );
    }
    catch(...)//use default if invalid value entered
    {
    }
}

void ConverterDialog::onCancelButtonPressed()
{
    this->close();
}

void ConverterDialog::onSaveAsProjectConfigButtonPressed()
{
    createConverterConfigFile();
    writeConverterConfigSettingsFile();
    this->close();
}

void ConverterDialog::onRestoreFactoryDefaultsButtonPressed()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //Setup FROM
    ui->comboBoxStaticGPSFRSetup->setCurrentIndex(TRIPOD);
    ui->comboBoxKinGPSFRSetup->setCurrentIndex(TRIPOD);
    ui->comboBoxDiffLevelsFRSetup->setCurrentIndex(TRIPOD);
    ui->comboBoxTSFRSetup->setCurrentIndex(TRIPOD);
    //Setup TO
    ui->comboBoxStaticGPSTOSetup->setCurrentIndex(TRIPOD);
    ui->comboBoxKinGPSTOSetup->setCurrentIndex(RANGE_POLE);
    ui->comboBoxDiffLevelsTOSetup->setCurrentIndex(TRIPOD);
    ui->comboBoxTSTOSetup->setCurrentIndex(TRIPOD);
    //Measurement Type
    ui->comboBoxDiffLevelsMeasType->setCurrentIndex(DIGITAL_PRECISE);
    //Sigma Source
    ui->comboBoxDiffLevelsSigmaSource->setCurrentIndex(SOURCE_DEFAULT);
    ui->comboBoxTSDistancesSigmaSource->setCurrentIndex(SOURCE_DEFAULT);
    ui->comboBoxTSHorzAnglesSigmaSource->setCurrentIndex(SOURCE_DEFAULT);
    ui->comboBoxTSVertAnglesSigmaSource->setCurrentIndex(SOURCE_DEFAULT);
    ui->comboBoxTSElevDiffsSigmaSource->setCurrentIndex(SOURCE_DEFAULT);
    ui->comboBoxTSInstrHghtSigmaSource->setCurrentIndex(INSTR_HGHT_MEASURE_DEFAULT);
    //PPMs
    ui->doubleSpinBoxKinGPSPPM->setValue(0.0);
    ui->doubleSpinBoxDiffLevelsPPM->setValue(0.0);
    ui->doubleSpinBoxStaticGPSPPM->setValue(0.0);
    ui->doubleSpinBoxTSDistIRPPM->setValue(0.0);
    ui->doubleSpinBoxTSDistRedPPM->setValue(0.0);
    ui->doubleSpinBoxTSDistNonPPM->setValue(0.0);
}

void ConverterDialog::onStaticGPSFRChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxStaticGPSFRCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxStaticGPSFRCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxStaticGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxStaticGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxStaticGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxStaticGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onKinGPSFRSetupChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxKinGPSFRCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxKinGPSFRCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxKinGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxKinGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxKinGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxKinGPSFRCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onDiffLevelsFRSetupChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxDiffLevelsFRCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxDiffLevelsFRCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxDiffLevelsFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxDiffLevelsFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxDiffLevelsFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxDiffLevelsFRCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onTSFRSetupChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxTSDistancesFRCent->setEnabled(false);
        ui->doubleSpinBoxTSHorzAnglesFRCent->setEnabled(false);
        ui->doubleSpinBoxTSVertAnglesFRCent->setEnabled(false);
        ui->doubleSpinBoxTSElevDiffsFRCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxTSDistancesFRCent->setEnabled(true);
        ui->doubleSpinBoxTSHorzAnglesFRCent->setEnabled(true);
        ui->doubleSpinBoxTSVertAnglesFRCent->setEnabled(true);
        ui->doubleSpinBoxTSElevDiffsFRCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxTSDistancesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
        ui->doubleSpinBoxTSHorzAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
        ui->doubleSpinBoxTSVertAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
        ui->doubleSpinBoxTSElevDiffsFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxTSDistancesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
        ui->doubleSpinBoxTSHorzAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
        ui->doubleSpinBoxTSVertAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
        ui->doubleSpinBoxTSElevDiffsFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxTSDistancesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
        ui->doubleSpinBoxTSHorzAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
        ui->doubleSpinBoxTSVertAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
        ui->doubleSpinBoxTSElevDiffsFRCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxTSDistancesFRCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
        ui->doubleSpinBoxTSHorzAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
        ui->doubleSpinBoxTSVertAnglesFRCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
        ui->doubleSpinBoxTSElevDiffsFRCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onStaticGPSTOSetupChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxStaticGPSTOCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxStaticGPSTOCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxStaticGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxStaticGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxStaticGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxStaticGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onKinGPSTOSetupChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxKinGPSTOCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxKinGPSTOCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxKinGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxKinGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxKinGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxKinGPSTOCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onDiffLevelsTOSetupChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxDiffLevelsTOCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxDiffLevelsTOCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxDiffLevelsTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxDiffLevelsTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxDiffLevelsTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxDiffLevelsTOCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onTSTOSetupChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    //allow editing of values if Custom selected
    if(index == TRIPOD ||
       index == VERT_COLLIM ||
       index == FORCED_CENTER ||
       index == RANGE_POLE)
    {
        ui->doubleSpinBoxTSDistancesTOCent->setEnabled(false);
        ui->doubleSpinBoxTSHorzAnglesTOCent->setEnabled(false);
        ui->doubleSpinBoxTSVertAnglesTOCent->setEnabled(false);
        ui->doubleSpinBoxTSElevDiffsTOCent->setEnabled(false);
    }
    else if (index == SETUP_CUSTOM)
    {
        ui->doubleSpinBoxTSDistancesTOCent->setEnabled(true);
        ui->doubleSpinBoxTSHorzAnglesTOCent->setEnabled(true);
        ui->doubleSpinBoxTSVertAnglesTOCent->setEnabled(true);
        ui->doubleSpinBoxTSElevDiffsTOCent->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == TRIPOD)
    {
        ui->doubleSpinBoxTSDistancesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
        ui->doubleSpinBoxTSHorzAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
        ui->doubleSpinBoxTSVertAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
        ui->doubleSpinBoxTSElevDiffsTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_STANDARDPLUMMET_M);
    }
    else if(index == VERT_COLLIM)
    {
        ui->doubleSpinBoxTSDistancesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
        ui->doubleSpinBoxTSHorzAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
        ui->doubleSpinBoxTSVertAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
        ui->doubleSpinBoxTSElevDiffsTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_VERTCOLLIMATOR_M);
    }
    else if(index == FORCED_CENTER)
    {
        ui->doubleSpinBoxTSDistancesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
        ui->doubleSpinBoxTSHorzAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
        ui->doubleSpinBoxTSVertAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
        ui->doubleSpinBoxTSElevDiffsTOCent->setValue(lsa::DEFAULT_SETUPERROR_TRIPOD_FORCEDCENTERINGPLATE_M);
    }
    else if(index == RANGE_POLE)
    {
        ui->doubleSpinBoxTSDistancesTOCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
        ui->doubleSpinBoxTSHorzAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
        ui->doubleSpinBoxTSVertAnglesTOCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
        ui->doubleSpinBoxTSElevDiffsTOCent->setValue(lsa::DEFAULT_SETUPERROR_RANGEPOLE_M);
    }
}

void ConverterDialog::onDiffLevelsMeasTypeChanged(int index)
{
    //update populate fields with saved values
    int sourceIndex = ui->comboBoxDiffLevelsSigmaSource->currentIndex();
    if(sourceIndex == SOURCE_DEFAULT)
    {
        if(index == DIGITAL_PRECISE)
        {
            ui->doubleSpinBoxDiffLevelsSigma->setValue(lsa::DEFAULT_SIGMA_DIGITALLEVELS_PRECISERODS_M);
        }
        else if(index == DIGITAL_STANDARD)
        {
            ui->doubleSpinBoxDiffLevelsSigma->setValue(lsa::DEFAULT_SIGMA_DIGITALLEVELS_STANDARDRODS_M);
        }
        else if(index == SPIRIT_STANDARD)
        {
            ui->doubleSpinBoxDiffLevelsSigma->setValue(lsa::DEFAULT_SIGMA_SPIRITLEVELS_STANDARDRODS_M);
        }
    }
}

void ConverterDialog::onDiffLevelsSigmaSourceChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    if(index == SOURCE_DEFAULT ||
       index == SOURCE_INST)
    {
        ui->doubleSpinBoxDiffLevelsSigma->setEnabled(false);
    }
    else if(index == SOURCE_CUSTOM)
    {
        ui->doubleSpinBoxDiffLevelsSigma->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == SOURCE_DEFAULT)
    {
        int measTypeIndex = ui->comboBoxDiffLevelsMeasType->currentIndex();
        if(measTypeIndex == DIGITAL_PRECISE)
        {
            ui->doubleSpinBoxDiffLevelsSigma->setValue(lsa::DEFAULT_SIGMA_DIGITALLEVELS_PRECISERODS_M);
        }
        else if(measTypeIndex == DIGITAL_STANDARD)
        {
            ui->doubleSpinBoxDiffLevelsSigma->setValue(lsa::DEFAULT_SIGMA_DIGITALLEVELS_STANDARDRODS_M);
        }
        else if(measTypeIndex == SPIRIT_STANDARD)
        {
            ui->doubleSpinBoxDiffLevelsSigma->setValue(lsa::DEFAULT_SIGMA_SPIRITLEVELS_STANDARDRODS_M);
        }
    }
    else if(index == SOURCE_INST)
    {
        ui->doubleSpinBoxDiffLevelsSigma->setValue(INST);
    }
}

void ConverterDialog::onTSDistancesSigmaSourceChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    if(index == SOURCE_DEFAULT ||
       index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSDistIRSigma->setEnabled(false);
        ui->doubleSpinBoxTSDistRedSigma->setEnabled(false);
        ui->doubleSpinBoxTSDistNoEDMSigma->setEnabled(false);
    }
    else if(index == SOURCE_CUSTOM)
    {
        ui->doubleSpinBoxTSDistIRSigma->setEnabled(true);
        ui->doubleSpinBoxTSDistRedSigma->setEnabled(true);
        ui->doubleSpinBoxTSDistNoEDMSigma->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == SOURCE_DEFAULT)
    {
        ui->doubleSpinBoxTSDistIRSigma->setValue(lsa::DEFAULT_SIGMA_TOTALSTATION_DISTANCE_INFRARED_M);
        ui->doubleSpinBoxTSDistRedSigma->setValue(lsa::DEFAULT_SIGMA_TOTALSTATION_DISTANCE_REDLASER_M);
        ui->doubleSpinBoxTSDistNoEDMSigma->setValue(lsa::DEFAULT_SIGMA_TOTALSTATION_DISTANCE_UNKNOWNPRISM_M);
    }
    else if(index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSDistIRSigma->setValue(INST);
        ui->doubleSpinBoxTSDistRedSigma->setValue(INST);
        ui->doubleSpinBoxTSDistNoEDMSigma->setValue(INST);
    }
}

void ConverterDialog::onTSHorzAnglesSigmaSourceChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    if(index == SOURCE_DEFAULT ||
       index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSHorzAnglesSigma->setEnabled(false);
    }
    else if(index == SOURCE_CUSTOM)
    {
        ui->doubleSpinBoxTSHorzAnglesSigma->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == SOURCE_DEFAULT)
    {
        ui->doubleSpinBoxTSHorzAnglesSigma->setValue(lsa::DEFAULT_SIGMA_TOTALSTATION_HORZANGLES_SOA);
    }
    else if(index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSHorzAnglesSigma->setValue(INST);
    }
}

void ConverterDialog::onTSVertAnglesSigmaSourceChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    if(index == SOURCE_DEFAULT ||
       index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSVertAnglesSigma->setEnabled(false);
    }
    else if(index == SOURCE_CUSTOM)
    {
        ui->doubleSpinBoxTSVertAnglesSigma->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == SOURCE_DEFAULT)
    {
        ui->doubleSpinBoxTSVertAnglesSigma->setValue(lsa::DEFAULT_SIGMA_TOTALSTATION_VERTANGLES_SOA);
    }
    else if(index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSVertAnglesSigma->setValue(INST);
    }
}

void ConverterDialog::onTSElevDiffsSigmaSourceChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    if(index == SOURCE_DEFAULT ||
       index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSElevDiffsSigma->setEnabled(false);
    }
    else if(index == SOURCE_CUSTOM)
    {
        ui->doubleSpinBoxTSElevDiffsSigma->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == SOURCE_DEFAULT)
    {
        ui->doubleSpinBoxTSElevDiffsSigma->setValue(lsa::DEFAULT_SIGMA_TOTALSTATION_ELEVDIFFS_M);
    }
    else if(index == SOURCE_INST)
    {
        ui->doubleSpinBoxTSElevDiffsSigma->setValue(INST);
    }
}

/// Updates the instrument height sigma value depending on which dropdown is selected
/// @param index The index for the dropdown box
void ConverterDialog::onTSInstrHghtSigmaSourceChanged(int index)
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    if(index == INSTR_HGHT_MEASURE_DEFAULT ||
       index == INSTR_HGHT_MEASURE_ROD ||
       index == INSTR_HGHT_MEASURE_TAPE)
    {
        ui->doubleSpinBoxTSInstrHghtSigma->setEnabled(false);
    }
    else if(index == INSTR_HGHT_MEASURE_CUSTOM)
    {
        ui->doubleSpinBoxTSInstrHghtSigma->setEnabled(true);
    }

    //update populate fields with saved values
    if(index == INSTR_HGHT_MEASURE_DEFAULT)
    {
        ui->doubleSpinBoxTSInstrHghtSigma->setValue(lsa::DEFAULT_SIGMA_TOTALSTATION_INSTR_HGHT_M);
    }
    else if(index == INSTR_HGHT_MEASURE_ROD)
    {
        ui->doubleSpinBoxTSInstrHghtSigma->setValue(lsa::MEASURE_ROD_SIGMA_TOTALSTATION_INSTR_HGHT_M);
    }
    else if(index == INSTR_HGHT_MEASURE_TAPE)
    {
        ui->doubleSpinBoxTSInstrHghtSigma->setValue(lsa::MEASURE_TAPE_SIGMA_TOTALSTATION_INSTR_HGHT_M);
    }
}

panelSettings ConverterDialog::getPanelSettings()
{
    panelSettings panel_settings;

    panel_settings.StaticGPSFRCent = ui->comboBoxStaticGPSFRSetup->currentIndex();
    panel_settings.StaticGPSTOCent = ui->comboBoxStaticGPSTOSetup->currentIndex();
    panel_settings.KinematicGPSFRCent = ui->comboBoxKinGPSFRSetup->currentIndex();
    panel_settings.KinematicGPSTOCent = ui->comboBoxKinGPSTOSetup->currentIndex();
    panel_settings.LevelsFRCent = ui->comboBoxDiffLevelsFRSetup->currentIndex();
    panel_settings.LevelsTOCent = ui->comboBoxDiffLevelsTOSetup->currentIndex();
    panel_settings.TSFRCent = ui->comboBoxTSFRSetup->currentIndex();
    panel_settings.TSTOCent = ui->comboBoxTSTOSetup->currentIndex();
    panel_settings.DiffLevelsMeasType = ui->comboBoxDiffLevelsMeasType->currentIndex();
    panel_settings.DiffLevelsSigmaSource = ui->comboBoxDiffLevelsSigmaSource->currentIndex();
    panel_settings.TSDistancesSigmaSource = ui->comboBoxTSDistancesSigmaSource->currentIndex();
    panel_settings.TSHorzAnglesSigmaSource = ui->comboBoxTSHorzAnglesSigmaSource->currentIndex();
    panel_settings.TSVertAnglesSigmaSource = ui->comboBoxTSVertAnglesSigmaSource->currentIndex();
    panel_settings.TSElevDiffsSigmaSource = ui->comboBoxTSElevDiffsSigmaSource->currentIndex();
    panel_settings.TSInstrHghtSigmaSource = ui->comboBoxTSInstrHghtSigmaSource->currentIndex();
    panel_settings.GPSStaticPPM = ui->doubleSpinBoxStaticGPSPPM->value();
    panel_settings.GPSKinematicPPM = ui->doubleSpinBoxKinGPSPPM->value();
    panel_settings.DiffLevelsPPM = ui->doubleSpinBoxDiffLevelsPPM->value();
    panel_settings.DistanceIRPPM = ui->doubleSpinBoxTSDistIRPPM->value();
    panel_settings.DistanceRedPPM = ui->doubleSpinBoxTSDistRedPPM->value();
    panel_settings.DistanceNonPPM = ui->doubleSpinBoxTSDistNonPPM->value();
    panel_settings.CustomStaticGPSFR = ui->doubleSpinBoxStaticGPSFRCent->value();
    panel_settings.CustomStaticGPSTO = ui->doubleSpinBoxStaticGPSTOCent->value();
    panel_settings.CustomKinematicGPSFR = ui->doubleSpinBoxKinGPSFRCent->value();
    panel_settings.CustomKinematicGPSTO = ui->doubleSpinBoxKinGPSTOCent->value();
    panel_settings.CustomLevelsFR = ui->doubleSpinBoxDiffLevelsFRCent->value();
    panel_settings.CustomLevelsTO = ui->doubleSpinBoxDiffLevelsTOCent->value();
    panel_settings.CustomTSDistFR = ui->doubleSpinBoxTSDistancesFRCent->value();
    panel_settings.CustomTSDistTO = ui->doubleSpinBoxTSDistancesTOCent->value();
    panel_settings.CustomTSHorzFR = ui->doubleSpinBoxTSHorzAnglesFRCent->value();
    panel_settings.CustomTSHorzTO = ui->doubleSpinBoxTSHorzAnglesTOCent->value();
    panel_settings.CustomTSVertFR = ui->doubleSpinBoxTSVertAnglesFRCent->value();
    panel_settings.CustomTSVertTO = ui->doubleSpinBoxTSVertAnglesTOCent->value();
    panel_settings.CustomTSElevDiffsFR = ui->doubleSpinBoxTSElevDiffsFRCent->value();
    panel_settings.CustomTSElevDiffsTO = ui->doubleSpinBoxTSElevDiffsTOCent->value();
    panel_settings.CustomLevelsSigma = ui->doubleSpinBoxDiffLevelsSigma->value();
    panel_settings.CustomTSDistIRSigma = ui->doubleSpinBoxTSDistIRSigma->value();
    panel_settings.CustomTSDistRedSigma = ui->doubleSpinBoxTSDistRedSigma->value();
    panel_settings.CustomTSDistNonSigma = ui->doubleSpinBoxTSDistNoEDMSigma->value();
    panel_settings.CustomTSHorzSigma = ui->doubleSpinBoxTSHorzAnglesSigma->value();
    panel_settings.CustomTSVertSigma = ui->doubleSpinBoxTSVertAnglesSigma->value();
    panel_settings.CustomTSElevDiffsSigma = ui->doubleSpinBoxTSElevDiffsSigma->value();
    panel_settings.CustomTSInstrHghtSigma = ui->doubleSpinBoxTSInstrHghtSigma->value();


    return panel_settings;
}

panelValues ConverterDialog::getPanelValues()
{
    panelValues panel_values;

    panel_values.StaticGPSFR = ui->doubleSpinBoxStaticGPSFRCent->value();
    panel_values.StaticGPSTO = ui->doubleSpinBoxStaticGPSTOCent->value();
    panel_values.KinematicGPSFR = ui->doubleSpinBoxKinGPSFRCent->value();
    panel_values.KinematicGPSTO = ui->doubleSpinBoxKinGPSTOCent->value();
    panel_values.LevelsFR = ui->doubleSpinBoxDiffLevelsFRCent->value();
    panel_values.LevelsTO = ui->doubleSpinBoxDiffLevelsTOCent->value();
    panel_values.GPSStaticPPM = ui->doubleSpinBoxStaticGPSPPM->value();
    panel_values.GPSKinematicPPM = ui->doubleSpinBoxKinGPSPPM->value();
    panel_values.DiffLevelsPPM = ui->doubleSpinBoxDiffLevelsPPM->value();
    panel_values.DistanceIRPPM = ui->doubleSpinBoxTSDistIRPPM->value();
    panel_values.DistanceRedPPM = ui->doubleSpinBoxTSDistRedPPM->value();
    panel_values.DistanceNonPPM = ui->doubleSpinBoxTSDistNonPPM->value();
    if(ui->doubleSpinBoxDiffLevelsSigma->text() == QString("INST"))
        panel_values.LevelsSigma = -1;
    else
        panel_values.LevelsSigma = ui->doubleSpinBoxDiffLevelsSigma->value();
    panel_values.TSDistFR = ui->doubleSpinBoxTSDistancesFRCent->value();
    panel_values.TSDistTO = ui->doubleSpinBoxTSDistancesTOCent->value();
    if(ui->doubleSpinBoxTSDistIRSigma->text() == QString("INST"))
        panel_values.TSDistIRSigma = -1;
    else
        panel_values.TSDistIRSigma = ui->doubleSpinBoxTSDistIRSigma->value();
    if(ui->doubleSpinBoxTSDistRedSigma->text() == QString("INST"))
        panel_values.TSDistRedSigma = -1;
    else
        panel_values.TSDistRedSigma = ui->doubleSpinBoxTSDistRedSigma->value();
    if(ui->doubleSpinBoxTSDistNoEDMSigma->text() == QString("INST"))
        panel_values.TSDistNonSigma = -1;
    else
        panel_values.TSDistNonSigma = ui->doubleSpinBoxTSDistNoEDMSigma->value();
    panel_values.TSHorzFR = ui->doubleSpinBoxTSHorzAnglesFRCent->value();
    panel_values.TSHorzTO = ui->doubleSpinBoxTSHorzAnglesTOCent->value();
    if(ui->doubleSpinBoxTSHorzAnglesSigma->text() == QString("INST"))
        panel_values.TSHorzSigma = -1;
    else
        panel_values.TSHorzSigma = ui->doubleSpinBoxTSHorzAnglesSigma->value();
    panel_values.TSVertFR = ui->doubleSpinBoxTSVertAnglesFRCent->value();
    panel_values.TSVertTO = ui->doubleSpinBoxTSVertAnglesTOCent->value();
    if(ui->doubleSpinBoxTSVertAnglesSigma->text() == QString("INST"))
        panel_values.TSVertSigma = -1;
    else
        panel_values.TSVertSigma = ui->doubleSpinBoxTSVertAnglesSigma->value();
    panel_values.TSElevDiffsFR = ui->doubleSpinBoxTSElevDiffsFRCent->value();
    panel_values.TSElevDiffsTO = ui->doubleSpinBoxTSElevDiffsTOCent->value();
    if(ui->doubleSpinBoxTSElevDiffsSigma->text() == QString("INST"))
        panel_values.TSElevDiffsSigma = -1;
    else
        panel_values.TSElevDiffsSigma = ui->doubleSpinBoxTSElevDiffsSigma->value();

    panel_values.TSInstrHghtSigma = ui->doubleSpinBoxTSInstrHghtSigma->value();

    return panel_values;
}

void ConverterDialog::createConverterConfigFile()
{
    //Get the current values from the combo box settings
    panelValues panel_values = this->getPanelValues();

    //Write the values to the file
    QFile newFile(configFile);
    newFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream newStream(&newFile);

    newStream << "MEAS_TYPE\tLABEL\t\t\tSIGMA\tPPM\tAT_C\tFROM_C\tTO_C\n\n";
    newStream << "GPS_STATIC\t" << LABEL_GPS_STATIC << "\t0\t" << QString::number(panel_values.GPSStaticPPM) << "\t0\t" << QString::number(panel_values.StaticGPSFR) << "\t" << QString::number(panel_values.StaticGPSTO) << "\n";
    newStream << "GPS_KINEMATIC\t" << LABEL_GPS_KINEMATIC << "\t0\t" << QString::number(panel_values.GPSKinematicPPM) << "\t0\t" << QString::number(panel_values.KinematicGPSFR) << "\t" << QString::number(panel_values.KinematicGPSTO) << "\n";
    if(panel_values.LevelsSigma < 0.0)
        newStream << "DIFF_LEVELS\t" << LABEL_DIFF_LEVELS << "\t\tINST\t" << QString::number(panel_values.DiffLevelsPPM) << "\t0\t" << QString::number(panel_values.LevelsFR) << "\t" << QString::number(panel_values.LevelsTO) << "\n";
    else
        newStream << "DIFF_LEVELS\t" << LABEL_DIFF_LEVELS << "\t\t" << QString::number(panel_values.LevelsSigma) << "\t" << QString::number(panel_values.DiffLevelsPPM) << "\t0\t" << QString::number(panel_values.LevelsFR) << "\t" << QString::number(panel_values.LevelsTO) << "\n";
    if(panel_values.TSDistIRSigma < 0.0)
        newStream << "TOTSTA_DIST_IR\t" << LABEL_TOTSTA_DIST_IR << "\t\tINST\t" << QString::number(panel_values.DistanceIRPPM) << "\t0\t" << QString::number(panel_values.TSDistFR) << "\t" << QString::number(panel_values.TSDistTO) << "\n";
    else
        newStream << "TOTSTA_DIST_IR\t" << LABEL_TOTSTA_DIST_IR << "\t\t" << QString::number(panel_values.TSDistIRSigma) << "\t" << QString::number(panel_values.DistanceIRPPM) << "\t0\t" << QString::number(panel_values.TSDistFR) << "\t" << QString::number(panel_values.TSDistTO) << "\n";
    if(panel_values.TSDistRedSigma < 0.0)
        newStream << "TOTSTA_DIST_RED\t" << LABEL_TOTSTA_DIST_RED << "\t\tINST\t" << QString::number(panel_values.DistanceRedPPM) << "\t0\t" << QString::number(panel_values.TSDistFR) << "\t" << QString::number(panel_values.TSDistTO) << "\n";
    else
        newStream << "TOTSTA_DIST_RED\t" << LABEL_TOTSTA_DIST_RED << "\t\t" << QString::number(panel_values.TSDistRedSigma) << "\t" << QString::number(panel_values.DistanceRedPPM) << "\t0\t" << QString::number(panel_values.TSDistFR) << "\t" << QString::number(panel_values.TSDistTO) << "\n";
    if(panel_values.TSDistNonSigma < 0.0)
        newStream << "TOTSTA_DIST_NON\t" << LABEL_TOTSTA_DIST_NON << "\tINST\t" << QString::number(panel_values.DistanceNonPPM) << "\t0\t" << QString::number(panel_values.TSDistFR) << "\t" << QString::number(panel_values.TSDistTO) << "\n";
    else
        newStream << "TOTSTA_DIST_NON\t" << LABEL_TOTSTA_DIST_NON << "\t" << QString::number(panel_values.TSDistNonSigma) << "\t" << QString::number(panel_values.DistanceNonPPM) << "\t0\t" << QString::number(panel_values.TSDistFR) << "\t" << QString::number(panel_values.TSDistTO) << "\n";
    if(panel_values.TSHorzSigma < 0.0)
        newStream << "TOTSTA_HORZ_DIR\t" << LABEL_TOTSTA_HORZ_DIR << "\t\tINST\t0\t0\t" << QString::number(panel_values.TSHorzFR) << "\t" << QString::number(panel_values.TSHorzTO) << "\n";
    else
        newStream << "TOTSTA_HORZ_DIR\t" << LABEL_TOTSTA_HORZ_DIR << "\t\t" << QString::number(panel_values.TSHorzSigma) << "\t0\t0\t" << QString::number(panel_values.TSHorzFR) << "\t" << QString::number(panel_values.TSHorzTO) << "\n";
    if(panel_values.TSHorzSigma < 0.0)
        newStream << "TOTSTA_HORZ_ANG\t" << LABEL_TOTSTA_HORZ_ANG << "\t\tINST\t0\t" << QString::number(0.001) << "\t" << QString::number(panel_values.TSHorzFR) << "\t" << QString::number(panel_values.TSHorzTO) << "\n";
    else
        newStream << "TOTSTA_HORZ_ANG\t" << LABEL_TOTSTA_HORZ_ANG << "\t\t" << QString::number(panel_values.TSHorzSigma) << "\t0\t" << QString::number(0.001) << "\t" << QString::number(panel_values.TSHorzFR) << "\t" << QString::number(panel_values.TSHorzTO) << "\n";
    if(panel_values.TSVertSigma < 0.0)
        newStream << "TOTSTA_VERT\t" << LABEL_TOTSTA_VERT << "\t\tINST\t0\t0\t" << QString::number(panel_values.TSVertFR) << "\t" << QString::number(panel_values.TSVertTO) << "\n";
    else
        newStream << "TOTSTA_VERT\t" << LABEL_TOTSTA_VERT << "\t\t" << QString::number(panel_values.TSVertSigma) << "\t0\t0\t" << QString::number(panel_values.TSVertFR) << "\t" << QString::number(panel_values.TSVertTO) << "\n";
    if(panel_values.TSElevDiffsSigma < 0.0)
        newStream << "TOTSTA_DE\t" << LABEL_TOTSTA_DE << "\t\tINST\t0\t0\t" << QString::number(panel_values.TSElevDiffsFR) << "\t" << QString::number(panel_values.TSElevDiffsTO) << "\n";
    else
        newStream << "TOTSTA_DE\t" << LABEL_TOTSTA_DE << "\t\t" << QString::number(panel_values.TSElevDiffsSigma) << "\t0\t0\t" << QString::number(panel_values.TSElevDiffsFR) << "\t" << QString::number(panel_values.TSElevDiffsTO) << "\n";

    newStream << "TOTSTA_INSTR_HT\t" << LABEL_TOTSTA_INSTR_HGHT << "\t" << QString::number(panel_values.TSInstrHghtSigma) << "\t0\t0\t0\t0" << "\n";


    newFile.close();
}

//bool ConverterDialog::almostEquals(double a, double b)
//{
//    if(fabs(a-b) < lsa::ZERO_BOUND)
//        return true;
//    else
//        return false;
//}

bool ConverterDialog::readConverterConfigSettingsFile()
{
    QString configSettingsFile(configFile);
    configSettingsFile.replace(QString(".cfg"),QString(".ini"));
    QFile cFile(configSettingsFile);
    QString line, widget, value;
    QStringList parts;

    if(cFile.exists())
    {
        cFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream newStream(&cFile);
        bool ok(false);

        while (!cFile.atEnd())
        {
            line = cFile.readLine();
            parts = line.split(" ");
            widget = parts[0];
            value = parts[1];

            if(line.startsWith(QString("StaticGPSFRCent")))
            {
                ui->comboBoxStaticGPSFRSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("StaticGPSTOCent")))
            {
                ui->comboBoxStaticGPSTOSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("KinematicGPSFRCent")))
            {
                ui->comboBoxKinGPSFRSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("KinematicGPSTOCent")))
            {
                ui->comboBoxKinGPSTOSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("LevelsFRCent")))
            {
                ui->comboBoxDiffLevelsFRSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("LevelsTOCent")))
            {
                ui->comboBoxDiffLevelsTOSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("TSFRCent")))
            {
                ui->comboBoxTSFRSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("TSTOCent")))
            {
                ui->comboBoxTSTOSetup->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("DiffLevelsMeasType")))
            {
                ui->comboBoxDiffLevelsMeasType->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("DiffLevelsSigmaSource")))
            {
                ui->comboBoxDiffLevelsSigmaSource->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("TSDistancesSigmaSource")))
            {
                ui->comboBoxTSDistancesSigmaSource->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("TSHorzAnglesSigmaSource")))
            {
                ui->comboBoxTSHorzAnglesSigmaSource->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("TSVertAnglesSigmaSource")))
            {
                ui->comboBoxTSVertAnglesSigmaSource->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("TSElevDiffsSigmaSource")))
            {
                ui->comboBoxTSElevDiffsSigmaSource->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("TSInstrHghtSigmaSource")))
            {
                ui->comboBoxTSInstrHghtSigmaSource->setCurrentIndex(value.toInt(&ok));
            }
            else if(line.startsWith(QString("GPSStaticPPM")))
            {
                ui->doubleSpinBoxStaticGPSPPM->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("GPSKinematicPPM")))
            {
                ui->doubleSpinBoxKinGPSPPM->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("DiffLevelsPPM")))
            {
                ui->doubleSpinBoxDiffLevelsPPM->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("DistanceIRPPM")))
            {
                ui->doubleSpinBoxTSDistIRPPM->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("DistanceRedPPM")))
            {
                ui->doubleSpinBoxTSDistRedPPM->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("DistanceNonPPM")))
            {
                ui->doubleSpinBoxTSDistNonPPM->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomStaticGPSFR")) && (ui->comboBoxStaticGPSFRSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxStaticGPSFRCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomStaticGPSTO")) && (ui->comboBoxStaticGPSTOSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxStaticGPSTOCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomKinematicGPSFR")) && (ui->comboBoxKinGPSFRSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxKinGPSFRCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomKinematicGPSTO")) && (ui->comboBoxKinGPSTOSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxKinGPSTOCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomLevelsFR")) && (ui->comboBoxDiffLevelsFRSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxDiffLevelsFRCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomLevelsTO")) && (ui->comboBoxDiffLevelsTOSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxDiffLevelsTOCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomLevelsSigma")) && (ui->comboBoxDiffLevelsSigmaSource->currentIndex() == SOURCE_CUSTOM))
            {
                ui->doubleSpinBoxDiffLevelsSigma->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSDistFR")) && (ui->comboBoxTSFRSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSDistancesFRCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSDistTO")) && (ui->comboBoxTSTOSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSDistancesTOCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSDistIRSigma")) && (ui->comboBoxTSDistancesSigmaSource->currentIndex() == SOURCE_CUSTOM))
            {
                ui->doubleSpinBoxTSDistIRSigma->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSDistRedSigma")) && (ui->comboBoxTSDistancesSigmaSource->currentIndex() == SOURCE_CUSTOM))
            {
                ui->doubleSpinBoxTSDistRedSigma->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSDistNonSigma")) && (ui->comboBoxTSDistancesSigmaSource->currentIndex() == SOURCE_CUSTOM))
            {
                ui->doubleSpinBoxTSDistNoEDMSigma->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSHorzFR")) && (ui->comboBoxTSFRSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSHorzAnglesFRCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSHorzTO")) && (ui->comboBoxTSTOSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSHorzAnglesTOCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSHorzSigma")) && (ui->comboBoxTSHorzAnglesSigmaSource->currentIndex() == SOURCE_CUSTOM))
            {
                ui->doubleSpinBoxTSHorzAnglesSigma->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSVertFR")) && (ui->comboBoxTSFRSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSVertAnglesFRCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSVertTO")) && (ui->comboBoxTSTOSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSVertAnglesTOCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSVertSigma")) && (ui->comboBoxTSVertAnglesSigmaSource->currentIndex() == SOURCE_CUSTOM))
            {
                ui->doubleSpinBoxTSVertAnglesSigma->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSElevDiffsFR")) && (ui->comboBoxTSFRSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSElevDiffsFRCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSElevDiffsTO")) && (ui->comboBoxTSTOSetup->currentIndex() == SETUP_CUSTOM))
            {
                ui->doubleSpinBoxTSElevDiffsTOCent->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSElevDiffsSigma")) && (ui->comboBoxTSElevDiffsSigmaSource->currentIndex() == SOURCE_CUSTOM))
            {
                ui->doubleSpinBoxTSElevDiffsSigma->setValue(value.toDouble(&ok));
            }
            else if(line.startsWith(QString("CustomTSInstrHghtSigma")) && (ui->comboBoxTSInstrHghtSigmaSource->currentIndex() == INSTR_HGHT_MEASURE_CUSTOM))
            {
                ui->doubleSpinBoxTSInstrHghtSigma->setValue(value.toDouble(&ok));
            }
         }

        cFile.close();

        return true;
    }
    else
        return false;
}

void ConverterDialog::writeConverterConfigSettingsFile()
{
    panelSettings panel_settings = getPanelSettings();
    QString configSettingsFile(configFile);
    configSettingsFile.replace(QString(".cfg"),QString(".ini"));

    QFile cFile(configSettingsFile);
    cFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream newStream(&cFile);

    newStream << "StaticGPSFRCent " << panel_settings.StaticGPSFRCent << endl;
    newStream << "StaticGPSTOCent " << panel_settings.StaticGPSTOCent << endl;
    newStream << "KinematicGPSFRCent " << panel_settings.KinematicGPSFRCent << endl;
    newStream << "KinematicGPSTOCent " << panel_settings.KinematicGPSTOCent << endl;
    newStream << "LevelsFRCent " << panel_settings.LevelsFRCent << endl;
    newStream << "LevelsTOCent " << panel_settings.LevelsTOCent << endl;
    newStream << "TSFRCent " << panel_settings.TSFRCent << endl;
    newStream << "TSTOCent " << panel_settings.TSTOCent << endl;
    newStream << "DiffLevelsMeasType " << panel_settings.DiffLevelsMeasType << endl;
    newStream << "DiffLevelsSigmaSource " << panel_settings.DiffLevelsSigmaSource << endl;
    newStream << "TSDistancesSigmaSource " << panel_settings.TSDistancesSigmaSource << endl;
    newStream << "TSHorzAnglesSigmaSource " << panel_settings.TSHorzAnglesSigmaSource << endl;
    newStream << "TSVertAnglesSigmaSource " << panel_settings.TSVertAnglesSigmaSource << endl;
    newStream << "TSElevDiffsSigmaSource " << panel_settings.TSElevDiffsSigmaSource << endl;
    newStream << "TSInstrHghtSigmaSource " << panel_settings.TSInstrHghtSigmaSource << endl;
    newStream << "GPSStaticPPM " << panel_settings.GPSStaticPPM << endl;
    newStream << "GPSKinematicPPM " << panel_settings.GPSKinematicPPM << endl;
    newStream << "DiffLevelsPPM " << panel_settings.DiffLevelsPPM << endl;
    newStream << "DistanceIRPPM " << panel_settings.DistanceIRPPM << endl;
    newStream << "DistanceRedPPM " << panel_settings.DistanceRedPPM << endl;
    newStream << "DistanceNonPPM " << panel_settings.DistanceNonPPM << endl;
    newStream << "CustomStaticGPSFR " << panel_settings.CustomStaticGPSFR << endl;
    newStream << "CustomStaticGPSTO " << panel_settings.CustomStaticGPSTO << endl;
    newStream << "CustomKinematicGPSFR " << panel_settings.CustomKinematicGPSFR << endl;
    newStream << "CustomKinematicGPSTO " << panel_settings.CustomKinematicGPSTO << endl;
    newStream << "CustomLevelsFR " << panel_settings.CustomLevelsFR << endl;
    newStream << "CustomLevelsTO " << panel_settings.CustomLevelsTO << endl;
    newStream << "CustomLevelsSigma " << panel_settings.CustomLevelsSigma << endl;
    newStream << "CustomTSDistFR " << panel_settings.CustomTSDistFR << endl;
    newStream << "CustomTSDistTO " << panel_settings.CustomTSDistTO << endl;
    newStream << "CustomTSDistIRSigma " << panel_settings.CustomTSDistIRSigma << endl;
    newStream << "CustomTSDistRedSigma " << panel_settings.CustomTSDistRedSigma << endl;
    newStream << "CustomTSDistNonSigma " << panel_settings.CustomTSDistNonSigma << endl;
    newStream << "CustomTSHorzFR " << panel_settings.CustomTSHorzFR << endl;
    newStream << "CustomTSHorzTO " << panel_settings.CustomTSHorzTO << endl;
    newStream << "CustomTSHorzSigma " << panel_settings.CustomTSHorzSigma << endl;
    newStream << "CustomTSVertFR " << panel_settings.CustomTSVertFR << endl;
    newStream << "CustomTSVertTO " << panel_settings.CustomTSVertTO << endl;
    newStream << "CustomTSVertSigma " << panel_settings.CustomTSVertSigma << endl;
    newStream << "CustomTSElevDiffsFR " << panel_settings.CustomTSElevDiffsFR << endl;
    newStream << "CustomTSElevDiffsTO " << panel_settings.CustomTSElevDiffsTO << endl;
    newStream << "CustomTSElevDiffsSigma " << panel_settings.CustomTSElevDiffsSigma << endl;
    newStream << "CustomTSInstrHghtSigmaSource " << panel_settings.CustomTSInstrHghtSigma << endl;

    cFile.close();
}
