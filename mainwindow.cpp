#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QLineEdit>
#include <QPen>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace {

QPointF scenePoint(double x, double y)
{
    // В модели Y направлена вверх, в QGraphicsScene - вниз.
    return QPointF(x, -y);
}

bool readAcceleration(const QLineEdit *edit, double &value)
{
    QString text = edit->text().trimmed();
    text.replace(',', '.');

    bool ok = false;
    value = text.toDouble(&ok);
    return ok && std::isfinite(value) && value >= -10.0 && value <= 10.0;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      scene(new QGraphicsScene(this)),
      timer(new QTimer(this))
{
    ui->setupUi(this);
    ui->graphicsView->setScene(scene);

    QPen truePen(Qt::darkGreen, 2);
    QPen estimatedPen(Qt::blue, 2);
    truePen.setCosmetic(true);
    estimatedPen.setCosmetic(true);

    trueItem = scene->addPath(truePath, truePen);
    estimatedItem = scene->addPath(estimatedPath, estimatedPen);
    trueItem->setZValue(1);
    estimatedItem->setZValue(2);

    trueMarker = scene->addEllipse(-4, -4, 8, 8, QPen(Qt::NoPen),
                                  QBrush(Qt::darkGreen));
    estimatedMarker = scene->addEllipse(-3, -3, 6, 6, QPen(Qt::NoPen),
                                       QBrush(Qt::blue));
    trueMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    estimatedMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    trueMarker->setZValue(3);
    estimatedMarker->setZValue(4);

    legend = scene->addSimpleText(QString());
    legend->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    legend->setZValue(5);

    timer->setInterval(50);
    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &MainWindow::tick);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(ui->btnCorrect, &QPushButton::clicked, this, &MainWindow::correctPosition);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::resetSimulation);

    resetSimulation();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startSimulation()
{
    double ax = 0.0;
    double ay = 0.0;
    const bool validX = readAcceleration(ui->lineEditAx, ax);
    const bool validY = readAcceleration(ui->lineEditAy, ay);

    if (!validX || !validY) {
        status = QStringLiteral("Введите ускорения от -10 до 10 м/с².");
        updateStatus();
        QLineEdit *invalidEdit = validX ? ui->lineEditAy : ui->lineEditAx;
        invalidEdit->setFocus();
        invalidEdit->selectAll();
        return;
    }

    model.ax = ax;
    model.ay = ay;
    ui->lineEditAx->setEnabled(false);
    ui->lineEditAy->setEnabled(false);
    ui->btnStart->setEnabled(false);
    ui->btnCorrect->setEnabled(true);

    status = QStringLiteral("Работает");
    updateStatus();
    timer->start();
}

void MainWindow::correctPosition()
{
    model.correctPosition();
    // Коррекция - скачок оценки, а не реальное перемещение аппарата.
    estimatedPath.moveTo(scenePoint(model.xEst, model.yEst));
    status = QStringLiteral("Работает. Позиция скорректирована.");
    updateScene();
    updateStatus();
}

void MainWindow::resetSimulation()
{
    timer->stop();
    model.reset();

    truePath = QPainterPath();
    estimatedPath = QPainterPath();
    truePath.moveTo(0.0, 0.0);
    estimatedPath.moveTo(0.0, 0.0);

    sceneBounds = QRectF(-300.0, -200.0, 600.0, 400.0);
    scene->setSceneRect(sceneBounds);
    drawGrid();

    ui->lineEditAx->setEnabled(true);
    ui->lineEditAy->setEnabled(true);
    ui->btnStart->setEnabled(true);
    ui->btnCorrect->setEnabled(false);
    status = QStringLiteral("Ожидание");

    updateScene();
    updateStatus();
}

void MainWindow::tick()
{
    model.step();
    truePath.lineTo(scenePoint(model.xTrue, model.yTrue));
    estimatedPath.lineTo(scenePoint(model.xEst, model.yEst));
    updateScene();
    updateStatus();
}

