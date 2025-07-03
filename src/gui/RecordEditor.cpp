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
#include "RecordEditor.hpp"
#include "ui_RecordEditor.h"
#include "LSAGuiConstants.hpp"
#include "lsaUtils.hpp"//getPrecisionForUnits
#include "LabelChangeDialog.hpp"

// Event filter method to ignore QEvent::Wheel events
bool MouseWheelEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
    {
        return true;
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}

/***********************************************
 *
 *  Constructor and Destructor
 *
 ***********************************************/

RecordEditor::RecordEditor(GuiModel *guiModel, QWidget *parent) :
    QDockWidget(parent), ui(new Ui::RecordEditor), wheelFilter(new MouseWheelEventFilter),
    currentLSARecord(NULL), currentModifier(NULL), latDMSIsNeg(false), lonDMSIsNeg(false), dmsAngleIsNeg(false)
{
    suppressGuiUpdates(true);
    {
        ui->setupUi(this);

        setFocusPolicy(Qt::StrongFocus); // required for RecordEditor::focusOutEvent to function

        setStyleSheet("QDockWidget > QWidget { background: #e0e0e0;}"
                      "QDockWidget::title    { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 lightgray, stop: 1 #e0e0e0);} ");

        currentLSARecord = NULL;

        setupControls();

        // Reinitialize all control states
        resetControlStates();

        // Hide all the controls.
        hideControls();

        setFocusProxy(ui->checkEnabled);

        setGuiModel(guiModel);
    }
    suppressGuiUpdates(false);

}

RecordEditor::~RecordEditor()
{
    delete ui;
}

/***********************************************
 *
 *  Control Slots
 *
 ***********************************************/

void RecordEditor::radioUseDMSChanged(bool checked)
{
    useDMS_Changed = true;
    if (checked) // only update if we are converting to DMS, otherwise let DecDeg handle the update
    {
        useDMS = true;
        // Note: azimuth uses latitude control in order to show N/S
        if (currentLSAType == LSAType::POSC || currentLSAType == LSAType::POSG || currentLSAType == LSAType::AZIM)
        {
            convertLatLonDecDegToDMS();
        }
        else
        {
            convertAngleDecDegToDMS();
        }
    }
    else
    {
        useDMS = false;
        if (currentLSAType == LSAType::POSC || currentLSAType == LSAType::POSG || currentLSAType == LSAType::AZIM)
        {
            convertLatLonDMSToDecDeg();
        }
        else
        {
            convertAngleDMSToDecDeg();
        }
    }

    //Fix to Bug #659.  Seemed to be a race condition between this method and the corresponding pushChanges method.
    pushChangesToGuiModel();

    return;
}

void RecordEditor::checkRefractiveIndexChanged(bool isChecked)
{
    if (!suppressRecordEditorUpdates)
    {
        checkApplyRefraction_Changed = true;
        ui->doubleRefractiveIndex->setEnabled(isChecked);
        if(currentModifier!=NULL) ui->doubleRefractiveIndex->setValue(currentModifier->getRawRefract());
    }

}

void RecordEditor::handleChangeHeightState(QString newComboVal)
{
    bool valueMixed = (newComboVal != lsa::Q_OPTION_ELLIPSOID && newComboVal != lsa::Q_OPTION_ORTHOMETRIC);

    if(suppressRecordEditorUpdates || valueMixed)
    {
       return;
    }

    comboPosgHeight_Changed = true;

    // At this point mixed combo box text should be removed to prevent it from being implemented
    // for future choices.
    int mixedStatusIndex = ui->comboPOSGHeight->findText(lsa::Q_CONTROLVALUE_MIXED);
    if(mixedStatusIndex != -1)
    {
        ui->comboPOSGHeight->removeItem(mixedStatusIndex);
    }

    pushChangesToGuiModel();
}

void RecordEditor::handleBtnAngleNegDMSClicked()
{
    if (!suppressRecordEditorUpdates)
    {
        btnAngleNegDMS_Changed = true ;
    }

    dmsAngleIsNeg = !dmsAngleIsNeg;
    QString buttonText = QString::fromStdString(dmsAngleIsNeg ? "-" : "+");
    ui->btnAngleNegDMS->setText(buttonText);

    pushChangesToGuiModel();

    return;
}

void RecordEditor::handleBtnLatNegDMSClicked()
{
    if (!suppressRecordEditorUpdates)
    {
        btnLatDMSNeg_Changed = true ;
    }

    latDMSIsNeg = !latDMSIsNeg;
    QString buttonText = QString::fromStdString(latDMSIsNeg ? "-" : "+");
    ui->btnLatDMSNeg->setText(buttonText);

    pushChangesToGuiModel();

    return;
}

void RecordEditor::handleBtnLonNegDMSClicked()
{
    if (!suppressRecordEditorUpdates)
    {
        btnLonDMSNeg_Changed = true ;
    }

    lonDMSIsNeg = !lonDMSIsNeg;
    QString buttonText = QString::fromStdString(lonDMSIsNeg ? "-" : "+");
    ui->btnLonDMSNeg->setText(buttonText);

    pushChangesToGuiModel();

    return;
}

void RecordEditor::handleBtnBrowseClicked()
{
    // Let the user select a new or existing .lsa file
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    QString lastLSAPath = settings.value(lsa::QSETTINGS_LASTPATH, "~" ).toString();
    QString selfilter("LSA Files (*.lsa)");

    QFileDialog dialog(this, QString("Select a .lsa file"), lastLSAPath, selfilter);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setModal(true);
    QStringList filenames;
    if (dialog.exec())
    {
        filenames = dialog.selectedFiles();
    }

    if(filenames.size()>0)
    {
        QString fileName = filenames.at(filenames.size()-1);
        QString currentDir = QString::fromStdString(getPathWithoutFileName(fileName.toStdString()));
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
        settings.setValue(lsa::QSETTINGS_LASTPATH, currentDir);

        emit replaceInclude(currentIndex, filenames);
    }
}

void RecordEditor::modifyLabels(LSAType lsaType, QString oldLabel, QString newLabel)
{
    // If the label isn't actually changing, don't do anything
    if (oldLabel == newLabel) return;
    if (!lsaType.isPosition() && !lsaType.isModifier() && lsaType != LSAType::DGRP) return;

    // Tell the user how many records reference this label and ask if they want to update references
    LabelChangeDialog dialog(this, guiModel, lsaType, oldLabel.toStdString(), LSAOperationType::Modify);
    QMessageBox::StandardButton reply;
    bool oldValue = suppressGuiUpdates(true); // hack to prevent infinite loop pushing QMessageBoxes
    {
        reply = (QMessageBox::StandardButton)dialog.exec();
    }
    suppressGuiUpdates(oldValue);

    if(reply == QMessageBox::Yes)
    {
        if (lsaType.isPosition())
        {
            guiModel->renamePositionLabel(oldLabel.toStdString(), newLabel.toStdString());
        }
        else
        {
            guiModel->renameModifierLabel(lsaType, oldLabel.toStdString(), newLabel.toStdString());
        }
    }
}

void RecordEditor::handleHeightToChanged()
{

    comboHeightTo_Changed = true;
    doubleHeightToValue_Changed = true;
    comboHeightToUnits_Changed = true;
    updateHeightControls(ui->doubleHeightToValue, ui->comboHeightToUnits, ui->comboHeightTo->currentText() );

    return;
}

void RecordEditor::handleHeightFromChanged()
{
    comboHeightFrom_Changed = true;
    doubleHeightFromValue_Changed = true;
    comboHeightFromUnits_Changed = true;

    updateHeightControls(ui->doubleHeightFromValue, ui->comboHeightFromUnits, ui->comboHeightFrom->currentText());

    return;
}

void RecordEditor::handleComboVSCAChanged()
{
    comboVSCA_Changed = true;
    pushChangesToGuiModel(); // fix to bug 1066, need to refresh the entire gui here to ensure multi-select is handled correctly
    update();
}

void RecordEditor::handleGuiModelDataChanged(QModelIndex first, QModelIndex last, QVector<int> roles)
{
    // If the user clicked on a check box in the tree view, make sure the record editor knows about it.
    // Specfically, make sure the project gets flagged as needing a save before calculating adjustment.
    if (roles.contains(Qt::CheckStateRole)) checkEnabled_Changed = true;
}

void RecordEditor::updateHeightControls(QDoubleSpinBox* doubleValue, QComboBox *comboUnits, const QString &arg1)
{
    if (lsa::Q_CONTROLVALUE_NONE == arg1)
    {
        doubleValue->setValue(0.0);
        doubleValue->setEnabled(false);
        comboUnits->setEnabled(false);
    }
    else if (lsa::Q_CONTROLVALUE_VALUE == arg1)
    {
        doubleValue->setEnabled(true);
        comboUnits->setEnabled(true);
    }
    else
    {
        double newValue;
        std::string newUnits;
        double newSigma;
        std::string newSigmaUnits;
        guiModel->getHeightDataByLabel(arg1.toStdString(), newValue, newUnits, newSigma, newSigmaUnits);

        doubleValue->setValue(newValue);
        comboUnits->setCurrentText(QString::fromStdString(newUnits));
        doubleValue->setEnabled(false);
        comboUnits->setEnabled(false);
    }

    return;
}

void RecordEditor::updateDYXZ_CovENU(double e, double n, double u)
{
    ui->labelSigE->setText(" σE: ");
    ui->labelSigN->setText(" σN: ");
    ui->labelSigU->setText(" σU: ");

    LSADelta *lsaDelta = static_cast<LSADelta *>(currentLSARecord);
    QString currentUnits = QString::fromStdString(lsaDelta->linUnits).toLower();
    if (currentUnits == lsa::Q_UNITS_M)
    {
        ui->sigE->setText(QString::number(e * 1000, 'f', lsa::NUM_DECIMALS_COV_SIGMAS) + " mm");
        ui->sigN->setText(QString::number(n * 1000, 'f', lsa::NUM_DECIMALS_COV_SIGMAS) + " mm");
        ui->sigU->setText(QString::number(u * 1000, 'f', lsa::NUM_DECIMALS_COV_SIGMAS) + " mm");
    }
    else
    {
        ui->sigE->setText(QString::number(e, 'e', lsa::NUM_DECIMALS_COV_SIGMAS));
        ui->sigN->setText(QString::number(n, 'e', lsa::NUM_DECIMALS_COV_SIGMAS));
        ui->sigU->setText(QString::number(u, 'e', lsa::NUM_DECIMALS_COV_SIGMAS));
    }
}

/***********************************************
 *
 *  Private Methods
 *
 ***********************************************/


void RecordEditor::setGuiModel(GuiModel* newModel)
{
    guiModel      = newModel;

    pointMap      = guiModel->getPoints();
    heightMap     = guiModel->getHeightMap();
    uncrMap      = guiModel->getUncrMap();
    varMap        = guiModel->getVarScalingMap();
    dirGroupMap   = guiModel->getDirGroupMap();

    connect(guiModel, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)), this, SLOT(handleGuiModelDataChanged(QModelIndex, QModelIndex, QVector<int>)));

    hideControls();
    resetControlStates();

    currentIndex = QModelIndex();//clear cached value from previous edits (Bug 1182)
    return;
}

void RecordEditor::convertLatLonDMSToDecDeg()
{
    showLatLonControls();

    bool oldState = suppressGuiUpdates(true);
    {
        // Convert latDir
        QString latDir = ui->comboLatDMSNS->currentText();
        ui->comboLatDecDegNS->setCurrentText(latDir);

        // Convert lonDir
        QString lonDir = ui->comboLonDMSEW->currentText();
        ui->comboLonDecDegEW->setCurrentText(lonDir);
    }
    suppressGuiUpdates(oldState);

    // Convert lat and lon
    convertDMSControlToDecimalDegrees(latDMSIsNeg, ui->intLatDMSDeg, ui->intLatDMSMin, ui->doubleLatDMSSec, ui->doubleLatDecDeg);
    convertDMSControlToDecimalDegrees(lonDMSIsNeg, ui->intLonDMSDeg, ui->intLonDMSMin, ui->doubleLonDMSSec, ui->doubleLonDecDeg);

    // set all the changed flags so new values will be saved
    doubleLatDecDeg_Changed = true;
    doubleLonDecDeg_Changed = true;
    useDMS_Changed = true;

    return;
}

void RecordEditor::convertLatLonDecDegToDMS()
{
    showLatLonControls();

    bool oldState = suppressGuiUpdates(true);
    {
        // Convert latDir
        QString latDir = ui->comboLatDecDegNS->currentText();
        ui->comboLatDMSNS->setCurrentText(latDir);

        // Convert lonDir
        QString lonDir = ui->comboLonDecDegEW->currentText();
        ui->comboLonDMSEW->setCurrentText(lonDir);
    }
    suppressGuiUpdates(oldState);

    // Convert lat and lon
    convertDecimalDegreeControlToDMS( ui->doubleLatDecDeg, latDMSIsNeg, ui->btnLatDMSNeg, ui->intLatDMSDeg, ui->intLatDMSMin, ui->doubleLatDMSSec);
    convertDecimalDegreeControlToDMS( ui->doubleLonDecDeg, lonDMSIsNeg, ui->btnLonDMSNeg, ui->intLonDMSDeg, ui->intLonDMSMin, ui->doubleLonDMSSec);

    // set all the changed flags so new values will be saved
    btnLatDMSNeg_Changed = true;
    intLatDMSDeg_Changed = true;
    intLatDMSMin_Changed = true;
    doubleLatDMSSec_Changed = true;

    btnLonDMSNeg_Changed = true;
    intLonDMSDeg_Changed = true;
    intLonDMSMin_Changed = true;
    doubleLonDMSSec_Changed = true;

    useDMS_Changed = true;

    return;
}

void RecordEditor::convertAngleDMSToDecDeg()
{
    ui->labelAngleDMS->hide();
    ui->frameAngleDMS->hide();
    ui->labelAngleDecDeg->show();
    ui->doubleAngleDecDeg->show();

    convertDMSControlToDecimalDegrees(dmsAngleIsNeg, ui->intAngleDMSDeg, ui->intAngleDMSMin, ui->doubleAngleDMSSec, ui->doubleAngleDecDeg);

    doubleAngleDecDeg_Changed = true;

    return;
}

void RecordEditor::convertAngleDecDegToDMS()
{
    ui->labelAngleDecDeg->hide();
    ui->doubleAngleDecDeg->hide();
    ui->labelAngleDMS->show();
    ui->frameAngleDMS->show();

    convertDecimalDegreeControlToDMS(ui->doubleAngleDecDeg, dmsAngleIsNeg, ui->btnAngleNegDMS, ui->intAngleDMSDeg, ui->intAngleDMSMin, ui->doubleAngleDMSSec);

    btnAngleNegDMS_Changed = true;
    intAngleDMSDeg_Changed = true;
    intAngleDMSMin_Changed = true;
    doubleAngleDMSSec_Changed = true;

    return;
}

void RecordEditor::convertDMSControlToDecimalDegrees(bool DMSIsNeg, QSpinBox *dmsDegControl, QSpinBox *dmsMinControl, QDoubleSpinBox *dmsSecControl, QDoubleSpinBox *decDegControl)
{
    // Get the DMS values from the DMS controls
    int dmsDegrees = dmsDegControl->value();
    int dmsMinutes = dmsMinControl->value();
    double dmsSeconds = dmsSecControl->value();

    // Convert to decimal degrees
    double decimalDegrees = DMSToDeg(DMSIsNeg, dmsDegrees, dmsMinutes, dmsSeconds);

    // Set the value on the decimal degree control
    decDegControl->setValue(decimalDegrees);

    return;
}
void RecordEditor::convertDecimalDegreeControlToDMS(QDoubleSpinBox *decDegControl, bool &dmsIsNeg, QPushButton *dmsNegControl, QSpinBox *dmsDegControl, QSpinBox *dmsMinControl, QDoubleSpinBox *dmsSecControl)
{
    // Get the decimal degree value
    double decimalDegrees = decDegControl->value();

    // Convert to DMS and update dmsIsNeg boolean
    int dmsDegrees = 0;
    int dmsMinutes = 0;
    double dmsSeconds = 0.0;
    degToDMS(decimalDegrees, dmsIsNeg, dmsDegrees, dmsMinutes, dmsSeconds);

    // Set the values on the DMS controls
    QString dmsSign = "+";
    if(decimalDegrees < 0)
        dmsSign = "-";
    dmsNegControl->setText(dmsSign);
    dmsDegControl->setValue(dmsDegrees);
    dmsMinControl->setValue(dmsMinutes);
    dmsSecControl->setValue(dmsSeconds);

    return;
}

void RecordEditor::comboValueUnitsChanged(const QString &arg1)
{
    if (suppressRecordEditorUpdates) return;

    comboValueAUnits_Changed = true;
    comboValueBUnits_Changed = true;
    comboValueCUnits_Changed = true;

    // Make all units match
    bool oldValue = suppressGuiUpdates(true);
    {
        ui->comboValueAUnits->setCurrentText(arg1);
        ui->comboValueBUnits->setCurrentText(arg1);
        ui->comboValueCUnits->setCurrentText(arg1);
    }
    suppressGuiUpdates(oldValue);

    pushChangesToGuiModel();

    return;
}

void RecordEditor::comboLatDMSNSChanged()
{
    if (suppressRecordEditorUpdates) return;

    comboLatDMSNS_Changed = true;
    pushChangesToGuiModel();

    return;
}

void RecordEditor::comboLatDecDegNSChanged()
{
    if (suppressRecordEditorUpdates) return;

    comboLatDecDegNS_Changed = true;

    doubleLatDecDeg_Changed = true;

    pushChangesToGuiModel();

    return;
}

void RecordEditor::comboLonDMSEWChanged()
{
    if (suppressRecordEditorUpdates) return;

    comboLonDMSEW_Changed = true;
    pushChangesToGuiModel();
}

void RecordEditor::comboLonDecDegEWChanged()
{
    if (suppressRecordEditorUpdates) return;

    comboLonDecDegEW_Changed = true;
    doubleLonDecDeg_Changed = true;

    pushChangesToGuiModel();

    return;

}


void RecordEditor::comboFixedFloatChanged(QString comboContents)
{
    comboFixedFloat_Changed = true;
    pushChangesToGuiModel();
}

void RecordEditor::checkNorthFixedChanged(int)
{
    if (!suppressRecordEditorUpdates)
    {
        checkNorthFixed_Changed = true;
        pushChangesToGuiModel();
    }
    return;
}

void RecordEditor::checkEastFixedChanged(int)
{
    if (!suppressRecordEditorUpdates)
    {
        checkEastFixed_Changed = true;
        pushChangesToGuiModel();
    }
    return;
}

void RecordEditor::checkUpFixedChanged(int)
{
    if (!suppressRecordEditorUpdates)
    {
        checkUpFixed_Changed = true;
        pushChangesToGuiModel();
    }
    return;
}

void RecordEditor::configureNumDigits(QDoubleSpinBox *control, LSAType lsaType, QString units)
{
    control->setDecimals(grabNumDigits(lsaType, units));

    return;
}

void RecordEditor::setupHeightControlLabels(QComboBox *selectorControl)
{
    selectorControl->show();
    selectorControl->clear();

    // Set all the possible height labels on the combo box
    selectorControl->addItem(lsa::Q_CONTROLVALUE_NONE);
    for(LSAHGHTMap::iterator iter = heightMap->begin(); iter != heightMap->end(); iter++)
    {
        QString label = QString::fromStdString(iter->first);
        selectorControl->addItem(label);
    }
    selectorControl->addItem(lsa::Q_CONTROLVALUE_VALUE); // used to enter HeightFrom or HeightTo manually

    return;
}

bool RecordEditor::setupMultiHeightControlLabels(QComboBox *selectorControl,std::string Label)
{
    selectorControl->show();
    selectorControl->clear();

    // Set all the possible height labels on the combo box
    bool supported = true; //toggles if hgt label not a record
    bool currentFound = false;
    QString currentLabelQ = QString::fromStdString(Label);
    selectorControl->addItem(lsa::Q_CONTROLVALUE_NONE);
    for(LSAHGHTMap::iterator iter = heightMap->begin(); iter != heightMap->end(); iter++)
    {
        QString label = QString::fromStdString(iter->first);
        selectorControl->addItem(label);
        if (Label==iter->first) currentFound=true;
    }
    if(!Label.empty() && !currentFound && (!(currentLabelQ==lsa::Q_CONTROLVALUE_NONE)) && (!(currentLabelQ==lsa::Q_CONTROLVALUE_VALUE))){
        if(!(currentLabelQ==lsa::Q_CONTROLVALUE_MIXED)){
            supported=false;
            selectorControl->addItem(QIcon(":/guiIcons/alertIcon.png"),currentLabelQ);
            selectorControl->setToolTip("Invalid height.");
        }
        else selectorControl->addItem(currentLabelQ);
    }
    selectorControl->addItem(lsa::Q_CONTROLVALUE_VALUE); // used to enter HeightFrom or HeightTo manually
    return supported;
}

void RecordEditor::setupUncrControlLabels(std::string currentLabel)
{
    QComboBox *selectorControl = ui->comboUncrModifier;

    selectorControl->show();
    selectorControl->clear();

    // Set all the possible uncr labels on the combo box
    selectorControl->addItem(lsa::Q_CONTROLVALUE_NONE);

    bool currentLabelFound = false;
    for(LSAUNCRMap::iterator iter = uncrMap->begin(); iter != uncrMap->end(); iter++)
    {
        QString label = QString::fromStdString(iter->first);
        selectorControl->addItem(label);
        currentLabelFound = currentLabelFound || (label.toStdString() == currentLabel);
    }

    if ( !currentLabelFound && !currentLabel.empty() )
    {
        selectorControl->addItem(QIcon(":/guiIcons/alertIcon.png"),QString::fromStdString(currentLabel));
        selectorControl->setToolTip("Invalid uncertainty.");
    }
    return;
}

void RecordEditor::setupVSCAControlLabels(std::string currentLabel)
{
    QComboBox *selectorControl = ui->comboVSCA;

    ui->frameVSCA->show();
    selectorControl->clear();

    // Set all the possible variance scaling labels on the combo box
    selectorControl->addItem(lsa::Q_CONTROLVALUE_NONE);

    bool currentLabelFound = false;
    if (currentIndex == guiModel->index(0,0))
    {
        LSARecord *lsaRecord= guiModel->getLSARecord(currentIndex);
        LSAInclude *rootInclude = static_cast<LSAInclude*>(lsaRecord);
        for (std::list<LSARecord*>::iterator iter = rootInclude->childRecords.begin(); iter != rootInclude->childRecords.end(); iter++)
        {
            LSARecord *lsaRecord = *iter;
            if ( lsaRecord->getRecType() == LSAType::VSCA )
            {
                LSAVarScaling *lsaVSCA = static_cast<LSAVarScaling*>(lsaRecord);
                std::string newVSCAlabel = lsaVSCA->getLabel();
                if ( !newVSCAlabel.empty() )
                {
                    selectorControl->addItem( QString::fromStdString(newVSCAlabel) );
                    currentLabelFound = currentLabelFound || (newVSCAlabel == currentLabel);
                }
            }
        }
    }
    else
    {
        selectorControl->setUpdatesEnabled(false);
        for(LSAVSCAMap::iterator iter = varMap->begin(); iter != varMap->end(); iter++)
        {
            selectorControl->addItem(QString::fromStdString(iter->first));
            currentLabelFound = currentLabelFound || (iter->first == currentLabel);
        }
        selectorControl->addItem(lsa::Q_CONTROLVALUE_VALUE); // used to enter VSCA factor manually
        selectorControl->setUpdatesEnabled(true);
    }

    if ( !currentLabelFound && !currentLabel.empty() )
    {
        selectorControl->addItem(QIcon(":/guiIcons/alertIcon.png"),QString::fromStdString(currentLabel));
        selectorControl->setToolTip("Invalid VSCA.");
    }

    return;
}

void RecordEditor::setupDirGroupControlLabels(std::string dirGroupLabel)
{
    QComboBox *selectorControl = ui->comboDirGroup;
    selectorControl->show();
    selectorControl->clear();

    // Set all the possible point labels on the combo box

    QString qDirGroupLabel = QString::fromStdString(dirGroupLabel);
    bool currentLabelFound = false;
    for(LSADirGroupMap::iterator iter = dirGroupMap->begin(); iter != dirGroupMap->end(); iter++)
    {
        QString label = QString::fromStdString(iter->first);
        selectorControl->addItem(label);
        currentLabelFound = currentLabelFound || (label == qDirGroupLabel);
    }

    if ( !currentLabelFound && !dirGroupLabel.empty() )
    {
        selectorControl->addItem(qDirGroupLabel);
    }

    return;
}

/***********************************************
 *
 *  Configure LSARecord Methods
 *
 ***********************************************/

void RecordEditor::update()
{
    selectedRows = guiModel->getSelectedRows();
    int numSelected = selectedRows.size();

    if (numSelected == 1)
    {
        configureForSingleRecord();
    }
    else if (numSelected > 1)
    {
        configureForMultipleRecords();
    }
    else
    {
        resetControlStates();
        hideControls();
        setWindowTitle(baseWindowName);
    }

    return;
}

bool RecordEditor::suppressGuiUpdates(bool value)
{
    bool oldState = suppressRecordEditorUpdates;

    suppressRecordEditorUpdates = value;
    emit signalSuppressGuiUpdates(value);

    return oldState;
}

