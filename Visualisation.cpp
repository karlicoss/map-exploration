#include <QtGui>

#include <queue>
#include <cmath>
#include <algorithm>

#include "Visualisation.h"
#include "tools.h"

Visualisation::Visualisation(int width_, int height_, QVector<QVector<QPointF> > map_, QWidget *parent):
    QWidget(parent),
    moveSpeed(10.0), rotSpeed(0.1),
    fovDist(200.0), fovAngle(degr2rad(60.0)),
    curPos(1, 1), curAngle(degr2rad(-45.0)),
    pivotOffset(8.0), cellSize(8.0),
    control(AIControl),
    state(NoState),
    map(map_),
    targetPos(curPos),
    timer(new QTimer(this))
{
   qsrand(10);
   setFixedSize(width_, height_);
   setFocusPolicy(Qt::StrongFocus);

   connect(timer, SIGNAL(timeout()), this, SLOT(makeMove()));

   int cellsx = width() / cellSize + 1;
   int cellsy = height() / cellSize + 1;
   isDiscovered = QVector<QVector<bool> > (cellsx, QVector<bool> (cellsy, false));
   isDiscovered[0][0] = true;
   isDiscovered[1][1] = true;
   isDiscovered[0][1] = true;
   isDiscovered[1][0] = true;

   visits = QVector<QVector<int> > (cellsx, QVector<int> (cellsy, 0));
   potential = QVector<QVector<qreal> > (cellsx, QVector<qreal> (cellsy, 0.0));

   QPointF p00 = QPointF(0, 0), p10 = QPointF(width() - 1, 0), p01 = QPointF(0, height() - 1), p11 = QPointF(width() - 1, height() - 1);
   QVector<QPointF> edge;
   edge.append(p00);
   edge.append(p01);
   edge.append(p11);
   edge.append(p10);
   edge.append(p00);
   map.append(edge);

   for (int i = 0; i < map.size(); i++)
       mapPivots += getPivots(map[i], true);

   timer->start(50);// let it begin!
}

void Visualisation::keyPressEvent(QKeyEvent *e)
{
    pressedKeys[e->key()] = true;
}

void Visualisation::keyReleaseEvent(QKeyEvent *e)
{
    pressedKeys[e->key()] = false;
}

