#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "simulationmodel.h"

#include <QList>
#include <QMainWindow>
#include <QPainterPath>
#include <QRectF>
#include <QString>

class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsScene;
class QGraphicsSimpleTextItem;
class QResizeEvent;
class QShowEvent;
class QTimer;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void startSimulation();
    void correctPosition();
    void resetSimulation();
    void tick();

private:
    void updateScene();
    void updateStatus();
    void drawGrid();
    void fitScene();

    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QTimer *timer;
    SimulationModel model;

    QPainterPath truePath;
    QPainterPath estimatedPath;
    QGraphicsPathItem *trueItem;
    QGraphicsPathItem *estimatedItem;
    QGraphicsEllipseItem *trueMarker;
    QGraphicsEllipseItem *estimatedMarker;
    QGraphicsSimpleTextItem *legend;
    QList<QGraphicsLineItem *> gridLines;

    QRectF sceneBounds;
    QString status;
};

#endif // MAINWINDOW_H
