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
#include "ConfigDialog.hpp"
#include "ui_ConfigDialog.h"
#include "lsaUtils.hpp"//addQuotes
#include <StringUtils.hpp>
#include <QMessageBox>
#include <fstream>
#include <QSettings>
#include <QDir>
#include <expandpath.hpp>
#include "LSASupportedToVersion.hpp"

ConfigDialog::ConfigDialog(QString projectConfigFile, QWidget *parent) :
    configFile(projectConfigFile), QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    setupControls();

    converterDialog = NULL;

    connect(ui->buttonBoxConfigDialog, SIGNAL(accepted()),    this, SLOT(onOKButtonPressed())             );
    connect(ui->pushButtonConverterConfig, SIGNAL(clicked()), this, SLOT(onConverterConfigButtonPressed()));
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::setupControls()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));

    ui->radioButtonYesCOMAP->setChecked(false);
    ui->radioButtonNoCOMAP->setChecked(true);
    ui->spinBoxMaxIter->setValue(15);
    ui->lineEditConvCriteria->setText(QString("1.0e-5"));
    ui->doubleSpinBoxConfidenceLevel->setValue(0.95);
    ui->radioButtonUnscaled->setChecked(false);
    ui->radioButtonScaleByAPV->setChecked(true);
    ui->radioButtonWest->setChecked(false);
    ui->radioButtonEast->setChecked(true);
    ui->radioButton1Sigma->setChecked(true);
    ui->radioButton90->setChecked(false);
    ui->radioButton95->setChecked(false);
    ui->spinBoxLinearPrecP->setValue(qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_METERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_POSITION_METERS).toInt());
    ui->spinBoxLinearPrecM->setValue(qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS, lsa::DEFAULT_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS).toInt());
    ui->spinBoxAngularPrecPoints->setValue(qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_SOA, lsa::DEFAULT_NUM_DECIMALS_ANGLE_POSITION_SOA).toInt());
    ui->spinBoxAngularPrecMeas->setValue(qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA, lsa::DEFAULT_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA).toInt());
    ui->checkBoxVerbose->setChecked(false);
    ui->checkBoxPartialsMatrixFile->setChecked(false);
    ui->radioButtonAllowFast->setChecked(true);
    ui->radioButtonForceFast->setChecked(false);
    ui->radioButtonForceStable->setChecked(false);
    ui->radioButtonYesExtRelVect->setChecked(true);
    ui->radioButtonNoExtRelVect->setChecked(false);
    ui->doubleSpinBoxWarnExtRelVect->setValue(qsettings.value( lsa::QSETTINGS_MAX_EXT_MAG_WARNING, lsa::DEFAULT_MAX_EXT_MAG_WARNING).toDouble());
    ui->doubleSpinBoxErrorExtRelVect->setValue(qsettings.value( lsa::QSETTINGS_MAX_EXT_MAG_ERROR, lsa::DEFAULT_MAX_EXT_MAG_ERROR).toDouble());
    ui->checkBoxIncludeUnusedPoints->setChecked(false);

    QStringList geoidFiles = findInstalledGeoidFiles();//uncludes empty string for no geoid file
    while(ui->comboBoxGeoidFile->count()>0)
        ui->comboBoxGeoidFile->removeItem(0);
    ui->comboBoxGeoidFile->addItems(geoidFiles);
    ui->comboBoxGeoidFile->setCurrentIndex(ui->comboBoxGeoidFile->findText(QString("NONE")));
    ui->checkBoxGeoidFileH5Overwrite->setChecked(true);

