/*
    Copyright (c) 2026 OpenAI
    This file is part of the SALSA surveyor UI prototype.

    SALSA is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License version 3 (GPL-3.0-only).
*/
#include "SurveyorWorkspace.hpp"

#include <GuiModel.hpp>
#include <LSARecord.hpp>
#include <LSAPosC.hpp>
#include <LSAPosG.hpp>
#include <LSADist.hpp>

#include <QAbstractItemView>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPaintEvent>
#include <QPointF>
#include <QRectF>
#include <QTreeView>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QStringList>
#include <QTableWidgetItem>

#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

namespace
{
class NetworkCanvas : public QWidget
{
public:
    explicit NetworkCanvas(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumSize(320, 240);
        setAutoFillBackground(true);
    }

    void setProjectModel(GuiModel *model) { projectModel = model; update(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), palette().base());
        painter.setPen(palette().text().color());
        if (!projectModel) {
            painter.drawText(rect(), Qt::AlignCenter, tr("Open a project to view its network"));
            return;
        }

        std::map<std::string, QPointF> points;
        std::vector<std::pair<std::string, std::string>> links;
        std::vector<QModelIndex> pending;
        for (int row = projectModel->rowCount() - 1; row >= 0; --row)
            pending.push_back(projectModel->index(row, 0));
        while (!pending.empty()) {
            QModelIndex index = pending.back();
            pending.pop_back();
            for (int row = projectModel->rowCount(index) - 1; row >= 0; --row)
                pending.push_back(projectModel->index(row, 0, index));
            LSARecord *record = projectModel->getLSARecord(index);
            if (!record) continue;
            LSAType type = record->getRecType();
            if (type == LSAType::POSC) {
                LSAPosC *p = static_cast<LSAPosC *>(record);
                points[p->label] = QPointF(p->x, p->y);
            } else if (type == LSAType::DIST) {
                LSADist *d = static_cast<LSADist *>(record);
                links.emplace_back(d->From, d->To);
            }
        }

        if (points.empty()) {
            painter.drawText(rect(), Qt::AlignCenter, tr("No Cartesian POSC points available. Geodetic POSG plotting will be added with CRS-aware projection."));
            return;
        }

        double minX = points.begin()->second.x(), maxX = minX;
        double minY = points.begin()->second.y(), maxY = minY;
        for (const auto &entry : points) {
            minX = std::min(minX, entry.second.x()); maxX = std::max(maxX, entry.second.x());
            minY = std::min(minY, entry.second.y()); maxY = std::max(maxY, entry.second.y());
        }
        const double spanX = std::max(1e-9, maxX - minX);
        const double spanY = std::max(1e-9, maxY - minY);
        const QRectF plot = QRectF(rect()).adjusted(36, 28, -36, -36);
        const double scale = std::min(plot.width() / spanX, plot.height() / spanY);
        const QPointF center((minX + maxX) / 2.0, (minY + maxY) / 2.0);
        auto screen = [&](const QPointF &p) {
            return QPointF(plot.center().x() + (p.x() - center.x()) * scale,
                           plot.center().y() - (p.y() - center.y()) * scale);
        };

        painter.setPen(QPen(palette().mid().color(), 1.2));
        for (const auto &link : links) {
            auto a = points.find(link.first), b = points.find(link.second);
            if (a != points.end() && b != points.end())
                painter.drawLine(screen(a->second), screen(b->second));
        }
        for (const auto &entry : points) {
            const QPointF p = screen(entry.second);
            painter.setPen(QPen(QColor("#1767a8"), 1));
            painter.setBrush(QColor("#d9ecfb"));
            painter.drawEllipse(p, 4.5, 4.5);
            painter.setPen(palette().text().color());
            painter.drawText(p + QPointF(7, -7), QString::fromStdString(entry.first));
        }
        painter.setPen(palette().mid().color());
        painter.drawText(QRect(8, 6, width() - 16, 18), Qt::AlignLeft | Qt::AlignVCenter,
                         tr("Initial POSC coordinates · %1 points · %2 distance links").arg(static_cast<int>(points.size())).arg(static_cast<int>(links.size())));
    }

private:
    GuiModel *projectModel = nullptr;
};

QString numberText(double value)
{
    std::ostringstream stream;
    stream << std::setprecision(12) << value;
    return QString::fromStdString(stream.str());
}

void prepareTable(QTableWidget *table, const QStringList &headers)
{
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
}

void putCell(QTableWidget *table, int row, int column, const QString &text)
{
    table->setItem(row, column, new QTableWidgetItem(text));
}
}

