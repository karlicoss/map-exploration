#include <QtGui>

#include <queue>
#include <cmath>
#include <algorithm>

#include "Visualisation.h"
#include "tools.h"

Visualisation::Visualisation(int width_, int height_, QVector<QVector<QPointF> > map_, QWidget *parent):
    QWidget(parent),
    fovDist(200.0), fovAngle(degr2rad(60.0)),
    moveSpeed(10.0), rotSpeed(0.1),
    curPos(1, 1), curAngle(degr2rad(-45.0)),
    pivotOffset(8.0), cellSize(8.0),
    timer(new QTimer(this)),
    control(AIControl),
    map(map_),
    state(NoState),
    startPos(curPos), targetPos(startPos)
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

   visitsCount = QVector<QVector<int> > (cellsx, QVector<int> (cellsy, 0));
   force = QVector<QVector<qreal> > (cellsx, QVector<qreal> (cellsy, 0.0));

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

void Visualisation::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        //startPos = e->pos();
    }
    else if (e->button() == Qt::RightButton)
    {
        //targetPos = e->pos();
    }
    //calculatePath(startPos, targetPos);
    update();
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
            if (i % 2 == 0 && j % 2 == 0 && force[i][j] > -10000)
                p.drawText(cellSize * QPointF(i, j), QString::number(int(force[i][j])));
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
    p.drawEllipse(startPos, 6, 6);
    p.drawEllipse(targetPos, 6, 6);

    QPainter q(this);
    q.drawImage(QPointF(0, 0), img);// And finally..Drawing the whole image on the widget.

}

QPair<QPointF, QPointF> Visualisation::getVertexPivots(const QPointF &a, const QPointF &b, const QPointF &c)
{
    QLineF dir1 = QLineF(b, a).unitVector(),
           dir2 = QLineF(b, c).unitVector();
    QLineF helper = QLineF(a, c).normalVector();

    QPointF med = (dir1.p2() + dir2.p2()) / 2.0;
    QLineF dir = QLineF(b, med);
    dir.setLength(pivotOffset);
    QPointF d1 = dir.p2(), d2 = dir.p1() - (dir.p2() - dir.p1());

    int angle = helper.angleTo(dir);
    if (90 <= angle && angle < 270) //First point is always in the inner side
        return qMakePair(d2, d1);
    else
        return qMakePair(d1, d2);
}


QVector<QPointF> Visualisation::getPivots(const QVector<QPointF> &v, bool mapP)
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
        if (mapP)
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

void bfs(QVector<QVector<int> > *v, int stx, int sty, int cid) // cid = component id.
                                                               //0 - unvisited, >=1 - components, -1 - undiscovered zone
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

QVector<QVector<int> > Visualisation::determineConnComp()
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

QVector<QPointF> optimizeVirtualWall(const QVector<QPointF> &w)
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

        QVector<QPointF> optResult = optimizeVirtualWall(result);
#ifdef DEBUG
//        qDebug() << "Result was " << result.size() << " optimized to " << optResult.size() << endl;
#endif
        virtualWalls.append(optResult);
        curComp++;
    }
}

QHash<QPointF, QVector<QPointF> > Visualisation::getGraph(const QPointF &startPos, const QPointF &targetPos)
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

void Visualisation::calculatePath(const QPointF &startPos, const QPointF &targetPos)
{
    QHash<QPointF, QVector<QPointF> > graph = getGraph(startPos, targetPos);
    
    path.clear();
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
                path.append(cur);
                cur = parent[cur];
            }
            path.append(startPos);
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
    std::reverse(path.begin(), path.end());
}

bool fits(const QPointF &p, const QPointF &centre, qreal radius, qreal dirAngle, qreal spanAngle)
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

bool Visualisation::discoverMap()
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
            if (!isDiscovered[i][j] && fits(cellSize * QPointF(i, j), curPos, fovDist, curAngle, fovAngle) && !wallOnPath(cellSize * QPointF(i, j)))
            {
                isDiscovered[i][j] = true;
                discovered = true;
            }
        }
    }
    updateVirtualWalls();
    //calculateForce();
    //setTarget();
    //calculatePath(startPos, targetPos);
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
        discoverMap();
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

