/*
    Copyright (c) 2026 OpenAI
    This file is part of the SALSA surveyor UI prototype.

    SALSA is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License version 3 (GPL-3.0-only).
*/
#include "SurveyorWorkspace.hpp"

#include <GuiModel.hpp>

#include <QAbstractItemView>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

SurveyorWorkspace::SurveyorWorkspace(GuiModel *model, QWidget *parent)
    : QDockWidget(tr("Surveyor Workspace"), parent),
      projectTree(new QTreeView(this)),
      projectSummary(new QLabel(this))
{
    setObjectName(QStringLiteral("surveyorWorkspaceDock"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    setMinimumWidth(300);

    QWidget *contents = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(contents);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    QLabel *title = new QLabel(tr("Network workspace"), contents);
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    title->setFont(titleFont);

    QLabel *description = new QLabel(
        tr("Live project data from SALSA's existing model. Use the adjustment command to run the current solver workflow."),
        contents);
    description->setWordWrap(true);
    description->setStyleSheet(QStringLiteral("color: #687887;"));

    QFrame *separator = new QFrame(contents);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);

    projectSummary->setObjectName(QStringLiteral("surveyorProjectSummary"));
    projectSummary->setText(tr("No project data loaded"));
    projectSummary->setStyleSheet(QStringLiteral("font-weight: 600; color: #245f8b;"));

    projectTree->setObjectName(QStringLiteral("surveyorProjectTree"));
    projectTree->setAlternatingRowColors(true);
    projectTree->setRootIsDecorated(true);
    projectTree->setUniformRowHeights(true);
    projectTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projectTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    projectTree->setSelectionMode(QAbstractItemView::SingleSelection);
    projectTree->header()->setStretchLastSection(true);

    QPushButton *runButton = new QPushButton(tr("Run adjustment"), contents);
    runButton->setObjectName(QStringLiteral("surveyorRunAdjustmentButton"));
    runButton->setMinimumHeight(34);
    runButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: #1767a8; color: white; border: 1px solid #1767a8;"
        "border-radius: 4px; padding: 6px 12px; font-weight: 600; }"
        "QPushButton:hover { background: #10588f; }"
        "QPushButton:pressed { background: #0d4a78; }"));
    connect(runButton, &QPushButton::clicked, this, &SurveyorWorkspace::runAdjustmentRequested);

    QLabel *notice = new QLabel(
        tr("The network canvas and adjustment diagnostics will be connected to real solver results in a later milestone."),
        contents);
    notice->setWordWrap(true);
    notice->setStyleSheet(QStringLiteral("color: #7a8794; font-size: 10px;"));

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(separator);
    layout->addWidget(projectSummary);
    layout->addWidget(projectTree, 1);
    layout->addWidget(runButton);
    layout->addWidget(notice);

    setWidget(contents);
    setProjectModel(model);
}

void SurveyorWorkspace::setProjectModel(GuiModel *model)
{
    projectTree->setModel(model);
    if (model)
    {
        projectSummary->setText(tr("Project model: %1 top-level records")
                                .arg(model->rowCount()));
        projectTree->expandToDepth(0);
    }
    else
    {
        projectSummary->setText(tr("No project data loaded"));
    }
}