void Visualisation::paintEvent(QPaintEvent *)
{
    QPainterPath posMark;
    qreal markWidth = 20, markHeight = 10;
    QPolygonF triangle;
    triangle << QPointF(-markWidth / 2, markHeight / 2) << QPointF(-markWidth / 2, -markHeight / 2) << QPointF(markWidth / 2, 0);
    triangle << triangle.front();
    QMatrix rotM;
    rotM.translate(curPos.x(), curPos.y());
    rotM.rotate(-rad2degr(curAngle));
    triangle = rotM.map(triangle);
    posMark.addPolygon(triangle);

    QPen pathPen(QColor(51, 204, 255), 2);
    QPen mapPen(QColor(45, 0, 179), 3);
    QPen bkgPen(Qt::white);
    QPen undiscPen(Qt::black);
    QPen posMarkPen(Qt::green);

    QBrush bkgBrush(Qt::white);
    QBrush undiscBrush(Qt::black);
    QBrush posMarkBrush(Qt::green);

    QImage img(width(), height(), QImage::Format_RGB32);
    QPainter p(&img);// Drawing on the widget directly causes perfomance loss.


    p.setPen(bkgPen);
    p.setBrush(bkgBrush);
    p.drawRect(QRectF(0, 0, width() - 1, height() - 1));

    p.setPen(mapPen);
    for (int i = 0; i < map.size(); i++)
        for (int j = 0; j < map[i].size() - 1; j++)
            p.drawLine(QLineF(map[i][j], map[i][j + 1]));

    p.setPen(undiscPen);
#ifdef DEBUG
    p.setPen(Qt::green);
#endif
    p.setBrush(undiscBrush);
#ifdef DEBUG
    p.setPen(Qt::yellow);
#endif
    for (int i = 0; i < isDiscovered.size(); i++)
    {
        for (int j = 0; j < isDiscovered[0].size(); j++)
        {
            if (!isDiscovered[i][j])
            {
                p.drawEllipse(cellSize * QPointF(i, j), cellSize, cellSize);
            }
        }
    }
    
    p.setPen(pathPen);
    for (int i = 0; i < path.size() - 1; i++)
        p.drawLine(QLineF(path[i], path[i + 1]));

    p.setPen(posMarkPen);
    p.setBrush(posMarkBrush);
    p.drawPath(posMark);

#ifdef DEBUG
    for (int i = 0; i < dbgCompNumber.size(); i++)
        for (int j = 0; j < dbgCompNumber[0].size(); j++)
        {
            if (dbgCompNumber[i][j] == 2)//aka undiscovered
            {
                p.setPen(Qt::red);
                p.setBrush(Qt::red);
                p.drawEllipse(cellSize * QPointF(i, j), 2, 2);
            }
            else
            {
                p.setPen(Qt::green);
                p.setBrush(Qt::green);
                p.drawEllipse(cellSize * QPointF(i, j), 1, 1);
            }
            p.setPen(Qt::blue);
            if (i % 2 == 0 && j % 2 == 0 && potential[i][j] > -10000)
                p.drawText(cellSize * QPointF(i, j), QString::number(int(potential[i][j])));
        }
   
    p.setPen(Qt::magenta);// Drawing the FOV
    p.setBrush(Qt::transparent);
    p.drawPie(QRectF(curPos - QPointF(fovDist, fovDist), curPos + QPointF(fovDist, fovDist)), rad2degr(curAngle - fovAngle / 2) * 16, rad2degr(fovAngle) * 16);

    p.setPen(QPen(Qt::blue, 5)); // Drawing the virtual wals
    p.setBrush(Qt::blue);
    for (int i = 0; i < virtualWalls.size(); i++)
    {
        for (int j = 0; j < virtualWalls[i].size() - 1; j++)
        {
            p.drawLine(QLineF(virtualWalls[i][j], virtualWalls[i][j + 1]));
            p.drawEllipse(virtualWalls[i][j], 1, 1);
        }
    }
/*
    p.setPen(Qt::green); // Drawing the pivots
    p.setBrush(Qt::green);
    for (int i = 0; i < dbgPivots.size(); i++)
    {
        p.drawEllipse(dbgPivots[i], 3, 3);
        p.drawText(dbgPivots[i], QString::number(i));
    }
*/
#endif
    p.drawEllipse(targetPos, 3, 3);

    QPainter q(this);
    q.drawImage(QPointF(0, 0), img);// And finally..Drawing the whole image on the widget.

}

QPair<QPointF, QPointF> Visualisation::getVertexPivots(const QPointF &a, const QPointF &b, const QPointF &c) const
{
    QLineF dir1 = QLineF(b, a).unitVector(),
           dir2 = QLineF(b, c).unitVector();
    QLineF helper = QLineF(a, c).normalVector();

    QPointF med = (dir1.p2() + dir2.p2()) / 2.0;
    QLineF dir = QLineF(b, med);
    dir.setLength(pivotOffset);
    QPointF d1 = dir.p2(), d2 = dir.p1() - (dir.p2() - dir.p1());

    int angle = helper.angleTo(dir);
    if (90 <= angle && angle < 270) //First point will always be in the inner side
        return qMakePair(d2, d1);
    else
        return qMakePair(d1, d2);
}


QVector<QPointF> Visualisation::getPivots(const QVector<QPointF> &v, bool mapPivots) const
{
    bool innerZone = isDiscovered[v[0].x() / cellSize][v[0].y() / cellSize];
    QVector<QPointF> ans;
    if (v.size() == 1)
    {
        ans.append(v.front() - QPointF(pivotOffset, 0));
        ans.append(v.front() - QPointF(0, pivotOffset));
        ans.append(v.front() + QPointF(pivotOffset, 0));
        ans.append(v.front() + QPointF(0, pivotOffset));
        return ans;
    }
    if (v.front() == v.back() && v.size() > 2) // the line is enclosed
    {
        QPair<QPointF, QPointF> pv = getVertexPivots(v[v.size() - 2], v[0], v[1]);
        ans.append(pv.first);
        ans.append(pv.second);
    }
    else
    {
        // The line is not enclosed, so we'll use two points on its extension to the first and the last vertices.
        QLineF dir1 = QLineF(v[0], v[1]),
               dir2 = QLineF(v[v.size() - 1], v[v.size() - 2]);
        dir1.setLength(pivotOffset);
        dir2.setLength(pivotOffset);
        ans.push_back(dir1.p1() - (dir1.p2() - dir1.p1()));
        ans.push_back(dir2.p1() - (dir2.p2() - dir2.p1()));

        // And four on the perpendiculars to the first and the last segment(extended to both sides).
        QLineF norm1 = QLineF(v[0], v[1]).normalVector(),
               norm2 = QLineF(v[v.size() - 1], v[v.size() - 2]).normalVector();
        norm1.setLength(pivotOffset);
        norm2.setLength(pivotOffset);
        ans.push_back(norm1.p2());
        ans.push_back(norm1.p1() - (norm1.p2() - norm1.p1()));
        ans.push_back(norm2.p2());
        ans.push_back(norm2.p1() - (norm2.p2() - norm2.p1()));
    }
    
    for (int i = 1; i < v.size() - 1; i++)
    {
        QPair<QPointF, QPointF> pv = getVertexPivots(v[i - 1], v[i], v[i + 1]);
        if (mapPivots)
        {
            ans.append(pv.first);
            ans.append(pv.second);
        }
        else
        {
            if (innerZone)
                ans.append(pv.first);
            else
                ans.append(pv.second);
        }
    }
    
    return ans;
}

