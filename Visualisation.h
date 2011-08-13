#ifndef VISUALISATION_H
#define VISUALISATION_H

#include <QtGui>

class Visualisation: public QWidget
{
    Q_OBJECT

public:
    Visualisation(int width_, int height_, QWidget *parent = NULL);
    void setMap(QVector<QVector<QPointF> >);

private slots:
    void handleKeys();

private:
    void mousePressEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

    void restart();// Resets the state to initial(curPos = (0, 0), curAngle = 45 degrees), all the internal variables will be reset too
    void calculatePath(const QPointF &, const QPointF &);// Calculates the path and saves it in the "path" variable.
    bool move();// Tries to move and returns true if moved
    void discover();
    void updateVirtualWalls();
    QPair<QPointF, QPointF> pivots(const QPointF &, const QPointF &, const QPointF &);
    QVector<QPointF> getPivots(const QVector<QPointF> &);


    qreal pivotOffset;// A parameter for conflicts exclusion. Path should be binded not to segments' joints, but to the nearby located points.
                      // This parameter sets there points' offset from the joint points.
    qreal cellSize;// The discovered zones edges are being drawn as circles, so this parameter affects smoothing.
                   // Also, it significantly affects the perfomance.
    qreal fovDist, fovAngle;// FOV is a circle sector with the radius fovDist and the central angle fovAngle(in radians)
    qreal moveSpeed, rotSpeed;// rotSpeed is in radians

    QPointF curPos;
    qreal curAngle;

    QVector<QVector<QPointF > > map;// contains just map, shoudn't be changed
    QVector<QVector<bool> > isDiscovered;// The Visualisation field is a grid, so some points of this grid are already discovered, some not
                                         // To convert grid nodes into real coordinates, you'll just multiply it by cellSize.
    QVector<QPointF> path;// An internal variable, contains bath between startPos and targetPos

    QPointF startPos, targetPos;// Internal variables, programm will find the path between these points on every discover() action(if any).
    
    QHash<int, bool> pressed;// This map contains the states of the keys, to support key combinations(e.g. to rotate and move simultaneously)

    QVector<QVector<QPointF> > virtualWalls;// These walls are formed by the undiscover zone.
                                            // They are "virtual" because they don't physically exist(unlike the walls formed by the "map" variable, their purpose is limitation for the path search algorithm
#ifdef DEBUG
    QVector<QPointF> dbgPivots;
    QVector<QPointF> dbgConnComp;
#endif

};

#endif //VISUALISATION_H
