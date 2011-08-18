#ifndef VISUALISATION_H
#define VISUALISATION_H

#include <QtGui>

class Visualisation: public QWidget
{
    Q_OBJECT

public:
    Visualisation(int width_, int height_, QVector<QVector<QPointF> > map_,  QWidget *parent = NULL);

public slots:
    void togglePause();
    void toggleManualControl();

private slots:
    void makeMove();

private:
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

    void makeAIMove();// Follows the path in the "path" variable
    bool makeManualMove();// Tries to move and returns true if could. If couldn't, tries to rotate.

    void handleKeys();

    bool makeMoveByLine(const QPointF &a, const QPointF &b);// helper method for makeAIMove. Rotates while curAngle isn't equal to
                                                            // Line(a, b).angle, then follows this line.
    QPointF getAITarget() const;// Finds the point with the hightest potential.

    bool exploreMap();// Updates the "isExplored" variable. Returns true if finds a new point
    QVector<QVector<int> > determineConnComp() const;// A helper function for updateVirtualWalls
    void updateVirtualWalls();

    void updatePotential();

    QPair<QPointF, QPointF> getVertexPivots(const QPointF &a, const QPointF &b, const QPointF &c) const;// Calculates the pivots for line [a, b][b, c]. The first element in the return value will always be the "inner" point
    QVector<QPointF> getPivots(const QVector<QPointF> &, bool mapPivots = false) const;// Calculates pivots for the polyline(might be enclosed). Additional parameter is for correct handling of map pivots generating.
    QHash<QPointF, QVector<QPointF> > getGraph(const QPointF &startPos, const QPointF &targetPos) const;// Calculates the visibility graph and returns it
    QVector<QPointF> getPath(const QPointF &startPos, const QPointF &targetPos) const;// Calculates the path and returns it
    bool wallOnPathTo(const QPointF &a) const;// Returns true if there's a wall on the line from curPoint to a
    void addVisitsCount(const QPointF &p, qreal value = 20.0);//Adds visits count to the point and its neighbours(affection radius is set in the method).


    qreal moveSpeed, rotSpeed;// rotSpeed is in radians
    qreal fovDist, fovAngle;// FOV(field of view) is a circle sector with the radius fovDist and the central angle fovAngle(in radians)
    QPointF curPos;
    qreal curAngle;

    qreal pivotOffset; // A parameter for conflicts exclusion. Path should be binded not to polygonal chains' vertices, but to the nearby located point, soThis parameter sets there points' offset from the vertices.
    qreal cellSize;// The discovered zones edges are being drawn as circles, so this parameter affects "smoothing". Also, it significantly affects the perfomance.

    enum Control
    {
        ManualContol,
        AIControl
    };
    Control control;

    enum ExplorationState
    {
        NoState,
        FollowPathState,
        PointExploreStateCW,// starts rotating and tries to stop with the given probality each moment(I consider it as a dirty hack, but don't see another sufficient method to handle it)
        PointExploreStateCCW //the same, but counter-clockwise
    };
    ExplorationState state;

    QVector<QVector<qreal> > potential;// The potential heuristic is formed by the nearby located undiscovered point(they increase it) and by the nearby located points' visits(they decrease it).
    QVector<QVector<int> > visits;// Not exactly the visits count, but comparatively to other points, it's the time the bot was close to the point.
    QVector<QVector<QPointF > > map;// Contains just the map, shoudn't be changed during the visualisation. Changed once in the constructor to add the screen edges.
    QVector<QPointF> mapPivots;// Initialized at the startup, for the better perfomance.
    QVector<QVector<bool> > isDiscovered;// The Visualisation field is a grid, so some points of this grid are already discovered, some not
                                         // To convert grid nodes into real coordinates, you'll just multiply it by cellSize.
    QVector<QPointF> path;// Contains the path to targetPos

    QPointF targetPos;// Program will follow the path to this point
    
    QHash<int, bool> pressedKeys;// This map contains the states of the keys, to support key combinations(e.g. to rotate and move simultaneously)

    QVector<QVector<QPointF> > virtualWalls;// These walls are formed by the edges of the undiscovered zone.
                                            // They are "virtual" because they don't physically exist(unlike the walls formed by the "map" variable, their purpose is limitation for the path search algorithm

    QTimer* timer;// Calls makeMove
#ifdef DEBUG
    mutable QVector<QPointF> dbgPivots;
    mutable QVector<QPointF> dbgConnComp;
    mutable QVector<QVector<int> > dbgCompNumber;
#endif

};

#endif //VISUALISATION_H