void RecordEditor::configureForSingleRecord()
{
    QModelIndexList selectedIndices = guiModel->getSelectedRows();
    QModelIndex newIndex = selectedIndices.at(0);
    Q_ASSERT(newIndex.isValid());
    if (!newIndex.isValid())
    {
        return;
    }

    LSARecord* newRecord = guiModel->getLSARecord(newIndex);
    Q_ASSERT_X(newRecord != NULL,"RecordEditor::update()", "NULL newRecord");
    if (newRecord == NULL)
    {
        return;
    }

    bool oldState = suppressGuiUpdates(true);
    {
        if (!newIndex.isValid())
        {
            return;
        }
        currentIndex = newIndex;
        currentLSARecord = newRecord;
        currentLSAType = currentLSARecord->getRecType();

        // Set the window title
        QString recordString = QString::fromStdString(currentLSARecord->asTypeString());
        setWindowTitle(" " + recordString + baseWindowName);

        // Reinitialize all control states
        resetControlStates();

        // Hide all the controls.  The Configure method will show needed controls.
        hideControls();

        // populate checkEnabled
        ui->labelEnabled->show();
        ui->checkEnabled->show();
        ui->checkEnabled->setChecked(!currentLSARecord->isCommented);

        // Configure the widget for the LSARecord type
        if      (currentLSAType == LSAType::POSC) configurePOSC();
        else if (currentLSAType == LSAType::POSG) configurePOSG();
        else if (currentLSAType == LSAType::DXYZ) configureDXYZ();
        else if (currentLSAType == LSAType::DIST) configureDIST();
        else if (currentLSAType == LSAType::ZANG) configureZANG();
        else if (currentLSAType == LSAType::VANG) configureVANG();
        else if (currentLSAType == LSAType::HANG) configureHANG();
        else if (currentLSAType == LSAType::AZIM) configureAZIM();
        else if (currentLSAType == LSAType::HDIF) configureHDIF();
        else if (currentLSAType == LSAType::DGRP) configureDGRP();
        else if (currentLSAType == LSAType::HDIR) configureHDIR();
        else if (currentLSAType == LSAType::UNCR) configureUNCR();
        else if (currentLSAType == LSAType::VSCA) configureVSCA();
        else if (currentLSAType == LSAType::HGHT) configureHGHT();
        else if (currentLSAType == LSAType::INCLUDE) configureINCLUDE();
        else if (currentLSAType == LSAType::COMMENT) configureCOMMENT();
        else if (currentLSAType == LSAType::CONFIG)  configureCONFIG();
        else if (currentLSAType == LSAType::MEAN) configureMEAN();
        else if (currentLSAType == LSAType::ENUO) configureENUO();

        // i30 - Add a general notes text box to all records (except comments, autogen points, and the project record)
        if (currentLSAType != LSAType::COMMENT && !currentLSARecord->isAutogenerated && newIndex != guiModel->index(0,0))
        {
            ui->textNotes->appendPlainText(currentLSARecord->textNotes);
            ui->textNotes->show();
            ui->labelTextNotes->show();
            ui->textNotes->moveCursor(QTextCursor::Start);
        }

        resetChangeStateTrackers();
    }
    suppressGuiUpdates(oldState);

    return;
}

void RecordEditor::configureForMultipleRecords()
{
    currentLSARecord = NULL;
    currentModifier = NULL;

    bool oldState = suppressGuiUpdates(true);
    {
        // Set the window title
        QString title = " Editing " + QString::number( selectedRows.size() ) + " Records";
        this->setWindowTitle(title);

        // Reinitialize all control states
        resetControlStates();

        // Hide all the controls.  Needed controls are re-enabled below.
        hideControls();

        configureEnableCheckboxesForMultipleRecords();
        configureModifierForMultipleRecords();

        configureConstraintControls();
        configureHeightStateControl();
        configureVSCAForMultipleRecords();
        configureDGRPForMultipleRecords();
        configureUNCRForMultipleRecords();

        resetChangeStateTrackers();

    }
    suppressGuiUpdates(oldState);

    return;
}

void RecordEditor::configureEnableCheckboxesForMultipleRecords()
{
    // Local variables used to determine if enable checkboxes are checked, unchecked or mixed
    bool someEnableChecked = false;
    bool someEnableUnchecked = false;

    // Determine if none, some, or all checkboxes are checked
    QModelIndexList selectedRows = guiModel->getSelectedRows();
    bool atLeastOneRecordConfigured = false;
    foreach(QModelIndex index, selectedRows)
    {
        if (!index.isValid()) continue;

        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        if (lsaRecord->getRecType() == LSAType::COMMENT) continue;

        // enable checkboxes
        someEnableChecked   = someEnableChecked   || !lsaRecord->isCommented;
        someEnableUnchecked = someEnableUnchecked ||  lsaRecord->isCommented;
        atLeastOneRecordConfigured = true;
    }

    if (!atLeastOneRecordConfigured) return;

    // populate checkEnabled
    ui->labelEnabled->show();
    ui->checkEnabled->show();
    Qt::CheckState enabledCheckState;
    if      (someEnableChecked &&  someEnableUnchecked) enabledCheckState = Qt::PartiallyChecked;
    else if (someEnableChecked && !someEnableUnchecked) enabledCheckState = Qt::Checked;
    else                                                enabledCheckState = Qt::Unchecked;
    ui->checkEnabled->setCheckState(enabledCheckState);

    return;
}

void RecordEditor::configureModifierForMultipleRecords(){
    //ModifierKeywords:
    // 0: MODKEY_HEIGHTFROM
    // 1: MODKEY_HEIGHTTO
    // 3: MODKEY_REFRACT
    // 4: MODKEY_REDUCED
    // 6: MODKEY_CURV
    // 7: MODKEY_OHC

    int keys[6] = {0,1,3,4,6,7};
    int nKeys = 6;// number of supported ModKeys;

    // Determine if none, some, or all checkboxes are checked
    for(int ii=0;ii<nKeys;ii++){//loop over modifiers
        // Local variables used to determine if checkboxes are checked, unchecked or mixed
        bool some=false;
        bool someNot=false;
        bool multiSupport=false;
        // Local variable to hold spin box values
        std::vector<double> Values;
        std::vector<std::string> Units;
        std::string Label ="";
        bool flagLabel = true;
        bool flagNone = false; //toggles if any value is "None"
        QModelIndexList selectedRows = guiModel->getSelectedRows();
        foreach(QModelIndex index, selectedRows){//loop over records
            if (!index.isValid()) continue;

            //Determine if modifier is relevant for this record
            LSARecord *lsaRecord = guiModel->getLSARecord(index);
            if (lsaRecord->getRecType() == LSAType::COMMENT) continue;

            // checkboxes
            bool modCheck=false;//is mod relevant for this record?
            if(keys[ii]==0) modCheck=lsaRecord->supportsModifier(MODKEY_HEIGHTFROM);
            else if(keys[ii]==1) modCheck=lsaRecord->supportsModifier(MODKEY_HEIGHTTO);
            else if(keys[ii]==3) modCheck=lsaRecord->supportsModifier(MODKEY_REFRACT);
            else if(keys[ii]==4) modCheck=lsaRecord->supportsModifier(MODKEY_REDUCED);
            else if(keys[ii]==6) modCheck=lsaRecord->supportsModifier(MODKEY_CURV);
            else if(keys[ii]==7) modCheck=lsaRecord->supportsModifier(MODKEY_OHC);
            if(!modCheck) continue;

            bool recCheck=false;//is mod enabled for this record?
            if((keys[ii]==0)||(keys[ii]==1)){
                if(lsaRecord->getRecType() == LSAType::DXYZ) ui->labelDXYZHeight->show(); //Display GNSS label if DXYZ record present
                std::string heightLabel;
                double      heightValue(0.0);
                std::string heightUnits("m");
                double      sigma(0.0);
                std::string sigmaUnits("m");
                bool        isNumeric = false;
                (keys[ii]==0) ? recCheck=guiModel->getHeightFromData(lsaRecord, heightLabel, heightValue, heightUnits, sigma, sigmaUnits, isNumeric):
                recCheck=guiModel->getHeightToData(lsaRecord, heightLabel, heightValue, heightUnits, sigma, sigmaUnits, isNumeric);//Get HeightFrom/HeightTo data
                if(heightLabel=="") heightLabel=lsa::CONTROLVALUE_NONE; //Set to None if ""
                Values.push_back(heightValue);//Fill in Height values
                (isNumeric) ? Units.push_back(heightUnits) : Units.push_back("m");//Fill in Units
                if (heightLabel==lsa::CONTROLVALUE_NONE) flagNone=true;
                if (flagLabel){Label = heightLabel;flagLabel=false;}//First instance of record with HI/HT
                Label = (Label == heightLabel) ? heightLabel : lsa::CONTROLVALUE_MIXED;//Determine if all "Value","None" or "Mixed"
            }
            if(keys[ii]==3){
                double val = 0.0;
                recCheck = guiModel->getRefractData(lsaRecord,val);
                Values.push_back(val);
            }
            else if(keys[ii]==4) recCheck=lsaRecord->isReducedToEllipsoid();
            else if(keys[ii]==6) recCheck=lsaRecord->getCurvCorr();
            else if(keys[ii]==7) recCheck=lsaRecord->getOHC();

            some=(some||recCheck); //Is at least one checked?
            someNot=(someNot||(!recCheck)); //Is at least one unchecked?
            multiSupport=(multiSupport||modCheck); //Does at least one support?
        }
        Qt::CheckState checkState;
        if(multiSupport){//if at least one supports
            if(some && someNot)
                checkState = Qt::PartiallyChecked;
            else if (some && !someNot)
                checkState = Qt::Checked;
            else
                checkState = Qt::Unchecked;
            // populate Modifiers
            if(keys[ii]==0){
                ui->labelHeightFrom->show();
                ui->frameHeightFrom->show();
                bool supported = setupMultiHeightControlLabels(ui->comboHeightFrom,Label);
                ui->doubleHeightFromValue->setEnabled(false); // disallow direct edits to numeric values
                ui->comboHeightFromUnits->setEnabled(false); // disallow direct edits to values
                //All are "None"
                if (Label==lsa::CONTROLVALUE_NONE) ui->comboHeightFrom->setCurrentText(lsa::Q_CONTROLVALUE_NONE);
                //Some are "None" and some are "Value"
                else if (Label==lsa::CONTROLVALUE_MIXED){
                    ui->comboHeightFrom->setCurrentText(QString::fromStdString(Label));
                    ui->doubleHeightFromValue->setValue(0);
                    if (flagNone==false){
                        bool allHaveSameValue = true;
                        bool allHaveSameUnits = true;
                        double e = Values[0];
                        for(std::size_t i = 1, s = Values.size();i<s && allHaveSameValue;i++) allHaveSameValue=(e==Values[i]);
                        std::string eU = Units[0];
                        for(std::size_t i = 1, s = Units.size();i<s && allHaveSameUnits;i++) allHaveSameUnits=(eU==Units[i]);
                        if (allHaveSameValue && allHaveSameUnits)
                            ui->doubleHeightFromValue->setValue(Values[0]);
                    }
                }
                //All were same "Value"
                else{
                    if (Label==lsa::CONTROLVALUE_VALUE){
                        ui->doubleHeightFromValue->setEnabled(true); // allow direct edits to numeric values
                        ui->comboHeightFromUnits->setEnabled(true); // allow direct edits to values
                    }
                    ui->comboHeightFrom->setCurrentText(QString::fromStdString(Label));
                    bool allHaveSameValue = true;
                    bool allHaveSameUnits = true;
                    double e = Values[0];
                    for(std::size_t i = 1, s = Values.size();i<s && allHaveSameValue;i++) allHaveSameValue=(e==Values[i]);
                    std::string eU = Units[0];
                    for(std::size_t i = 1, s = Units.size();i<s && allHaveSameUnits;i++) allHaveSameUnits=(eU==Units[i]);
                    if (allHaveSameValue && allHaveSameUnits)
                        ui->doubleHeightFromValue->setValue(Values[0]);
                }
                if(!supported){
                    ui->doubleHeightFromValue->setValue(0);
                    ui->doubleHeightFromValue->setEnabled(false); // allow direct edits to numeric values
                    ui->comboHeightFromUnits->setEnabled(false); // allow direct edits to values
                }
            }
            if(keys[ii]==1){
                ui->labelHeightTo->show();
                ui->frameHeightTo->show();
                bool supported = setupMultiHeightControlLabels(ui->comboHeightTo,Label);
                ui->doubleHeightToValue->setEnabled(false); // disallow direct edits to numeric values
                ui->comboHeightToUnits->setEnabled(false); // disallow direct edits to values
                //All are "None"
                if (Label==lsa::CONTROLVALUE_NONE)ui->comboHeightTo->setCurrentText(lsa::Q_CONTROLVALUE_NONE);
                //Some are "None" and some are "Value"
                else if (Label==lsa::CONTROLVALUE_MIXED){
                    ui->comboHeightTo->setCurrentText(lsa::Q_CONTROLVALUE_MIXED);
                    ui->doubleHeightToValue->setValue(0);
                    if (flagNone==false){
                        bool allHaveSameValue = true;
                        bool allHaveSameUnits = true;
                        double e = Values[0];
                        for(std::size_t i = 1, s = Values.size();i<s && allHaveSameValue;i++) allHaveSameValue=(e==Values[i]);
                        std::string eU = Units[0];
                        for(std::size_t i = 1, s = Units.size();i<s && allHaveSameUnits;i++) allHaveSameUnits=(eU==Units[i]);
                        if (allHaveSameValue && allHaveSameUnits)
                            ui->doubleHeightToValue->setValue(Values[0]);
                    }
                }
                //All are same "Value"
                else{
                    if (Label==lsa::CONTROLVALUE_VALUE){
                        ui->doubleHeightToValue->setEnabled(true); // allow direct edits to numeric values
                        ui->comboHeightToUnits->setEnabled(true); // allow direct edits to values
                    }
                    ui->comboHeightTo->setCurrentText(QString::fromStdString(Label));
                    bool allHaveSameValue = true;
                    bool allHaveSameUnits = true;
                    double e = Values[0];
                    for(std::size_t i = 1, s = Values.size();i<s && allHaveSameValue;i++) allHaveSameValue=(e==Values[i]);
                    std::string eU = Units[0];
                    for(std::size_t i = 1, s = Units.size();i<s && allHaveSameUnits;i++) allHaveSameUnits=(eU==Units[i]);
                    if (allHaveSameValue && allHaveSameUnits)
                        ui->doubleHeightToValue->setValue(Values[0]);
                }
                if(!supported){
                    ui->doubleHeightToValue->setValue(0);
                    ui->doubleHeightToValue->setEnabled(false); // allow direct edits to numeric values
                    ui->comboHeightToUnits->setEnabled(false); // allow direct edits to values
                }
            }
            if(keys[ii]==3){
                ui->labelRefractiveIndex->show();
                ui->frameRefraction->show();
                (checkState==Qt::Unchecked) ? ui->doubleRefractiveIndex->setEnabled(false):ui->doubleRefractiveIndex->setEnabled(true);
                bool allHaveSameValue = true;
                double e = Values[0];
                for(std::size_t i = 1, s = Values.size();i<s && allHaveSameValue;i++) allHaveSameValue=(e==Values[i]);
                if (allHaveSameValue) ui->doubleRefractiveIndex->setValue(Values[0]);
                ui->checkApplyRefraction->setCheckState(checkState);

            }
            else if(keys[ii]==4){
                ui->labelReduced->show();
                ui->checkReduced->show();
                ui->checkReduced->setCheckState(checkState);
            }
            else if(keys[ii]==6){
                ui->labelCurvature->show();
                ui->checkCurvature->show();
                ui->checkCurvature->setCheckState(checkState);
            }
            else if(keys[ii]==7){
                ui->labelOHC->show();
                ui->checkOHC->show();
                ui->checkOHC->setCheckState(checkState);
            }
        }
    }
    return;
}

void RecordEditor::configureConstraintControls()
{
    QModelIndexList selectedRows = guiModel->getSelectedRows();

    bool posRecordFound = false;

    bool someLatChecked = false;
    bool someLatUnchecked = false;
    bool someLonChecked = false;
    bool someLonUnchecked = false;
    bool someHgtChecked = false;
    bool someHgtUnchecked = false;

    bool someFloatingSelected = false;
    bool someConstrainedSelected = false;
    bool someFixedSelected = false;

    bool invalidCovarianceFound = false;

    ui->checkNorthFixed->setEnabled(true);
    ui->checkEastFixed->setEnabled(true);
    ui->checkUpFixed->setEnabled(true);

    // Collect some data on the state of the selected records
    foreach(QModelIndex index, selectedRows)
    {
        if (!index.isValid()) continue;

        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        LSAType lsaType = lsaRecord->getRecType();

        if (lsaType.isPosition())
        {
            posRecordFound = true;

            bool latIsChecked = false;
            bool lonIsChecked = false;
            bool hgtIsChecked = false;

            bool floatingSelected    = false;
            bool constrainedSelected = false;
            bool fixedSelected       = false;

            if (lsaType == LSAType::POSG)
            {
                LSAPosG *lsaPosG = static_cast<LSAPosG*>(lsaRecord);

                latIsChecked = lsaPosG->isNorthFixed;
                lonIsChecked = lsaPosG->isEastFixed;
                hgtIsChecked = lsaPosG->isUpFixed;

                floatingSelected    = (lsaPosG->fixedState == LSAFixedState::FLOATING);
                constrainedSelected = (lsaPosG->fixedState == LSAFixedState::CONSTRAINED);
                fixedSelected       = (lsaPosG->fixedState == LSAFixedState::FIXED);
            }

            else if (lsaType == LSAType::POSC)
            {
                LSAPosC *lsaPosC = static_cast<LSAPosC*>(lsaRecord);

                latIsChecked = lsaPosC->isNorthFixed;
                lonIsChecked = lsaPosC->isEastFixed;
                hgtIsChecked = lsaPosC->isUpFixed;

                floatingSelected    = (lsaPosC->fixedState == LSAFixedState::FLOATING);
                constrainedSelected = (lsaPosC->fixedState == LSAFixedState::CONSTRAINED);
                fixedSelected       = (lsaPosC->fixedState == LSAFixedState::FIXED);
            }

            if (constrainedSelected)
            {
                invalidCovarianceFound = !lsaRecord->hasValidCovariance();
            }

            someLatChecked   = someLatChecked   || latIsChecked;
            someLatUnchecked = someLatUnchecked || !latIsChecked;

            someLonChecked   = someLonChecked   || lonIsChecked;
            someLonUnchecked = someLonUnchecked || !lonIsChecked;

            someHgtChecked   = someHgtChecked   || hgtIsChecked;
            someHgtUnchecked = someHgtUnchecked || !hgtIsChecked;

            someFloatingSelected    = someFloatingSelected    || floatingSelected;
            someConstrainedSelected = someConstrainedSelected || constrainedSelected;
            someFixedSelected       = someFixedSelected       || fixedSelected;
        }
    }

    // Configure controls based on the data we found
    if (posRecordFound)
    {
        // Setup the fixed checkboxes
        ui->frameConstraints->show();

        Qt::CheckState latCheckState;
        if      ( someLatChecked && !someLatUnchecked) latCheckState = Qt::Checked;
        else if (!someLatChecked &&  someLatUnchecked) latCheckState = Qt::Unchecked;
        else                                           latCheckState = Qt::PartiallyChecked;
        ui->checkNorthFixed->setCheckState(latCheckState);


        Qt::CheckState lonCheckState;
        if      ( someLonChecked && !someLonUnchecked) lonCheckState = Qt::Checked;
        else if (!someLonChecked &&  someLonUnchecked) lonCheckState = Qt::Unchecked;
        else                                           lonCheckState = Qt::PartiallyChecked;
        ui->checkEastFixed->setCheckState(lonCheckState);

        Qt::CheckState hgtCheckState;
        if      ( someHgtChecked && !someHgtUnchecked) hgtCheckState = Qt::Checked;
        else if (!someHgtChecked &&  someHgtUnchecked) hgtCheckState = Qt::Unchecked;
        else                                           hgtCheckState = Qt::PartiallyChecked;
        ui->checkUpFixed->setCheckState(hgtCheckState);

        // Setup the combo box
        ui->labelType->show();
        ui->frameFixedFloat->show();

        QString comboLabel;
        ui->comboFixedFloat->clear();
        if (latCheckState == Qt::Checked && lonCheckState == Qt::Checked && hgtCheckState == Qt::Checked)
        {
            // If all the checkboxes are checked, set the combo box to "fixed"
            comboLabel = comboLabel = QString::fromStdString(LSAFixedState("Fixed").asString());
            enableCOVControls(false);
            ui->comboFixedFloat->setDisabled(true);
        }
        else if (someFloatingSelected && !someConstrainedSelected && !someFixedSelected)
        {
            comboLabel = QString::fromStdString(LSAFixedState("Floating").asString());
            enableCOVControls(false);
            ui->comboFixedFloat->setEnabled(true);
        }
        else if (!someFloatingSelected && someConstrainedSelected && !someFixedSelected)
        {
            comboLabel = QString::fromStdString(LSAFixedState("Constrained").asString());
            enableCOVControls(true);
            ui->comboFixedFloat->setEnabled(true);
        }
        else if (!someFloatingSelected && !someConstrainedSelected && someFixedSelected)
        {
            comboLabel = QString::fromStdString(LSAFixedState("Fixed").asString());
            enableCOVControls(false);
            ui->comboFixedFloat->setEnabled(true);

            // The combo box is fixed, so check all the checkboxes
            ui->checkNorthFixed->setCheckState(Qt::Checked);
            ui->checkEastFixed->setCheckState(Qt::Checked);
            ui->checkUpFixed->setCheckState(Qt::Checked);
            ui->checkNorthFixed->setDisabled(true);
            ui->checkEastFixed->setDisabled(true);
            ui->checkUpFixed->setDisabled(true);
        }
        else
        {
            comboLabel = lsa::Q_CONTROLVALUE_MIXED;
            ui->comboFixedFloat->addItem(comboLabel);
            ui->comboFixedFloat->setEnabled(true);
        }
        QStringList items;
        items << QString::fromStdString(LSAFixedState("Floating").asString());
        items << QString::fromStdString(LSAFixedState("Constrained").asString());
        items << QString::fromStdString(LSAFixedState("Fixed").asString());

        ui->comboFixedFloat->addItems(items);
        ui->comboFixedFloat->setCurrentText(comboLabel);

        if (invalidCovarianceFound)
        {
            ui->labelCovWarning->show();
        }
        else
        {
            ui->labelCovWarning->hide();
        }

        if(guiModel->hasAutogeneratedRow())
        {
            ui->checkNorthFixed->setEnabled(false);
            ui->checkEastFixed->setEnabled(false);
            ui->checkUpFixed->setEnabled(false);
            ui->comboFixedFloat->setEnabled(false);
        }
    }

    return;
}

void RecordEditor::configureHeightStateControl()
{
    //QModelIndexList selectedRows = guiModel->getSelectedRows();
    bool someEllipsoidal = false;
    bool someOrthometric = false;
    bool allPosg = true;

    foreach (QModelIndex index, selectedRows)
    {
        if(!index.isValid())
            continue;

        LSARecord *lsaRecord = guiModel->getLSARecord(index);

        if (lsaRecord->getRecType() != LSAType::POSG)
        {
            allPosg = false;
            break;
        }

        LSAPosG *lsaPosG = static_cast<LSAPosG*>(lsaRecord);
        if(lsaPosG->heightIsEllipsoidal)
            someEllipsoidal = true;
        else
            someOrthometric = true;
    }

    if(!allPosg)
        return;

    ui->frameValueC->show();
    ui->comboPOSGHeight->clear();
    ui->comboPOSGHeight->addItem(lsa::Q_OPTION_ELLIPSOID);
    ui->comboPOSGHeight->addItem(lsa::Q_OPTION_ORTHOMETRIC);

    if(someEllipsoidal && someOrthometric)
    {
        ui->comboPOSGHeight->addItem(lsa::Q_CONTROLVALUE_MIXED);
        ui->comboPOSGHeight->setCurrentText(lsa::Q_CONTROLVALUE_MIXED);
    }

    else if(someEllipsoidal)
    {
        ui->comboPOSGHeight->setCurrentText(lsa::Q_OPTION_ELLIPSOID);
    }

    else if(someOrthometric)
    {
        ui->comboPOSGHeight->setCurrentText(lsa::Q_OPTION_ORTHOMETRIC);
    }

    ui->comboPOSGHeight->show();
}

void RecordEditor::enableCOVControls(bool enable)
{

    ui->doubleCovAA->setEnabled(enable);
    ui->doubleCovAB->setEnabled(enable);
    ui->doubleCovAC->setEnabled(enable);
    ui->doubleCovBB->setEnabled(enable);
    ui->doubleCovBC->setEnabled(enable);
    ui->doubleCovCC->setEnabled(enable);

    return;
}

