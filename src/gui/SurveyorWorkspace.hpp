/*
    Copyright (c) 2026 OpenAI
    This file is part of the SALSA surveyor UI prototype.

    SALSA is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License version 3 (GPL-3.0-only).
*/
#ifndef SALSA_SURVEYOR_WORKSPACE_HPP
#define SALSA_SURVEYOR_WORKSPACE_HPP

#include <QDockWidget>

class GuiModel;
class QTreeView;
class QLabel;
class QTableWidget;
class QTabWidget;
class QWidget;

class SurveyorWorkspace : public QDockWidget
{
    Q_OBJECT

public:
    explicit SurveyorWorkspace(GuiModel *model, QWidget *parent = nullptr);
    void setProjectModel(GuiModel *model);

signals:
    void runAdjustmentRequested();

private:
    void populateRecordTables(GuiModel *model);

    QTreeView *projectTree;
    QLabel *projectSummary;
    QTableWidget *pointsTable;
    QTableWidget *observationsTable;
    QTabWidget *dataTabs;
    QWidget *networkView;
};

#endif // SALSA_SURVEYOR_WORKSPACE_HPP