//TODO - Add validators to the line edits (issues with scientific notation exceeding bounds)
//    QDoubleValidator *conv = new QDoubleValidator(1.0e-20, 1.0, 5, this);
//    conv->setNotation(QDoubleValidator::ScientificNotation);
//    ui->lineEditConvCriteria->setValidator(conv);
//    QDoubleValidator *conf = new QDoubleValidator(0.00001, .99999, 3, this);
//    ui->lineEditConfidenceLevel->setValidator(conf);

    std::ifstream istrm;
    istrm.open(configFile.toStdString().c_str(),std::ios::in);
    if(!istrm.is_open())
       return;
    std::string line;
    std::vector<std::string> parts;
    bool parseError = false;

    //TODO: exception handling for parse errors
    while(1)
    {
       line = std::string("");
       getline(istrm,line);
       gnsstk::StringUtils::stripTrailing(line,'\r');
       gnsstk::StringUtils::stripTrailing(line,'\n');
       if((istrm.eof() || !istrm.good()) && line.empty()) break;
       if(line.empty())
           continue;

       if(line.at(0)!='#')
       {
           try
           {
               if(line.find("--allowCOM")!=std::string::npos)
               {
                   ui->radioButtonYesCOMAP->setChecked(true);
                   ui->radioButtonNoCOMAP->setChecked(false);
               }
               else if(line.find("--niter")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->spinBoxMaxIter->setValue(atoi(parts[1].c_str()));
               }
               else if((line.find("--conv")!=std::string::npos)&&(line.size()>7))
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->lineEditConvCriteria->setText(QString::fromStdString(parts[1]));
               }
               else if((line.find("--alpha")!=std::string::npos)&&(line.size()>8))
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');                   
                   ui->doubleSpinBoxConfidenceLevel->setValue(1.0 - atof(parts[1].c_str()));
               }
               else if(line.find("--noAPV")!=std::string::npos)
               {
                    ui->radioButtonUnscaled->setChecked(true);
               }
               else if(line.find("--APV")!=std::string::npos)
               {
                    ui->radioButtonScaleByAPV->setChecked(true);
               }
               else if((line.find("--geoidfile")!=std::string::npos)&&line.size()>12)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->comboBoxGeoidFile->setCurrentIndex(ui->comboBoxGeoidFile->findText(QString::fromStdString(parts[1])));
               }
               else if(line.find("--patchoverwrite")!=std::string::npos)
               {
                   ui->checkBoxGeoidFileH5Overwrite->setChecked(true);
               }
               else if(line.find("--westLon")!=std::string::npos)
               {
                   ui->radioButtonWest->setChecked(true);
               }
               else if(line.find("--confid")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   if(parts.at(1) == std::string("1sig"))
                       ui->radioButton1Sigma->setChecked(true);
                   else if(parts.at(1) == std::string("90"))
                       ui->radioButton90->setChecked(true);
                   else
                       ui->radioButton95->setChecked(true);
               }
               else if(line.find("--linprecM")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->spinBoxLinearPrecM->setValue(atoi(parts[1].c_str()));
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS, ui->spinBoxLinearPrecM->value() );

                   // Determine the values of the associated qsettings
                   if(ui->spinBoxLinearPrecM->value() >= 3)
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_KILOMETERS, ui->spinBoxLinearPrecM->value() + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_CENTIMETERS, ui->spinBoxLinearPrecM->value() - 2 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET, ui->spinBoxLinearPrecM->value() - 2 );
                   }
                   else
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_KILOMETERS, ui->spinBoxLinearPrecM->value() + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_CENTIMETERS, 1 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET, 1 );
                   }
               }
               else if(line.find("--linprecP")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->spinBoxLinearPrecP->setValue(atoi(parts[1].c_str()));
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_METERS, ui->spinBoxLinearPrecP->value() );

                   // Determine the values of the associated qsettings
                   if(ui->spinBoxLinearPrecP->value() >= 3)
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_KILOMETERS, ui->spinBoxLinearPrecP->value() + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_CENTIMETERS, ui->spinBoxLinearPrecP->value() - 2 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_FEET, ui->spinBoxLinearPrecP->value() - 2 );
                   }
                   else
                   {
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_KILOMETERS, ui->spinBoxLinearPrecP->value() + 3 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_CENTIMETERS, 1 );
                       qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_FEET, 1 );
                   }
               }
               else if(line.find("--angprecP")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->spinBoxAngularPrecPoints->setValue(atoi(parts[1].c_str()));
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_SOA, ui->spinBoxAngularPrecPoints->value() );

                   // Determine the values of the associated qsettings
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_DEGREES, ui->spinBoxAngularPrecPoints->value() + 4 );
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_RADIANS, ui->spinBoxAngularPrecPoints->value() + 5 );
               }
               else if(line.find("--angprecM")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->spinBoxAngularPrecMeas->setValue(atoi(parts[1].c_str()));
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA, ui->spinBoxAngularPrecMeas->value() );
                   // Determine the values of the associated qsettings
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_DEGREES, ui->spinBoxAngularPrecMeas->value() + 3 );
                   qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_RADIANS, ui->spinBoxAngularPrecMeas->value() + 5 );
               }
               else if(line.find("--warningExtRelVect")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->doubleSpinBoxWarnExtRelVect->setValue(atof(parts[1].c_str()));
                   qsettings.setValue( lsa::QSETTINGS_MAX_EXT_MAG_WARNING, ui->doubleSpinBoxWarnExtRelVect->value() );
               }
               else if(line.find("--errorExtRelVect")!=std::string::npos)
               {
                   parts = gnsstk::StringUtils::splitWithDoubleQuotes(line,' ');
                   ui->doubleSpinBoxErrorExtRelVect->setValue(atof(parts[1].c_str()));
                   qsettings.setValue( lsa::QSETTINGS_MAX_EXT_MAG_ERROR, ui->doubleSpinBoxErrorExtRelVect->value() );
               }
               else if(line.find("--calcExtRelVect")!=std::string::npos)
               {
                   ui->radioButtonYesExtRelVect->setChecked(true);
                   ui->radioButtonNoExtRelVect->setChecked(false);
               }
               else if(line.find("--noExtRelVect")!=std::string::npos)
               {
                   ui->radioButtonYesExtRelVect->setChecked(false);
                   ui->radioButtonNoExtRelVect->setChecked(true);
               }
               else if(line.find("--verbose")!=std::string::npos)
               {
                   ui->checkBoxVerbose->setChecked(true);
               }
               else if(line.find("--eqnout")!=std::string::npos)
               {
                   ui->checkBoxPartialsMatrixFile->setChecked(true);
               }
               else if(line.find("--forcefast")!=std::string::npos)
               {
                   ui->radioButtonForceFast->setChecked(true);
                   ui->radioButtonAllowFast->setChecked(false);
                   ui->radioButtonForceStable->setChecked(false);
               }
               else if(line.find("--forcestable")!=std::string::npos)
               {
                   ui->radioButtonForceStable->setChecked(true);
                   ui->radioButtonAllowFast->setChecked(false);
                   ui->radioButtonForceFast->setChecked(false);
               }
               if(line.find("--includeUnused")!=std::string::npos)
               {
                   ui->checkBoxIncludeUnusedPoints->setChecked(true);
               }

           }
           catch(...)
           {
               parseError = true;
               continue;
           }
       }
    }
    istrm.close();

    if(parseError)
    {
        QMessageBox::warning(this,
                             QString("Error parsing project configuration file"),
                             QString("Problem value(s) replaced with default(s).  Press OK on Project Configuration panel to save corrected file."));
    }
}