void RecordEditor::configureVSCAForMultipleRecords()
{
    // Local variables used to determine settings for VSCA controls
    bool oneOrMoreRecordsSupportVSCA = false;
    std::vector<double> vscaValues;

    QModelIndexList selectedRows = guiModel->getSelectedRows();

    std::string vscaLabel;
    bool atLeastOneRecordConfigured = false;
    bool flagNone = false; //True if at least one record set to None
    foreach(QModelIndex index, selectedRows)
    {
        if (!index.isValid()) continue;

        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        if (lsaRecord->getRecType() == LSAType::COMMENT) continue;

        if (!atLeastOneRecordConfigured)
        {
            if ( lsaRecord->vscaIsNumeric() )                          vscaLabel = lsa::CONTROLVALUE_VALUE;
            else if (lsaRecord->getModifierLabel(MODKEY_VSCA).empty()) vscaLabel = lsa::CONTROLVALUE_NONE;
            else                                                       vscaLabel = lsaRecord->getModifierLabel(MODKEY_VSCA);
            atLeastOneRecordConfigured = true;
        }

        vscaValues.push_back(guiModel->getVSCAValue(lsaRecord));

        // current values
        std::string currentVSCALabel;
        bool currentVSCAIsNumeric = lsaRecord->vscaIsNumeric();
        if (currentVSCAIsNumeric)
        {
            currentVSCALabel = lsa::CONTROLVALUE_VALUE;
        }
        else if (lsaRecord->getModifierLabel(MODKEY_VSCA).empty())
        {
            currentVSCALabel = lsa::CONTROLVALUE_NONE;
            flagNone=true;
        }
        else
        {
            currentVSCALabel = lsaRecord->getModifierLabel(MODKEY_VSCA);
        }

        // Does at least one of the selected records support VSCA?
        oneOrMoreRecordsSupportVSCA = oneOrMoreRecordsSupportVSCA || lsaRecord->supportsModifier(MODKEY_VSCA);

        // Do all selected records have the same vsca label selected?
        vscaLabel = (vscaLabel == currentVSCALabel) ? currentVSCALabel : lsa::CONTROLVALUE_MIXED;
    }

    if (!atLeastOneRecordConfigured) return;

    // Check if all vsca have the same value
    bool allVSCAHaveSameValue = true;
    double previousValue = vscaValues.at(0);
    for (int i = 1; i < vscaValues.size(); ++i)
    {
        double currentValue = vscaValues.at(i);
        bool currentMatchesPrevious = ( std::abs(currentValue - previousValue) < 1e-10);
        allVSCAHaveSameValue = allVSCAHaveSameValue && currentMatchesPrevious;
        previousValue = currentValue;
    }

    // Configure the Record Editor controls
    if (oneOrMoreRecordsSupportVSCA)
    {
        //Fix to Bug #1221
        ui->doubleVSCA->setMinimum(0.0);

        // show controls
        ui->labelVSCA->show();
        ui->doubleVSCA->show();

        // combobox and numeric control
        if (vscaLabel == lsa::CONTROLVALUE_NONE)
        {
            setupVSCAControlLabels();
            ui->comboVSCA->setCurrentText(lsa::Q_CONTROLVALUE_NONE);
            ui->doubleVSCA->setValue(1.0);
        }
        else if (vscaLabel == lsa::CONTROLVALUE_VALUE)
        {
            setupVSCAControlLabels();
            ui->comboVSCA->setCurrentText(lsa::Q_CONTROLVALUE_VALUE);
            if (allVSCAHaveSameValue)
            {
                ui->doubleVSCA->setValue(previousValue);
            }
            else
            {
                ui->doubleVSCA->setSpecialValueText("multiple");
                ui->doubleVSCA->setValue(0.0); // set to min value to trigger special text
            }
        }
        else
        {
            setupVSCAControlLabels(vscaLabel);
            ui->comboVSCA->setCurrentText(QString::fromStdString( vscaLabel ));
            if(!flagNone && allVSCAHaveSameValue) ui->doubleVSCA->setValue(previousValue);
            else if (!flagNone){
                ui->doubleVSCA->setSpecialValueText("multiple");
                ui->doubleVSCA->setValue(0.0); // set to min value to trigger special text
            }

            LSAVSCAMap::iterator iter = varMap->find(vscaLabel);
            if (vscaLabel.empty() )
            {
            }
            else if ( iter != varMap->end() )
            {
                LSAVarScaling *vsca = iter->second;
                double scaleFactor = vsca->varFactor;
                ui->doubleVSCA->setValue(scaleFactor);
            }
        }
        //fix to Bug #1221 - like updateVSCAControls(), but that method uses currentLSARecord
        if (lsa::Q_CONTROLVALUE_NONE == QString::fromStdString(vscaLabel))
            ui->doubleVSCA->setEnabled(false);
        else if (lsa::Q_CONTROLVALUE_VALUE == QString::fromStdString(vscaLabel))
            ui->doubleVSCA->setEnabled(true);
        else
            ui->doubleVSCA->setEnabled(false);

        // if a parent and child are selected at the same time, disable the vsca combo box
        if (guiModel->parentAndChildSelected() )
        {
            ui->comboVSCA->setDisabled(true);
            ui->comboVSCA->setToolTip("Include and its children must be edited separately.");        }
        else
        {
            ui->comboVSCA->setEnabled(true);
            ui->comboVSCA->setToolTip("");
        }

        // Hide the net VSCA controls in the record editor
        ui->doubleNetVSCA->hide();
        ui->labelNetVarScaling->hide();

        if(guiModel->hasAutogeneratedRow())
        {
            ui->comboVSCA->setEnabled(false);
            ui->doubleVSCA->setEnabled(false);
        }
    }

    return;
}

void RecordEditor::configureDGRPForMultipleRecords()
{
    // Local variables used to determine settings for VSCA controls
    bool oneOrMoreRecordsSupportDGRP = false;

    QModelIndexList selectedRows = guiModel->getSelectedRows();

    std::string dgrpLabel;
    std::string fromStationLabel;
    bool atLeastOneRecordIsHDir = false;
    foreach(QModelIndex index, selectedRows)
    {
        if (!index.isValid()) continue;

        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        if (lsaRecord->getRecType() != LSAType::HDIR) continue;

        LSAHDir *lsaHDir = static_cast<LSAHDir*>(lsaRecord);

        // Set the initial value for dgrpLabel
        if (!atLeastOneRecordIsHDir)
        {
            dgrpLabel = lsaHDir->dirGroupLabel;
            fromStationLabel = guiModel->getParentDGRPFromStation(lsaHDir);
            atLeastOneRecordIsHDir = true;
        }

        // Do all selected HDIR records point to the same DGRP label?
        std::string nextDGRPLabel = lsaHDir->dirGroupLabel;
        if (nextDGRPLabel != dgrpLabel)
        {
            dgrpLabel = lsa::CONTROLVALUE_MIXED;
            break;
        }
    }

    // Configure the Record Editor controls
    if (atLeastOneRecordIsHDir)
    {
        // show controls
        ui->labelDirGroup->show();
        ui->frameDirGroup->show();

        // configure DGRP combo box
        setupDirGroupControlLabels(dgrpLabel);
        ui->comboDirGroup->setCurrentText(QString::fromStdString(dgrpLabel));

        // Configure at station label
        if (dgrpLabel != lsa::CONTROLVALUE_MIXED)
        {
            fromStationLabel = "(At Point: " + fromStationLabel + ")";
        }
        else
        {
            fromStationLabel = "(mixed)";
        }
        ui->labelDirGroupValue->setText(QString::fromStdString(fromStationLabel));
    }
    else
    {
        ui->labelDirGroup->hide();
        ui->frameDirGroup->hide();
    }

    return;
}

void RecordEditor::configureUNCRForMultipleRecords()
{
    // Local variables used to determine settings for UNCR controls
    bool oneOrMoreRecordsSupportUNCR = false;
    bool noRecordsHaveUNCR = true;
    bool allSelectedRecordsAreSameType = true;

    QModelIndexList selectedRows = guiModel->getSelectedRows();

    LSAType previousType;
    std::string uncrLabel;
    bool atLeastOneRecordConfigured = false;
    foreach(QModelIndex index, selectedRows)
    {
        if (!index.isValid()) continue;

        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        LSAType currentType = lsaRecord->getRecType();

        if (currentType == LSAType::COMMENT) continue;
        if (!atLeastOneRecordConfigured)
        {
            previousType = lsaRecord->getRecType();
            uncrLabel = lsaRecord->getModifierLabel(MODKEY_UNCR);
            atLeastOneRecordConfigured = true;
        }

        std::string currentUNCRLabel = lsaRecord->getModifierLabel(MODKEY_UNCR);

        oneOrMoreRecordsSupportUNCR = oneOrMoreRecordsSupportUNCR || lsaRecord->supportsModifier(MODKEY_UNCR);

        // Do all selected rows have the same type?
        allSelectedRecordsAreSameType = allSelectedRecordsAreSameType && (currentType == previousType);
        previousType = currentType;

        // Do all of the selected records have no VSCA selected?
        noRecordsHaveUNCR = noRecordsHaveUNCR && currentUNCRLabel.empty();

        // Do all selected records have the same vsca label selected?
        uncrLabel = (uncrLabel == currentUNCRLabel) ? currentUNCRLabel : lsa::Q_CONTROLVALUE_MIXED.toStdString();
    }

    if (!atLeastOneRecordConfigured) return;

    // Configure the Record Editor controls
    if (oneOrMoreRecordsSupportUNCR && allSelectedRecordsAreSameType)
    {
        if (uncrLabel.empty())
            uncrLabel = "None"; // This is a quick-fix for bug 1269. configureUNCRControls needs a non-empty string in order to work
        configureUNCRControls(uncrLabel);
        setupUncrControlLabels(uncrLabel);
        ui->comboUncrModifier->setCurrentText(QString::fromStdString(uncrLabel));
        if (uncrLabel == lsa::CONTROLVALUE_MIXED) ui->labelUncrValues->hide();
    }
    else if (oneOrMoreRecordsSupportUNCR)
    {
        setupUncrControlLabels(uncrLabel);
        ui->labelUncrModifier->show();
        ui->frameUncrModifier->show();
        ui->comboUncrModifier->setCurrentText(QString::fromStdString(uncrLabel));
        ui->comboUncrModifier->setDisabled(true);
        ui->labelUncrValues->hide();
        ui->labelUncrComments->setText("( edit one type at a time )");
        ui->labelUncrComments->show();
    }

    return;
}