namespace
{

void bfs(QVector<QVector<int> > *v, int stx, int sty, int cid) // Helper function for connectivity components determining. cid = component id.
{
    QVector<QVector<int> > &lv = *v;

    QQueue<QPair<int, int> > q;
    q.enqueue(qMakePair(stx, sty));
    lv[stx][sty] = cid;
    while (!q.isEmpty())
    {
        QPair<int, int> top = q.head(); 
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                if ((top.first + i >= 0) && (top.first + i < lv.size()) && 
                    (top.second + j >= 0) && (top.second + j < lv[0].size()) &&
                    lv[top.first + i][top.second + j] == 0)
                {
                    q.enqueue(qMakePair(top.first + i, top.second + j));
                    lv[top.first + i][top.second + j] = cid;
                }
            }
        }
        q.dequeue();
    }
}
}

QVector<QVector<int> > Visualisation::determineConnComp() const
{
    QVector<QVector<int> > isVisited(isDiscovered.size(), QVector<int>(isDiscovered[0].size(), 0));
    for (int i = 0; i < isVisited.size(); i++)// First, we marks _discovered_ nodes as "walls"
    {
        for (int j = 0; j < isVisited[0].size(); j++)
        {
            if (isDiscovered[i][j])
                isVisited[i][j] = 1;
        }
    }
    int compCount = 1;
    for (int i = 0; i < isVisited.size(); i++)// Searching for the connected components
    {
        for (int j = 0; j < isVisited[0].size(); j++)
        {
            if (isVisited[i][j] == 0)
            {
                bfs(&isVisited, i, j, compCount + 1);// components ids are 1-indexed
                compCount += 1;
            }
        }
    }
#ifdef DEBUG
    //qDebug() << isVisited << endl;
    dbgCompNumber = isVisited;
#endif
    return isVisited;
}

namespace
{

QVector<QPointF> optimizeWall(const QVector<QPointF> &w)// Optimising walls is merging some its consequent segments into one. For example:
                                                        // {..(20, 100),(20, 200),(20, 300)..} might be merged into {..(20, 100); (20, 300)..}
                                                        // {..(20, 100),(20, 200),(21, 300)..} can't be merged because the second segment is not on the first segment's line.
{
    QVector<QPointF> ans;
    if (w.size() >= 2)
    {
        ans.append(w[0]);
        ans.append(w[1]);
    }
    else
    {
        return w;
    }
    qreal eps = 0.001;
    for (int i = 2; i < w.size(); i++)
    {
        QLineF l1(ans[ans.size() - 2], ans[ans.size() - 1]);
        QLineF l2(ans[ans.size() - 1], w[i]);
        if (qAbs(l1.angle() - l2.angle()) < eps)
        {
            ans.pop_back();
        }
        ans.append(w[i]);
    }
    return ans;
}

}

