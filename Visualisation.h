#ifndef VISUALISATION_H
#define VISUALISATION_H

class Visualisation: public QWidget
{
public:
    Visualisation(int width_, int height_, QWidget *parent = NULL);
    void setMap(QVector<QVector<QPointF> >);

private:
    void mousePressEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *);

    void restart();
    void calculatePath();
    QPair<QPointF, QPointF> pivots(const QPointF &, const QPointF &, const QPointF &);
    QVector<QPointF> getPivots(const QVector<QPointF> &);
    void discover();

    QPointF curPos;
    qreal curAngle;
    qreal pivotOffset;
    qreal cellSize;
    qreal fovDist, fovAngle;
    qreal moveSpeed, rotSpeed;
    QVector<QVector<QPointF > > map;
    QVector<QVector<bool> > isDiscovered;
    QVector<QPointF> path;

    QPointF startPos, targetPos;

#ifdef DEBUG
    QVector<QPointF> dbgPivots;
#endif

};

#endif //VISUALISATION_H
