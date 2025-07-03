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
#ifndef OPTIONSDIALOG_HPP
#define OPTIONSDIALOG_HPP

#include <QDialog>
#include <QColorDialog>
#include <QComboBox>
#include "mapping.hpp"

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(Mapping *mapWidget, QWidget *parent = 0);
    ~OptionsDialog();

    // Write the options set in OptionsDialog to the Mapping instance set in the constructor
    void pushToMapping();

private slots:
    // Slots to display the color picker dialog when each Color button is pushed
    void on_buttonColorAZIM_clicked();
    void on_buttonColorAZIMReferenceLines_clicked();
    void on_buttonColorHANG_clicked();
    void on_buttonColorHANGReferenceLines_clicked();
    void on_buttonColorZANG_clicked();
    void on_buttonColorDIST_clicked();
    void on_buttonColorDXYZ_clicked();
    void on_buttonColorHDIR_clicked();
    void on_buttonColorHDIF_clicked();
    void on_buttonColorVANG_clicked();
    void on_buttonColorEllipse_clicked();
    void on_buttonColorRectangle_clicked();
    void on_buttonColorHighlighting_clicked();

    void on_checkResetToDefaults_clicked();

private:
    Ui::OptionsDialog *ui;
    Mapping *map;

    // sets all style controls enabled (enabled == true) or disabled (enabled = false)
    // used to disable controls while reset to defaults is checked
    void setStyleControlsEnabled(bool enabled);

    // helper method for constructor to draw a color icon on a button
    void setButtonIcon(QPushButton *button, QColor color);

    // helper method for constructor to set up a combo box to select pen styles
    void setupPenStyleComboBox(QComboBox * comboBox, qint32 currentStyle);

    // The color for each placemark type needs to be stored, but the line widths and pen styles will
    // be read directly from the double spin boxes and combo boxes, respectively.
    QColor colorAZIM;
    QColor colorAZIMReferenceLines;
    QColor colorHANG;
    QColor colorHANGReferenceLines;
    QColor colorZANG;
    QColor colorDIST;
    QColor colorDXYZ;
    QColor colorHDIR;
    QColor colorHDIF;
    QColor colorVANG;
    QColor colorEllipse; 
    QColor colorRectangle;
    QColor colorHighlighting;
};

#endif // OPTIONSDIALOG_HPP
