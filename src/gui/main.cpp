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
#include "MainWindow.hpp"
#include <string>
#include <QApplication>
#include <QStyleFactory>
#include <LSAProxyStyle.hpp>

#include "LSASupportedToVersion.hpp"
#include "QtUtilityMethods.hpp"

#ifdef _WIN32
    // Don't open a console window when launching the gui in windows
    #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif




int main(int argc, char *argv[])
{
    QApplication salsaApplication(argc, argv);

    // Set some data required by QSettings
    salsaApplication.setOrganizationName(lsa::salsaOrgName);
    salsaApplication.setOrganizationDomain(lsa::salsaOrgDomain);
    salsaApplication.setApplicationName(lsa::salsaApplicationName);

    // Remove question mark in windows
    salsaApplication.setAttribute(Qt::AA_DisableWindowContextHelpButton);

    // Set application font
    QFont font(lsa::SALSA_PRIMARY_FONT);
    int pSize = getIdealFontSize();
    font.setPointSize(pSize);
    font.setWeight(QFont::Normal);

    salsaApplication.setFont(font);

    qputenv("salsaAppName","SALSA");

    QString lsaFile = "";
    if (argc > 1 && strcmp(argv[1],"-lsafile") == 0)
    {
        lsaFile = argv[2];
    }
    else if (QApplication::arguments().size() > 1)
    {
        const QString FILENAME = QApplication::arguments().at(1);
        QFileInfo file(FILENAME);

        bool validFile = false;

        if(file.exists())
        {
            QFile inputFile(FILENAME);
            if(inputFile.open(QIODevice::ReadOnly))
            {
                QTextStream in(&inputFile);
                QString line = in.readLine();

                // Verify that the first line contains "Created by SALSA version"
                if(line.contains(LSAVERSION_FILE_CREATED_STRING.c_str()))
                    validFile = true;

                inputFile.close();
            }
        }

        if(validFile)
            lsaFile = file.absoluteFilePath();
    }

    // Create the mainwindow and apply some geometry settings cached in QSettings
    MainWindow mainWindow(lsaFile);
    mainWindow.loadAndApplyWindowSettings();
    QObject::connect(&salsaApplication, SIGNAL(focusChanged(QWidget*, QWidget*)),
                     &mainWindow, SLOT(handleFocusChanged(QWidget*, QWidget*)) );

    // Connect application state changed signal and slot
    //QObject::connect(&salsaApplication, SIGNAL(applicationStateChanged(Qt::ApplicationState)), &mainWindow, SLOT(handleApplicationStateChanged(Qt::ApplicationState)) );

    // Set style sheets
    salsaApplication.setStyleSheet(
        "QSplitter::handle:horizontal      {image: url(:/guiIcons/vbars.png); }"
        "QSplitter::handle:vertical        {image: url(:/guiIcons/hbars.png); }"

        "QMainWindow::separator            {width: 14px; height: 14px; }"
        "QMainWindow::separator:horizontal {image: url(:/guiIcons/hbars.png); }"
        "QMainWindow::separator:vertical   {image: url(:/guiIcons/vbars.png); }"

        "QTreeView { background-color: white; }"
        "QTreeView { alternate-background-color: rgb(218,240,210);}"
        "QTreeView::item:selected:active{ background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6ea1f1, stop: 1 #567dbc);}"
        "QTreeView::item:selected:!active { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6b9be8, stop: 1 #577fbf);}"           
        "QTreeView::branch:closed:has-children{ border-image: none; image: url(:/guiIcons/branchClosed.png); }"
        "QTreeView::branch:open:has-children { border-image: none; image: url(:/guiIcons/branchOpen.png); }"
        "QTableView::item:selected:active{ background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6ea1f1, stop: 1 #567dbc);}"
        "QTableView::item:selected:!active { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6b9be8, stop: 1 #577fbf);}"
        );

    // Set the application style so that tree view items expand on mouse release instead of mouse press
    auto lsaStyle = new LSAProxyStyle;
    lsaStyle->setBaseStyle(QStyleFactory::create("fusion"));
    salsaApplication.setStyle(lsaStyle);

    mainWindow.show();
    mainWindow.putAdjustedPositionsOnTop();

    // Set the application icon
    QIcon icon(":/guiIcons/salsa_icon.png");
    salsaApplication.setWindowIcon(icon);

    return salsaApplication.exec();
}