void ConfigDialog::onOKButtonPressed()
{
    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    try
    {// Set relevent qsettings
        // Reliability
        qsettings.setValue( lsa::QSETTINGS_MAX_EXT_MAG_ERROR, ui->doubleSpinBoxErrorExtRelVect->value() );
        qsettings.setValue( lsa::QSETTINGS_MAX_EXT_MAG_WARNING, ui->doubleSpinBoxWarnExtRelVect->value() );

        // Linear measurement precision
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_METERS, ui->spinBoxLinearPrecM->value() );
        if(ui->spinBoxLinearPrecM->value() >= 3)
        {
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_KILOMETERS, ui->spinBoxLinearPrecM->value() + 3 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_CENTIMETERS, ui->spinBoxLinearPrecM->value() - 2 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET, ui->spinBoxLinearPrecM->value() - 2 );
        }
        else
        {
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_KILOMETERS, ui->spinBoxLinearPrecM->value() + 3 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_CENTIMETERS, 1 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_MEASUREMENT_FEET, 1 );
        }

        // Linear position precsion
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_METERS, ui->spinBoxLinearPrecP->value() );
        if(ui->spinBoxLinearPrecP->value() >= 3)
        {
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_KILOMETERS, ui->spinBoxLinearPrecP->value() + 3 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_CENTIMETERS, ui->spinBoxLinearPrecP->value() - 2 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_FEET, ui->spinBoxLinearPrecP->value() - 2 );
        }
        else
        {
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_KILOMETERS, ui->spinBoxLinearPrecP->value() + 3 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_CENTIMETERS, 1 );
            qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_LINEAR_POSITION_FEET, 1 );
        }

        // Angle measurement precision
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_SOA, ui->spinBoxAngularPrecMeas->value() );
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_DEGREES, ui->spinBoxAngularPrecMeas->value() + 3 );
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_RADIANS, ui->spinBoxAngularPrecMeas->value() + 5 );

        // Angle position precision
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_SOA, ui->spinBoxAngularPrecPoints->value() );
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_DEGREES, ui->spinBoxAngularPrecPoints->value() + 4 );
        qsettings.setValue( lsa::QSETTINGS_NUM_DECIMALS_ANGLE_POSITION_RADIANS, ui->spinBoxAngularPrecPoints->value() + 5 );
    }
    catch(...)//use default if invalid value entered
    {
    }

    std::ifstream istrm;
    bool oldConfigExists = true;
    istrm.open(configFile.toStdString().c_str(),std::ios::in);
    if(!istrm.is_open())
        oldConfigExists = false;

    std::ofstream ostrm;
    ostrm.open((configFile.left(configFile.lastIndexOf(".")) + QString(".tmp")).toStdString().c_str(),std::ios::out);
    if(!ostrm.is_open())
    {
        istrm.close();
        return;
    }

    ostrm << "# " << LSAVERSION_FILE_STRING << std::endl;

    //write out new configuration
    if(ui->radioButtonYesCOMAP->isChecked())
        ostrm << "--allowCOM" << std::endl;
    ostrm << "--niter " << ui->spinBoxMaxIter->value() << std::endl;
    ostrm << "--conv " << ui->lineEditConvCriteria->text().toStdString() << std::endl;
    ostrm << "--alpha " << QString::number(1.0 - ui->doubleSpinBoxConfidenceLevel->value()).toStdString() << std::endl;
    if(ui->radioButtonUnscaled->isChecked())
    {
        ostrm << "--noAPV" << std::endl;
    }
    else
    {
        ostrm << "--APV" << std::endl;
    }

    if(ui->radioButtonForceFast->isChecked())
    {
        ostrm << "--forcefast" << std::endl;
    }
    else if(ui->radioButtonForceStable->isChecked())
    {
        ostrm << "--forcestable" << std::endl;
    }
    if(!ui->comboBoxGeoidFile->currentText().contains("NONE"))
    {
        ostrm << "--geoidfile " << ui->comboBoxGeoidFile->currentText().toStdString() << std::endl;
    }
    if(ui->radioButtonWest->isChecked())
    {
        ostrm << "--westLon" << std::endl;
    }
    if(ui->radioButton1Sigma->isChecked())
    {
        ostrm << "--confid 1sig" << std::endl;
    }
    else if(ui->radioButton90->isChecked())
    {
        ostrm << "--confid 90" << std::endl;
    }
    else
    {
        ostrm << "--confid 95" << std::endl;
    }
    ostrm << "--linprecP " << ui->spinBoxLinearPrecP->value() << std::endl;
    ostrm << "--linprecM " << ui->spinBoxLinearPrecM->value() << std::endl;
    ostrm << "--angprecP " << ui->spinBoxAngularPrecPoints->value() << std::endl;
    ostrm << "--angprecM " << ui->spinBoxAngularPrecMeas->value() << std::endl;
    ostrm << "--warningExtRelVect " << ui->doubleSpinBoxWarnExtRelVect->value() << std::endl;
    ostrm << "--errorExtRelVect " << ui->doubleSpinBoxErrorExtRelVect->value() << std::endl;
    if(ui->radioButtonYesExtRelVect->isChecked())
        ostrm << "--calcExtRelVect" << std::endl;
    if(ui->radioButtonNoExtRelVect->isChecked())
        ostrm << "--noExtRelVect" << std::endl;
    if(ui->checkBoxGeoidFileH5Overwrite->isChecked())
        ostrm << "--patchoverwrite" << std::endl;
    if(ui->checkBoxVerbose->isChecked())
        ostrm << "--verbose" << std::endl;
    if(ui->checkBoxPartialsMatrixFile->isChecked())
    {
        QString eqnFileName = configFile.left(configFile.lastIndexOf(".")) + QString(".eqn");
        ostrm << "--eqnout " << addQuotes(eqnFileName.toStdString()) << std::endl;
    }
    if(ui->checkBoxIncludeUnusedPoints->isChecked())
        ostrm << "--includeUnused" << std::endl;
    ostrm << std::endl;

    if(oldConfigExists)
    {
        std::string line = std::string("");
        std::vector<std::string> parts;
        while(1)
        {
           line = std::string("");
           getline(istrm,line);
           if((istrm.eof() || !istrm.good()) && line.empty()) break;
           if(line.empty())
               continue;

           //preserve comments, except previous version string
           if(line.at(0)=='#')
           {
               if((line.find("Created") != std::string::npos) || (line.find("Deployed") != std::string::npos))
                   continue;
               else
                   ostrm << line << std::endl;
           }
           //comment out options that conflict with the gui-provided options
           else if((line.find(std::string("--out"))!=std::string::npos) ||
                   (line.find(std::string("--csv"))!=std::string::npos) ||
                   (line.find(std::string("--bin"))!=std::string::npos) ||
                   (line.find(std::string("--file"))!=std::string::npos) ||
                   (line.find(std::string("--geoidpath"))!=std::string::npos)
                   )
           {
               ostrm << "#" << line << std::endl;
               //TODO: Push warning to status window
           }
           else if(line.at(0)=='-' && line.at(1)=='-')
           {
               //remove previous configuration
               if( (line.find(std::string("--apquit"))!=std::string::npos) ||
                   (line.find(std::string("--allowCOM"))!=std::string::npos) ||
                   (line.find(std::string("--niter"))!=std::string::npos) ||
                   (line.find(std::string("--conv"))!=std::string::npos) ||
                   (line.find(std::string("--alpha"))!=std::string::npos) ||
                   (line.find(std::string("--noAPV"))!=std::string::npos) ||
                   (line.find(std::string("--forcefast"))!=std::string::npos) ||
                   (line.find(std::string("--forcestable"))!=std::string::npos) ||
                   (line.find(std::string("--APV"))!=std::string::npos) ||
                   (line.find(std::string("--geoidfile"))!=std::string::npos) ||
                   (line.find(std::string("--patchoverwrite"))!=std::string::npos) ||
                   (line.find(std::string("--westLon"))!=std::string::npos) ||
                   (line.find(std::string("--confid"))!=std::string::npos) ||
                   (line.find(std::string("--linprecP"))!=std::string::npos) ||
                   (line.find(std::string("--linprecM"))!=std::string::npos) ||
                   (line.find(std::string("--angprecP"))!=std::string::npos) ||
                   (line.find(std::string("--angprecM"))!=std::string::npos) ||
                   (line.find(std::string("--warningExtRelVect"))!=std::string::npos) ||
                   (line.find(std::string("--errorExtRelVect"))!=std::string::npos) ||
                   (line.find(std::string("--calcExtRelVect"))!=std::string::npos) ||
                   (line.find(std::string("--noExtRelVect"))!=std::string::npos) ||
                   (line.find(std::string("--verbose"))!=std::string::npos) ||
                   (line.find(std::string("--includeUnused"))!=std::string::npos) ||
                   (line.find(std::string("--eqnout"))!=std::string::npos) )
               {
                   continue;
               }
               else//pass through --somethings
               {
                   ostrm << line << std::endl;
               }
           }
           else
           {
               //alert user of non-standard config options
               //TODO
           }
        }
        istrm.close();
    }

    ostrm.close();

    //replace previous config file
    remove(configFile.toStdString().c_str());
    rename((configFile.left(configFile.lastIndexOf(".")) + QString(".tmp")).toStdString().c_str(),configFile.toStdString().c_str());
}