void RecordEditor::configurePOSC()
{
    LSAPosC *lsaPoint(static_cast<LSAPosC *>(currentLSARecord));
    currentModifier =  &lsaPoint->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(lsaPoint->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Station name
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText("Label:");
    ui->lineStationA->setText(QString(lsaPoint->label.c_str()));

    QString currentUnits = QString::fromStdString(lsaPoint->posUnits).toLower();
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_KM << lsa::Q_UNITS_FT;
    // x coordinate
    ui->labelValueA->show();
    ui->frameValueA->show();
    ui->labelValueA->setText("X:");
    configureNumDigits(ui->doubleValueA, currentLSAType, currentUnits);
    ui->doubleValueA->setValue(lsaPoint->x);
    ui->comboValueAUnits->clear();
    ui->comboValueAUnits->insertItems(0, allowedUnits);
    ui->comboValueAUnits->setCurrentText(currentUnits);

    // y coordinate
    ui->labelValueB->show();
    ui->frameValueB->show();
    ui->labelValueB->setText("Y:");
    configureNumDigits(ui->doubleValueB, currentLSAType, currentUnits);
    ui->doubleValueB->setValue(lsaPoint->y);
    ui->comboValueBUnits->clear();
    ui->comboValueBUnits->insertItems(0, allowedUnits);
    ui->comboValueBUnits->setCurrentText(currentUnits);

    std::set<std::string>::iterator iter;
    for(iter=lsaPoint->warningMessages_persistent.begin(); iter!=lsaPoint->warningMessages_persistent.end(); iter++)
    {
        if(iter->find("pole singularity")!=std::string::npos)
        {
            lsaPoint->warningMessages_persistent.erase(iter);
            break;
        }
    }

    if(std::abs(ui->doubleValueA->value()) < lsa::EPSILON && std::abs(ui->doubleValueB->value()) < lsa::EPSILON)
    {
        ui->labelCartesianWarning->show();
        std::string message = "Having x and y coordinates near zero yields pole singularity.";
        lsaPoint->warningMessages_persistent.insert(message);
    }
    else
    {
        ui->labelCartesianWarning->hide();
    }

    // z coordinate
    ui->labelValueC->show();
    ui->frameValueC->show();
    ui->labelValueC->setText("Z:");
    configureNumDigits(ui->doubleValueC, currentLSAType, currentUnits);
    ui->doubleValueC->setValue(lsaPoint->z);
    ui->comboValueCUnits->clear();
    ui->comboValueCUnits->insertItems(0, allowedUnits);
    ui->comboValueCUnits->setCurrentText(currentUnits);

    // covariance
    ui->labelCovariance->show();
    ui->frameCovariance->show();
    ui->doubleCovAA->setText(QString::number(lsaPoint->covxx, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovAB->setText(QString::number(lsaPoint->covxy, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovAC->setText(QString::number(lsaPoint->covxz, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovBB->setText(QString::number(lsaPoint->covyy, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovBC->setText(QString::number(lsaPoint->covyz, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovCC->setText(QString::number(lsaPoint->covzz, 'e', lsa::NUM_DECIMALS_COVARIANCE));

    ui->labelCovAA->setText(" xx: ");
    ui->labelCovAB->setText(" xy: ");
    ui->labelCovAC->setText(" xz: ");
    ui->labelCovBB->setText(" yy: ");
    ui->labelCovBC->setText(" yz: ");
    ui->labelCovCC->setText(" zz: ");

    ui->labelType->show();
    ui->frameConstraints->show();
    ui->frameFixedFloat->show();
    ui->comboFixedFloat->setCurrentText( QString::fromStdString(lsaPoint->fixedState.asString()) );

    configureConstraintControls();

    // Variance Scaling
    configureVSCAControls();

    return;
}

void RecordEditor::configurePOSG()
{
    LSAPosG *lsaPoint(static_cast<LSAPosG *>(currentLSARecord));
    currentModifier = &lsaPoint->modifiers;

    //recover from disabling that might have occurred from selecting an autogen POSG
    enableAllPOSGControls(true);

    // Record Description
    QString recordDescription;
    if(lsaPoint->isAutogenerated)
    {
        recordDescription = QString("POSG - AUTO-GENERATED Station Position (Geodetic) [READ-ONLY]");
        ui->labelAutogenPOSGInfo->show();
    }
    else
    {
        recordDescription = QString::fromStdString(lsaPoint->getRecType().getDescription());
        ui->labelAutogenPOSGInfo->hide();
    }
    setWindowTitle(recordDescription);

    // Station name
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText("Label:");
    ui->lineStationA->setText(QString(lsaPoint->label.c_str()));

    // lat and lon
    useDMS = !lsaPoint->useDecimalDegrees;
    showLatLonControls();

    //need to configure BOTH DecDeg AND DMS controls, or see issues when toggling DMZ<-->DecDeg
    configureNumDigits(ui->doubleLatDMSSec, currentLSAType, lsa::Q_UNITS_SOA);
    configureNumDigits(ui->doubleLatDecDeg, currentLSAType, lsa::Q_UNITS_DEG);

    ui->intLatDMSDeg->setMaximum(90);
    ui->intLatDMSDeg->setMinimum(0);
    ui->intLatDMSMin->setMaximum(59);
    ui->intLatDMSMin->setMinimum(0);
    ui->doubleLatDMSSec->setMaximum(59.99999999);
    ui->doubleLatDMSSec->setMinimum(0);

    ui->intLonDMSDeg->setMaximum(359);
    ui->intLonDMSDeg->setMinimum(0);
    ui->intLonDMSMin->setMaximum(59);
    ui->intLonDMSMin->setMinimum(0);
    ui->doubleLonDMSSec->setMaximum(59.99999999);
    ui->doubleLonDMSSec->setMinimum(0);

    ui->doubleLatDecDeg->setMaximum(90.0);
    ui->doubleLatDecDeg->setMinimum(-90.0);
    ui->doubleLonDecDeg->setMaximum(359.99999999);
    ui->doubleLonDecDeg->setMinimum(-359.99999999);

    if (useDMS)
    {
        // lat
        latDMSIsNeg = lsaPoint->latDMSIsNeg;
        QString latSignText = QString::fromStdString(lsaPoint->latDMSIsNeg ? "-" : "+");
        ui->btnLatDMSNeg->setText(latSignText);
        ui->intLatDMSDeg->setValue(lsaPoint->latDeg);
        ui->intLatDMSMin->setValue(lsaPoint->latMin);
        ui->doubleLatDMSSec->setValue(lsaPoint->latSec);

        //lat dir
        QString latDir = QString::fromStdString(lsaPoint->latDir);
        ui->comboLatDMSNS->setCurrentText(latDir);
        if(!LSARecord::validateDMSBool(lsaPoint->latDMSIsNeg,lsaPoint->latDeg,lsaPoint->latMin,lsaPoint->latSec,lsa::MIN_LATITUDE_BOUNDARY,lsa::MAX_LATITUDE_BOUNDARY))
        {
            ui->labelLatDMSWarning->show();
        }
        else
        {
            ui->labelLatDMSWarning->hide();
        }

        // lon
        lonDMSIsNeg = lsaPoint->lonDMSIsNeg;
        QString lonSignText = QString::fromStdString(lsaPoint->lonDMSIsNeg ? "-" : "+");
        ui->btnLonDMSNeg->setText(lonSignText);
        ui->intLonDMSDeg->setValue(lsaPoint->lonDeg);
        ui->intLonDMSMin->setValue(lsaPoint->lonMin);
        configureNumDigits(ui->doubleLonDMSSec, currentLSAType, lsa::Q_UNITS_SOA);
        ui->doubleLonDMSSec->setValue(lsaPoint->lonSec);

        // lon dir
        QString lonDir = QString::fromStdString(lsaPoint->lonDir);
        ui->comboLonDMSEW->setCurrentText(lonDir);
    }
    else
    {
        // lat
        ui->doubleLatDecDeg->setValue(lsaPoint->latDecDeg);
        if(ui->doubleLatDecDeg->value() < lsa::MIN_LATITUDE_BOUNDARY || ui->doubleLatDecDeg->value() > lsa::MAX_LATITUDE_BOUNDARY)
        {
            ui->labelLatDecDegWarning->show();
        }
        else
        {
            ui->labelLatDecDegWarning->hide();
        }

        // lat dir
        QString latDir = QString::fromStdString(lsaPoint->latDir);
        ui->comboLatDecDegNS->setCurrentText(latDir);

        // lon
        configureNumDigits(ui->doubleLonDecDeg, currentLSAType, lsa::Q_UNITS_DEG);
        ui->doubleLonDecDeg->setValue(lsaPoint->lonDecDeg);

        // lon dir
        QString lonDir = QString::fromStdString(lsaPoint->lonDir);
        ui->comboLonDecDegEW->setCurrentText(lonDir);
    }

    // height

    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_CM << lsa::Q_UNITS_FT;
    QString currentUnits = QString::fromStdString(lsaPoint->heightUnits).toLower();
    ui->comboValueCUnits->clear();
    ui->comboValueCUnits->insertItems(0, allowedUnits);
    ui->comboValueCUnits->setCurrentText(currentUnits);

    // Orthometric or Ellipsoidal Marker
    QStringList heightTypes;
    heightTypes << lsa::Q_OPTION_ELLIPSOID << lsa::Q_OPTION_ORTHOMETRIC;
    ui->comboPOSGHeight->clear();
    ui->comboPOSGHeight->insertItems(0, heightTypes);
    QString markerLabel;

    if(lsaPoint->heightIsEllipsoidal)
    {
        markerLabel = lsa::Q_OPTION_ELLIPSOID;
    }

    else
    {
        markerLabel = lsa::Q_OPTION_ORTHOMETRIC;
    }

    ui->comboPOSGHeight->setCurrentText(markerLabel);
    ui->comboPOSGHeight->show();

    ui->labelValueC->show();
    ui->frameValueC->show();
    ui->labelValueC->setText("Height: ");
    configureNumDigits(ui->doubleValueC, currentLSAType, currentUnits);
    ui->doubleValueC->setValue(lsaPoint->height);

    // covariance
    ui->labelCovariance->show();
    ui->frameCovariance->show();
    ui->doubleCovAA->setText(QString::number(lsaPoint->covnn, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovAB->setText(QString::number(lsaPoint->covne, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovAC->setText(QString::number(lsaPoint->covnu, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovBB->setText(QString::number(lsaPoint->covee, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovBC->setText(QString::number(lsaPoint->coveu, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovCC->setText(QString::number(lsaPoint->covuu, 'e', lsa::NUM_DECIMALS_COVARIANCE));

    ui->labelCovAA->setText(" nn: ");
    ui->labelCovAB->setText(" ne: ");
    ui->labelCovAC->setText(" nu: ");
    ui->labelCovBB->setText(" ee: ");
    ui->labelCovBC->setText(" eu: ");
    ui->labelCovCC->setText(" uu: ");

    ui->labelType->show();
    ui->frameConstraints->show();
    ui->frameFixedFloat->show();
    ui->comboFixedFloat->setCurrentText( QString::fromStdString(lsaPoint->fixedState.asString()) );

    configureConstraintControls();
    configureHeightStateControl();

    // Variance Scaling
    configureVSCAControls();

    if(lsaPoint->isAutogenerated)
        enableAllPOSGControls(false);

    return;
}

void RecordEditor::enableAllPOSGControls(bool enable)
{
    ui->lineStationA->setEnabled(true);//always editable - Fix to Bug #1314

    ui->intLatDMSDeg->setEnabled(enable);
    ui->intLatDMSMin->setEnabled(enable);
    ui->doubleLatDMSSec->setEnabled(enable);
    ui->intLonDMSDeg->setEnabled(enable);
    ui->intLonDMSMin->setEnabled(enable);
    ui->doubleLonDMSSec->setEnabled(enable);
    ui->doubleLatDecDeg->setEnabled(enable);
    ui->doubleLonDecDeg->setEnabled(enable);
    ui->btnLatDMSNeg->setEnabled(enable);
    ui->comboLatDMSNS->setEnabled(enable);
    ui->btnLonDMSNeg->setEnabled(enable);
    ui->comboLonDMSEW->setEnabled(enable);
    ui->comboLatDecDegNS->setEnabled(enable);
    ui->comboLonDecDegEW->setEnabled(enable);
    ui->doubleValueC->setEnabled(enable);
    ui->comboValueCUnits->setEnabled(enable);
    ui->comboPOSGHeight->setEnabled(enable);
    ui->doubleCovAA->setEnabled(enable);
    ui->doubleCovAB->setEnabled(enable);
    ui->doubleCovAC->setEnabled(enable);
    ui->doubleCovBB->setEnabled(enable);
    ui->doubleCovBC->setEnabled(enable);
    ui->doubleCovCC->setEnabled(enable);
    ui->comboFixedFloat->setEnabled(enable);
    ui->checkNorthFixed->setEnabled(enable);
    ui->checkEastFixed->setEnabled(enable);
    ui->checkUpFixed->setEnabled(enable);
    ui->comboVSCA->setEnabled(enable);
    ui->doubleVSCA->setEnabled(enable);
}

void RecordEditor::configureDXYZ()
{
    LSADelta *lsaDelta = static_cast<LSADelta *>(currentLSARecord);
    currentModifier = &lsaDelta->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(lsaDelta->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Station name
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(lsaDelta->From.c_str()));

    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText(lsa::Q_CONTROLVALUE_TO);
    ui->lineStationB->setText(QString(lsaDelta->To.c_str()));

    QString currentUnits = QString::fromStdString(lsaDelta->linUnits).toLower();
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_KM << lsa::Q_UNITS_FT;

    // delta x coordinate
    ui->labelValueA->show();
    ui->frameValueA->show();
    ui->labelValueA->setText("Delta X:");
    configureNumDigits(ui->doubleValueA, currentLSAType, currentUnits);
    ui->doubleValueA->setValue(lsaDelta->dx);
    ui->comboValueAUnits->clear();
    ui->comboValueAUnits->insertItems(0, allowedUnits);
    ui->comboValueAUnits->setCurrentText(currentUnits);

    // y coordinate
    ui->labelValueB->show();
    ui->frameValueB->show();
    ui->labelValueB->setText("Delta Y:");
    configureNumDigits(ui->doubleValueB, currentLSAType, currentUnits);
    ui->doubleValueB->setValue(lsaDelta->dy);
    ui->comboValueBUnits->clear();
    ui->comboValueBUnits->insertItems(0, allowedUnits);
    ui->comboValueBUnits->setCurrentText(currentUnits);

    // z coordinate
    ui->labelValueC->show();
    ui->frameValueC->show();
    ui->labelValueC->setText("Delta Z:");
    configureNumDigits(ui->doubleValueC, currentLSAType, currentUnits);
    ui->doubleValueC->setValue(lsaDelta->dz);
    ui->comboValueCUnits->clear();
    ui->comboValueCUnits->insertItems(0, allowedUnits);
    ui->comboValueCUnits->setCurrentText(currentUnits);

    // covariance
    ui->labelCovariance->show();
    ui->frameCovariance->show();
    ui->frameCovarianceMatrix->setEnabled(true);
    ui->doubleCovAA->setText(QString::number(lsaDelta->covxx, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovAB->setText(QString::number(lsaDelta->covxy, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovAC->setText(QString::number(lsaDelta->covxz, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovBB->setText(QString::number(lsaDelta->covyy, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovBC->setText(QString::number(lsaDelta->covyz, 'e', lsa::NUM_DECIMALS_COVARIANCE));
    ui->doubleCovCC->setText(QString::number(lsaDelta->covzz, 'e', lsa::NUM_DECIMALS_COVARIANCE));

    ui->labelCovAA->setText(" xx: ");
    ui->labelCovAB->setText(" xy: ");
    ui->labelCovAC->setText(" xz: ");
    ui->labelCovBB->setText(" yy: ");
    ui->labelCovBC->setText(" yz: ");
    ui->labelCovCC->setText(" zz: ");

    // Show a warning icon if the covariance is not positive semi-definite
    if ( currentLSARecord->hasValidCovariance() )
    {
        ui->labelCovWarning->hide();
    }
    else
    {
        ui->labelCovWarning->show();
    }

    ui->sigE->show();
    ui->sigN->show();
    ui->sigU->show();
    ui->labelSigE->show();
    ui->labelSigN->show();
    ui->labelSigU->show();

    ui->labelDXYZHeight->show();

    // covariance diagonal as ENU
    LSARecord* refPoint = guiModel->getPointWithLabel(lsaDelta->From);
    gnsstk::Matrix<double> covenu;
    if(refPoint)
    {
        gnsstk::Triple lonlat = getLonLatHeightRads(refPoint);
        gnsstk::Matrix<double> covMatrix(3, 3);
        covMatrix(0,0) = lsaDelta->covxx; covMatrix(0,1) = lsaDelta->covxy; covMatrix(0,2) = lsaDelta->covxz;
        covMatrix(1,0) = lsaDelta->covxy; covMatrix(1,1) = lsaDelta->covyy; covMatrix(1,2) = lsaDelta->covyz;
        covMatrix(2,0) = lsaDelta->covxz; covMatrix(2,1) = lsaDelta->covyz; covMatrix(2,2) = lsaDelta->covzz;
        covenu = convertECEFtoENU({lonlat[0], lonlat[1]}, covMatrix);

        double xx = std::abs(covenu[0][0]);
        double yy = std::abs(covenu[1][1]);
        double zz = std::abs(covenu[2][2]);
        updateDYXZ_CovENU(std::sqrt(xx), std::sqrt(yy), std::sqrt(zz));
    }

    // HeightTo
    configureHeightTo();

    // HeightFrom
    configureHeightFrom();

    // Uncr modifier
    configureUNCRControls();

    // Variance Scaling
    configureVSCAControls();

    return;
}

void RecordEditor::configureENUO()
{
    LSAEnuo *lsaEnuo = static_cast<LSAEnuo *>(currentLSARecord);

    // Record Description
    QString recordDescription = QString::fromStdString(lsaEnuo->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Station name
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(lsaEnuo->From.c_str()));

    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText("Label: ");
    ui->lineStationB->setText(QString(lsaEnuo->To.c_str()));

    QString currentUnits = QString::fromStdString(lsaEnuo->linUnits).toLower();
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_KM << lsa::Q_UNITS_FT;

    // de coordinate
    ui->labelValueA->show();
    ui->frameValueA->show();
    ui->labelValueA->setText("Delta E:");
    configureNumDigits(ui->doubleValueA, currentLSAType, currentUnits);
    ui->doubleValueA->setValue(lsaEnuo->de);
    ui->comboValueAUnits->clear();
    ui->comboValueAUnits->insertItems(0, allowedUnits);
    ui->comboValueAUnits->setCurrentText(currentUnits);

    // dn coordinate
    ui->labelValueB->show();
    ui->frameValueB->show();
    ui->labelValueB->setText("Delta N:");
    configureNumDigits(ui->doubleValueB, currentLSAType, currentUnits);
    ui->doubleValueB->setValue(lsaEnuo->dn);
    ui->comboValueBUnits->clear();
    ui->comboValueBUnits->insertItems(0, allowedUnits);
    ui->comboValueBUnits->setCurrentText(currentUnits);

    // du coordinate
    ui->labelValueC->show();
    ui->frameValueC->show();
    ui->labelValueC->setText("Delta U:");
    configureNumDigits(ui->doubleValueC, currentLSAType, currentUnits);
    ui->doubleValueC->setValue(lsaEnuo->du);
    ui->comboValueCUnits->clear();
    ui->comboValueCUnits->insertItems(0, allowedUnits);
    ui->comboValueCUnits->setCurrentText(currentUnits);

    // Sigmas
    ui->labelValueA1->show();
    ui->labelValueB1->show();
    ui->labelValueC1->show();
    ui->doubleValueA1->show();
    ui->doubleValueB1->show();
    ui->doubleValueC1->show();

    ui->labelValueA1->setText("Sigma E: ");
    ui->labelValueB1->setText("Sigma N: ");
    ui->labelValueC1->setText("Sigma U: ");
    ui->doubleValueA1->setValue(lsaEnuo->se);
    ui->doubleValueB1->setValue(lsaEnuo->sn);
    ui->doubleValueC1->setValue(lsaEnuo->su);


    return;
}

void RecordEditor::configureDIST()
{
    LSADist *lsaDist = static_cast<LSADist *>(currentLSARecord);
    currentModifier = &lsaDist->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(lsaDist->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // From Station
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(lsaDist->From.c_str()));

    // To Station
    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText(lsa::Q_CONTROLVALUE_TO);
    ui->lineStationB->setText(QString(lsaDist->To.c_str()));

    // Distance value
    QStringList allowedUnits;

    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_CM << lsa::Q_UNITS_KM << lsa::Q_UNITS_FT;
    ui->labelValueA->show();
    ui->frameValueA->show();
    ui->labelValueA->setText("Distance :");
    QString linUnits = QString::fromStdString(lsaDist->linUnits).toLower();
    configureNumDigits(ui->doubleValueA, currentLSAType, linUnits);
    ui->doubleValueA->setValue(lsaDist->distance);
    ui->comboValueAUnits->clear();
    ui->comboValueAUnits->insertItems(0, allowedUnits);
    ui->comboValueAUnits->setCurrentText(linUnits);

    // Sigma
    ui->labelSigma->show();
    ui->frameSigma->show();
    configureNumDigits(ui->doubleSigma, currentLSAType, linUnits);
    ui->doubleSigma->setValue(lsaDist->sigma);
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(linUnits);

    // HeightTo
    configureHeightTo();

    // HeightFrom
    configureHeightFrom();

    // Uncr modifier
    configureUNCRControls();

    // Variance Scaling
    configureVSCAControls();

    return;
}

void RecordEditor::configureZANG()
{
    LSAZAngle *zAngle = static_cast<LSAZAngle *>(currentLSARecord);
    currentModifier = &zAngle->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(zAngle->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // From
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(zAngle->From.c_str()));

    // To
    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText(lsa::Q_CONTROLVALUE_TO);
    ui->lineStationB->setText(QString(zAngle->To.c_str()));

    // DMS to Dec Deg radio buttons
    ui->frameUseDMS->show();
    useDMS = !zAngle->usesDecDeg;
    ui->radioUseDMS->setChecked(useDMS);
    ui->radioUseDecDeg->setChecked(!useDMS);

    //set gui min and max for Angle values
    ui->intAngleDMSDeg->setMaximum(180);
    ui->intAngleDMSDeg->setMinimum(0);
    ui->intAngleDMSMin->setMaximum(59);
    ui->intAngleDMSMin->setMinimum(0);
    ui->doubleAngleDMSSec->setMaximum(59.99999999);
    ui->doubleAngleDMSSec->setMinimum(0);

    ui->doubleAngleDecDeg->setMaximum(180.0);
    ui->doubleAngleDecDeg->setMinimum(0.0);

    // Angle
    if (useDMS)
    {
        // show DMS controls
        ui->labelAngleDMS->show();
        ui->frameAngleDMS->show();
        // don't show btnAngleNegDMS because ZANG is always positive

        // set deg, min, src
        ui->intAngleDMSDeg->setValue(zAngle->angleDeg);
        ui->intAngleDMSMin->setValue(zAngle->angleMin);
        configureNumDigits(ui->doubleAngleDMSSec, currentLSAType, lsa::Q_UNITS_SOA);
        ui->doubleAngleDMSSec->setValue(zAngle->angleSec);

        if(!LSARecord::validateDMSBool(zAngle->angleDMSIsNeg,zAngle->angleDeg,zAngle->angleMin,zAngle->angleSec,0.0,180.0))
        {
            ui->labelAngleDMSWarning->show();
        }
        else
        {
            ui->labelAngleDMSWarning->hide();
        }
    }
    else
    {
        // show control and set decimal degree value
        ui->labelAngleDecDeg->show();
        ui->doubleAngleDecDeg->show();
        configureNumDigits(ui->doubleAngleDecDeg, currentLSAType, lsa::Q_UNITS_DEG);
        ui->doubleAngleDecDeg->setValue(zAngle->angleDecDeg);
    }

    // Sigma
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_RAD << lsa::Q_UNITS_DEG << lsa::Q_UNITS_SOA;
    ui->labelSigma->show();
    ui->frameSigma->show();
    QString sigmaUnits = QString::fromStdString(zAngle->sigmaUnits).toLower();
    configureNumDigits(ui->doubleSigma, currentLSAType, sigmaUnits);
    ui->doubleSigma->setValue(zAngle->sigma);
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(sigmaUnits);

    // HeightTo
    configureHeightTo();

    // HeightFrom
    configureHeightFrom();

    // Uncr modifier
    configureUNCRControls();

    // Refractive index correction
    configureRefractionControls();

    // Reduced Ellipsoid
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_REDUCED));
    ui->labelReduced->show();
    ui->checkReduced->show();
    ui->checkReduced->setChecked(currentModifier->isReducedToEllipsoid());

    // Variance Scaling
    configureVSCAControls();

    return;
}

void RecordEditor::configureVANG()
{
    LSAVAngle *vAngle = static_cast<LSAVAngle *>(currentLSARecord);
    currentModifier = &vAngle->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(vAngle->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // From
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(vAngle->From.c_str()));

    // To
    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText(lsa::Q_CONTROLVALUE_TO);
    ui->lineStationB->setText(QString(vAngle->To.c_str()));

    // DMS to Dec Deg radio buttons
    ui->frameUseDMS->show();
    useDMS = !vAngle->usesDecDeg;
    ui->radioUseDMS->setChecked(useDMS);
    ui->radioUseDecDeg->setChecked(!useDMS);

    //set gui min and max for Angle values
    ui->intAngleDMSDeg->setMaximum(90);
    ui->intAngleDMSDeg->setMinimum(0);
    ui->intAngleDMSMin->setMaximum(59);
    ui->intAngleDMSMin->setMinimum(0);
    ui->doubleAngleDMSSec->setMaximum(59.99999999);
    ui->doubleAngleDMSSec->setMinimum(0);

    ui->doubleAngleDecDeg->setMaximum(90.0);
    ui->doubleAngleDecDeg->setMinimum(-90.0);

    // Angle
    if (useDMS)
    {
        // show DMS controls
        ui->labelAngleDMS->show();
        ui->frameAngleDMS->show();
        ui->btnAngleNegDMS->show();

        // set deg, min, sec
        dmsAngleIsNeg = vAngle->angleDMSIsNeg;
        if(vAngle->angleDMSIsNeg)
            ui->btnAngleNegDMS->setText("-");
        else
           ui->btnAngleNegDMS->setText("+");
        ui->intAngleDMSDeg->setValue(vAngle->angleDeg);
        ui->intAngleDMSMin->setValue(vAngle->angleMin);
        configureNumDigits(ui->doubleAngleDMSSec, currentLSAType, lsa::Q_UNITS_SOA);
        ui->doubleAngleDMSSec->setValue(vAngle->angleSec);

        if(!LSARecord::validateDMSBool(vAngle->angleDMSIsNeg,vAngle->angleDeg,vAngle->angleMin,vAngle->angleSec,-90.0,90.0))
        {
            ui->labelAngleDMSWarning->show();
        }
        else
        {
            ui->labelAngleDMSWarning->hide();
        }
    }
    else
    {
        // show control and set decimal degree value
        ui->labelAngleDecDeg->show();
        ui->doubleAngleDecDeg->show();
        configureNumDigits(ui->doubleAngleDecDeg, currentLSAType, lsa::Q_UNITS_DEG);
        ui->doubleAngleDecDeg->setValue(vAngle->angleDecDeg);
    }

    // Sigma
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_RAD << lsa::Q_UNITS_DEG << lsa::Q_UNITS_SOA;
    ui->labelSigma->show();
    ui->frameSigma->show();
    QString sigmaUnits = QString::fromStdString(vAngle->sigmaUnits).toLower();
    configureNumDigits(ui->doubleSigma, currentLSAType, sigmaUnits);
    ui->doubleSigma->setValue(vAngle->sigma);
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(sigmaUnits);

    // HeightTo
    configureHeightTo();

    // HeightFrom
    configureHeightFrom();

    // Uncr modifier
    configureUNCRControls();

    // Refractive index correction
    configureRefractionControls();

    // Geoid
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_REDUCED));
    ui->labelReduced->show();
    ui->checkReduced->show();
    ui->checkReduced->setChecked(currentModifier->isReducedToEllipsoid());

    // Variance Scaling
    configureVSCAControls();

    return;
}

void RecordEditor::configureHANG()
{
    LSAHAngle *hAngle = static_cast<LSAHAngle *>(currentLSARecord);
    currentModifier = &hAngle->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(hAngle->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // From
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(hAngle->From.c_str()));

    // At
    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText(lsa::Q_CONTROLVALUE_AT);
    ui->lineStationB->setText(QString(hAngle->At.c_str()));

    // To
    ui->labelStationC->show();
    ui->lineStationC->show();
    ui->labelStationC->setText(lsa::Q_CONTROLVALUE_TO);
    ui->lineStationC->setText(QString(hAngle->To.c_str()));

    // DMS to Dec Deg radio buttons
    ui->frameUseDMS->show();
    useDMS = !hAngle->usesDecDeg;
    ui->radioUseDMS->setChecked(useDMS);
    ui->radioUseDecDeg->setChecked(!useDMS);

    //set gui min and max for Angle values
    ui->intAngleDMSDeg->setMaximum(359);
    ui->intAngleDMSDeg->setMinimum(0);
    ui->intAngleDMSMin->setMaximum(59);
    ui->intAngleDMSMin->setMinimum(0);
    ui->doubleAngleDMSSec->setMaximum(59.99999999);
    ui->doubleAngleDMSSec->setMinimum(0);

    ui->doubleAngleDecDeg->setMaximum(359.99999999);
    ui->doubleAngleDecDeg->setMinimum(-359.99999999);


    // Angle
    if (useDMS)
    {
        // show DMS controls
        ui->labelAngleDMS->show();
        ui->frameAngleDMS->show();
        ui->btnAngleNegDMS->show();

        // set deg, min, src
        dmsAngleIsNeg = hAngle->angleDMSIsNeg;
        if(hAngle->angleDMSIsNeg)
            ui->btnAngleNegDMS->setText("-");
        else
            ui->btnAngleNegDMS->setText("+");
        ui->intAngleDMSDeg->setValue(hAngle->angleDeg);
        ui->intAngleDMSMin->setValue(hAngle->angleMin);
        configureNumDigits(ui->doubleAngleDMSSec, currentLSAType, lsa::Q_UNITS_SOA);
        ui->doubleAngleDMSSec->setValue(hAngle->angleSec);
    }
    else
    {
        // show control and set decimal degree value
        ui->labelAngleDecDeg->show();
        ui->doubleAngleDecDeg->show();
        configureNumDigits(ui->doubleAngleDecDeg, currentLSAType, lsa::Q_UNITS_DEG);
        ui->doubleAngleDecDeg->setValue(hAngle->angleDecDeg);
    }

    // Sigma
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_RAD << lsa::Q_UNITS_DEG << lsa::Q_UNITS_SOA;
    ui->labelSigma->show();
    ui->frameSigma->show();
    QString sigmaUnits = QString::fromStdString(hAngle->sigmaUnits).toLower();
    configureNumDigits(ui->doubleSigma, currentLSAType, sigmaUnits);
    ui->doubleSigma->setValue(hAngle->sigma);
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(sigmaUnits);

    // HeightTo
    configureHeightTo();

    // HeightFrom
    configureHeightFrom();

    // Uncr modifier
    configureUNCRControls();

    // Geoid
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_REDUCED));
    ui->labelReduced->show();
    ui->checkReduced->show();
    ui->checkReduced->setChecked(currentModifier->isReducedToEllipsoid());

    // Variance Scaling
    configureVSCAControls();

    return;
}

void RecordEditor::configureAZIM()
{
try
{
    LSAAzimuth *azimuth = static_cast<LSAAzimuth *>(currentLSARecord);
    currentModifier = &azimuth->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(azimuth->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // From
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(azimuth->From.c_str()));

    // To
    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText(lsa::Q_CONTROLVALUE_TO);
    ui->lineStationB->setText(QString(azimuth->To.c_str()));

    // DMS to Dec Deg radio buttons
    useDMS = !azimuth->usesDecDeg;
    ui->frameUseDMS->show();
    ui->radioUseDMS->setChecked(useDMS);
    ui->radioUseDecDeg->setChecked(!useDMS);

    // angle
    // NOTE: use the latitude control so we can specify N|S
    ui->labelLatDMS->clear();
    ui->labelLatDMS->setText("Angle: ");
    ui->labelLatDecDeg->clear();
    ui->labelLatDecDeg->setText("Angle: ");

    //need to configure BOTH DecDeg AND DMS controls, or see issues when toggling DMZ<-->DecDeg
    configureNumDigits(ui->doubleLatDMSSec, currentLSAType, lsa::Q_UNITS_SOA);
    configureNumDigits(ui->doubleLatDecDeg, currentLSAType, lsa::Q_UNITS_DEG);

    //set gui min and max for Angle values
    ui->intLatDMSDeg->setMaximum(359);
    ui->intLatDMSDeg->setMinimum(0);
    ui->intLatDMSMin->setMaximum(59);
    ui->intLatDMSMin->setMinimum(0);
    ui->doubleLatDMSSec->setMaximum(59.99999999);
    ui->doubleLatDMSSec->setMinimum(0);

    //using Latitude control for N/S options
    ui->doubleLatDecDeg->setMaximum(359.99999999);
    ui->doubleLatDecDeg->setMinimum(-359.99999999);

    if (useDMS)
    {
        // show/hide controls
        ui->labelLatDecDeg->hide();
        ui->frameLatDecDeg->hide();
        ui->labelLatDMS->show();
        ui->frameLatDMS->show();
        ui->btnAngleNegDMS->show();

        // set deg, min, sec
        latDMSIsNeg = azimuth->angleDMSIsNeg;
        if(azimuth->angleDMSIsNeg)
           ui->btnLatDMSNeg->setText("-");
        else
           ui->btnLatDMSNeg->setText("+");

        // DMS angle
        ui->intLatDMSDeg->setValue(azimuth->angleDeg);
        ui->intLatDMSMin->setValue(azimuth->angleMin);
        ui->doubleLatDMSSec->setValue(azimuth->angleSec);

        // Angle direction
        QString latDir = QString::fromStdString(azimuth->fromDir);
        ui->comboLatDMSNS->setCurrentText(latDir);
    }
    else
    {
        // show/hide controls
        ui->labelLatDMS->hide();
        ui->frameLatDMS->hide();
        ui->labelLatDecDeg->show();
        ui->frameLatDecDeg->show();

        // Angle dec deg
        ui->doubleLatDecDeg->setValue(azimuth->angleDecDeg);

        // Angle dir
        QString latDir = QString::fromStdString(azimuth->fromDir);
        ui->comboLatDecDegNS->setCurrentText(latDir);
    }

    // Sigma
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_RAD << lsa::Q_UNITS_DEG << lsa::Q_UNITS_SOA;
    ui->labelSigma->show();
    ui->frameSigma->show();
    QString sigmaUnits = QString::fromStdString(azimuth->sigmaUnits).toLower();
    configureNumDigits(ui->doubleSigma, currentLSAType, sigmaUnits);
    ui->doubleSigma->setValue(azimuth->sigma);
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(sigmaUnits);

    // HeightTo
    configureHeightTo();

    // HeightFrom
    configureHeightFrom();

    // Uncr modifier
    configureUNCRControls();

    // Geoid
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_REDUCED));
    ui->labelReduced->show();
    ui->checkReduced->show();
    ui->checkReduced->setChecked(currentModifier->isReducedToEllipsoid());

    // Variance Scaling
    configureVSCAControls();

    return;
}
catch(...) { emit exceptionCaught(__FUNCTION__); }
}

void RecordEditor::configureHDIF()
{
    LSAHeightDiff *heightDiff = static_cast<LSAHeightDiff *>(currentLSARecord);
    currentModifier = &heightDiff->modifiers;

    bool isEllipsoidalHeight = currentModifier->isReducedToEllipsoid();

    // Record Description
    QString recordDescription = QString::fromStdString(heightDiff->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Station name
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_FROM);
    ui->lineStationA->setText(QString(heightDiff->From.c_str()));

    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText(lsa::Q_CONTROLVALUE_TO);
    ui->lineStationB->setText(QString(heightDiff->To.c_str()));

    // height difference
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_CM << lsa::Q_UNITS_FT;
    ui->labelValueA->show();
    ui->frameValueA->show();
    if (isEllipsoidalHeight)
        ui->labelValueA->setText("Ellips Hght Diff: ");
    else
        ui->labelValueA->setText("Ortho Hght Diff: ");
    QString linUnits = QString::fromStdString(heightDiff->linUnits).toLower();
    configureNumDigits(ui->doubleValueA, currentLSAType, linUnits);
    ui->doubleValueA->setValue(heightDiff->heightDiff);
    ui->comboValueAUnits->insertItems(0, allowedUnits);
    ui->comboValueAUnits->setCurrentText(linUnits);

    // Sigma
    ui->labelSigma->show();
    ui->frameSigma->show();
    QString sigmaUnits = QString::fromStdString(heightDiff->linUnits).toLower();
    configureNumDigits(ui->doubleSigma, currentLSAType, sigmaUnits);
    ui->doubleSigma->setValue(heightDiff->sigma);
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(sigmaUnits);

    // Uncr modifier
    configureUNCRControls();

    // Refractive index correction
    configureRefractionControls();

    // Reduced to Ellipsoid
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_REDUCED));
    ui->labelReduced->show();
    ui->checkReduced->show();
    ui->checkReduced->setChecked(currentModifier->isReducedToEllipsoid());
    if (isEllipsoidalHeight)
    {
        ui->labelHDIFEllipsoidal->show();
        ui->labelHDIFOrthometric->hide();
        ui->labelHDIFOutput->show();
    }
    else
    {
        ui->labelHDIFEllipsoidal->hide();
        ui->labelHDIFOrthometric->show();
        ui->labelHDIFOutput->show();
    }

    // Variance Scaling
    configureVSCAControls();

    // Curvature
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_CURV));
    ui->labelCurvature->show();
    ui->checkCurvature->show();
    ui->checkCurvature->setChecked(currentModifier->getCurvCorr() );

    // OHC
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_OHC));
    ui->labelOHC->show();
    ui->checkOHC->show();
    ui->checkOHC->setChecked(currentModifier->getOHC() );

    return;
}

void RecordEditor::configureDGRP()
{
    LSADirGroup *dirGroup = static_cast<LSADirGroup *>(currentLSARecord);
    currentModifier = &dirGroup->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(dirGroup->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Station name
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_LABEL);
    ui->lineStationA->setText(QString(dirGroup->label.c_str()));

    // From point
    ui->labelStationB->show();
    ui->labelStationB->setText(QString("From Point: "));
    ui->lineStationB->show();
    ui->lineStationB->setText(QString::fromStdString(dirGroup->fromLabel));

    // Uncr modifier
    configureUNCRControls();

    // Geoid
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_REDUCED));
    ui->labelReduced->show();
    ui->checkReduced->show();
    ui->checkReduced->setChecked(currentModifier->isReducedToEllipsoid());

    // Variance Scaling
    configureVSCAControls();

    return;
}

void RecordEditor::configureHDIR()
{
    LSAHDir *hDir = static_cast<LSAHDir *>(currentLSARecord);
    currentModifier = &hDir->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(hDir->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Dir group combo
    ui->labelDirGroup->show();
    ui->frameDirGroup->show();
    setupDirGroupControlLabels(hDir->dirGroupLabel);
    ui->comboDirGroup->setCurrentText(QString::fromStdString(hDir->dirGroupLabel));
    std::string fromStation = guiModel->getParentDGRPFromStation(hDir);
    ui->labelDirGroupValue->setText(QString::fromStdString(" (From Point: " + fromStation + ")"));

   // To point
    ui->labelStationB->show();
    ui->labelStationB->setText(QString("To Point: "));
    ui->lineStationB->show();
    ui->lineStationB->setText(QString::fromStdString(hDir->toLabel));

    // DMS to Dec Deg radio buttons
    ui->frameUseDMS->show();
    useDMS = !hDir->usesDecDeg;
    ui->radioUseDMS->setChecked(useDMS);
    ui->radioUseDecDeg->setChecked(!useDMS);

    //set gui min and max for Angle values
    ui->intAngleDMSDeg->setMaximum(359);
    ui->intAngleDMSDeg->setMinimum(0);
    ui->intAngleDMSMin->setMaximum(59);
    ui->intAngleDMSMin->setMinimum(0);
    ui->doubleAngleDMSSec->setMaximum(59.99999999);
    ui->doubleAngleDMSSec->setMinimum(0);

    ui->doubleAngleDecDeg->setMaximum(359.99999999);
    ui->doubleAngleDecDeg->setMinimum(-359.99999999);

    // Angle
    if (useDMS)
    {
        // show DMS controls
        ui->labelAngleDMS->show();
        ui->frameAngleDMS->show();
        ui->btnAngleNegDMS->show();

        // set deg, min, src
        dmsAngleIsNeg = hDir->angleDMSIsNeg;
        if(hDir->angleDMSIsNeg)
            ui->btnAngleNegDMS->setText("-");
        else
            ui->btnAngleNegDMS->setText("+");
        ui->intAngleDMSDeg->setValue(hDir->angleDeg);
        ui->intAngleDMSMin->setValue(hDir->angleMin);
        ui->doubleAngleDMSSec->setValue(hDir->angleSec);
    }
    else
    {
        // show control and set decimal degree value
        ui->labelAngleDecDeg->show();
        ui->doubleAngleDecDeg->show();
        ui->doubleAngleDecDeg->setValue(hDir->angleDecDeg);
    }

    // Sigma
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_RAD << lsa::Q_UNITS_DEG << lsa::Q_UNITS_SOA;
    ui->labelSigma->show();
    ui->frameSigma->show();
    QString sigmaUnits = QString::fromStdString(hDir->sigmaUnits).toLower();
    configureNumDigits(ui->doubleSigma, currentLSAType, sigmaUnits);
    ui->doubleSigma->setValue(hDir->sigma);
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(sigmaUnits);

    // HeightTo
    configureHeightTo();

    return;
}

void RecordEditor::configureUNCR()
{
    LSAUncertainty *lsaUncrtainty = static_cast<LSAUncertainty *>(currentLSARecord);
    currentModifier = NULL;

    // Record Description
    QString recordDescription = QString::fromStdString(lsaUncrtainty->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // UNCR help label
    ui->labelUncrHelp->show();

    // Label
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_LABEL);
    ui->lineStationA->setText(QString(lsaUncrtainty->label.c_str()));

    // Add sigma
    ui->labelSigma->show();
    ui->frameSigma->show();
    QString sigmaUnits = QString::fromStdString(lsaUncrtainty->sigmaUnits).toLower();
    configureNumDigits(ui->doubleSigma, currentLSAType, sigmaUnits);
    ui->doubleSigma->setValue(lsaUncrtainty->Sigma);
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_CM << lsa::Q_UNITS_FT<< lsa::Q_UNITS_RAD << lsa::Q_UNITS_DEG << lsa::Q_UNITS_SOA;
    ui->comboSigmaUnits->clear();
    ui->comboSigmaUnits->insertItems(0, allowedUnits);
    ui->comboSigmaUnits->setCurrentText(sigmaUnits);

    // PPM
    ui->labelValueD->show();
    ui->labelValueD->setText("PPM: ");
    ui->doubleValueD->show();
    // Set PPM precision directly as value is dimensionless
    ui->doubleValueD->setDecimals(lsa::NUM_DECIMALS_UNCR_PPM);
    ui->doubleValueD->setValue(lsaUncrtainty->PPM);
    ui->labelScaleWarning2->hide();

    // At centering error
    QString centUnits = QString::fromStdString(lsaUncrtainty->linUnits).toLower();
    QStringList allowedCentUnits;
    allowedCentUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_CM << lsa::Q_UNITS_FT;

    ui->labelValueA->show();
    ui->labelValueA->setText("At Cent Err:");
    ui->frameValueA->show();
    configureNumDigits(ui->doubleValueA, currentLSAType, centUnits);
    ui->doubleValueA->setValue(lsaUncrtainty->AtCenter);
    ui->comboValueAUnits->clear();
    ui->comboValueAUnits->insertItems(0, allowedCentUnits);
    ui->comboValueAUnits->setCurrentText(centUnits);

    // From centering error
    ui->labelValueB->show();
    ui->labelValueB->setText("From Cent Err:");
    ui->frameValueB->show();
    configureNumDigits(ui->doubleValueB, currentLSAType, centUnits);
    ui->doubleValueB->setValue(lsaUncrtainty->FromCenter);
    ui->comboValueBUnits->clear();
    ui->comboValueBUnits->insertItems(0, allowedCentUnits);
    ui->comboValueBUnits->setCurrentText(centUnits);

    // To centering error
    ui->labelValueC->show();
    ui->labelValueC->setText("To Cent Err:");
    ui->frameValueC->show();
    configureNumDigits(ui->doubleValueC, currentLSAType, centUnits);
    ui->doubleValueC->setValue(lsaUncrtainty->ToCenter);
    ui->comboValueCUnits->clear();
    ui->comboValueCUnits->insertItems(0, allowedCentUnits);
    ui->comboValueCUnits->setCurrentText(centUnits);

    return;
}

void RecordEditor::configureVSCA()
{
    LSAVarScaling *varScaling = static_cast<LSAVarScaling *>(currentLSARecord);
    currentModifier = NULL;

    // Record Description
    QString recordDescription = QString::fromStdString(varScaling->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Label
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_LABEL);
    ui->lineStationA->setText(QString(varScaling->label.c_str()));

    // Scale Factor
    ui->labelValueD->show();
    ui->labelValueD->setText("Scale Factor: ");
    ui->doubleValueD->show();
    ui->doubleValueD->setDecimals(lsa::NUM_DECIMALS_VARIANCE_SCALING);
    ui->doubleValueD->setValue(varScaling->varFactor);

    if(varScaling->varFactor < lsa::MIN_VARIANCE_SCALE || varScaling->varFactor > lsa::MAX_VARIANCE_SCALE )
    {
        ui->labelScaleWarning2->show();
    }
    else
    {
        ui->labelScaleWarning2->hide();
    }
}

void RecordEditor::configureHGHT()
{
    LSAHeight *height = static_cast<LSAHeight *>(currentLSARecord);
    currentModifier = NULL;

    // Record Description
    QString recordDescription = QString::fromStdString(height->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Label
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText(lsa::Q_CONTROLVALUE_LABEL);
    ui->lineStationA->setText(QString(height->label.c_str()));

    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_CM << lsa::Q_UNITS_FT;
    ui->comboValueAUnits->clear();
    ui->comboValueAUnits->insertItems(0, allowedUnits);
    QString heightUnits = QString::fromStdString(height->units).toLower();
    QString sigmaUnits = QString::fromStdString(height->sigmaUnits).toLower();
    ui->comboValueAUnits->setCurrentText(heightUnits);

    ui->labelValueA->show();
    ui->labelValueA->setText("Height: ");
    ui->frameValueA->show();
    configureNumDigits(ui->doubleValueA, currentLSAType, heightUnits);
    ui->doubleValueA->setValue(height->value);

    ui->comboValueBUnits->clear();
    ui->comboValueBUnits->insertItems(0, allowedUnits);
    ui->comboValueBUnits->setCurrentText(sigmaUnits);

    ui->labelValueB->show();
    ui->labelValueB->setText("Sigma: ");
    ui->frameValueB->show();
    configureNumDigits(ui->doubleValueB, currentLSAType, sigmaUnits);
    ui->doubleValueB->setValue(height->sigmaValue);
}

void RecordEditor::configureHeightTo()
{
    if (currentLSARecord == NULL || currentModifier == NULL)
        return;

    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_HEIGHTTO));

    ui->labelHeightTo->show();
    ui->frameHeightTo->show();
    setupHeightControlLabels(ui->comboHeightTo);

    if ( currentModifier->hasToHeight() )
    {
        std::string      heightLabel;
        double           heightValue;
        std::string      heightUnits;
        double           heightSigma;
        std::string heightSigmaUnits;
        bool               isNumeric;


        bool heightRecordFound = guiModel->getHeightToData(currentLSARecord, heightLabel, heightValue, heightUnits, heightSigma, heightSigmaUnits, isNumeric);

        ui->comboHeightTo->setToolTip("");
        if (heightRecordFound)
        {
            // If we found a height for the given height label, populate the value and units
            ui->doubleHeightToValue->setValue(heightValue);
            ui->comboHeightToUnits->setCurrentText(QString::fromStdString(heightUnits).toLower());
        }
        else if ( !heightLabel.empty() )
        {
            // If the height doesn't exist, add a warning icon and tooltip to the combobox
            ui->comboHeightTo->addItem(QIcon(":/guiIcons/alertIcon.png"),QString::fromStdString(heightLabel));
            ui->comboHeightTo->setToolTip("Invalid height.");
        }
        ui->comboHeightTo->setCurrentText(QString::fromStdString(heightLabel));

        ui->doubleHeightToValue->setEnabled(isNumeric); // allow direct edits to numeric values
        ui->comboHeightToUnits->setEnabled(isNumeric); // allow direct edits to numeric values

    }
    else
    {
        ui->doubleHeightToValue->setEnabled(false);
        ui->comboHeightToUnits->setEnabled(false);
    }

    return;
}

void RecordEditor::configureHeightFrom()
{
    if (currentLSARecord == NULL || currentModifier == NULL)
        return;

    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_HEIGHTFROM));

    ui->labelHeightFrom->show();
    ui->frameHeightFrom->show();
    setupHeightControlLabels(ui->comboHeightFrom);

    if ( currentModifier->hasFromHeight() )
    {
        std::string      heightLabel;
        double           heightValue;
        std::string      heightUnits;
        double           heightSigma;
        std::string heightSigmaUnits;
        bool               isNumeric;

        bool heightRecordFound = guiModel->getHeightFromData(currentLSARecord, heightLabel, heightValue, heightUnits, heightSigma, heightSigmaUnits, isNumeric);

        ui->comboHeightTo->setToolTip("");
        if (heightRecordFound)
        {
            ui->doubleHeightFromValue->setValue(heightValue);
            ui->comboHeightFromUnits->setCurrentText(QString::fromStdString(heightUnits).toLower());
        }
        else if ( !heightLabel.empty() )
        {
            // If the height doesn't exist, add a warning icon and tooltip to the combobox
            ui->comboHeightFrom->addItem(QIcon(":/guiIcons/alertIcon.png"),QString::fromStdString(heightLabel));
            ui->comboHeightFrom->setToolTip("Invalid height.");
        }
        ui->comboHeightFrom->setCurrentText(QString::fromStdString(heightLabel));

        ui->doubleHeightFromValue->setEnabled(isNumeric); // allow direct edits to numeric values
        ui->comboHeightFromUnits->setEnabled(isNumeric); // allow direct edits to numeric values
    }
    else
    {
        ui->doubleHeightFromValue->setEnabled(false);
        ui->comboHeightFromUnits->setEnabled(false);
    }

    return;
}

void RecordEditor::configureUNCRControls(std::string uncrLabel)
{
    // This method is can be called in multiple circumstances
    // 1) One record selected - currentLSARecord and currentModifier must not be null
    // 2) More than one record selected - uncrLabel must not be empty
    if (uncrLabel.empty() && (currentLSARecord == NULL || currentModifier == NULL))
        return;

    if (uncrLabel.empty())
    {
        uncrLabel = currentModifier->getUncrLabel();
    }

    setupUncrControlLabels(uncrLabel);
    ui->labelUncrModifier->show();
    ui->frameUncrModifier->show();

    if (uncrLabel.empty())
    {
        ui->comboUncrModifier->setCurrentText(QString::fromStdString(currentModifier->getUncrLabel() ));
    }
    else
    {
        ui->comboUncrModifier->setCurrentText(QString::fromStdString(uncrLabel));
    }

    int numSelectedRecords = guiModel->getSelectedRows().size();
    if (numSelectedRecords != 1)
    {
        ui->labelUncrValues->hide();
    }
    else
    {
        LSAUncertainty* uncrRecord= guiModel->getUNCRRecord(currentLSARecord);
        // Check into modifying here
        if (uncrRecord != NULL)
        {
            QStringList labelContents;
            QString atCenterStr, fromCenterStr, toCenterStr, ppmValueStr, addSigmaStr;

            uncrRecord->isAtCenterApplicable(currentLSAType) ?
                        atCenterStr = QString::number(uncrRecord->getAtCenterValue()) : atCenterStr = "N/A";
            uncrRecord->isFromCenterApplicable(currentLSAType) ?
                        fromCenterStr = QString::number(uncrRecord->getFromCenterValue()) : fromCenterStr = "N/A";
            uncrRecord->isToCenterApplicable(currentLSAType) ?
                        toCenterStr = QString::number(uncrRecord->getToCenterValue()) : toCenterStr = "N/A";
            uncrRecord->isPPMApplicable(currentLSAType) ?
                        ppmValueStr = QString::number(uncrRecord->getPPMValue()) : ppmValueStr = "N/A";
            uncrRecord->isSigmaApplicable(currentLSAType) ?
                        addSigmaStr = QString::number(uncrRecord->getAddSigmaValue()) : addSigmaStr = "N/A";

            labelContents << "<table border=\"1\" bordercolor=\"#dcdcdc\" cellspacing=\"0\" cellpadding=\"2\" width=\"250\">";
            labelContents <<    "<tr>";
            labelContents <<        "<th ALIGN=CENTER>At</th>";
            labelContents <<        "<th ALIGN=CENTER>From</th>";
            labelContents <<        "<th ALIGN=CENTER>To</th>";
            labelContents <<        "<th ALIGN=CENTER>PPM</th>";
            labelContents <<        "<th ALIGN=CENTER>Sigma</th>";
            labelContents <<    "</tr>";
            labelContents <<    "<tr>";
            labelContents <<        "<td ALIGN=CENTER>" << atCenterStr << "</td>";
            labelContents <<        "<td ALIGN=CENTER>" << fromCenterStr << "</td>";
            labelContents <<        "<td ALIGN=CENTER>" << toCenterStr << "</td>";
            labelContents <<        "<td ALIGN=CENTER>" << ppmValueStr << "</td>";
            labelContents <<        "<td ALIGN=CENTER>" << addSigmaStr << "</td>";
            labelContents <<    "</tr>";
            labelContents << "</table>";

            ui->labelUncrValues->setText(labelContents.join(""));
            ui->labelUncrValues->show();
        }
    }
}

void RecordEditor::configureINCLUDE()
{
    LSAInclude *lsaInclude(static_cast<LSAInclude*>(currentLSARecord));
    currentModifier = &lsaInclude->modifiers;

    // Record Description
    QString recordDescription = QString::fromStdString(lsaInclude->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Station name
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText("File:");
    ui->lineStationA->setText(QString(lsaInclude->getLSAPath().c_str()));
    ui->lineStationA->setReadOnly(true);
    ui->lineStationA->setEnabled(true);  // the control for the included file is read only

    // browse button
    ui->btnBrowse->show();

    // Allow variance Scaling on all includes except the root project include
    configureVSCAControls();

    if(lsaInclude->isAutogenerated)
    {
        ui->comboVSCA->setEnabled(false);
        ui->doubleVSCA->setEnabled(false);
        ui->btnBrowse->setEnabled(false);
    }
    else
    {
        ui->btnBrowse->setEnabled(true);
    }

    // Don't allow the root project include to be disabled
    if (guiModel->rootIncludeIsSelected())
    {
        ui->labelEnabled->hide();
        ui->checkEnabled->hide();
        ui->btnBrowse->setEnabled(false);
    }

    return;
}

void RecordEditor::configureCOMMENT()
{
    LSAComment *lsaComment(static_cast<LSAComment*>(currentLSARecord));

    // Record Description
    QString recordDescription = QString::fromStdString(lsaComment->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // hide checkEnabled
    ui->labelEnabled->hide();
    ui->checkEnabled->hide();

    // Comment contents
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText("Comment:");
     std::string commentToDisplay = lsaComment->lineContents;
    if (commentToDisplay.size() > 0)
    {
        commentToDisplay = commentToDisplay.substr(1); // strip leading #
    }
    ui->lineStationA->setText(QString::fromStdString(commentToDisplay));
    ui->lineStationA->setCursorPosition(0);

    // Parse Warnings
    if(lsaComment->hasParseWarnings())
    {
        int totalHeight = 0;
        std::vector<std::string> warnings = lsaComment->parseWarnings;
        QString warningsToDisplay;
        ui->labelParseWarningsList->clear();
        if(warnings.size() == 1)
        {
           warningsToDisplay = QString::fromStdString(warnings[0]);
        }
        else
        {
           for (size_t i = 0; i < warnings.size(); ++i)
           {
               warningsToDisplay += QString::fromStdString(warnings[i] + "\n");
           }
        }
        ui->labelParseWarnings->show();
        ui->labelParseWarningsList->setText(warningsToDisplay);
        ui->labelParseWarningsList->show();
    }
    return;
}

void RecordEditor::configureCONFIG()
{
    LSAConfig *lsaConfig(static_cast<LSAConfig*>(currentLSARecord));

    // Record Description
    QString recordDescription = QString::fromStdString(lsaConfig->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Config Type
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText("Config Type:");
    ui->lineStationA->setText(QString(lsaConfig->configType.c_str()));

    // Config Value
    ui->labelStationB->show();
    ui->lineStationB->show();
    ui->labelStationB->setText("Config Value:");
    ui->lineStationB->setText(QString(lsaConfig->configValue.c_str()));

    return;
}

void RecordEditor::configureMEAN()
{
    ui->labelPoints->show();

    LSAMean *lsaMean = static_cast<LSAMean*>(currentLSARecord);

    // Record Description
    QString recordDescription = QString::fromStdString(lsaMean->getRecType().getDescription());
    setWindowTitle(recordDescription);

    // Label
    ui->labelStationA->show();
    ui->lineStationA->show();
    ui->labelStationA->setText("Label: ");
    ui->lineStationA->setText(QString(lsaMean->label.c_str()));

    // Add sigma
    ui->labelSigmaMEAN->show();
    ui->frameSigmaMEAN->show();
    ui->doubleSigmaMEAN->setDecimals(4);
    QString linUnits = QString::fromStdString(lsaMean->linUnits).toLower();
    configureNumDigits(ui->doubleSigmaMEAN, currentLSAType, linUnits);
    ui->doubleSigmaMEAN->setValue(lsaMean->Sigma);
    QStringList allowedUnits;
    allowedUnits << lsa::Q_UNITS_M << lsa::Q_UNITS_CM << lsa::Q_UNITS_FT;
    ui->comboSigmaUnitsMEAN->clear();
    ui->comboSigmaUnitsMEAN->insertItems(0, allowedUnits);
    ui->comboSigmaUnitsMEAN->setCurrentText(linUnits);

    // Points
    ui->framePoints->show();
    int numPoints = lsaMean->points.size();
    QString textEditContents;
    for(int i = 0; i < numPoints; ++i)
    {
        textEditContents += QString::fromStdString(lsaMean->points[i]) + "\n";
    }
    ui->textInputPoints->setText(textEditContents);


    return;
}

void RecordEditor::configureRefractionControls()
{
    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_REFRACT));
    ui->labelRefractiveIndex->show();
    ui->frameRefraction->show();

    bool hasRefraction = currentModifier->hasRefractionCorrection();
    ui->checkApplyRefraction->setChecked(hasRefraction);

    ui->doubleRefractiveIndex->setValue(currentModifier->getRawRefract());
    ui->doubleRefractiveIndex->setEnabled(hasRefraction);

    return;
}

void RecordEditor::configureVSCAControls()
{
    if (currentLSARecord == NULL || currentModifier == NULL)
        return;

    Q_ASSERT(LSAModifier::supportsModifierControl(currentLSAType, MODKEY_VSCA));

    // var label
    ui->labelVSCA->show();

    // combobox
    std::string label = currentModifier->getVSCALabel();

    setupVSCAControlLabels( label );
    if ( !currentModifier->hasVSCACorrection())
    {
        ui->comboVSCA->setCurrentText(lsa::Q_CONTROLVALUE_NONE);
        ui->doubleVSCA->setDisabled(true);
    }
    else if ( currentModifier->hasNumericVSCA() )
    {
        ui->comboVSCA->setCurrentText(lsa::Q_CONTROLVALUE_VALUE);
        ui->doubleVSCA->setEnabled(true);
    }
    else
    {
        ui->comboVSCA->setCurrentText(QString::fromStdString( label ));
        ui->doubleVSCA->setDisabled(true);
    }

    // numeric controls
    ui->doubleVSCA->show();
    ui->doubleNetVSCA->show();
    ui->labelNetVarScaling->show();
    double scaleFactor = guiModel->getVSCAValue(currentLSARecord);
    ui->doubleVSCA->setValue(scaleFactor);
    double netScaleFactor = guiModel->getNetVSCAFactor(currentIndex);
    ui->doubleNetVSCA->setValue(netScaleFactor);

    if(!guiModel->hasValidScaling(currentIndex))
    {
        ui->labelScaleWarning->show();
    }
    else
    {
        ui->labelScaleWarning->hide();
    }

    return;
}

void RecordEditor::prepareToEditNewRecord()
{
    ui->lineStationA->setFocus();
}

void RecordEditor::prepareToEditSeparatorBar()
{
    // Set focus on the comment editor field
    ui->lineStationA->setFocus();

    // Programmatically select the small section of the bar so the user
    // can just start typing
    ui->lineStationA->setSelection(17, 1);
}

bool RecordEditor::hasPendingChanges()
{
    return  checkEnabled_Changed      ||
            lineStationA_Changed      ||
            lineStationB_Changed      ||
            lineStationC_Changed      ||
            comboDirGroup_Changed     ||
            btnLatDMSNeg_Changed      ||
            intLatDMSDeg_Changed      ||
            intLatDMSMin_Changed      ||
            doubleLatDMSSec_Changed   ||
            comboLatDMSNS_Changed     ||
            btnLonDMSNeg_Changed      ||
            intLonDMSDeg_Changed      ||
            intLonDMSMin_Changed      ||
            doubleLonDMSSec_Changed   ||
            comboLonDMSEW_Changed     ||
            doubleLatDecDeg_Changed   ||
            comboLatDecDegNS_Changed  ||
            doubleLonDecDeg_Changed   ||
            comboLonDecDegEW_Changed  ||
            btnAngleNegDMS_Changed    ||
            intAngleDMSDeg_Changed    ||
            intAngleDMSMin_Changed    ||
            doubleAngleDMSSec_Changed ||
            doubleAngleDecDeg_Changed ||
            useDMS_Changed            ||
            doubleValueA_Changed      ||
            comboValueAUnits_Changed  ||
            doubleValueB_Changed      ||
            comboValueBUnits_Changed  ||
            doubleValueC_Changed      ||
            comboValueCUnits_Changed  ||
            doubleValueA1_Changed     ||
            doubleValueB1_Changed     ||
            doubleValueC1_Changed     ||
            doubleSigma_Changed       ||
            comboSigmaUnits_Changed   ||
            doubleSigmaMEAN_Changed   ||
            comboSigmaUnitsMEAN_Changed   ||
            doubleCovAA_Changed       ||
            doubleCovAB_Changed       ||
            doubleCovAC_Changed       ||
            doubleCovBB_Changed       ||
            doubleCovBC_Changed       ||
            doubleCovCC_Changed       ||
            comboFixedFloat_Changed   ||
            comboPosgHeight_Changed   ||
            checkNorthFixed_Changed   ||
            checkEastFixed_Changed    ||
            checkUpFixed_Changed      ||
            comboHeightTo_Changed     ||
            doubleHeightToValue_Changed   ||
            comboHeightToUnits_Changed    ||
            comboHeightFrom_Changed       ||
            doubleHeightFromValue_Changed ||
            comboHeightFromUnits_Changed  ||
            comboUncrModifier_Changed     ||
            doubleRefractiveIndex_Changed ||
            checkApplyRefraction_Changed  ||
            comboVSCA_Changed         ||
            doubleVSCA_Changed        ||
            checkReduced_Changed        ||
            checkCurvature_Changed    ||
            checkOHC_Changed          ||
            textInputPoints_Changed    ||
            doubleValueD_Changed      ||
            generalTextNotes_Changed;
}

/***********************************************
 *
 *  Update LSARecord Methods
 *
 ***********************************************/

void RecordEditor::keyPressEvent(QKeyEvent* event)
{
    // if we pressed enter or return after editing a comment, give focus to the tree view
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        bool isComment = ui->lineStationA->hasFocus();

        pushChangesToGuiModel();

        if (isComment)
        {
            emit giveFocusToTreeView();
        }
    }
}

void RecordEditor::pushChangesToGuiModel()
{
    if (suppressRecordEditorUpdates) return;

    setParentIsModifiedFlag();

    bool posgControlEdited = posgControlChanged();  // need to cache this value before making updates

    guiModel->undoStack()->beginMacro("Data changing via the RE.");
    QList<QPersistentModelIndex> selectedIndices;
    foreach(QModelIndex index, selectedRows)
    {
        selectedIndices.push_back(QPersistentModelIndex(index));
    }
    foreach(QModelIndex index, selectedRows)
    {
        currentIndex = index;
        currentLSARecord = guiModel->getLSARecord(index);
        currentLSAType = currentLSARecord->getRecType();

        std::string oldValue = currentLSARecord->getSingleLSAString();

        // Push values common to all record types
        if (currentLSAType != LSAType::COMMENT)
        {
            if (checkEnabled_Changed)
            {
                currentLSARecord->isCommented = !ui->checkEnabled->isChecked();
            }
            if (generalTextNotes_Changed)
            {
                currentLSARecord->textNotes = ui->textNotes->toPlainText();
            }
        }

        // Verify that the project record is enabled (fix to bug 1569)
        if(guiModel->getLSARecord(guiModel->index(0,0))->isCommented)
        {
            guiModel->getLSARecord(guiModel->index(0,0))->isCommented = false;
        }

        // Push values for each record type
        if      (currentLSAType == LSAType::POSC)    pushChangesToPOSC();
        else if (currentLSAType == LSAType::POSG)    pushChangesToPOSG();
        else if (currentLSAType == LSAType::DXYZ)    pushChangesToDXYZ();
        else if (currentLSAType == LSAType::DIST)    pushChangesToDIST();
        else if (currentLSAType == LSAType::ZANG)    pushChangesToZANG();
        else if (currentLSAType == LSAType::VANG)    pushChangesToVANG();
        else if (currentLSAType == LSAType::HANG)    pushChangesToHANG();
        else if (currentLSAType == LSAType::AZIM)    pushChangesToAZIM();
        else if (currentLSAType == LSAType::HDIF)    pushChangesToHDIF();
        else if (currentLSAType == LSAType::COMMENT) pushChangesToCOMMENT();
        else if (currentLSAType == LSAType::CONFIG)  pushChangesToCONFIG();
        else if (currentLSAType == LSAType::DGRP)    pushChangesToDGRP();
        else if (currentLSAType == LSAType::HDIR)    pushChangesToHDIR();
        else if (currentLSAType == LSAType::UNCR)    pushChangesToUNCR();
        else if (currentLSAType == LSAType::VSCA)    pushChangesToVSCA();
        else if (currentLSAType == LSAType::HGHT)    pushChangesToHGHT();
        else if (currentLSAType == LSAType::INCLUDE) pushChangesToINCLUDE();
        else if (currentLSAType == LSAType::MEAN)    pushChangesToMEAN();
        else if (currentLSAType == LSAType::ENUO)    pushChangesToENUO();

        std::string newValue = currentLSARecord->getSingleLSAString();
        guiModel->pushChangeValue(currentIndex, selectedIndices, oldValue, newValue, false);
    }

    // Check for auto-generated points that have been edited
    if (posgControlEdited && !useDMS_Changed //don't make point permanent if only toggled DMS/DecDeg
                          && !lineStationA_Changed)// or changed name -- Fix to Bug #1314
    {
        checkForEditedAutoGenPoints();
    }

    resetChangeStateTrackers();

    guiModel->synchSelectedItemsToRecords();
    guiModel->undoStack()->endMacro();

    return;
}

void RecordEditor::checkForEditedAutoGenPoints()
{
    // Cache a list of persistent model indexes that wont change as we delete and insert records
    QList<QPersistentModelIndex> persistentSelectedRows;
    foreach(QModelIndex index, selectedRows)
    {
        persistentSelectedRows.push_back(QPersistentModelIndex(index));
    }

    // If any of the selected records are autogenerated, convert them to a normal point
    foreach(QPersistentModelIndex index, persistentSelectedRows)
    {
        LSARecord *lsaRecord = guiModel->getLSARecord(index);
        LSAType lsaType = lsaRecord->getRecType();

        if (lsaType == LSAType::POSG && lsaRecord->isAutogenerated)
            guiModel->convertAutogeneratedToNormalPoint(index);
    }
}

void RecordEditor::pushChangesToPOSC()
{
    LSAPosC *lsaPoint = static_cast<LSAPosC *>(currentLSARecord);
    currentModifier = &lsaPoint->modifiers;

    // station name
    if (lineStationA_Changed)
    {
        // update pointMap
        std::string oldLabel = lsaPoint->label;
        std::string newLabel = ui->lineStationA->text().trimmed().toStdString();
        modifyLabels(LSAType::POSC, QString::fromStdString(oldLabel), QString::fromStdString(newLabel));
        lsaPoint->label = newLabel;
    }

    // xyz position values
    if (doubleValueA_Changed) lsaPoint->x = ui->doubleValueA->value();
    if (doubleValueB_Changed) lsaPoint->y = ui->doubleValueB->value();
    if (doubleValueC_Changed) lsaPoint->z = ui->doubleValueC->value();

    // xyz units
    if (comboValueAUnits_Changed) lsaPoint->posUnits = ui->comboValueAUnits->currentText().toStdString();
    if (comboValueBUnits_Changed) lsaPoint->posUnits = ui->comboValueBUnits->currentText().toStdString();
    if (comboValueCUnits_Changed) lsaPoint->posUnits = ui->comboValueCUnits->currentText().toStdString();

    // covariance
    if (doubleCovAA_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covxx = ui->doubleCovAA->text().trimmed().toDouble();
    }
    if (doubleCovAB_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covxy = ui->doubleCovAB->text().trimmed().toDouble();
    }
    if (doubleCovAC_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covxz = ui->doubleCovAC->text().trimmed().toDouble();
    }
    if (doubleCovBB_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covyy = ui->doubleCovBB->text().trimmed().toDouble();
    }
    if (doubleCovBC_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covyz = ui->doubleCovBC->text().trimmed().toDouble();
    }
    if (doubleCovCC_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covzz = ui->doubleCovCC->text().trimmed().toDouble();
    }

    // constraints
    if (comboFixedFloat_Changed) lsaPoint->fixedState = LSAFixedState(ui->comboFixedFloat->currentText().toStdString());

    if (checkNorthFixed_Changed) lsaPoint->isNorthFixed = ui->checkNorthFixed->isChecked();
    if (checkEastFixed_Changed ) lsaPoint->isEastFixed  = ui->checkEastFixed->isChecked();
    if (checkUpFixed_Changed   ) lsaPoint->isUpFixed    = ui->checkUpFixed->isChecked();

    // Variance Scaling
    updateVSCA();

    return;
}
void RecordEditor::pushChangesToPOSG()
{
    LSAPosG *lsaPoint = static_cast<LSAPosG *>(currentLSARecord);
    currentModifier = &lsaPoint->modifiers;

    // station name
    if (lineStationA_Changed)
    {
        // update pointMap
        std::string oldLabel = lsaPoint->label;
        std::string newLabel = ui->lineStationA->text().trimmed().toStdString();
        modifyLabels(LSAType::POSG, QString::fromStdString(oldLabel), QString::fromStdString(newLabel));
        lsaPoint->label = newLabel;
    }

    // lat and lon
    if (useDMS_Changed) lsaPoint->useDecimalDegrees = !useDMS;
    if (useDMS)
    {
        // latDMS
        if (btnLatDMSNeg_Changed)    lsaPoint->latDMSIsNeg = latDMSIsNeg;
        if (intLatDMSDeg_Changed)    lsaPoint->latDeg = ui->intLatDMSDeg->value();
        if (intLatDMSMin_Changed)    lsaPoint->latMin = ui->intLatDMSMin->value();
        if (doubleLatDMSSec_Changed) lsaPoint->latSec = ui->doubleLatDMSSec->value();
        if (comboLatDMSNS_Changed)   lsaPoint->latDir = ui->comboLatDMSNS->currentText().toStdString();
        if (btnLatDMSNeg_Changed || intLatDMSDeg_Changed || intLatDMSMin_Changed || doubleLatDMSSec_Changed)//Fix to Bug #1212
        {
            std::set<std::string>::iterator iter;
            std::vector<std::string> warningMessages;
            for(iter=lsaPoint->warningMessages_persistent.begin(); iter!=lsaPoint->warningMessages_persistent.end(); iter++ )
            {
                if((*iter).find(" is out of bounds ")!=std::string::npos)
                {
                    lsaPoint->warningMessages_persistent.erase(iter);
                    break;
                }
            }
            if(!LSARecord::validateDMSBool(lsaPoint->latDMSIsNeg,lsaPoint->latDeg,lsaPoint->latMin,lsaPoint->latSec,lsa::MIN_LATITUDE_BOUNDARY,lsa::MAX_LATITUDE_BOUNDARY,&warningMessages))
            {
                lsaPoint->warningMessages_persistent.insert(warningMessages.at(0));
            }
        }

        // lonDMS
        if (btnLonDMSNeg_Changed)    lsaPoint->lonDMSIsNeg = lonDMSIsNeg;
        if (intLonDMSDeg_Changed)    lsaPoint->lonDeg = ui->intLonDMSDeg->value();
        if (intLonDMSMin_Changed)    lsaPoint->lonMin = ui->intLonDMSMin->value();
        if (doubleLonDMSSec_Changed) lsaPoint->lonSec = ui->doubleLonDMSSec->value();
        if (comboLonDMSEW_Changed)   lsaPoint->lonDir = ui->comboLonDMSEW->currentText().toStdString();
    }
    else
    {   // Lat and lon decDeg
       if (doubleLatDecDeg_Changed)
       {
            std::set<std::string>::iterator iter;
            lsaPoint->latDecDeg = ui->doubleLatDecDeg->value();
            for(iter=lsaPoint->warningMessages_persistent.begin(); iter!=lsaPoint->warningMessages_persistent.end(); iter++ )
            {
                if((*iter).find(" is out of bounds ")!=std::string::npos)
                {
                    lsaPoint->warningMessages_persistent.erase(iter);
                    break;
                }
            }
            if(lsaPoint->latDecDeg < lsa::MIN_LATITUDE_BOUNDARY || lsaPoint->latDecDeg > lsa::MAX_LATITUDE_BOUNDARY)
            {
                std::stringstream message;
                QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
                message << "Parse Warning - " << std::fixed << std::setprecision(qsettings.value(lsa::QSETTINGS_NUM_DECIMALS_ANGLE_MEASUREMENT_DEGREES, lsa::DEFAULT_NUM_DECIMALS_ANGLE_MEASUREMENT_DEGREES).toInt()) << lsaPoint->latDecDeg
                        << " is out of bounds (" << lsa::MIN_LATITUDE_BOUNDARY << ", " << lsa::MAX_LATITUDE_BOUNDARY << ").";
                lsaPoint->warningMessages_persistent.insert(message.str());
            }
       }
       if (comboLatDecDegNS_Changed)lsaPoint->latDir    = ui->comboLatDecDegNS->currentText().toStdString();
       if (doubleLonDecDeg_Changed) lsaPoint->lonDecDeg = ui->doubleLonDecDeg->value();
       if (comboLonDecDegEW_Changed)lsaPoint->lonDir    = ui->comboLonDecDegEW->currentText().toStdString();
    }

    // height
    if (doubleValueC_Changed) lsaPoint->height = ui->doubleValueC->value();
    if (comboValueCUnits_Changed) lsaPoint->heightUnits = ui->comboValueCUnits->currentText().toStdString();
    if(comboPosgHeight_Changed)
    {
        QString comboValHeightState = ui->comboPOSGHeight->currentText();
        lsaPoint->heightIsEllipsoidal = (comboValHeightState == lsa::Q_OPTION_ELLIPSOID);
    }

    // covariance
    if (doubleCovAA_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covnn = ui->doubleCovAA->text().trimmed().toDouble();
    }
    if (doubleCovAB_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covne = ui->doubleCovAB->text().trimmed().toDouble();
    }
    if (doubleCovAC_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covnu = ui->doubleCovAC->text().trimmed().toDouble();
    }
    if (doubleCovBB_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covee = ui->doubleCovBB->text().trimmed().toDouble();
    }
    if (doubleCovBC_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->coveu = ui->doubleCovBC->text().trimmed().toDouble();
    }
    if (doubleCovCC_Changed)
    {
        lsaPoint->hasCovariance = true;
        lsaPoint->covuu = ui->doubleCovCC->text().trimmed().toDouble();
    }

    // constraints
    if (comboFixedFloat_Changed) lsaPoint->fixedState = LSAFixedState(ui->comboFixedFloat->currentText().toStdString());

    if (checkNorthFixed_Changed) lsaPoint->isNorthFixed = ui->checkNorthFixed->isChecked();
    if (checkEastFixed_Changed ) lsaPoint->isEastFixed  = ui->checkEastFixed->isChecked();
    if (checkUpFixed_Changed   ) lsaPoint->isUpFixed    = ui->checkUpFixed->isChecked();

    // Variance Scaling
    updateVSCA();

    return;
}

bool RecordEditor::posgControlChanged()
{
    // A control change from DMS to DecDeg not a real change
    // VSCA modifiers have no effect on autogenerated points because they have no covariance
    bool labelChanged = lineStationA_Changed;
    bool latLonChanged = btnLatDMSNeg_Changed || intLatDMSDeg_Changed  || intLatDMSMin_Changed || doubleLatDMSSec_Changed ||
                         btnLonDMSNeg_Changed || intLonDMSDeg_Changed  || intLonDMSMin_Changed || doubleLonDMSSec_Changed ||
                         doubleLonDecDeg_Changed || doubleLatDecDeg_Changed ||
                         comboLatDMSNS_Changed || comboLonDMSEW_Changed   || comboLatDecDegNS_Changed || comboLonDecDegEW_Changed;
    bool heightChanged = doubleValueC_Changed || comboValueCUnits_Changed || comboPosgHeight_Changed;
    bool fixedStateChanged = checkNorthFixed_Changed || checkEastFixed_Changed || checkUpFixed_Changed || comboFixedFloat_Changed;

    return labelChanged || latLonChanged || heightChanged || fixedStateChanged;
}

void RecordEditor::pushChangesToDXYZ()
{
    LSADelta *lsaDelta = static_cast<LSADelta *>(currentLSARecord);
    currentModifier = &lsaDelta->modifiers;

    // From and To
    if (lineStationA_Changed) lsaDelta->From = ui->lineStationA->text().trimmed().toStdString();
    if (lineStationB_Changed) lsaDelta->To = ui->lineStationB->text().trimmed().toStdString();

    // delta values
    if (doubleValueA_Changed) lsaDelta->dx = ui->doubleValueA->value();
    if (doubleValueB_Changed) lsaDelta->dy = ui->doubleValueB->value();
    if (doubleValueC_Changed) lsaDelta->dz = ui->doubleValueC->value();

    // xyz units
    if (comboValueAUnits_Changed) lsaDelta->linUnits = ui->comboValueAUnits->currentText().toStdString();
    if (comboValueBUnits_Changed) lsaDelta->linUnits = ui->comboValueBUnits->currentText().toStdString();
    if (comboValueCUnits_Changed) lsaDelta->linUnits = ui->comboValueCUnits->currentText().toStdString();

    // covariance
    if (doubleCovAA_Changed) lsaDelta->covxx = ui->doubleCovAA->text().trimmed().toDouble();
    if (doubleCovAB_Changed) lsaDelta->covxy = ui->doubleCovAB->text().trimmed().toDouble();
    if (doubleCovAC_Changed) lsaDelta->covxz = ui->doubleCovAC->text().trimmed().toDouble();
    if (doubleCovBB_Changed) lsaDelta->covyy = ui->doubleCovBB->text().trimmed().toDouble();
    if (doubleCovBC_Changed) lsaDelta->covyz = ui->doubleCovBC->text().trimmed().toDouble();
    if (doubleCovCC_Changed) lsaDelta->covzz = ui->doubleCovCC->text().trimmed().toDouble();

    // covariance diagonal as ENU
    LSARecord* refPoint = guiModel->getPointWithLabel(lsaDelta->From);
    gnsstk::Matrix<double> covenu;
    if(refPoint)
    {
        gnsstk::Triple lonlat = getLonLatHeightRads(refPoint);
        gnsstk::Matrix<double> covMatrix(3, 3);
        covMatrix(0,0) = lsaDelta->covxx; covMatrix(0,1) = lsaDelta->covxy; covMatrix(0,2) = lsaDelta->covxz;
        covMatrix(1,0) = lsaDelta->covxy; covMatrix(1,1) = lsaDelta->covyy; covMatrix(1,2) = lsaDelta->covyz;
        covMatrix(2,0) = lsaDelta->covxz; covMatrix(2,1) = lsaDelta->covyz; covMatrix(2,2) = lsaDelta->covzz;
        covenu = convertECEFtoENU({lonlat[0], lonlat[1]}, covMatrix);

        double xx = std::abs(covenu[0][0]);
        double yy = std::abs(covenu[1][1]);
        double zz = std::abs(covenu[2][2]);
        updateDYXZ_CovENU(std::sqrt(xx), std::sqrt(yy), std::sqrt(zz));
    }

    // Height TO
    updateHeightTo();

    // HeightFrom
    updateHeightFrom();

    // Uncr modifier
    updateUNCR();

    // Variance Scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToENUO()
{
    LSAEnuo *lsaEnuo = static_cast<LSAEnuo *>(currentLSARecord);

    // From and To
    if (lineStationA_Changed) lsaEnuo->From = ui->lineStationA->text().trimmed().toStdString();
    if (lineStationB_Changed) lsaEnuo->To = ui->lineStationB->text().trimmed().toStdString();

    // delta values
    if (doubleValueA_Changed) lsaEnuo->de = ui->doubleValueA->value();
    if (doubleValueB_Changed) lsaEnuo->dn = ui->doubleValueB->value();
    if (doubleValueC_Changed) lsaEnuo->du = ui->doubleValueC->value();

    // enu units
    if (comboValueAUnits_Changed) lsaEnuo->linUnits = ui->comboValueAUnits->currentText().toStdString();
    if (comboValueBUnits_Changed) lsaEnuo->linUnits = ui->comboValueBUnits->currentText().toStdString();
    if (comboValueCUnits_Changed) lsaEnuo->linUnits = ui->comboValueCUnits->currentText().toStdString();

    // sigmas
    if (doubleValueA1_Changed) lsaEnuo->se = ui->doubleValueA1->value();
    if (doubleValueB1_Changed) lsaEnuo->sn = ui->doubleValueB1->value();
    if (doubleValueC1_Changed) lsaEnuo->su = ui->doubleValueC1->value();

    return;
}

void RecordEditor::pushChangesToDIST()
{
    LSADist *lsaDist = static_cast<LSADist *>(currentLSARecord);
    currentModifier = &lsaDist->modifiers;

    // From and To
    if (lineStationA_Changed) lsaDist->From = ui->lineStationA->text().trimmed().toStdString();
    if (lineStationB_Changed) lsaDist->To = ui->lineStationB->text().trimmed().toStdString();

    // distance
    if (doubleValueA_Changed) lsaDist->distance = ui->doubleValueA->value();

    // Sigma
    if (doubleSigma_Changed)     lsaDist->sigma = ui->doubleSigma->value();
    if (comboSigmaUnits_Changed)
    {
        lsaDist->linUnits = ui->comboSigmaUnits->currentText().toStdString();
        if(ui->comboSigmaUnits->currentText()!=ui->comboValueAUnits->currentText())
            ui->comboValueAUnits->setCurrentText(ui->comboSigmaUnits->currentText());
    }
    if (comboValueAUnits_Changed)
    {
        lsaDist->linUnits = ui->comboValueAUnits->currentText().toStdString();
        if(ui->comboSigmaUnits->currentText()!=ui->comboValueAUnits->currentText())
            ui->comboSigmaUnits->setCurrentText(ui->comboValueAUnits->currentText());
    }

    // Height TO
    updateHeightTo();

    // HeightFrom
    updateHeightFrom();

    // Uncr modifier
    updateUNCR();

    // Variance Scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToZANG()
{
    LSAZAngle *zAngle = static_cast<LSAZAngle *>(currentLSARecord);
    currentModifier = &zAngle->modifiers;

    // From
    if (lineStationA_Changed) zAngle->From = ui->lineStationA->text().trimmed().toStdString();

    // To
    if (lineStationB_Changed) zAngle->To = ui->lineStationB->text().trimmed().toStdString();

    // Angle
    if (useDMS_Changed) zAngle->usesDecDeg = !useDMS;
    if (useDMS)
    {
        // DMS
        if (intAngleDMSDeg_Changed)    zAngle->angleDeg = ui->intAngleDMSDeg->value();
        if (intAngleDMSMin_Changed)    zAngle->angleMin = ui->intAngleDMSMin->value();
        if (doubleAngleDMSSec_Changed) zAngle->angleSec = ui->doubleAngleDMSSec->value();
        if (intAngleDMSDeg_Changed || intAngleDMSMin_Changed || doubleAngleDMSSec_Changed)//Fix to Bug #1212
        {
            std::vector<std::string> warningMessages;
            std::set<std::string>::iterator iter;
            for(iter=zAngle->warningMessages_persistent.begin(); iter!=zAngle->warningMessages_persistent.end(); iter++ )
            {
                if((*iter).find(" is out of bounds ")!=std::string::npos)
                {
                    zAngle->warningMessages_persistent.erase(iter);
                    break;
                }
            }
            if(!LSARecord::validateDMSBool(zAngle->angleDMSIsNeg,zAngle->angleDeg,zAngle->angleMin,zAngle->angleSec,0.0,180.0,&warningMessages))
            {
                zAngle->warningMessages_persistent.insert(warningMessages.at(0));
            }
        }
    }
    else
    {   // Dec Deg
       if (doubleAngleDecDeg_Changed) zAngle->angleDecDeg = ui->doubleAngleDecDeg->value();
    }

    // Sigma
    if (doubleSigma_Changed)     zAngle->sigma = ui->doubleSigma->value();
    if (comboSigmaUnits_Changed) zAngle->sigmaUnits = ui->comboSigmaUnits->currentText().toStdString();

    // Height TO
    updateHeightTo();

    // HeightFrom
    updateHeightFrom();

    // Uncr modifier
    updateUNCR();

    // Refractive index
    if (checkApplyRefraction_Changed)  currentModifier->enableRefract(ui->checkApplyRefraction->isChecked());
    if (doubleRefractiveIndex_Changed) currentModifier->setRefract(ui->doubleRefractiveIndex->value());

    // Reduced
    if (checkReduced_Changed) currentModifier->setIsReduced(ui->checkReduced->isChecked());

    // Variance Scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToVANG()
{
    LSAVAngle *vAngle = static_cast<LSAVAngle *>(currentLSARecord);
    currentModifier = &vAngle->modifiers;

    // From
    if (lineStationA_Changed) vAngle->From = ui->lineStationA->text().trimmed().toStdString();

    // To
    if (lineStationB_Changed) vAngle->To = ui->lineStationB->text().trimmed().toStdString();

    // Angle
    if (useDMS_Changed) vAngle->usesDecDeg = !useDMS;
    if (useDMS)
    {
        // DMS
        if (btnAngleNegDMS_Changed)    vAngle->angleDMSIsNeg = dmsAngleIsNeg;
        if (intAngleDMSDeg_Changed)    vAngle->angleDeg = ui->intAngleDMSDeg->value();
        if (intAngleDMSMin_Changed)    vAngle->angleMin = ui->intAngleDMSMin->value();
        if (doubleAngleDMSSec_Changed) vAngle->angleSec = ui->doubleAngleDMSSec->value();
        if (btnAngleNegDMS_Changed || intAngleDMSDeg_Changed || intAngleDMSMin_Changed || doubleAngleDMSSec_Changed)//Fix to Bug #1212
        {
            std::vector<std::string> warningMessages;
            std::set<std::string>::iterator iter;
            for(iter=vAngle->warningMessages_persistent.begin(); iter!=vAngle->warningMessages_persistent.end(); iter++ )
            {
                if((*iter).find(" is out of bounds ")!=std::string::npos)
                {
                    vAngle->warningMessages_persistent.erase(iter);
                    break;
                }
            }
            if(!LSARecord::validateDMSBool(vAngle->angleDMSIsNeg,vAngle->angleDeg,vAngle->angleMin,vAngle->angleSec,-90.0,90.0,&warningMessages))
            {
                vAngle->warningMessages_persistent.insert(warningMessages.at(0));
            }
        }
    }
    else
    {   // Dec Deg
       if (doubleAngleDecDeg_Changed) vAngle->angleDecDeg = ui->doubleAngleDecDeg->value();
    }

    // Sigma
    if (doubleSigma_Changed)     vAngle->sigma = ui->doubleSigma->value();
    if (comboSigmaUnits_Changed) vAngle->sigmaUnits = ui->comboSigmaUnits->currentText().toStdString();

    // Height TO
    updateHeightTo();

    // HeightFrom
    updateHeightFrom();

    // Uncr modifier
    updateUNCR();

    // Refractive index
    if (checkApplyRefraction_Changed)  currentModifier->enableRefract(ui->checkApplyRefraction->isChecked());
    if (doubleRefractiveIndex_Changed) currentModifier->setRefract(ui->doubleRefractiveIndex->value());

    // Geoid
    if (checkReduced_Changed) currentModifier->setIsReduced(ui->checkReduced->isChecked());

    // Variance Scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToHANG()
{
    LSAHAngle *hAngle = static_cast<LSAHAngle *>(currentLSARecord);
    currentModifier = &hAngle->modifiers;

    // From
    if (lineStationA_Changed) hAngle->From = ui->lineStationA->text().trimmed().toStdString();

    // At
    if (lineStationB_Changed) hAngle->At = ui->lineStationB->text().trimmed().toStdString();

    // To
    if (lineStationC_Changed) hAngle->To = ui->lineStationC->text().trimmed().toStdString();

    // Angle
    if (useDMS_Changed) hAngle->usesDecDeg = !useDMS;
    if (useDMS)
    {
        // DMS
        if (btnAngleNegDMS_Changed)    hAngle->angleDMSIsNeg = dmsAngleIsNeg;
        if (intAngleDMSDeg_Changed)    hAngle->angleDeg = ui->intAngleDMSDeg->value();
        if (intAngleDMSMin_Changed)    hAngle->angleMin = ui->intAngleDMSMin->value();
        if (doubleAngleDMSSec_Changed) hAngle->angleSec = ui->doubleAngleDMSSec->value();
    }
    else
    {   // Dec Deg
       if (doubleAngleDecDeg_Changed) hAngle->angleDecDeg = ui->doubleAngleDecDeg->value();
    }

    // Sigma
    if (doubleSigma_Changed)     hAngle->sigma = ui->doubleSigma->value();
    if (comboSigmaUnits_Changed) hAngle->sigmaUnits = ui->comboSigmaUnits->currentText().toStdString();

    // Height TO
    updateHeightTo();

    // HeightFrom
    updateHeightFrom();

    // Uncr modifier
    updateUNCR();

    // Reduced
    if (checkReduced_Changed) currentModifier->setIsReduced(ui->checkReduced->isChecked());

    // Variance Scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToAZIM()
{
    LSAAzimuth *azimuth = static_cast<LSAAzimuth *>(currentLSARecord);
    currentModifier = &azimuth->modifiers;

    // From
    if (lineStationA_Changed) azimuth->From = ui->lineStationA->text().trimmed().toStdString();

    // To
    if (lineStationB_Changed) azimuth->To = ui->lineStationB->text().trimmed().toStdString();

    // Angle
    if (useDMS_Changed) azimuth->usesDecDeg = !useDMS;
    if (useDMS)
    {
        // DMS
        if (btnLatDMSNeg_Changed)  azimuth->angleDMSIsNeg = latDMSIsNeg;
        if (intLatDMSDeg_Changed)    azimuth->angleDeg = ui->intLatDMSDeg->value();
        if (intLatDMSMin_Changed)    azimuth->angleMin = ui->intLatDMSMin->value();
        if (doubleLatDMSSec_Changed) azimuth->angleSec = ui->doubleLatDMSSec->value();
    }
    else
    {   // Dec Deg
        if (doubleLatDecDeg_Changed) azimuth->angleDecDeg = ui->doubleLatDecDeg->value();
    }
    if (comboLatDMSNS_Changed)
    {
        azimuth->fromDir = ui->comboLatDMSNS->currentText().toStdString();
    }
    if (comboLatDecDegNS_Changed)
    {
        azimuth->fromDir = ui->comboLatDecDegNS->currentText().toStdString();
        double newDegree = azimuth->angleDecDeg + 180.0;
        if(newDegree>360.0) newDegree -= 360.0;
        ui->doubleLatDecDeg->setValue(newDegree);
    }

    // Sigma
    if (doubleSigma_Changed)     azimuth->sigma = ui->doubleSigma->value();
    if (comboSigmaUnits_Changed) azimuth->sigmaUnits = ui->comboSigmaUnits->currentText().toStdString();

    // Height TO
    updateHeightTo();

    // HeightFrom
    updateHeightFrom();

    // Uncr modifier
    updateUNCR();

    // Reduced
    if (checkReduced_Changed) currentModifier->setIsReduced(ui->checkReduced->isChecked());

    // Variance Scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToCOMMENT()
{
    LSAComment *lsaCOMMENT(static_cast<LSAComment*>(currentLSARecord));

    // Comment contents
    if (lineStationA_Changed)
    {
        std::string commentToPush = "#" + ui->lineStationA->text().toStdString();
        lsaCOMMENT->lineContents = commentToPush;
        // Check if parse warnings were fixed
        if(lsaCOMMENT->hasParseWarnings())
        {
            resetChangeStateTrackers();

            guiModel->synchSelectedItemsToRecords();
            emit reloadAfterParseWarning();
        }
    }

    return;
}

void RecordEditor::pushChangesToCONFIG()
{
    LSAConfig *lsaConfig(static_cast<LSAConfig*>(currentLSARecord));

    // Comment contents
    if (lineStationA_Changed) lsaConfig->configType = ui->lineStationA->text().trimmed().toStdString();
    if (lineStationB_Changed) lsaConfig->configValue = ui->lineStationB->text().trimmed().toStdString();

    return;
}

void RecordEditor::pushChangesToHDIF()
{
    LSAHeightDiff *heightDiff = static_cast<LSAHeightDiff *>(currentLSARecord);
    currentModifier = &heightDiff->modifiers;

    // From and To
    if (lineStationA_Changed) heightDiff->From = ui->lineStationA->text().trimmed().toStdString();
    if (lineStationB_Changed) heightDiff->To = ui->lineStationB->text().trimmed().toStdString();

    // distance
    if (doubleValueA_Changed) heightDiff->heightDiff = ui->doubleValueA->value();

    // Sigma
    if (doubleSigma_Changed)     heightDiff->sigma = ui->doubleSigma->value();
    if (comboSigmaUnits_Changed)
    {
        heightDiff->linUnits = ui->comboSigmaUnits->currentText().toStdString();
        if(ui->comboSigmaUnits->currentText()!=ui->comboValueAUnits->currentText())
            ui->comboValueAUnits->setCurrentText(ui->comboSigmaUnits->currentText());
    }
    if (comboValueAUnits_Changed)
    {
        heightDiff->linUnits = ui->comboValueAUnits->currentText().toStdString();
        if(ui->comboSigmaUnits->currentText()!=ui->comboValueAUnits->currentText())
            ui->comboSigmaUnits->setCurrentText(ui->comboValueAUnits->currentText());
    }

    // Uncr modifier
    updateUNCR();

    // Refractive index
    if (checkApplyRefraction_Changed)  currentModifier->enableRefract(ui->checkApplyRefraction->isChecked());
    if (doubleRefractiveIndex_Changed) currentModifier->setRefract(ui->doubleRefractiveIndex->value());

    // Reduced
    if (checkReduced_Changed) currentModifier->setIsReduced(ui->checkReduced->isChecked());

    // Variance Scaling
    updateVSCA();

    // Curvature
    if (checkCurvature_Changed) currentModifier->setCurvCorr(ui->checkCurvature->isChecked());

    // OHC
    if (checkOHC_Changed) currentModifier->setOHC(ui->checkOHC->isChecked());

    return;
}

void RecordEditor::pushChangesToDGRP()
{
    LSADirGroup *dirGroup = static_cast<LSADirGroup *>(currentLSARecord);
    currentModifier = &dirGroup->modifiers;

    // label
    if (lineStationA_Changed)
    {
        QString oldLabel = QString::fromStdString(dirGroup->label);
        QString newLabel = ui->lineStationA->text().trimmed();

        modifyLabels(LSAType::DGRP, oldLabel, newLabel);
        dirGroup->label = newLabel.toStdString();
    }

    // At point
    if (lineStationB_Changed) dirGroup->fromLabel = ui->lineStationB->text().trimmed().toStdString();

    // Uncr modifier
    updateUNCR();

    // Reduced
    if (checkReduced_Changed) currentModifier->setIsReduced(ui->checkReduced->isChecked());

    // Variance Scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToHDIR()
{
    LSAHDir *hDir = static_cast<LSAHDir *>(currentLSARecord);
    currentModifier = &hDir->modifiers;

    // dir group
    if (comboDirGroup_Changed) hDir->dirGroupLabel = ui->comboDirGroup->currentText().toStdString();

    // To point
    if (lineStationB_Changed) hDir->toLabel = ui->lineStationB->text().trimmed().toStdString();

    // Angle
    if (useDMS_Changed) hDir->usesDecDeg = !useDMS;
    if (useDMS)
    {
        // DMS
        if (btnAngleNegDMS_Changed)    hDir->angleDMSIsNeg = dmsAngleIsNeg;
        if (intAngleDMSDeg_Changed)    hDir->angleDeg = ui->intAngleDMSDeg->value();
        if (intAngleDMSMin_Changed)    hDir->angleMin = ui->intAngleDMSMin->value();
        if (doubleAngleDMSSec_Changed) hDir->angleSec = ui->doubleAngleDMSSec->value();
    }
    else
    {   // Dec Deg
       if (doubleAngleDecDeg_Changed) hDir->angleDecDeg = ui->doubleAngleDecDeg->value();
    }

    // Sigma
    if (doubleSigma_Changed)     hDir->sigma = ui->doubleSigma->value();
    if (comboSigmaUnits_Changed) hDir->sigmaUnits = ui->comboSigmaUnits->currentText().toStdString();

    // Height TO
    updateHeightTo();

    return;
}

void RecordEditor::pushChangesToHGHT()
{
    LSAHeight *height = static_cast<LSAHeight *>(currentLSARecord);
    currentModifier = NULL;

    // Label
    if (lineStationA_Changed)
    {
        QString oldLabel = QString::fromStdString(height->label);
        QString newLabel = ui->lineStationA->text().trimmed();

        modifyLabels(LSAType::HGHT, oldLabel, newLabel);
        height->label = newLabel.toStdString();
    }

    // height value
    if (doubleValueA_Changed) height->value = ui->doubleValueA->value();

    // height units
    if (comboValueAUnits_Changed) height->units = ui->comboValueAUnits->currentText().toStdString();

    // sigma value
    if (doubleValueB_Changed) height->sigmaValue = ui->doubleValueB->value();

    // sigma units
    if (comboValueBUnits_Changed) height->sigmaUnits = ui->comboValueBUnits->currentText().toStdString();

}

void RecordEditor::pushChangesToUNCR()
{
    LSAUncertainty *sigma = static_cast<LSAUncertainty *>(currentLSARecord);
    currentModifier = NULL;

    // label
    if (lineStationA_Changed)
    {
        // update uncrMap
        QString oldLabel = QString::fromStdString(sigma->label);
        QString newLabel = ui->lineStationA->text().trimmed();

        modifyLabels(LSAType::UNCR, oldLabel, newLabel);
        sigma->label = newLabel.toStdString();
    }

    // Additional sigma
    if (doubleSigma_Changed)
    {
        sigma->Sigma = ui->doubleSigma->value();
        sigma->hasAddSigma = true;
    }
    if (comboSigmaUnits_Changed) sigma->sigmaUnits = ui->comboSigmaUnits->currentText().toStdString();

    // PPM
    if (doubleValueD_Changed)
    {
        sigma->PPM = ui->doubleValueD->value();
        sigma->hasPPM = true;
    }

    // At centering error
    if (doubleValueA_Changed)
    {
        sigma->AtCenter = ui->doubleValueA->value();
        sigma->hasAtCenter = true;
    }

    if (comboValueAUnits_Changed) sigma->linUnits = ui->comboValueAUnits->currentText().toStdString();


    // From centering error
    if (doubleValueB_Changed)
    {
        sigma->FromCenter = ui->doubleValueB->value();
        sigma->hasFromCenter = true;
    }

    if (comboValueBUnits_Changed) sigma->linUnits = ui->comboValueBUnits->currentText().toStdString();


    // To centering error
    if (doubleValueC_Changed)
    {
        sigma->ToCenter = ui->doubleValueC->value();
        sigma->hasToCenter = true;
    }

    if (comboValueCUnits_Changed) sigma->linUnits = ui->comboValueCUnits->currentText().toStdString();

    return;
}

void RecordEditor::pushChangesToVSCA()
{
    LSAVarScaling *varScaling = static_cast<LSAVarScaling *>(currentLSARecord);
    currentModifier = NULL;

    // Label
    if (lineStationA_Changed)
    {
        // update varScalingMap
        QString oldLabel = QString::fromStdString(varScaling->label);
        QString newLabel = ui->lineStationA->text().trimmed();

        modifyLabels(LSAType::VSCA, oldLabel, newLabel);
        varScaling->label = newLabel.toStdString();
    }

    // scaling factor
    if (doubleValueD_Changed) varScaling->varFactor = ui->doubleValueD->value();

    return;
}

void RecordEditor::pushChangesToINCLUDE()
{
    LSAInclude *lsaInclude = static_cast<LSAInclude *>(currentLSARecord);
    currentModifier = &lsaInclude->modifiers;

    // We don't support changing the filename in the record editor

    // Variance Scaling
    if (comboVSCA_Changed)
    {
        bool editingRootInclude = ( currentIndex == guiModel->index(0,0) );
        if (editingRootInclude)
        {
            // Remove toParent flag from the old vsca record
            std::string oldVSCALabel = lsaInclude->modifiers.getVSCALabel();
            LSAVSCAMap::iterator oldVSCAIter = varMap->find(oldVSCALabel);
            if ( oldVSCAIter != varMap->end() )
            {
                LSAVarScaling* oldVSCA = oldVSCAIter->second;
                oldVSCA->isAppliedToParent = false;
            }

            // Set the toParent flag on the new vsca record
            std::string newVSCALabel = ui->comboVSCA->currentText().toStdString();
            LSAVSCAMap::iterator newVSCAIter = varMap->find(newVSCALabel);
            if ( newVSCAIter != varMap->end() )
            {
                LSAVarScaling* newVSCA = newVSCAIter->second;
                newVSCA->isAppliedToParent = true;
            }
        }


    }
    // variance scaling
    updateVSCA();

    return;
}

void RecordEditor::pushChangesToMEAN()
{
    LSAMean *lsaMean = static_cast<LSAMean *>(currentLSARecord);

    // label
    if (lineStationA_Changed) lsaMean->label = ui->lineStationA->text().trimmed().toStdString();

    // Additional sigma
    if (doubleSigmaMEAN_Changed)
    {
        lsaMean->Sigma = ui->doubleSigmaMEAN->value();
    }
    if (comboSigmaUnitsMEAN_Changed)
    {
        lsaMean->linUnits = ui->comboSigmaUnitsMEAN->currentText().toStdString();
    }
    // Points
    if (textInputPoints_Changed)
    {
        QString textEditContents = ui->textInputPoints->toPlainText();
        QStringList contents = textEditContents.split("\n");

        lsaMean->points.clear();
        foreach (QString point, contents)
        {
            if (point.isEmpty())
                continue;

            lsaMean->points.push_back(point.trimmed().toStdString());
        }
    }

    return;
}


void RecordEditor::updateHeightTo()
{
    if (comboHeightTo_Changed || doubleHeightToValue_Changed || comboHeightToUnits_Changed)
    {
        std::string label = ui->comboHeightTo->currentText().toStdString();
        double value = ui->doubleHeightToValue->value();
        std::string units = ui->comboHeightToUnits->currentText().toStdString();
        currentModifier->setHeightToData(label, value, units);
    }

    return;
}

void RecordEditor::updateHeightFrom()
{
    if (comboHeightFrom_Changed || doubleHeightFromValue_Changed || comboHeightFromUnits_Changed)
    {
        std::string label = ui->comboHeightFrom->currentText().toStdString();
        double value = ui->doubleHeightFromValue->value();
        std::string units = ui->comboHeightFromUnits->currentText().toStdString();
        currentModifier->setHeightFromData(label, value, units);
    }

    return;
}

void RecordEditor::updateVSCA()
{
    if ( comboVSCA_Changed || doubleVSCA_Changed )
    {
        std::string label = ui->comboVSCA->currentText().toStdString();
        double      value = ui->doubleVSCA->value();

        currentModifier->setVSCAData(label, value);
    }

    return;
}

void RecordEditor::updateUNCR()
{
    std::string label = ui->comboUncrModifier->currentText().toStdString();

    if (label != lsa::Q_CONTROLVALUE_MIXED.toStdString() )
    {
        if (comboUncrModifier_Changed) currentModifier->setUncrLabel(label);
    }
}

void RecordEditor::updateVSCAControls(const QString &label)
{
    if (lsa::Q_CONTROLVALUE_NONE == label)
    {
        ui->doubleVSCA->setValue(1.0);
        ui->doubleVSCA->setEnabled(false);

    }
    else if (lsa::Q_CONTROLVALUE_VALUE == label)
    {
        ui->doubleVSCA->setEnabled(true);
    }
    else
    {
        ui->doubleVSCA->setValue( guiModel->getVSCAValue(currentLSARecord) );
        ui->doubleVSCA->setEnabled(false);
    }

    return;
}

/***********************************************
 *
 *  Manage Controls
 *
 ***********************************************/
void RecordEditor::setupControls()
{
try
{
    const int MAX_GENERIC_LINEAR_DECIMALS=4;
    const double MAXIMUM_GENERIC_LINEAR=99999999;
    const double MINIMUM_GENERIC_LINEAR = -99999999;

    const int MAX_ANGLE_DMS_SEC_DECIMALS=4;
    const int MAX_LATLON_DMS_SEC_DECIMALS=6;
    const int MAX_ANGLE_DEGREES_DECIMALS=8;

    const int MAX_COVARIANCE_DECIMALS=5;
    const double MAXIMUM_COVARIANCE= 9.9999999;
    const double MINIMUM_COVARIANCE=-9.9999999;

    const int MAX_ALL_UNITS_UNCR_DECIMALS=8;
    const double MAXIMUM_ALL_UNITS_UNCR=9999;

    const int MAX_HEIGHT_FROMTO_DECIMALS=4;
    const double MAXIMUM_HEIGHT_FROMTO=99999;

    // Setup the uncr help label
    // Get the path to the user manual

    QDir userManualDir(QString::fromStdString(getExecutablePath()) + QString("/../doc"));
    QString userManualPath = QString::fromStdString("file:") + QDir::cleanPath(userManualDir.absoluteFilePath("SalsaUserManual.pdf"));
    // TODO: The addition or subtraction of a chapter will render the "Ch. 10" text below incorrect.
    // Resolving this will require manual testing unless/until we implement something more sophisticated,
    // or we strike the chapter number from the below string.

    ui->labelUncrHelp->setText("<a href=\"" + userManualPath + "\">See Ch. 10 of the Salsa User Manual for more information on UNCR records.</a>");
    ui->labelUncrHelp->setTextFormat(Qt::RichText);
    ui->labelUncrHelp->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelUncrHelp->setOpenExternalLinks(true);

    // Setup the covariance warning label
    QPixmap warningPixmap(":/guiIcons/alertIcon.png");
    ui->labelCovWarning->clear();
    ui->labelCovWarning->setPixmap(warningPixmap);
    ui->labelCovWarning->setToolTip("Invalid covariance matrix.");
    ui->labelCovWarning->hide();

    // Setup the scaling warning label
    ui->labelScaleWarning->clear();
    ui->labelScaleWarning->setPixmap(warningPixmap);
    ui->labelScaleWarning->setToolTip(QString::fromStdString(guiModel->INVALID_SCALING_MESSAGE));
    ui->labelScaleWarning->hide();
    ui->labelScaleWarning2->clear();
    ui->labelScaleWarning2->setPixmap(warningPixmap);
    ui->labelScaleWarning2->setToolTip(QString::fromStdString(guiModel->INVALID_SCALING_MESSAGE));
    ui->labelScaleWarning2->hide();

    // Setup the warning label for Cartesian pole
    ui->labelCartesianWarning->clear();
    ui->labelCartesianWarning->setPixmap(warningPixmap);
    ui->labelCartesianWarning->setToolTip("X and Y coordinates of zero give pole singularity");
    ui->labelCartesianWarning->hide();

    // Setup the warning label for Dec Deg latitude out of range
    ui->labelLatDecDegWarning->clear();
    ui->labelLatDecDegWarning->setPixmap(warningPixmap);
    ui->labelLatDecDegWarning->setToolTip("Dec Deg value is out of valid range");
    ui->labelLatDecDegWarning->hide();

    // Setup the warning labels for DMS controls collectively out of range (VANG, ZANG, POSG Lat)
    ui->labelLatDMSWarning->clear();
    ui->labelLatDMSWarning->setPixmap(warningPixmap);
    ui->labelLatDMSWarning->setToolTip("DMS value is out of valid range.");
    ui->labelLatDMSWarning->hide();

    ui->labelAngleDMSWarning->clear();
    ui->labelAngleDMSWarning->setPixmap(warningPixmap);
    ui->labelAngleDMSWarning->setToolTip("DMS value is out of valid range.");
    ui->labelAngleDMSWarning->hide();

    // set font on all controls
    QFont font;
    int pSize = getIdealFontSize();
    font.setPointSize(pSize);
    font.setWeight(QFont::Normal);
    font.setStyleHint(QFont::Courier, QFont::PreferAntialias);
    font.setKerning(false);

    ui->lineStationA->setFont(font);
    ui->lineStationB->setFont(font);
    ui->lineStationC->setFont(font);

    ui->doubleValueA->setFont(font);
    ui->doubleValueB->setFont(font);
    ui->doubleValueC->setFont(font);
    ui->doubleValueD->setFont(font);

    ui->comboValueAUnits->setFont(font);
    ui->comboValueBUnits->setFont(font);
    ui->comboValueCUnits->setFont(font);

    ui->doubleCovAA->setFont(font);
    ui->doubleCovAB->setFont(font);
    ui->doubleCovAC->setFont(font);
    ui->doubleCovBB->setFont(font);
    ui->doubleCovBC->setFont(font);
    ui->doubleCovCC->setFont(font);

    ui->sigE->setFont(font);
    ui->sigN->setFont(font);
    ui->sigU->setFont(font);

    ui->doubleSigma->setFont(font);
    ui->comboSigmaUnits->setFont(font);

    ui->intLatDMSDeg->setFont(font);
    ui->intLatDMSMin->setFont(font);
    ui->doubleLatDMSSec->setFont(font);
    ui->comboLatDMSNS->setFont(font);

    ui->intLonDMSDeg->setFont(font);
    ui->intLonDMSMin->setFont(font);
    ui->doubleLonDMSSec->setFont(font);
    ui->comboLonDMSEW->setFont(font);

    ui->doubleLatDecDeg->setFont(font);
    ui->comboLatDecDegNS->setFont(font);

    ui->doubleLonDecDeg->setFont(font);
    ui->comboLonDecDegEW->setFont(font);

    ui->intAngleDMSDeg->setFont(font);
    ui->intAngleDMSMin->setFont(font);
    ui->doubleAngleDMSSec->setFont(font);
    ui->doubleAngleDecDeg->setFont(font);

    ui->radioUseDMS->setFont(font);
    ui->radioUseDecDeg->setFont(font);

    ui->comboHeightTo->setFont(font);
    ui->doubleHeightToValue->setFont(font);
    ui->comboHeightToUnits->setFont(font);

    ui->comboHeightFrom->setFont(font);
    ui->doubleHeightFromValue->setFont(font);
    ui->comboHeightFromUnits->setFont(font);

    ui->comboUncrModifier->setFont(font);
    ui->doubleRefractiveIndex->setFont(font);
    ui->comboVSCA->setFont(font);
    ui->doubleVSCA->setFont(font);
    ui->doubleNetVSCA->setFont(font);
    ui->checkReduced->setFont(font);
    ui->checkCurvature->setFont(font);
    ui->checkOHC->setFont(font);

    ui->comboDirGroup->setFont(font);

    ui->comboPOSGHeight->setFont(font);

    ui->labelDXYZHeight->setFont(font);

    ui->doubleVSCA->setDisabled(true);
    ui->doubleNetVSCA->setDisabled(true);

    bool oldState = suppressGuiUpdates(true);
    {
        //generic linear unit controls
        ui->doubleValueA->setDecimals(MAX_GENERIC_LINEAR_DECIMALS);
        ui->doubleValueB->setDecimals(MAX_GENERIC_LINEAR_DECIMALS);
        ui->doubleValueC->setDecimals(MAX_GENERIC_LINEAR_DECIMALS);
        ui->doubleValueD->setDecimals(MAX_GENERIC_LINEAR_DECIMALS);

        ui->doubleValueA->setMaximum(MAXIMUM_GENERIC_LINEAR);
        ui->doubleValueB->setMaximum(MAXIMUM_GENERIC_LINEAR);
        ui->doubleValueC->setMaximum(MAXIMUM_GENERIC_LINEAR);
        ui->doubleValueD->setMaximum(MAXIMUM_GENERIC_LINEAR);

        ui->doubleValueA->setMinimum(MINIMUM_GENERIC_LINEAR);
        ui->doubleValueB->setMinimum(MINIMUM_GENERIC_LINEAR);
        ui->doubleValueC->setMinimum(MINIMUM_GENERIC_LINEAR);
        // ui->doubleValueD->setMinimum(MINIMUM_GENERIC_LINEAR);//want min vscale and uncrtainty ppm to be zero

        //DMS angle minutes
        ui->intAngleDMSMin->setRange(0,59);
        ui->intLatDMSMin->setRange(0,59);
        ui->intLonDMSMin->setRange(0,59);

        //DMS angle seconds
        ui->doubleAngleDMSSec->setDecimals(MAX_ANGLE_DMS_SEC_DECIMALS);
        ui->doubleLatDMSSec->setDecimals(MAX_LATLON_DMS_SEC_DECIMALS);
        ui->doubleLonDMSSec->setDecimals(MAX_LATLON_DMS_SEC_DECIMALS);
        ui->doubleAngleDMSSec->setMaximum(59.9999);
        ui->doubleLatDMSSec->setMaximum(59.999999);
        ui->doubleLonDMSSec->setMaximum(59.999999);

        ui->doubleAngleDMSSec->setMinimum(0.0);
        ui->doubleLatDMSSec->setMinimum(0.0);
        ui->doubleLonDMSSec->setMinimum(0.0);

        //decimal degrees
        ui->doubleAngleDecDeg->setDecimals(MAX_ANGLE_DEGREES_DECIMALS);
        ui->doubleLatDecDeg->setDecimals(MAX_ANGLE_DEGREES_DECIMALS);
        ui->doubleLonDecDeg->setDecimals(MAX_ANGLE_DEGREES_DECIMALS);

        ui->doubleLatDecDeg->setMaximum(90.0);
        ui->doubleAngleDecDeg->setMaximum(359.99999999);
        ui->doubleLonDecDeg->setMaximum(359.99999999);

        ui->doubleLatDecDeg->setMinimum(-90.0);
        ui->doubleAngleDecDeg->setMinimum(-359.99999999);
        ui->doubleLonDecDeg->setMinimum(-359.99999999);

        //covariance elements
        QDoubleValidator *covValidator = new QDoubleValidator(MINIMUM_COVARIANCE,
                                                              MAXIMUM_COVARIANCE,
                                                              MAX_COVARIANCE_DECIMALS,
                                                              this);
        covValidator->setNotation(QDoubleValidator::ScientificNotation);
        ui->doubleCovAA->setValidator(covValidator);
        ui->doubleCovAB->setValidator(covValidator);
        ui->doubleCovAC->setValidator(covValidator);
        ui->doubleCovBB->setValidator(covValidator);
        ui->doubleCovBC->setValidator(covValidator);
        ui->doubleCovCC->setValidator(covValidator);

        ui->comboFixedFloat->clear();

        LSAFixedState floating(LSAFixedState::FLOATING);
        LSAFixedState constrained(LSAFixedState::CONSTRAINED);
        LSAFixedState fixed(LSAFixedState::FIXED);

        ui->comboFixedFloat->addItem(QString::fromStdString(floating.asString()) );
        ui->comboFixedFloat->addItem(QString::fromStdString(constrained.asString()) );
        ui->comboFixedFloat->addItem(QString::fromStdString(fixed.asString()) );

        //sigma
        ui->doubleSigma->setDecimals(MAX_ALL_UNITS_UNCR_DECIMALS);
        ui->doubleSigma->setMaximum(MAXIMUM_ALL_UNITS_UNCR);
        ui->doubleSigma->setMinimum(0.0);

        //height from and height to
        ui->doubleHeightToValue->setDecimals(MAX_HEIGHT_FROMTO_DECIMALS);
        ui->doubleHeightFromValue->setDecimals(MAX_HEIGHT_FROMTO_DECIMALS);

        // Iterate over QSpinBox widgets to set StrongFocus and to filter wheel events
        QList<QSpinBox*> spinBoxList = this->findChildren<QSpinBox *>();
        foreach(QSpinBox *w, spinBoxList)
        {
            w->setFocusPolicy( Qt::StrongFocus );
            w->installEventFilter(this->wheelFilter);
        }

        // Iterate over QDoubleSpinBox widgets to set StrongFocus and to filter wheel events
        QList<QDoubleSpinBox*> doubleSpinBoxList = this->findChildren<QDoubleSpinBox *>();
        foreach(QDoubleSpinBox *w, doubleSpinBoxList)
        {
            w->setFocusPolicy( Qt::StrongFocus );
            w->installEventFilter(this->wheelFilter);
        }

    }
    suppressGuiUpdates(oldState);

    // line station A, B, C
    connect(ui->checkEnabled, SIGNAL(stateChanged(int)),    this, SLOT(checkEnabledChanged()) );

    connect(ui->lineStationA, SIGNAL(textChanged(QString)), this, SLOT(lineStationAChanged()) );
    connect(ui->lineStationB, SIGNAL(textChanged(QString)), this, SLOT(lineStationBChanged()) );
    connect(ui->lineStationC, SIGNAL(textChanged(QString)), this, SLOT(lineStationCChanged()) );

    connect(ui->btnBrowse,    SIGNAL(clicked()),            this, SLOT(handleBtnBrowseClicked()) );

    // Value A, Value B, Value C
    connect(ui->doubleValueA, SIGNAL(valueChanged(QString)), this, SLOT(doubleValueAChanged()) );
    connect(ui->doubleValueB, SIGNAL(valueChanged(QString)), this, SLOT(doubleValueBChanged()) );
    connect(ui->doubleValueC, SIGNAL(valueChanged(QString)), this, SLOT(doubleValueCChanged()) );
    connect(ui->doubleValueD, SIGNAL(valueChanged(double)),  this, SLOT(doubleValueDChanged()) );

    connect(ui->doubleValueA1, SIGNAL(valueChanged(QString)), this, SLOT(doubleValueA1Changed()) );
    connect(ui->doubleValueB1, SIGNAL(valueChanged(QString)), this, SLOT(doubleValueB1Changed()) );
    connect(ui->doubleValueC1, SIGNAL(valueChanged(QString)), this, SLOT(doubleValueC1Changed()) );

    connect(ui->comboValueAUnits, SIGNAL(currentTextChanged(QString)), this, SLOT(comboValueUnitsChanged(QString)) );
    connect(ui->comboValueBUnits, SIGNAL(currentTextChanged(QString)), this, SLOT(comboValueUnitsChanged(QString)) );
    connect(ui->comboValueCUnits, SIGNAL(currentTextChanged(QString)), this, SLOT(comboValueUnitsChanged(QString)) );

    // Covariance
    connect(ui->doubleCovAA, SIGNAL(textChanged(QString)), this, SLOT(doubleCovAAChanged()) );
    connect(ui->doubleCovAB, SIGNAL(textChanged(QString)), this, SLOT(doubleCovABChanged()) );
    connect(ui->doubleCovAC, SIGNAL(textChanged(QString)), this, SLOT(doubleCovACChanged()) );
    connect(ui->doubleCovBB, SIGNAL(textChanged(QString)), this, SLOT(doubleCovBBChanged()) );
    connect(ui->doubleCovBC, SIGNAL(textChanged(QString)), this, SLOT(doubleCovBCChanged()) );
    connect(ui->doubleCovCC, SIGNAL(textChanged(QString)), this, SLOT(doubleCovCCChanged()) );
    connect(ui->comboFixedFloat, SIGNAL(currentTextChanged(QString)), this, SLOT(comboFixedFloatChanged(QString)) );

    // Sigma
    connect(ui->doubleSigma, SIGNAL(valueChanged(double)), this, SLOT(doubleSigmaChanged()) );
    connect(ui->comboSigmaUnits, SIGNAL(currentTextChanged(QString)), this, SLOT(comboSigmaUnitsChanged()) );

    // SigmaMEAN
    connect(ui->doubleSigmaMEAN, SIGNAL(valueChanged(double)), this, SLOT(doubleSigmaMEANChanged()) );
    connect(ui->comboSigmaUnitsMEAN, SIGNAL(currentTextChanged(QString)), this, SLOT(comboSigmaUnitsMEANChanged()) );

    // North, East, Up fixed
    connect(ui->checkNorthFixed, SIGNAL(stateChanged(int)), this, SLOT(checkNorthFixedChanged(int)) );
    connect(ui->checkEastFixed,  SIGNAL(stateChanged(int)), this, SLOT(checkEastFixedChanged(int))  );
    connect(ui->checkUpFixed,    SIGNAL(stateChanged(int)), this, SLOT(checkUpFixedChanged(int))    );

    // Latitude DMS
    connect(ui->btnLatDMSNeg,     SIGNAL(clicked()),                   this, SLOT(handleBtnLatNegDMSClicked())  );
    connect(ui->intLatDMSDeg,     SIGNAL(valueChanged(int)),           this, SLOT(intLatDMSDegChanged()) );
    connect(ui->intLatDMSMin,     SIGNAL(valueChanged(int)),           this, SLOT(intLatDMSMinChanged()) );
    connect(ui->doubleLatDMSSec,  SIGNAL(valueChanged(double)),        this, SLOT(doubleLatDMSSecChanged()) );
    connect(ui->comboLatDMSNS,    SIGNAL(currentTextChanged(QString)), this, SLOT(comboLatDMSNSChanged()) );

    // Longitude DMS
    connect(ui->btnLonDMSNeg,     SIGNAL(clicked()),                   this, SLOT(handleBtnLonNegDMSClicked())  );
    connect(ui->intLonDMSDeg,     SIGNAL(valueChanged(int)),           this, SLOT(intLonDMSDegChanged()) );
    connect(ui->intLonDMSMin,     SIGNAL(valueChanged(int)),           this, SLOT(intLonDMSMinChanged()) );
    connect(ui->doubleLonDMSSec,  SIGNAL(valueChanged(double)),        this, SLOT(doubleLonDMSSecChanged()) );
    connect(ui->comboLonDMSEW,    SIGNAL(currentTextChanged(QString)), this, SLOT(comboLonDMSEWChanged()) );

    // Latitude decimal degrees
    connect(ui->doubleLatDecDeg,  SIGNAL(valueChanged(double)),        this, SLOT(doubleLatDecDegChanged())  );
    connect(ui->comboLatDecDegNS, SIGNAL(currentTextChanged(QString)), this, SLOT(comboLatDecDegNSChanged()) );

    // Longitude decimal degrees
    connect(ui->doubleLonDecDeg,  SIGNAL(valueChanged(double)),        this, SLOT(doubleLonDecDegChanged())  );
    connect(ui->comboLonDecDegEW, SIGNAL(currentTextChanged(QString)), this, SLOT(comboLonDecDegEWChanged()) );

    // Angle DMS and decimal decrees
    connect(ui->btnAngleNegDMS,     SIGNAL(clicked()),            this, SLOT(handleBtnAngleNegDMSClicked())  );
    connect(ui->intAngleDMSDeg,     SIGNAL(valueChanged(int)),    this, SLOT(intAngleDMSDegChanged())  );
    connect(ui->intAngleDMSMin,     SIGNAL(valueChanged(int)),    this, SLOT(intAngleDMSMinChanged())  );
    connect(ui->doubleAngleDMSSec,  SIGNAL(valueChanged(double)), this, SLOT(doubleAngleDMSSecChanged())  );
    connect(ui->doubleAngleDecDeg,  SIGNAL(valueChanged(double)), this, SLOT(doubleAngleDecDegChanged())  );

    // DMS to decimal degrees radio buttons
    connect(ui->radioUseDMS,    SIGNAL(toggled(bool)), this, SLOT(radioUseDMSChanged(bool))    );

    // Height Modifiers
    connect(ui->comboHeightTo, SIGNAL(currentTextChanged(QString)), this, SLOT(comboHeightToChanged()) );
    connect(ui->doubleHeightToValue, SIGNAL(valueChanged(double)), this, SLOT(doubleHeightToValueChanged()) );
    connect(ui->comboHeightToUnits, SIGNAL(currentTextChanged(QString)), this, SLOT(comboHeightToUnitsChanged()) );

    connect(ui->comboHeightFrom, SIGNAL(currentTextChanged(QString)), this, SLOT(comboHeightFromChanged()) );
    connect(ui->doubleHeightFromValue, SIGNAL(valueChanged(double)), this, SLOT(doubleHeightFromValueChanged()) );
    connect(ui->comboHeightFromUnits, SIGNAL(currentTextChanged(QString)), this, SLOT(comboHeightFromUnitsChanged()) );

    // Other Modifiers
    connect(ui->comboUncrModifier,     SIGNAL(currentTextChanged(QString)), this, SLOT(comboUncrModifierChanged())     );
    connect(ui->checkApplyRefraction,  SIGNAL(clicked(bool)),               this, SLOT(checkRefractiveIndexChanged(bool)) );
    connect(ui->doubleRefractiveIndex, SIGNAL(valueChanged(double)),        this, SLOT(doubleRefractiveIndexChanged()) );
    connect(ui->comboVSCA,             SIGNAL(currentTextChanged(QString)), this, SLOT(comboVarianceScalingChanged())  );
    connect(ui->doubleVSCA,            SIGNAL(valueChanged(double)),        this, SLOT(doubleVSCAChanged())            );
    connect(ui->checkReduced,          SIGNAL(clicked(bool)),               this, SLOT(checkReducedChanged())            );
    connect(ui->checkCurvature,        SIGNAL(clicked(bool)),               this, SLOT(checkCurvatureChanged())        );
    connect(ui->checkOHC,              SIGNAL(clicked(bool)),               this, SLOT(checkOHCChanged())        );

    // Text Edit Points
    connect(ui->textInputPoints,        SIGNAL(textChanged()),               this, SLOT(textInputPointsChanged())        );

    // DirGroup and HDir(point)
    connect(ui->comboDirGroup,    SIGNAL(currentTextChanged(QString)), this, SLOT(comboDirGroupChanged())    );

    // Changes to height state in Record Editor propagated to appropriate POSG record
    connect(ui->comboPOSGHeight,  SIGNAL(currentTextChanged(QString)), this, SLOT(handleChangeHeightState(QString)) );
    
    connect(ui->textNotes, SIGNAL(textChanged()), this, SLOT(handleGeneralTextNotesChanged()));

    return;
}
catch(...) { emit exceptionCaught(__FUNCTION__); }
}

void RecordEditor::hideAllLatLonControls()
{
try
{
    ui->frameUseDMS->hide();

    ui->labelLatDMS->hide();
    ui->frameLatDMS->hide();

    ui->labelLonDMS->hide();
    ui->frameLonDMS->hide();

    ui->labelLatDecDeg->hide();
    ui->frameLatDecDeg->hide();

    ui->labelLonDecDeg->hide();
    ui->frameLonDecDeg->hide();

    return;
}
    catch(...) { emit exceptionCaught(__FUNCTION__); }
}

gnsstk::Triple RecordEditor::getLonLatHeightRads(LSARecord *posRec)
{
    LSAType     lsaType   = posRec->getRecType();
    gnsstk::Triple enu;

    if (lsaType == LSAType::POSG)
    {
        LSAPosG *lsaPosG = static_cast<LSAPosG*>(posRec);
        lsaPosG->getLatLonHeight(enu[0], enu[1], enu[2]);
    }
    else if (lsaType == LSAType::POSC)
    {
        LSAPosC *lsaPosC = static_cast<LSAPosC*>(posRec);
        lsaPosC->getLatLonHeight(enu[0], enu[1], enu[2]);
    }
    else if ((lsaType == LSAType::MEAN) || (lsaType == LSAType::ENUO))
    {
        // should this be handled?
    }
    enu[0] = convertTo("rad", enu[0], "deg");
    enu[1] = convertTo("rad", enu[1], "deg");
    return enu;
}

void RecordEditor::showLatLonControls()
{
try
{
    ui->frameUseDMS->show();
    ui->radioUseDMS->setChecked(useDMS);
    ui->radioUseDecDeg->setChecked(!useDMS);

    // Set the lat label in case an azimuth record has changed it
    ui->labelLatDMS->clear();
    ui->labelLatDMS->setText("Latitude: ");
    ui->labelLatDecDeg->clear();
    ui->labelLatDecDeg->setText("Latitude: ");

    if (useDMS)
    {
        ui->labelLatDecDeg->hide();
        ui->frameLatDecDeg->hide();

        ui->labelLonDecDeg->hide();
        ui->frameLonDecDeg->hide();

        ui->labelLatDMS->show();
        ui->frameLatDMS->show();

        ui->labelLonDMS->show();
        ui->frameLonDMS->show();
    }
    else
    {
        ui->labelLatDMS->hide();
        ui->frameLatDMS->hide();

        ui->labelLonDMS->hide();
        ui->frameLonDMS->hide();

        ui->labelLatDecDeg->show();
        ui->frameLatDecDeg->show();

        ui->labelLonDecDeg->show();
        ui->frameLonDecDeg->show();
    }

    // if the current record is an azimuth, don't display the longitude controls
    if (currentLSAType == LSAType::AZIM)
    {
        ui->labelLonDecDeg->hide();
        ui->frameLonDecDeg->hide();
        ui->labelLonDMS->hide();
        ui->frameLonDMS->hide();
    }
    return;
}
catch(...) { emit exceptionCaught(__FUNCTION__); }
}

void RecordEditor::hideControls()
{
try
{
    bool oldState = suppressGuiUpdates(true);
    {
        ui->labelEnabled->hide();
        ui->checkEnabled->hide();

        ui->labelStationA->hide();
        ui->labelStationB->hide();
        ui->labelStationC->hide();

        ui->labelParseWarnings->hide();
        ui->labelParseWarningsList->hide();

        ui->lineStationA->hide();
        ui->lineStationB->hide();
        ui->lineStationC->hide();

        ui->labelValueA->hide();
        ui->labelValueB->hide();
        ui->labelValueC->hide();

        ui->frameValueA->hide();
        ui->frameValueB->hide();
        ui->frameValueC->hide();

        ui->comboPOSGHeight->hide();

        ui->btnBrowse->hide();

        ui->labelDirGroup->hide();
        ui->frameDirGroup->hide();

        hideAllLatLonControls();

        ui->btnAngleNegDMS->hide();
        ui->labelAngleDMS->hide();
        ui->frameAngleDMS->hide();

        ui->labelAngleDecDeg->hide();
        ui->doubleAngleDecDeg->hide();

        ui->labelSigma->hide();
        ui->frameSigma->hide();

        ui->labelSigmaMEAN->hide();
        ui->frameSigmaMEAN->hide();

        ui->labelCovariance->hide();
        ui->frameCovariance->hide();
        ui->frameFixedFloat->hide();

        ui->labelType->hide();
        ui->frameConstraints->hide();

        ui->labelDXYZHeight->hide();

        ui->labelHeightTo->hide();
        ui->frameHeightTo->hide();

        ui->labelHeightFrom->hide();
        ui->frameHeightFrom->hide();

        ui->labelUncrModifier->hide();
        ui->frameUncrModifier->hide();
        ui->labelUncrHelp->hide();

        ui->frameSigma->hide();

        ui->labelRefractiveIndex->hide();
        ui->frameRefraction->hide();

        ui->labelVSCA->hide();
        ui->frameVSCA->hide();

        ui->labelReduced->hide();
        ui->checkReduced->hide();
        ui->labelHDIFEllipsoidal->hide();
        ui->labelHDIFOrthometric->hide();
        ui->labelHDIFOutput->hide();

        ui->labelCurvature->hide();
        ui->checkCurvature->hide();

        ui->labelOHC->hide();
        ui->checkOHC->hide();

        ui->labelValueD->hide();
        ui->doubleValueD->hide();

        ui->labelPoints->hide();
        ui->framePoints->hide();

        ui->labelValueA1->hide();
        ui->labelValueB1->hide();
        ui->labelValueC1->hide();
        ui->doubleValueA1->hide();
        ui->doubleValueB1->hide();
        ui->doubleValueC1->hide();

        ui->labelAutogenPOSGInfo->hide();
        
        ui->textNotes->hide();
        ui->labelTextNotes->hide();

        ui->sigE->hide();
        ui->sigN->hide();
        ui->sigU->hide();
        ui->labelSigE->hide();
        ui->labelSigN->hide();
        ui->labelSigU->hide();
    }
    suppressGuiUpdates(oldState);

    return;
}
catch(...) { emit exceptionCaught(__FUNCTION__); }
}

void RecordEditor::setControlRanges()
{
    const int    MIN_INT_LIMIT = std::numeric_limits<int>::min();
    const int    MAX_INT_LIMIT = std::numeric_limits<int>::max();
    const double MAX_DOUBLE_LIMIT = std::numeric_limits<double>::max();
    const double MIN_DOUBLE_LIMIT = -1 * MAX_DOUBLE_LIMIT;


    // lat DMS
    ui->intLatDMSDeg->setRange(MIN_INT_LIMIT, MAX_INT_LIMIT);
    ui->intLatDMSMin->setRange(MIN_INT_LIMIT, MAX_INT_LIMIT);
    ui->doubleLatDMSSec->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);
    // lon DMS
    ui->intLonDMSDeg->setRange(MIN_INT_LIMIT, MAX_INT_LIMIT);
    ui->intLonDMSMin->setRange(MIN_INT_LIMIT, MAX_INT_LIMIT);
    ui->doubleLonDMSSec->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // lat decimal degrees
    ui->doubleLatDecDeg->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // lon decimal degrees
    ui->doubleLonDecDeg->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // angle DMS
    ui->intAngleDMSDeg->setRange(MIN_INT_LIMIT, MAX_INT_LIMIT);
    ui->intAngleDMSMin->setRange(MIN_INT_LIMIT, MAX_INT_LIMIT);

    // angle decimal degrees
    ui->doubleAngleDecDeg->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // ValueA
    ui->doubleValueA->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // ValueB
    ui->doubleValueB->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // ValueC
    ui->doubleValueC->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // ValueD
    ui->doubleValueD->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // Sigma
    ui->doubleSigma->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // heights
    ui->doubleHeightFromValue->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);
    ui->doubleHeightToValue->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);

    // Modifiers
    ui->doubleValueD->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);
    ui->doubleVSCA->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);
    ui->doubleNetVSCA->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);
    ui->doubleRefractiveIndex->setRange(MIN_DOUBLE_LIMIT, MAX_DOUBLE_LIMIT);
}

void RecordEditor::resetControlStates()
{
    setControlRanges();

    bool oldState = suppressGuiUpdates(true);
    {
        // station labels
        ui->lineStationA->clear();
        ui->lineStationB->clear();
        ui->lineStationC->clear();
        ui->lineStationA->setEnabled(true);
        ui->lineStationA->setReadOnly(false);

        // listParseWarnings
        ui->labelParseWarningsList->clear();

        // dir group controls
        ui->comboDirGroup->clear();

        // lat DMS
        ui->intLatDMSDeg->setValue(0);
        ui->intLatDMSMin->setValue(0);
        ui->doubleLatDMSSec->setValue(0.0);
        ui->comboLatDMSNS->setCurrentIndex(0);

        // lon DMS
        ui->intLonDMSDeg->setValue(0);
        ui->intLonDMSMin->setValue(0);
        ui->doubleLonDMSSec->setValue(0.0);
        ui->comboLonDMSEW->setCurrentIndex(0);

        // lat decimal degrees
        ui->doubleLatDecDeg->setValue(0.0);
        ui->comboLatDecDegNS->setCurrentIndex(0);

        // lon decimal degrees
        ui->doubleLonDecDeg->setValue(0.0);
        ui->comboLonDecDegEW->setCurrentIndex(0);

        // angle DMS
        ui->btnAngleNegDMS->setText("+");
        ui->intAngleDMSDeg->setValue(0);
        ui->intAngleDMSMin->setValue(0);
        ui->doubleAngleDMSSec->setValue(0.0);

        // angle decimal degrees
        ui->doubleAngleDecDeg->setValue(0.0);

        // ValueA
        ui->labelValueA->clear();
        ui->doubleValueA->clear();
        ui->comboValueAUnits->setCurrentIndex(0);
        ui->comboValueAUnits->clear();
        ui->comboValueAUnits->show();

        // ValueB
        ui->labelValueB->clear();
        ui->doubleValueB->clear();
        ui->comboValueBUnits->setCurrentIndex(0);
        ui->comboValueBUnits->clear();
        ui->comboValueBUnits->show();

        // ValueC
        ui->labelValueC->clear();
        ui->doubleValueC->clear();
        ui->comboValueCUnits->setCurrentIndex(0);
        ui->comboValueCUnits->clear();

        doubleValueC_Changed = false;
        comboValueCUnits_Changed = false;

        // ValueD
        ui->labelValueD->clear();
        ui->doubleValueD->clear();

        doubleValueD_Changed = false;

        // Sigma
        ui->doubleSigma->clear();
        ui->comboSigmaUnits->setCurrentIndex(0);
        ui->comboSigmaUnits->clear();
        ui->comboUncrModifier->setEnabled(true);
        ui->labelUncrComments->clear();

        // SigmaMEAN
        ui->doubleSigmaMEAN->clear();
        ui->comboSigmaUnitsMEAN->setCurrentIndex(0);
        ui->comboSigmaUnitsMEAN->clear();
        ui->comboSigmaUnitsMEAN->show();

        // covariance
        ui->doubleCovAA->clear();
        ui->doubleCovAB->clear();
        ui->doubleCovAC->clear();
        ui->doubleCovBB->clear();
        ui->doubleCovBC->clear();
        ui->doubleCovCC->clear();

        ui->doubleCovAA->setEnabled(true);
        ui->doubleCovAB->setEnabled(true);
        ui->doubleCovAC->setEnabled(true);
        ui->doubleCovBB->setEnabled(true);
        ui->doubleCovBC->setEnabled(true);
        ui->doubleCovCC->setEnabled(true);

        ui->sigE->clear();
        ui->sigN->clear();
        ui->sigU->clear();
        ui->sigE->setReadOnly(true);
        ui->sigN->setReadOnly(true);
        ui->sigU->setReadOnly(true);

        // covariance warning icon
        ui->labelCovWarning->hide();

        //scale warning icon
        ui->labelScaleWarning->hide();
        ui->labelScaleWarning2->hide();

        // Cartesian pole warning icon
        ui->labelCartesianWarning->hide();

        // Decimal degree latitude out of range icons
        ui->labelLatDecDegWarning->hide();

        //angle DMS out of range icons
        ui->labelLatDMSWarning->hide();
        ui->labelAngleDMSWarning->hide();

        // heights
        ui->comboHeightFrom->clear();
        ui->doubleHeightFromValue->clear();
        ui->comboHeightFromUnits->setCurrentText(lsa::Q_CONTROLVALUE_NONE);
        ui->comboHeightTo->clear();
        ui->doubleHeightToValue->clear();
        ui->comboHeightToUnits->setCurrentText(lsa::Q_CONTROLVALUE_NONE);

        // Modifiers
        ui->comboUncrModifier->clear();
        ui->labelUncrValues->clear();
        ui->labelUncrValues->hide();
        ui->doubleValueD->clear();
        ui->comboVSCA->clear();
        ui->comboVSCA->setEnabled(true);
        ui->comboVSCA->setToolTip("");
        ui->doubleVSCA->clear();
        ui->doubleNetVSCA->clear();
        ui->checkApplyRefraction->setCheckState(Qt::Unchecked);
        ui->doubleRefractiveIndex->clear();

        // listPoints
        ui->textInputPoints->clear();

        // sigmas
        ui->labelValueA1->clear();
        ui->labelValueB1->clear();
        ui->labelValueC1->clear();
        ui->doubleValueA1->clear();
        ui->doubleValueB1->clear();
        ui->doubleValueC1->clear();
        
        // general notes
        ui->textNotes->clear();

        // Make sure all POSG controls are enabled
        enableAllPOSGControls(true); // Fix for bug 1356

    }
    suppressGuiUpdates(oldState);
    resetChangeStateTrackers();

    return;
}

void RecordEditor::setParentIsModifiedFlag()
{
    if(hasPendingChanges())
    {
        if(currentIndex.isValid())
            guiModel->setParentIncludeModified(currentIndex);
        else//multi-select
        {
            foreach(QModelIndex index, selectedRows)
                guiModel->setParentIncludeModified(index);
        }
    }
}

void RecordEditor::resetChangeStateTrackers()
{
    checkEnabled_Changed      = false;
    useDMS_Changed            = false;

    // Line Stations
    lineStationA_Changed      = false;
    lineStationB_Changed      = false;
    lineStationC_Changed      = false;

    comboDirGroup_Changed     = false;

    // Lat DMS
    btnLatDMSNeg_Changed      = false;
    intLatDMSDeg_Changed      = false;
    intLatDMSMin_Changed      = false;
    doubleLatDMSSec_Changed   = false;
    comboLatDMSNS_Changed     = false;

    // Lon DMS
    btnLonDMSNeg_Changed      = false;
    intLonDMSDeg_Changed      = false;
    intLonDMSMin_Changed      = false;
    doubleLonDMSSec_Changed   = false;
    comboLonDMSEW_Changed     = false;

    // Lat DecDeg
    doubleLatDecDeg_Changed   = false;
    comboLatDecDegNS_Changed  = false;

    // Lon DecDeg
    doubleLonDecDeg_Changed   = false;
    comboLonDecDegEW_Changed  = false;

    // Angle DMS
    btnAngleNegDMS_Changed    = false;
    intAngleDMSDeg_Changed    = false;
    intAngleDMSMin_Changed    = false;
    doubleAngleDMSSec_Changed = false;
    doubleAngleDecDeg_Changed = false;

    // Values A,B,C
    doubleValueA_Changed      = false;
    comboValueAUnits_Changed  = false;
    doubleValueB_Changed      = false;
    comboValueBUnits_Changed  = false;
    doubleValueC_Changed      = false;
    comboValueCUnits_Changed  = false;
    doubleValueA1_Changed     = false;
    doubleValueB1_Changed     = false;
    doubleValueC1_Changed     = false;
    doubleValueD_Changed      = false;

    // Sigma
    doubleSigma_Changed         = false;
    comboSigmaUnits_Changed     = false;
    doubleSigmaMEAN_Changed     = false;
    comboSigmaUnitsMEAN_Changed = false;

    // Covariance
    doubleCovAA_Changed       = false;
    doubleCovAB_Changed       = false;
    doubleCovAC_Changed       = false;
    doubleCovBB_Changed       = false;
    doubleCovBC_Changed       = false;
    doubleCovCC_Changed       = false;

    // Fixed, Constrained, Float
    comboFixedFloat_Changed   = false;
    checkNorthFixed_Changed   = false;
    checkEastFixed_Changed    = false;
    checkUpFixed_Changed      = false;

    // Posg Height State
    comboPosgHeight_Changed = false;

    // Height From/To
    comboHeightTo_Changed         = false;
    doubleHeightToValue_Changed   = false;
    comboHeightToUnits_Changed    = false;
    comboHeightFrom_Changed       = false;
    doubleHeightFromValue_Changed = false;
    comboHeightFromUnits_Changed  = false;

    // UNCR
    comboUncrModifier_Changed     = false;

    // Refraction
    checkApplyRefraction_Changed  = false;
    doubleRefractiveIndex_Changed = false;

    // VSCA
    comboVSCA_Changed       = false;
    doubleVSCA_Changed      = false;

    // Reduced, Curv, OHC
    checkReduced_Changed    = false;
    checkCurvature_Changed  = false;
    checkOHC_Changed        = false;
    textInputPoints_Changed = false;
    
    generalTextNotes_Changed = false;

    return;
}

void RecordEditor::closeEvent (QCloseEvent *event)
{
    emit closing();
    event->accept();
}
