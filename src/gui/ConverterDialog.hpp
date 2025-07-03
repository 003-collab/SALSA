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
#ifndef CONVERTERDIALOG_HPP
#define CONVERTERDIALOG_HPP

#include <QDialog>
#include <QAbstractButton>
#include "LSAGuiConstants.hpp"

namespace Ui
{
class ConverterDialog;
class StoredValues;
}

class StoredValues
{

};

struct panelSettings {
    int StaticGPSFRCent;
    int StaticGPSTOCent;
    int KinematicGPSFRCent;
    int KinematicGPSTOCent;
    int LevelsFRCent;
    int LevelsTOCent;
    int TSFRCent;
    int TSTOCent;
    int DiffLevelsMeasType;
    int DiffLevelsSigmaSource;
    int TSDistancesSigmaSource;
    int TSHorzAnglesSigmaSource;
    int TSVertAnglesSigmaSource;
    int TSElevDiffsSigmaSource;
    int TSInstrHghtSigmaSource;
    double GPSStaticPPM;
    double GPSKinematicPPM;
    double DiffLevelsPPM;
    double DistanceIRPPM;
    double DistanceRedPPM;
    double DistanceNonPPM;
    double CustomStaticGPSFR;
    double CustomStaticGPSTO;
    double CustomKinematicGPSFR;
    double CustomKinematicGPSTO;
    double CustomLevelsFR;
    double CustomLevelsTO;
    double CustomLevelsSigma;
    double CustomTSDistFR;
    double CustomTSDistTO;
    double CustomTSDistIRSigma;
    double CustomTSDistRedSigma;
    double CustomTSDistNonSigma;
    double CustomTSHorzFR;
    double CustomTSHorzTO;
    double CustomTSHorzSigma;
    double CustomTSVertFR;
    double CustomTSVertTO;
    double CustomTSVertSigma;
    double CustomTSElevDiffsFR;
    double CustomTSElevDiffsTO;
    double CustomTSElevDiffsSigma;
    double CustomTSInstrHghtSigma;
};

struct panelValues {
    double StaticGPSFR;
    double StaticGPSTO;
    double KinematicGPSFR;
    double KinematicGPSTO;
    double LevelsFR;
    double LevelsTO;
    double GPSStaticPPM;
    double GPSKinematicPPM;
    double DiffLevelsPPM;
    double DistanceIRPPM;
    double DistanceRedPPM;
    double DistanceNonPPM;
    double LevelsSigma;
    double TSDistFR;
    double TSDistTO;
    double TSDistIRSigma;
    double TSDistRedSigma;
    double TSDistNonSigma;
    double TSHorzFR;
    double TSHorzTO;
    double TSHorzSigma;
    double TSVertFR;
    double TSVertTO;
    double TSVertSigma;
    double TSElevDiffsFR;
    double TSElevDiffsTO;
    double TSElevDiffsSigma;
    double TSInstrHghtSigma;
};


class ConverterDialog : public QDialog
{
    Q_OBJECT
    //combo box selections (indices)
    static const int TRIPOD;
    static const int VERT_COLLIM;
    static const int FORCED_CENTER;
    static const int RANGE_POLE;
    static const int SETUP_CUSTOM;
    static const int DIGITAL_PRECISE;
    static const int DIGITAL_STANDARD;
    static const int SPIRIT_STANDARD;
    static const int SOURCE_DEFAULT;
    static const int SOURCE_INST;
    static const int SOURCE_CUSTOM;
    static const int INST;
    static const int INSTR_HGHT_MEASURE_DEFAULT;
    static const int INSTR_HGHT_MEASURE_ROD;
    static const int INSTR_HGHT_MEASURE_TAPE;
    static const int INSTR_HGHT_MEASURE_CUSTOM;

    //UNCR labels
    static const QString LABEL_GPS_STATIC;
    static const QString LABEL_GPS_KINEMATIC;
    static const QString LABEL_DIFF_LEVELS;
    static const QString LABEL_TOTSTA_DIST_IR;
    static const QString LABEL_TOTSTA_DIST_RED;
    static const QString LABEL_TOTSTA_DIST_NON;
    static const QString LABEL_TOTSTA_HORZ_DIR;
    static const QString LABEL_TOTSTA_HORZ_ANG;
    static const QString LABEL_TOTSTA_VERT;
    static const QString LABEL_TOTSTA_DE;
    static const QString LABEL_TOTSTA_INSTR_HGHT;

    friend class TestLSAGui; ///< Test harness class used to execute gui unit and integration tests

public:
    explicit ConverterDialog(QString converterConfigFile, QWidget *parent = 0);
    ~ConverterDialog();
    Ui::ConverterDialog* getUi() { return ui; }
    void createConverterConfigFile();
    bool readConverterConfigSettingsFile();
    void writeConverterConfigSettingsFile();

private:
    Ui::ConverterDialog *ui;
    QString configFile;

    void setupControls();
    panelSettings getPanelSettings();
    panelValues getPanelValues();

private slots:
    void onSaveAsUserDefaultsButtonPressed();
    void onCancelButtonPressed();
    void onSaveAsProjectConfigButtonPressed();
    void onRestoreFactoryDefaultsButtonPressed();
    void onStaticGPSFRChanged(int index);
    void onKinGPSFRSetupChanged(int index);
    void onDiffLevelsFRSetupChanged(int index);
    void onTSFRSetupChanged(int index);
    void onStaticGPSTOSetupChanged(int index);
    void onKinGPSTOSetupChanged(int index);
    void onDiffLevelsTOSetupChanged(int index);
    void onTSTOSetupChanged(int index);
    void onDiffLevelsMeasTypeChanged(int index);
    void onDiffLevelsSigmaSourceChanged(int index);
    void onTSDistancesSigmaSourceChanged(int index);
    void onTSHorzAnglesSigmaSourceChanged(int index);
    void onTSVertAnglesSigmaSourceChanged(int index);
    void onTSElevDiffsSigmaSourceChanged(int index);
    void onTSInstrHghtSigmaSourceChanged(int index);

};

#endif // CONVERTERDIALOG_HPP