void ConfigDialog::onConverterConfigButtonPressed()
{
    if(converterDialog != NULL)
    {
        delete converterDialog;
        converterDialog = NULL;
    }
    QString converterConfigFile;
    converterConfigFile = configFile.left(configFile.lastIndexOf(QDir::separator())+1) + QString("converter.cfg");

    converterDialog = new ConverterDialog(converterConfigFile);
    converterDialog->show();
}

QStringList ConfigDialog::findInstalledGeoidFiles()
{
    //populate geoid dropdown menu with .und and hdf geoid files from data directory
    QString geoidPath = QString::fromStdString(getExecutablePath()) + QString("/../data/geoid");
    QDir directoryData(geoidPath);
    directoryData.setNameFilters(QStringList()<<"*.und"<<"*.h5"<<"*.asc");//only look for geoid files
    QStringList geoidFiles = directoryData.entryList();

    //populate geoid dropdown menu with custom geoid hdf files from the project directory
    QString projPath = QString::fromStdString(getPathWithoutFileName(configFile.toStdString())+"/geoid/");
    QDir directoryProj(projPath);
    directoryProj.setNameFilters(QStringList()<<"*.h5"<<"*.und");
    QStringList projFiles = directoryProj.entryList();
    geoidFiles.append(projFiles);

    //allow user to run without a geoid file
    geoidFiles.append(QString("NONE"));

    return geoidFiles;
}