SurveyorWorkspace::SurveyorWorkspace(GuiModel *model, QWidget *parent)
    : QDockWidget(tr("Surveyor Workspace"), parent),
      projectTree(new QTreeView(this)),
      projectSummary(new QLabel(this)),
      pointsTable(new QTableWidget(this)),
      observationsTable(new QTableWidget(this)),
      dataTabs(new QTabWidget(this)),
      networkView(new NetworkCanvas(this))
{
    setObjectName(QStringLiteral("surveyorWorkspaceDock"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    setMinimumWidth(420);

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
        tr("Survey points and observations are read from SALSA's existing project model. The adjustment engine is unchanged."),
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

    prepareTable(pointsTable, {tr("Point"), tr("Record"), tr("Initial coordinates"), tr("Constraint"), tr("Units")});
    pointsTable->setObjectName(QStringLiteral("surveyorPointsTable"));
    prepareTable(observationsTable, {tr("Type"), tr("From / At"), tr("To"), tr("Value"), tr("Sigma"), tr("Record")});
    observationsTable->setObjectName(QStringLiteral("surveyorObservationsTable"));

    dataTabs->addTab(networkView, tr("Network"));
    dataTabs->addTab(pointsTable, tr("Points"));
    dataTabs->addTab(observationsTable, tr("Observations"));
    dataTabs->addTab(projectTree, tr("Project records"));

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
        tr("Coordinates shown here are input values. Adjusted coordinates and residual diagnostics will be added from solver results in a later step."),
        contents);
    notice->setWordWrap(true);
    notice->setStyleSheet(QStringLiteral("color: #7a8794; font-size: 10px;"));

    layout->addWidget(title);
    layout->addWidget(description);
    layout->addWidget(separator);
    layout->addWidget(projectSummary);
    layout->addWidget(dataTabs, 1);
    layout->addWidget(runButton);
    layout->addWidget(notice);

    setWidget(contents);
    setProjectModel(model);
}

void SurveyorWorkspace::setProjectModel(GuiModel *model)
{
    projectTree->setModel(model);
    static_cast<NetworkCanvas *>(networkView)->setProjectModel(model);
    populateRecordTables(model);

    if (model)
    {
        projectSummary->setText(tr("%1 points · %2 observations · %3 top-level records")
                                .arg(pointsTable->rowCount())
                                .arg(observationsTable->rowCount())
                                .arg(model->rowCount()));
        projectTree->expandToDepth(0);
    }
    else
    {
        projectSummary->setText(tr("No project data loaded"));
    }
}

void SurveyorWorkspace::populateRecordTables(GuiModel *model)
{
    pointsTable->setSortingEnabled(false);
    observationsTable->setSortingEnabled(false);
    pointsTable->setRowCount(0);
    observationsTable->setRowCount(0);

    if (!model)
        return;

    // Walk the actual tree model and use SALSA's typed records. Do not parse
    // the UI display strings to reconstruct coordinates or measurement fields.
    std::vector<QModelIndex> pending;
    for (int row = model->rowCount() - 1; row >= 0; --row)
        pending.push_back(model->index(row, 0));

    while (!pending.empty())
    {
        const QModelIndex index = pending.back();
        pending.pop_back();

        for (int row = model->rowCount(index) - 1; row >= 0; --row)
            pending.push_back(model->index(row, 0, index));

        LSARecord *record = model->getLSARecord(index);
        if (!record)
            continue;

        LSAType type = record->getRecType();
        if (type == LSAType::POSC || type == LSAType::POSG)
        {
            const int row = pointsTable->rowCount();
            pointsTable->insertRow(row);
            QString label;
            QString coordinates;
            QString constraint;
            QString units;

            if (type == LSAType::POSC)
            {
                LSAPosC *point = static_cast<LSAPosC *>(record);
                label = QString::fromStdString(point->label);
                coordinates = QStringLiteral("X %1, Y %2, Z %3")
                    .arg(numberText(point->x), numberText(point->y), numberText(point->z));
                constraint = point->fixedState == LSAFixedState::FIXED ? tr("Fixed") : QString::fromStdString(point->fixedState.asString());
                units = QString::fromStdString(point->posUnits);
            }
            else
            {
                LSAPosG *point = static_cast<LSAPosG *>(record);
                label = QString::fromStdString(point->label);
                coordinates = QStringLiteral("Lat %1°, Lon %2°, H %3")
                    .arg(numberText(point->latDecDeg), numberText(point->lonDecDeg), numberText(point->height));
                constraint = point->fixedState == LSAFixedState::FIXED ? tr("Fixed") :
                    QString::fromStdString(point->fixedState.asString());
                units = QString::fromStdString(point->heightUnits);
            }

            putCell(pointsTable, row, 0, label);
            putCell(pointsTable, row, 1, QString::fromStdString(type.asString()));
            putCell(pointsTable, row, 2, coordinates);
            putCell(pointsTable, row, 3, constraint);
            putCell(pointsTable, row, 4, units);
        }
        else if (type.isMeasurement())
        {
            const int row = observationsTable->rowCount();
            observationsTable->insertRow(row);
            const std::vector<std::string> references = record->getReferencedPositions();
            const QString from = references.empty() ? QString() : QString::fromStdString(references[0]);
            const QString to = references.size() < 2 ? QString() : QString::fromStdString(references[1]);
            QString value;
            QString sigma;

            if (type == LSAType::DIST)
            {
                LSADist *distance = static_cast<LSADist *>(record);
                value = numberText(distance->distance) + QStringLiteral(" ") + QString::fromStdString(distance->linUnits);
                sigma = numberText(distance->sigma) + QStringLiteral(" ") + QString::fromStdString(distance->linUnits);
            }
            else
            {
                value = QString::fromStdString(record->getSingleLSAString());
                sigma = QStringLiteral("—");
            }

            putCell(observationsTable, row, 0, QString::fromStdString(type.asString()));
            putCell(observationsTable, row, 1, from);
            putCell(observationsTable, row, 2, to);
            putCell(observationsTable, row, 3, value);
            putCell(observationsTable, row, 4, sigma);
            putCell(observationsTable, row, 5, QString::fromStdString(record->getSingleLSAString()));
        }
    }

    pointsTable->setSortingEnabled(true);
    observationsTable->setSortingEnabled(true);
    pointsTable->resizeColumnsToContents();
    observationsTable->resizeColumnsToContents();
}