bool Visualisation::wallOnPath(const QPointF &b)
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

void Visualisation::calculateForce()
{
    int prob = 100;//%
    for (int i = 0; i < force.size(); i++)
    {
        for (int j = 0; j < force[0].size(); j++)
        {
            if (!isDiscovered[i][j])
            {
                force[i][j] = -1000000.0;
            }
            if (!(isDiscovered[i][j] &&
                i > 0 && i < force.size() - 1 &&
                j > 0 && j < force[0].size() - 1 &&
                isDiscovered[i - 1][j] && isDiscovered[i + 1][j] &&
                isDiscovered[i][j - 1] && isDiscovered[i][j + 1]))
            {
                continue;
            }
            force[i][j] = 0.0;
            for (int q = qMax(i - 5, 0); q < qMin(force.size(), i + 5); q++)
            {
                for (int w = qMax(j - 5, 0); w < qMin(force[0].size(), j + 5); w++)
                {
                    if (i == q && j == w)
                        continue;
                    if (!isDiscovered[q][w])
                    {
                        if (qrand() % 100 >= prob)
                            continue;
                        force[i][j] += 10.0 / qSqrt((q - i) * (q - i) + (w - j) * (w - j) + 0.0);
                    }
                }
            }
            force[i][j] -= visitsCount[i][j];
        }
    }
}

void Visualisation::setTarget()
{
    startPos = curPos;
    int mi = -1, mj = -1;
    for (int i = 0; i < force.size(); i++)
    {
        for (int j = 0; j < force[0].size(); j++)
        {
            if (isDiscovered[i][j] && (mi == -1 || mj == -1 || force[i][j] > force[mi][mj]))
            {
                mi = i;
                mj = j;
            }
        }
    }
    targetPos = cellSize * QPointF(mi, mj);
}

void Visualisation::makeAIMove()
{
    discoverMap();
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
            lowerPoint(targetPos);// We lower target point, as its probality to be chosen again
            state = NoState;// Second chance
        }
        else
        {
            if (path.size() <= 1)
            {
                state = PointExploreState;
                return;
            }
            lowerPoint(curPos);
            QPointF a = path[0];
            QPointF b = path[1];
            bool came = makeMoveTo(a, b);
            if (came) // we have passed a, so we can remove it from the path
                path.pop_front();
        }
    }
    else if (state == NoState)
    {
        calculateForce();
        setTarget();
        calculatePath(startPos, targetPos);
        state = FollowPathState;
    }
    else //state = PointExplorationState
    {
        int prob = 70;
        if (qrand() % 100 > prob)
        {
            state = NoState;
        }
        else
        {
            curAngle += rotSpeed;
        }
    }
}

bool isOnSegment(QPointF p, QPointF a, QPointF b) //returns true if p belongs to segment [a, b]
{
    //segment = [t * a + (1 - t) * b | 0 <= t <= 1]
    qreal eps = 0.001;
    if (qAbs(a.x() - b.x()) < eps)
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

bool Visualisation::makeMoveTo(const QPointF &a, const QPointF &b)
{
    QLineF dir(a, b);
    qreal angle = degr2rad(dir.angle());
    if (curAngle == angle)
    {
        if (curPos == b)
            return true;
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
    else if (curAngle + rotSpeed > angle)
    {
        curAngle = angle;
    }
    else
    {
        curAngle += rotSpeed;
    }
    return false;
}

void Visualisation::lowerPoint(const QPointF &p, int value)
{
    int cx = p.x() / cellSize, cy = p.y() / cellSize;
    for (int q = -1; q <= 1; q++)
    {
        if (cx + q < 0 || cx + q >= force.size())
            continue;
        for (int w = -1; w <= 1; w++)
        {
            if (cy + w < 0 || cy + w >= force[0].size())
                continue;
            visitsCount[cx + q][cy + w] += value;
        }
    }
    visitsCount[cx][cy] += value;
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