void Visualisation::updateVirtualWalls()
{
    virtualWalls.clear();

    QVector<QVector<int> > connComp = determineConnComp();

#ifdef DEBUG
    dbgCompNumber = connComp;
#endif

    int curComp = 1;
    while (true)
    {
        QVector<QPointF> result;
        int startx = -1, starty = -1;// Searching for the leftmost topmost point.
        for (int j = 0; j < connComp[0].size() && starty == -1; j++)
        {
            for (int i = 0; i < connComp.size() && startx == -1; i++)
            {
                if (connComp[i][j] == curComp)
                {
                    startx = i;
                    starty = j;
                    break;
                }
            }
        }
        if (startx == -1 && starty == -1)
            break;
        int curx = startx, cury = starty;

        int dx[] = {-1, -1, 0, 1, 1,  1,  0, -1};
        int dy[] = {0 ,  1, 1, 1, 0, -1, -1, -1};
        int curDir = 4;

        bool startVisited = false;
        while (!(startVisited && curx == startx && cury == starty))
        {
            if (curx == startx && cury == starty)
                startVisited = true;
            if (result.size() > 400)
            {
//                qDebug() << "oops";
            }
            result.append(cellSize * QPointF(curx, cury));

            int c = (curDir + 4) % 8;
            bool foundNew = false;
            for (int i = c + 1; i < 8 && !foundNew; i++)
            {
                int nx = curx + dx[i];
                int ny = cury + dy[i];
                if (nx >= 0 && nx < connComp.size() &&
                    ny >= 0 && ny < connComp[0].size() &&
                    (connComp[nx][ny] == curComp))
                {
                    curx = nx;
                    cury = ny;
                    foundNew = true;
                    curDir = i;
                }
            }
            for (int i = 0; i <= c && !foundNew; i++)
            {
                int nx = curx + dx[i];
                int ny = cury + dy[i];

                if (nx >= 0 && nx < connComp.size() &&
                    ny >= 0 && ny < connComp[0].size() &&
                    (connComp[nx][ny] == curComp))
                {
                    curx = nx;
                    cury = ny;
                    foundNew = true;
                    curDir = i;
                }
            }
            if (!foundNew)
            {
                break;
            }
        }
        if (result.size() >= 2)
        {
            result.append(result.front());
        }

        QVector<QPointF> optResult = optimizeWall(result);
#ifdef DEBUG
//        qDebug() << "Result was " << result.size() << " optimized to " << optResult.size() << endl;
#endif
        virtualWalls.append(optResult);
        curComp++;
    }
}

QHash<QPointF, QVector<QPointF> > Visualisation::getGraph(const QPointF &startPos, const QPointF &targetPos) const
{
    QVector<QPointF> points;

    points += mapPivots;

    for (int i = 0; i < virtualWalls.size(); i++)
        points += getPivots(virtualWalls[i]);

    points.push_back(startPos);
    points.push_back(targetPos);

#ifdef DEBUG
dbgPivots = points;
#endif

    QVector<QLineF> lines;
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size() - 1; j++)
        {
            lines.push_back(QLineF(map[i][j], map[i][j + 1]));
        }
    }
#ifdef DEBUG
//    qDebug() << "There are " << lines.size() << " solid and " << virtualWalls.size() << " virtual walls ,and " << points.size() << "points" << endl;
#endif
    QHash<QPointF, QVector<QPointF> > graph;//visibility graph
    QPointF trash;
    for (int i = 0; i < points.size(); i++)
    {
        for (int j = i + 1; j < points.size(); j++)
        {
            QLineF line(points[i], points[j]);
            bool wasIntersection = false;
            for (int l = 0; l < lines.size() && !wasIntersection; l++)
            {
                if (line.p1() != lines[l].p1() && line.p2() != lines[l].p1() && line.p1() != lines[l].p2() && line.p2() != lines[l].p2())//то есть линии не смежные
                    if (line.intersect(lines[l], &trash) == QLineF::BoundedIntersection)
                        wasIntersection = true;
            }
            for (int l = 0; l < virtualWalls.size() && !wasIntersection; l++)
            {
                for (int e = 0; e < virtualWalls[l].size() - 1 && !wasIntersection; e++)
                {
                    QLineF tline(virtualWalls[l][e], virtualWalls[l][e + 1]);
                    if (line.intersect(tline, &trash) == QLineF::BoundedIntersection)
                        wasIntersection = true;
                }
            }
            if (!wasIntersection)
            {
                graph[points[i]].push_back(points[j]);
                graph[points[j]].push_back(points[i]);
            }
        }
    }
    return graph;
}

