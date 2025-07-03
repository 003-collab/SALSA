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
#include "LabelChangeDialog.hpp"
#include <QCoreApplication>

LabelChangeDialog::LabelChangeDialog(QWidget *parent, GuiModel *guiModel, LSAType lsaType, std::string label, LSAOperationType operation) : QMessageBox(parent),
    guiModel(guiModel), lsaType(lsaType), label(label), operation(operation)
{
    // Get the number of references to label
    if (lsaType.isModifier())
    {
        numLabelRefs = guiModel->getNumReferencesToModifier(lsaType, label);
    }
    else if (lsaType == LSAType::DGRP)
    {
        numLabelRefs = guiModel->getRecordsReferencingDirGroup(label).size();
    }
    else if (lsaType.isPosition())
    {
        numLabelRefs = guiModel->getRecordsReferencingPosition(label).size();
    }

    QString operationType;
    if (operation == LSAOperationType::Delete)
        operationType = "remove";
    else if (operation == LSAOperationType::Modify)
        operationType = "modify";

    if (numLabelRefs > 0 )
    {
        QString qLabel        = QString::fromStdString(label);
        QString modifierType  = QString::fromStdString(lsaType.asString() + " ");

        QString info;
        QString question;
        if (numLabelRefs == 1)
        {
            info = "This project has 1 reference to " + modifierType + qLabel + ".";
            question = "Would you like to " + operationType + " the reference?";
        }
        else
        {
            info = "This project has " + QString::number(numLabelRefs) + " references to " + modifierType + qLabel + ".";
            question = "Would you like to " + operationType + " the references?";
        }

        setWindowModality(Qt::ApplicationModal);
        setIcon(QMessageBox::Question);
        setText(info);
        setInformativeText(question);
        if (operation == LSAOperationType::Delete)
        {
            setStandardButtons(QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes);
        }
        else
        {
            setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        }
        setDefaultButton(QMessageBox::Yes);
    }
}

int LabelChangeDialog::exec()
{  
    // Don't make changes for operations other than delete or modify
    if (operation != LSAOperationType::Delete && operation != LSAOperationType::Modify)
        return QMessageBox::No;

    // Don't attempt to make changes if there are no records referencing this label
    if (numLabelRefs == 0)
        return QMessageBox::No;

    return QMessageBox::exec();
}