void MainWindow::updateScene()
{
    trueItem->setPath(truePath);
    estimatedItem->setPath(estimatedPath);
    const QPointF truePosition = scenePoint(model.xTrue, model.yTrue);
    const QPointF estimatedPosition = scenePoint(model.xEst, model.yEst);
    trueMarker->setPos(truePosition);
    estimatedMarker->setPos(estimatedPosition);

    // Границы только расширяются: предыдущие участки остаются видны.
    QRectF bounds = sceneBounds;
    for (const QPointF &point : {truePosition, estimatedPosition}) {
        if (!bounds.adjusted(20.0, 20.0, -20.0, -20.0).contains(point)) {
            const double margin = 0.1 * std::max(bounds.width(), bounds.height());
            bounds.setLeft(std::min(bounds.left(), point.x() - margin));
            bounds.setRight(std::max(bounds.right(), point.x() + margin));
            bounds.setTop(std::min(bounds.top(), point.y() - margin));
            bounds.setBottom(std::max(bounds.bottom(), point.y() + margin));
        }
    }
    if (bounds != sceneBounds) {
        sceneBounds = bounds;
        scene->setSceneRect(sceneBounds);
        drawGrid();
    }
    fitScene();
}

void MainWindow::updateStatus()
{
    ui->labelStatus->setText(
        QStringLiteral("Статус: %1\n\nВремя: %2 с\nОшибка: %3 м\n\n"
                       "Смещение датчика:\n%4 м/с² по X и по Y")
            .arg(status)
            .arg(model.time(), 0, 'f', 2)
            .arg(model.error(), 0, 'f', 3)
            .arg(SimulationModel::bias, 0, 'f', 2));
}

void MainWindow::drawGrid()
{
    for (QGraphicsLineItem *line : gridLines)
        delete line;
    gridLines.clear();

    const double desiredStep = std::max(sceneBounds.width(), sceneBounds.height()) / 12.0;
    const double unit = std::pow(10.0, std::floor(std::log10(desiredStep)));
    const double fraction = desiredStep / unit;
    double multiplier = 10.0;
    if (fraction <= 1.0)
        multiplier = 1.0;
    else if (fraction <= 2.0)
        multiplier = 2.0;
    else if (fraction <= 5.0)
        multiplier = 5.0;
    const double step = unit * multiplier;

    QPen gridPen(QColor(225, 225, 225));
    QPen axisPen(QColor(150, 150, 150));
    gridPen.setCosmetic(true);
    axisPen.setCosmetic(true);

    for (double x = std::ceil(sceneBounds.left() / step) * step;
         x <= sceneBounds.right(); x += step) {
        auto *line = scene->addLine(x, sceneBounds.top(), x, sceneBounds.bottom(), gridPen);
        line->setZValue(-2);
        gridLines.append(line);
    }
    for (double y = std::ceil(sceneBounds.top() / step) * step;
         y <= sceneBounds.bottom(); y += step) {
        auto *line = scene->addLine(sceneBounds.left(), y, sceneBounds.right(), y, gridPen);
        line->setZValue(-2);
        gridLines.append(line);
    }

    auto *xAxis = scene->addLine(sceneBounds.left(), 0.0, sceneBounds.right(), 0.0, axisPen);
    auto *yAxis = scene->addLine(0.0, sceneBounds.top(), 0.0, sceneBounds.bottom(), axisPen);
    xAxis->setZValue(-1);
    yAxis->setZValue(-1);
    gridLines.append(xAxis);
    gridLines.append(yAxis);

    legend->setText(QStringLiteral("Синяя - оценка, зелёная - истина\n"
                                   "X вправо, Y вверх. Шаг сетки: %1 м")
                        .arg(step, 0, 'g', 3));
}

void MainWindow::fitScene()
{
    ui->graphicsView->fitInView(sceneBounds, Qt::KeepAspectRatio);
    const double scale = ui->graphicsView->transform().m11();
    if (scale > 0.0)
        legend->setPos(sceneBounds.left() + 10.0 / scale,
                       sceneBounds.top() + 10.0 / scale);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    fitScene();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    fitScene();
}