QVector<QPointF> Visualisation::getPath(const QPointF &startPos, const QPointF &targetPos) const
{
    QHash<QPointF, QVector<QPointF> > graph = getGraph(startPos, targetPos);
    
    QVector<QPointF> ans;

    QSet<QPointF> closed, open;
    QHash<QPointF, double> g_h, h_h;
    QHash<QPointF, QPointF> parent;

    open.insert(startPos);
    g_h[startPos] = 0;
    h_h[startPos] = distance(startPos, targetPos);

    while (!open.isEmpty())
    {
        QPointF top = *open.begin();
        for (QSet<QPointF>::ConstIterator it = open.begin(); it != open.end(); ++it)
        {
            if (g_h[*it] + h_h[*it] < g_h[top] + h_h[top])
                top = *it;
        }
        if (top == targetPos)
        {
            QPointF cur = targetPos;
            while (cur != startPos)
            {
                ans.append(cur);
                cur = parent[cur];
            }
            ans.append(startPos);
            break;
        }

        open.remove(top);
        closed.insert(top);
        QVector<QPointF> l = graph[top];
        
        for (int i = 0; i < l.size(); i++)
        {
            if (closed.contains(l[i]))
                continue;
            
            double new_g = g_h[top] + distance(top, l[i]);
            bool upd;
            if (!open.contains(l[i]))
            {
                open.insert(l[i]);
                upd = true;
            }
            else 
            {
                upd = new_g < g_h[l[i]];
            }
            if (upd)
            {
                parent[l[i]] = top;
                g_h[l[i]] = new_g;
                h_h[l[i]] = distance(l[i], targetPos);
            }
        }
    }
    std::reverse(ans.begin(), ans.end());
    return ans;
}

namespace
{

bool fits(const QPointF &p, const QPointF &centre, qreal radius, qreal dirAngle, qreal spanAngle)// checks if p would feet in a circle sector.
{
    dirAngle = fmod(dirAngle, PI() * 2);

    qreal st = dirAngle - spanAngle / 2;
    qreal fn = dirAngle + spanAngle / 2;

    st = fmod(st, PI() * 2);
    if (st < 0)
        st += 2 * PI();
    fn = fmod(fn, PI() * 2);
    if (fn < 0)
        fn += 2 * PI();
    if (st > fn)
        fn += 2 * PI();
    qreal angle = degr2rad(QLineF(centre, p).angle());

    if (angle < 0)
        angle += 2 * PI();
    if (angle < st)
        angle += 2 * PI();
#ifdef DEBUG
//    qDebug() << "Start angle " << rad2degr(st) << " Finish angle " << rad2degr(fn) << " Anlge " << rad2degr(angle) << endl;
#endif

    return (distance(p, centre) < radius && st <= angle && angle <= fn);
}

}

bool Visualisation::exploreMap()
{
    bool discovered = false;

    qreal stx = curPos.x() - fovDist, fnx = curPos.x() + fovDist;
    qreal sty = curPos.y() - fovDist, fny = curPos.y() + fovDist;
    int stxp = qMax(0.0, stx / cellSize), fnxp = qMin(qreal(isDiscovered.size() - 1), fnx / cellSize);
    int styp = qMax(0.0, sty / cellSize), fnyp = qMin(qreal(isDiscovered[0].size() - 1), fny / cellSize);
    for (int i = stxp; i <= fnxp; i++)
    {
        for (int j = styp; j <= fnyp; j++)
        {
            if (!isDiscovered[i][j] && fits(cellSize * QPointF(i, j), curPos, fovDist, curAngle, fovAngle) && !wallOnPathTo(cellSize * QPointF(i, j)))
            {
                isDiscovered[i][j] = true;
                discovered = true;
            }
        }
    }
    updateVirtualWalls();
    update();
    return discovered;
}

void Visualisation::handleKeys()
{
    bool needsDiscover = false;
    if (pressedKeys[Qt::Key_Left])
    {
        curAngle += rotSpeed;
        needsDiscover = true;
    }
    if (pressedKeys[Qt::Key_Right])
    {
        curAngle -= rotSpeed;
        needsDiscover = true;
    }
    if (pressedKeys[Qt::Key_Up])
    {
        makeManualMove();
        needsDiscover = true;
    }
    if (needsDiscover)
    {
        exploreMap();
    }
}

