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
#include "optionsdialog.hpp"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(Mapping *mapWidget, QWidget *parent) :
    map(mapWidget),
    QDialog(parent),
    ui(new Ui::OptionsDialog) {

    Q_ASSERT(map != NULL);
    // WestonTODO: possibly handle the case where map is NULL, probably an edge of the edge case
    ui->setupUi(this);

    // set up each group of controls on the dialog (each group of code does the same steps with the next section on the form)

    // set OptionsDialog's internal color member
    colorAZIM = map->getColorAZIM();
    // show the color icon on the button
    setButtonIcon(ui->buttonColorAZIM, colorAZIM);
    // Set the line width spin box
    ui->spinBoxLineWidthAZIM->setValue(map->getLineWidthAZIM());
    // Set the pen style combo box
    setupPenStyleComboBox(ui->comboBoxPenAZIM, map->getPenStyleAZIM());

    colorAZIMReferenceLines = map->getColorAZIMReferenceLines();
    setButtonIcon(ui->buttonColorAZIMReferenceLines, colorAZIMReferenceLines);
    ui->spinBoxLineWidthAZIMReferenceLines->setValue(map->getLineWidthAZIMReferenceLines());
    setupPenStyleComboBox(ui->comboBoxPenAZIMReferenceLines, map->getPenStyleAZIMReferenceLines());

    colorHANG = map->getColorHANG();
    setButtonIcon(ui->buttonColorHANG, colorHANG);
    ui->spinBoxLineWidthHANG->setValue(map->getLineWidthHANG());
    setupPenStyleComboBox(ui->comboBoxPenHANG, map->getPenStyleHANG());

    colorHANGReferenceLines = map->getColorHANGReferenceLines();
    setButtonIcon(ui->buttonColorHANGReferenceLines, colorHANGReferenceLines);
    ui->spinBoxLineWidthHANGReferenceLines->setValue(map->getLineWidthHANGReferenceLines());
    setupPenStyleComboBox(ui->comboBoxPenHANGReferenceLines, map->getPenStyleHANGReferenceLines());

    colorZANG = map->getColorZANG();
    setButtonIcon(ui->buttonColorZANG, colorZANG);
    ui->spinBoxLineWidthZANG->setValue(map->getLineWidthZANG());
    setupPenStyleComboBox(ui->comboBoxPenZANG, map->getPenStyleZANG());

    colorDIST = map->getColorDIST();
    setButtonIcon(ui->buttonColorDIST, colorDIST);
    ui->spinBoxLineWidthDIST->setValue(map->getLineWidthDIST());
    setupPenStyleComboBox(ui->comboBoxPenDIST, map->getPenStyleDIST());

    colorDXYZ = map->getColorDXYZ();
    setButtonIcon(ui->buttonColorDXYZ, colorDXYZ);
    ui->spinBoxLineWidthDXYZ->setValue(map->getLineWidthDXYZ());
    setupPenStyleComboBox(ui->comboBoxPenDXYZ, map->getPenStyleDXYZ());

    colorHDIR = map->getColorHDIR();
    setButtonIcon(ui->buttonColorHDIR, colorHDIR);
    ui->spinBoxLineWidthHDIR->setValue(map->getLineWidthHDIR());
    setupPenStyleComboBox(ui->comboBoxPenHDIR, map->getPenStyleHDIR());

    colorHDIF = map->getColorHDIF();
    setButtonIcon(ui->buttonColorHDIF, colorHDIF);
    ui->spinBoxLineWidthHDIF->setValue(map->getLineWidthHDIF());
    setupPenStyleComboBox(ui->comboBoxPenHDIF, map->getPenStyleHDIF());

    colorVANG = map->getColorVANG();
    setButtonIcon(ui->buttonColorVANG, colorVANG);
    ui->spinBoxLineWidthVANG->setValue(map->getLineWidthVANG());
    setupPenStyleComboBox(ui->comboBoxPenVANG, map->getPenStyleVANG());

    colorEllipse = map->getColorEllipse();
    setButtonIcon(ui->buttonColorEllipse, colorEllipse);
    ui->spinBoxLineWidthEllipse->setValue(map->getLineWidthEllipse());
    setupPenStyleComboBox(ui->comboBoxPenEllipse, map->getPenStyleEllipse());

    colorRectangle = map->getColorRectangle();
    setButtonIcon(ui->buttonColorRectangle, colorRectangle);
    ui->spinBoxLineWidthRectangle->setValue(map->getLineWidthRectangle());
    setupPenStyleComboBox(ui->comboBoxPenRectangle, map->getPenStyleRectangle());

    colorHighlighting = map->getColorHighlighting();
    setButtonIcon(ui->buttonColorHighlighting, colorHighlighting);
}