bool Visualisation::makeManualMove()
{
    QLineF dir = QLineF::fromPolar(moveSpeed, rad2degr(curAngle)).translated(curPos);
    QPointF trash;
    bool intersects = false;
    for (int i = 0; i < map.size() && !intersects; i++)
    {
        for (int j = 0; j < map[i].size() - 1 && !intersects; j++)
        {
            QLineF tmp(map[i][j], map[i][j + 1]);
            if (dir.intersect(tmp, &trash) == QLineF::BoundedIntersection)
            {
                intersects = true;

                qreal prod = dir.dx() * tmp.dx() + dir.dy() * tmp.dy();
                int angle = dir.angleTo(tmp);
                if ((0 <= angle && angle <= 180) ^ (prod > 0))
                    curAngle -= rotSpeed;
                else
                    curAngle += rotSpeed;
            }
        }
    }
    if (!intersects)
    {
        curPos += QLineF::fromPolar(moveSpeed, rad2degr(curAngle)).p2();
    }
    return !intersects;
}

bool Visualisation::wallOnPathTo(const QPointF &b) const
{
    QLineF line(curPos, b);
    QPointF trash;
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size() - 1; j++)
        {
            if (QLineF(map[i][j], map[i][j + 1]).intersect(line, &trash) == QLineF::BoundedIntersection)
            {
                return true;
            }
        }
    }
    return false;
}

void Visualisation::updatePotential()
{
    int prob = 100;//%
    for (int i = 0; i < potential.size(); i++)
    {
        for (int j = 0; j < potential[0].size(); j++)
        {
            if (!isDiscovered[i][j])
            {
                potential[i][j] = -1000000.0;
            }
            if (!(isDiscovered[i][j] &&
                i > 0 && i < potential.size() - 1 &&
                j > 0 && j < potential[0].size() - 1 &&
                isDiscovered[i - 1][j] && isDiscovered[i + 1][j] &&
                isDiscovered[i][j - 1] && isDiscovered[i][j + 1]))
            {
                continue;
            }
            potential[i][j] = 0.0;
            for (int q = qMax(i - 5, 0); q < qMin(potential.size(), i + 5); q++)
            {
                for (int w = qMax(j - 5, 0); w < qMin(potential[0].size(), j + 5); w++)
                {
                    if (i == q && j == w)
                        continue;
                    if (!isDiscovered[q][w])
                    {
                        if (qrand() % 100 >= prob)
                            continue;
                        potential[i][j] += 10.0 / qSqrt((q - i) * (q - i) + (w - j) * (w - j) + 0.0);
                    }
                }
            }
            potential[i][j] -= visits[i][j];
        }
    }
}

QPointF Visualisation::getAITarget() const
{
    int mi = -1, mj = -1;
    for (int i = 0; i < potential.size(); i++)
    {
        for (int j = 0; j < potential[0].size(); j++)
        {
            if (isDiscovered[i][j] && (mi == -1 || mj == -1 || potential[i][j] > potential[mi][mj]))
            {
                mi = i;
                mj = j;
            }
        }
    }
    int ci = mi, cj = mj;
    qreal maxpath = 0.0;
    QVector<QPointF> mp = getPath(curPos, cellSize * QPointF(mi, mj));
    for (int i = 0; i < mp.size() - 1; i++)
        maxpath += distance(mp[i], mp[i + 1]);

    for (int i = 0; i < potential.size(); i++)
    {
        for (int j = 0; j < potential[0].size(); j++)
        {
            if (isDiscovered[i][j])
            {
                if (potential[i][j] >= 0.95 * potential[mi][mj] &&
                    distance(curPos, cellSize * QPointF(i, j)) > 1.0)//epsilon
                {
                    qreal curpath = 0.0;
                    QVector<QPointF> cp = getPath(curPos, cellSize * QPointF(i, j));
                    for (int e = 0; e < cp.size() - 1; e++)
                        curpath += distance(cp[e], cp[e + 1]);
                    if (curpath < maxpath)
                    {
                        ci = i;
                        cj = j;
                        maxpath = curpath;
                    }
                }
            }
        }
    }
    return cellSize * QPointF(ci, cj);
}