OptionsDialog::~OptionsDialog() {
    delete ui;
}

// Slots to display the color dialog when each Color button is pressed
// The color returned by QColorDialog::getColor() is invalid if the user
// presses 'Cancel' rather than 'Ok', in which case OptionsDialog should do nothing

void OptionsDialog::on_buttonColorAZIM_clicked() {
    QColor color = QColorDialog::getColor(colorAZIM, this, "Set AZIM Color");
    if (color.isValid()) {
        colorAZIM = color;
        setButtonIcon(ui->buttonColorAZIM, colorAZIM);
    }
}

void OptionsDialog::on_buttonColorAZIMReferenceLines_clicked() {
    QColor color = QColorDialog::getColor(colorAZIMReferenceLines, this, "Set AZIM Reference Lines Color");
    if (color.isValid()) {
        colorAZIMReferenceLines = color;
        setButtonIcon(ui->buttonColorAZIMReferenceLines, colorAZIMReferenceLines);
    }
}

void OptionsDialog::on_buttonColorHANG_clicked() {
    QColor color = QColorDialog::getColor(colorHANG, this, "Set HANG Color");
    if (color.isValid()) {
        colorHANG = color;
        setButtonIcon(ui->buttonColorHANG, colorHANG);
    }
}

void OptionsDialog::on_buttonColorHANGReferenceLines_clicked() {
    QColor color = QColorDialog::getColor(colorHANGReferenceLines, this, "Set HANG Reference Lines Color");
    if (color.isValid()) {
        colorHANGReferenceLines = color;
        setButtonIcon(ui->buttonColorHANGReferenceLines, colorHANGReferenceLines);
    }
}

void OptionsDialog::on_buttonColorZANG_clicked() {
    QColor color = QColorDialog::getColor(colorZANG, this, "Set ZANG Color");
    if (color.isValid()) {
        colorZANG = color;
        setButtonIcon(ui->buttonColorZANG, colorZANG);
    }
}

void OptionsDialog::on_buttonColorDIST_clicked() {
    QColor color = QColorDialog::getColor(colorDIST, this, "Set DIST Color");
    if (color.isValid()) {
        colorDIST = color;
        setButtonIcon(ui->buttonColorDIST, colorDIST);
    }
}

void OptionsDialog::on_buttonColorDXYZ_clicked() {
    QColor color = QColorDialog::getColor(colorDXYZ, this, "Set DXYZ Color");
    if (color.isValid()) {
        colorDXYZ = color;
        setButtonIcon(ui->buttonColorDXYZ, colorDXYZ);
    }
}

void OptionsDialog::on_buttonColorHDIR_clicked() {
    QColor color = QColorDialog::getColor(colorHDIR, this, "Set HDIR Color");
    if (color.isValid()) {
        colorHDIR = color;
        setButtonIcon(ui->buttonColorHDIR, colorHDIR);
    }
}

void OptionsDialog::on_buttonColorHDIF_clicked() {
    QColor color = QColorDialog::getColor(colorHDIF, this, "Set HDIF Color");
    if (color.isValid()) {
        colorHDIF = color;
        setButtonIcon(ui->buttonColorHDIF, colorHDIF);
    }
}

void OptionsDialog::on_buttonColorVANG_clicked() {
    QColor color = QColorDialog::getColor(colorVANG, this, "Set VANG Color");
    if (color.isValid()) {
        colorVANG = color;
        setButtonIcon(ui->buttonColorVANG, colorVANG);
    }
}

void OptionsDialog::on_buttonColorEllipse_clicked() {
    QColor color = QColorDialog::getColor(colorEllipse, this, "Set Ellipse Color");
    if (color.isValid()) {
        colorEllipse = color;
        setButtonIcon(ui->buttonColorEllipse, colorEllipse);
    }
}

void OptionsDialog::on_buttonColorRectangle_clicked() {
    QColor color = QColorDialog::getColor(colorEllipse, this, "Set Rectangle Color");
    if (color.isValid()) {
        colorRectangle = color;
        setButtonIcon(ui->buttonColorRectangle, colorRectangle);
    }
}

void OptionsDialog::on_buttonColorHighlighting_clicked() {
    QColor color = QColorDialog::getColor(colorHighlighting, this, "Set Highlighting Color");
    if (color.isValid()) {
        colorHighlighting = color;
        setButtonIcon(ui->buttonColorHighlighting, colorHighlighting);
    }
}

void OptionsDialog::on_checkResetToDefaults_clicked() {
    if (ui->checkResetToDefaults->isChecked()) {
        // disable all style controls
        setStyleControlsEnabled(false);
    } else {
        // enable all style controls
        setStyleControlsEnabled(true);
    }
}

void OptionsDialog::setStyleControlsEnabled(bool enabled) {

    ui->buttonColorAZIM->setEnabled(enabled);
    ui->spinBoxLineWidthAZIM->setEnabled(enabled);
    ui->comboBoxPenAZIM->setEnabled(enabled);

    ui->buttonColorAZIMReferenceLines->setEnabled(enabled);
    ui->spinBoxLineWidthAZIMReferenceLines->setEnabled(enabled);
    ui->comboBoxPenAZIMReferenceLines->setEnabled(enabled);

    ui->buttonColorHANG->setEnabled(enabled);
    ui->spinBoxLineWidthHANG->setEnabled(enabled);
    ui->comboBoxPenHANG->setEnabled(enabled);

    ui->buttonColorHANGReferenceLines->setEnabled(enabled);
    ui->spinBoxLineWidthHANGReferenceLines->setEnabled(enabled);
    ui->comboBoxPenHANGReferenceLines->setEnabled(enabled);

    ui->buttonColorZANG->setEnabled(enabled);
    ui->spinBoxLineWidthZANG->setEnabled(enabled);
    ui->comboBoxPenZANG->setEnabled(enabled);

    ui->buttonColorDIST->setEnabled(enabled);
    ui->spinBoxLineWidthDIST->setEnabled(enabled);
    ui->comboBoxPenDIST->setEnabled(enabled);

    ui->buttonColorDXYZ->setEnabled(enabled);
    ui->spinBoxLineWidthDXYZ->setEnabled(enabled);
    ui->comboBoxPenDXYZ->setEnabled(enabled);

    ui->buttonColorHDIR->setEnabled(enabled);
    ui->spinBoxLineWidthHDIR->setEnabled(enabled);
    ui->comboBoxPenHDIR->setEnabled(enabled);

    ui->buttonColorHDIF->setEnabled(enabled);
    ui->spinBoxLineWidthHDIF->setEnabled(enabled);
    ui->comboBoxPenHDIF->setEnabled(enabled);

    ui->buttonColorVANG->setEnabled(enabled);
    ui->spinBoxLineWidthVANG->setEnabled(enabled);
    ui->comboBoxPenVANG->setEnabled(enabled);

    ui->buttonColorEllipse->setEnabled(enabled);
    ui->spinBoxLineWidthEllipse->setEnabled(enabled);
    ui->comboBoxPenEllipse->setEnabled(enabled);

    ui->buttonColorRectangle->setEnabled(enabled);
    ui->spinBoxLineWidthRectangle->setEnabled(enabled);
    ui->comboBoxPenRectangle->setEnabled(enabled);

    ui->buttonColorHighlighting->setEnabled(enabled);
}