void Visualisation::makeAIMove()
{
    exploreMap();
#ifdef DEBUG
    /*qDebug() << state << endl;
    if (state == FollowPathState)
    {
        qDebug() << path << endl;
    }
    qDebug() << " -------" << endl;*/
#endif
    if (state == FollowPathState)
    {
        if (path.size() == 0) // We haven't found a path(or it is incorrect)
        {
            addVisitsCount(targetPos);// We lower target point's potential, and its probality to be chosen again
            state = NoState;// Another chance
        }
        else
        {
            addVisitsCount(curPos);
            if (path.size() <= 1)
            {
                if (rand() % 2 == 0)
                    state = PointExploreStateCW;
                else
                    state = PointExploreStateCCW;
                return;
            }
            QPointF a = path[0];
            QPointF b = path[1];
            bool came = makeMoveByLine(a, b);
            if (came) // we have passed a, so we can remove it from the path
                path.pop_front();
        }
    }
    else if (state == NoState)
    {
        updatePotential();
        targetPos = getAITarget();
        path = getPath(curPos, targetPos);
        state = FollowPathState;
    }
    else if (state == PointExploreStateCW || state == PointExploreStateCCW)
    {
        int stopProbality = 90;// The parameter to control rotation time.
        if (qrand() % 100 > stopProbality)
        {
            state = NoState;
        }
        else
        {
            if (state == PointExploreStateCW)
                curAngle -= rotSpeed;
            else
                curAngle += rotSpeed;
        }
    }
}

namespace
{

bool isOnSegment(QPointF p, QPointF a, QPointF b) //returns true if p belongs to segment [a, b]
{
    //segment = [t * a + (1 - t) * b | 0 <= t <= 1]
    qreal eps = 0.001;
    if (qAbs(a.x() - b.x()) < eps) // to exclude division by zero
    {
        qSwap(a.rx(), a.ry());
        qSwap(b.rx(), b.ry());
        qSwap(p.rx(), p.ry());
    }
    qreal t = (p.x() - b.x()) / (a.x() - b.x());
    if (t >= 0 && t <= 1)
    {
        if (qAbs(t * a.y() + (1 - t) * b.y() - p.y()) < 0.01)
            return true;
    }
    return false;
}

}

bool Visualisation::makeMoveByLine(const QPointF &a, const QPointF &b)
{
    QLineF dir(a, b);
    qreal angle = degr2rad(dir.angle());

    while (curAngle + 2 * PI() < 0)
        curAngle += 2 * PI();
    while (curAngle - 2 * PI() >= 0)
        curAngle -= 2 * PI();

    if (curAngle == angle)
    {
        QLineF delta = QLineF::fromPolar(moveSpeed, rad2degr(curAngle));
        QPointF newPos = curPos + delta.p2();
        if (isOnSegment(newPos, a, b))
            curPos = newPos;
        else
        {
            curPos = b;
            return true;
        }
    }
    else
    {
        bool ccw;//counter-clockwise
        if (angle > curAngle)
        {
            if (curAngle + PI() > angle)
                ccw = true;
            else
                ccw = false;
        }
        else
        {
            if (angle + PI() > curAngle)
                ccw = false;
            else
                ccw = true;
        }
        if (ccw)
        {
            if (angle < curAngle)
                angle += 2 * PI();
            if (curAngle + rotSpeed >= angle)
                curAngle = angle;
            else
                curAngle += rotSpeed;
        }
        else
        {
            if (curAngle < angle)
                curAngle += 2 * PI();
            if (curAngle - rotSpeed <= angle)
                curAngle = angle;
            else
                curAngle -= rotSpeed;
        }
    }
    return false;
}

void Visualisation::addVisitsCount(const QPointF &p, qreal value)
{
    int affectionRadius = 3;
    int cx = p.x() / cellSize, cy = p.y() / cellSize;
    for (int q = -affectionRadius; q <= affectionRadius; q++)
    {
        if (cx + q < 0 || cx + q >= potential.size())
            continue;
        for (int w = -affectionRadius; w <= affectionRadius; w++)
        {
            if (cy + w < 0 || cy + w >= potential[0].size())
                continue;
            visits[cx + q][cy + w] += value / (abs(q) + abs(w) + 1.0);
        }
    }
    visits[cx][cy] += value;
}

void Visualisation::togglePause()
{
    if (timer->isActive())
        timer->stop();
    else
        timer->start(50);
}

void Visualisation::toggleManualControl()
{
    state = NoState;
    if (control == ManualContol)
        control = AIControl;
    else
        control = ManualContol;
}

void Visualisation::makeMove()
{
    if (control == ManualContol)
        handleKeys();
    else
        makeAIMove();
}