void OptionsDialog::setButtonIcon(QPushButton *button, QColor color) {
    QPixmap iconColor(32,16);
    iconColor.fill(color);
    button->setIcon(QIcon(iconColor));
}

void OptionsDialog::setupPenStyleComboBox(QComboBox * comboBox, qint32 currentStyle) {
    comboBox->addItem("Solid Line"       , (qint32)Qt::SolidLine);
    comboBox->addItem("Dash Line"        , (qint32)Qt::DashLine);
    comboBox->addItem("Dot Line"         , (qint32)Qt::DotLine);
    comboBox->addItem("Dash Dot Line"    , (qint32)Qt::DashDotDotLine);
    comboBox->addItem("Dash Dot Dot Line", (qint32)Qt::DashDotDotLine);
    qint32 currentIndex = comboBox->findData(currentStyle);
    comboBox->setCurrentIndex(currentIndex);
}

void OptionsDialog::pushToMapping() {
    if (ui->checkResetToDefaults->isChecked()) {
        map->useDefaultStyle();
        return;
    }

    map->setColorAZIM(colorAZIM);
    map->setLineWidthAZIM(ui->spinBoxLineWidthAZIM->value());
    map->setPenStyleAZIM(ui->comboBoxPenAZIM->currentData().toInt());

    map->setColorAZIMReferenceLines(colorAZIMReferenceLines);
    map->setLineWidthAZIMReferenceLines(ui->spinBoxLineWidthAZIMReferenceLines->value());
    map->setPenStyleAZIMReferenceLines(ui->comboBoxPenAZIMReferenceLines->currentData().toInt());

    map->setColorHANG(colorHANG);
    map->setLineWidthHANG(ui->spinBoxLineWidthHANG->value());
    map->setPenStyleHANG(ui->comboBoxPenHANG->currentData().toInt());

    map->setColorHANGReferenceLines(colorHANGReferenceLines);
    map->setLineWidthHANGReferenceLines(ui->spinBoxLineWidthHANGReferenceLines->value());
    map->setPenStyleHANGReferenceLines(ui->comboBoxPenHANGReferenceLines->currentData().toInt());

    map->setColorZANG(colorZANG);
    map->setLineWidthZANG(ui->spinBoxLineWidthZANG->value());
    map->setPenStyleZANG(ui->comboBoxPenZANG->currentData().toInt());

    map->setColorDIST(colorDIST);
    map->setLineWidthDIST(ui->spinBoxLineWidthDIST->value());
    map->setPenStyleDIST(ui->comboBoxPenDIST->currentData().toInt());

    map->setColorDXYZ(colorDXYZ);
    map->setLineWidthDXYZ(ui->spinBoxLineWidthDXYZ->value());
    map->setPenStyleDXYZ(ui->comboBoxPenDXYZ->currentData().toInt());

    map->setColorHDIR(colorHDIR);
    map->setLineWidthHDIR(ui->spinBoxLineWidthHDIR->value());
    map->setPenStyleHDIR(ui->comboBoxPenHDIR->currentData().toInt());

    map->setColorHDIF(colorHDIF);
    map->setLineWidthHDIF(ui->spinBoxLineWidthHDIF->value());
    map->setPenStyleHDIF(ui->comboBoxPenHDIF->currentData().toInt());

    map->setColorVANG(colorVANG);
    map->setLineWidthVANG(ui->spinBoxLineWidthVANG->value());
    map->setPenStyleVANG(ui->comboBoxPenVANG->currentData().toInt());

    map->setColorEllipse(colorEllipse);
    map->setLineWidthEllipse(ui->spinBoxLineWidthEllipse->value());
    map->setPenStyleEllipse(ui->comboBoxPenEllipse->currentData().toInt());

    map->setColorRectangle(colorRectangle);
    map->setLineWidthRectangle(ui->spinBoxLineWidthRectangle->value());
    map->setPenStyleRectangle(ui->comboBoxPenRectangle->currentData().toInt());

    map->setColorHighlighting(colorHighlighting);
}


