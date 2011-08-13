#include <QtGui>

#include <queue>
#include <cmath>

#include "Visualisation.h"
#include "tools.h"

Visualisation::Visualisation(int width_, int height_, QWidget *parent):
    QWidget(parent),
    pivotOffset(10.0),
    cellSize(20.0),
    fovDist(100.0), fovAngle(75.0 / 180.0 * 3.14),
    moveSpeed(10.0), rotSpeed(0.2)
{
   setFixedSize(width_, height_);
   setFocusPolicy(Qt::StrongFocus);
   int cellsx = width() / cellSize + 2;
   int cellsy = height() / cellSize + 2;
   isDiscovered = QVector<QVector<bool> > (cellsx, QVector<bool> (cellsy, false));
    
   QTimer *timer = new QTimer(this);
   connect(timer, SIGNAL(timeout()), this, SLOT(handleKeys()));
   timer->start(100);
   init();
}

void Visualisation::init()
{
    QPointF p00 = QPointF(0, 0), p10 = QPointF(width() - 1, 0), p01 = QPointF(0, height() - 1), p11 = QPointF(width() - 1, height() - 1);
    QVector<QPointF> edge;
    edge.append(p00);
    edge.append(p01);
    edge.append(p11);
    edge.append(p10);
    edge.append(p00);
    map.append(edge);

    curPos = QPointF(1, 1);
    curAngle = 45.0 / 180 * PI();
    startPos = curPos;
    targetPos = startPos;
    isDiscovered.fill(QVector<bool> (isDiscovered[0].size(), false));
    virtualWalls.clear();
    discover();
}


void Visualisation::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        startPos = e->pos();
    }
    else if (e->button() == Qt::RightButton)
    {
        targetPos = e->pos();
    }
    calculatePath(startPos, targetPos);
    update();
}

void Visualisation::paintEvent(QPaintEvent *)
{
    QImage img(width(), height(), QImage::Format_RGB32);
    QPainter p(&img);

    QPen pathPen;
    pathPen.setColor(QColor(51, 204, 255));
    pathPen.setWidth(2);

    QPen mapPen;
    mapPen.setColor(QColor(45, 0, 179));
    mapPen.setWidth(3);

    QPainterPath mapBackground;
    mapBackground.addRect(QRectF(0, 0, width() - 1, height() - 1));

    p.setPen(Qt::white);
    p.setBrush(Qt::white);
    p.drawPath(mapBackground);

    p.setPen(Qt::green);
    p.setBrush(Qt::green);
    qreal curLen = 20;
    qreal curRad = 10;
    p.drawEllipse(curPos, curRad, curRad);
    p.drawLine(QLineF(curPos, curPos + curLen * QPointF(qCos(curAngle), qSin(curAngle))));

    p.setPen(mapPen);
    for (int i = 0; i < map.size(); i++)
        for (int j = 0; j < map[i].size() - 1; j++)
            p.drawLine(QLineF(map[i][j], map[i][j + 1]));

    p.setPen(Qt::black);
#ifdef DEBUG
    p.setPen(Qt::green);
#endif
    p.setBrush(Qt::black);
    QVector<QVector<bool> > drawn = isDiscovered;
    qreal fix = 3 * qSqrt(2) / 4;
    for (int i = 1; i < drawn.size() - 1; i++)
    {
        for (int j = 1; j < drawn[0].size() - 1; j++)
        {
            if (!drawn[i - 1][j - 1] && !drawn[i - 1][j] && !drawn[i - 1][j + 1] &&
                !drawn[i][j - 1] && !drawn[i][j] && !drawn[i][j + 1] &&
                !drawn[i + 1][j - 1] && !drawn[i + 1][j] && !drawn[i + 1][j + 1])
            {
                p.drawEllipse(cellSize * QPointF(i, j), cellSize * 2 * fix, cellSize * 2 * fix);
                drawn[i - 1][j - 1] = true, drawn[i - 1][j] = true, drawn[i - 1][j + 1] = true;
                drawn[i][j - 1] = true, drawn[i][j] = true, drawn[i][j + 1] = true;
                drawn[i + 1][j - 1] = true, drawn[i + 1][j] = true,drawn[i + 1][j + 1] = true;
            }
        }
    }
#ifdef DEBUG
    p.setPen(Qt::yellow);
#endif
    for (int i = 0; i < drawn.size(); i++)
    {
        for (int j = 0; j < drawn[0].size(); j++)
        {
            if (!drawn[i][j])
            {
                p.drawEllipse(cellSize * QPointF(i, j), cellSize, cellSize);
            }
        }
    }
    
    p.setPen(pathPen);
    for (int i = 0; i < path.size() - 1; i++)
        p.drawLine(QLineF(path[i], path[i + 1]));
#ifdef DEBUG
    p.setPen(Qt::blue);
    p.setBrush(Qt::blue);
    for (qreal i = 0; i < width(); i += cellSize)
        for (qreal j = 0; j < height(); j += cellSize)
            p.drawEllipse(QPointF(i, j), 1, 1);
   
    p.setPen(Qt::magenta);
    p.setBrush(Qt::transparent);
    p.drawPie(QRectF(curPos - QPointF(fovDist, fovDist), curPos + QPointF(fovDist, fovDist)), rad2degr(-curAngle - fovAngle / 2) * 16, rad2degr(fovAngle) * 16);

    p.setPen(Qt::blue);
    p.setBrush(Qt::blue);
    for (int i = 0; i < virtualWalls.size(); i++)
    {
        for (int j = 0; j < virtualWalls[i].size() - 1; j++)
        {
            p.drawLine(QLineF(virtualWalls[i][j], virtualWalls[i][j + 1]));
        }
    }

    p.setPen(Qt::red);
    p.setBrush(Qt::red);
    for (int i = 0; i < dbgConnComp.size(); i++)
    {
        p.drawEllipse(dbgConnComp[i], 1, 1);
    }
    p.setPen(Qt::green);
    p.setBrush(Qt::green);
    for (int i = 0; i < dbgPivots.size(); i++)
        p.drawEllipse(dbgPivots[i], 1, 1);
        
#endif
    QPainter q(this);
    q.drawImage(QPointF(0, 0), img);

}

void Visualisation::setMap(QVector<QVector<QPointF> > m)
{
    map = m;
    restart();
}

void Visualisation::restart()
{
    //calculatePath();
    init();
    update();
}

QPair<QPointF, QPointF> Visualisation::pivots(const QPointF &a, const QPointF &b, const QPointF &c)
{
    QLineF dir1 = QLineF(b, a).unitVector(),
           dir2 = QLineF(b, c).unitVector();
    QPointF med = (dir1.p2() + dir2.p2()) / 2.0;
    QLineF dir = QLineF(b, med);
    dir.setLength(pivotOffset);
    return qMakePair(dir.p2(), dir.p1() - (dir.p2() - dir.p1()));
}


QVector<QPointF> Visualisation::getPivots(const QVector<QPointF> &v)
{
    Q_ASSERT(v.size() >= 2);

    QVector<QPointF> ans;
   
    if (v.front() == v.back()) // line is enclosed
    {
        QPair<QPointF, QPointF> pv = pivots(v[v.size() - 2], v[0], v[1]);
        ans.append(pv.first);
        ans.append(pv.second);
    }
    else
    {
        QLineF norm1 = QLineF(v[0], v[1]).normalVector(),
               norm2 = QLineF(v[v.size() - 1], v[v.size() - 2]).normalVector();
        norm1.setLength(pivotOffset);
        norm2.setLength(pivotOffset);
        ans.push_back(norm1.p2());
        ans.push_back(norm1.p1() - (norm1.p2() - norm1.p1()));
        ans.push_back(norm2.p2());
        ans.push_back(norm2.p1() - (norm2.p2() - norm2.p1()));

        QLineF dir1 = QLineF(v[0], v[1]),
               dir2 = QLineF(v[v.size() - 1], v[v.size() - 2]);
        dir1.setLength(pivotOffset);
        dir2.setLength(pivotOffset);
        ans.push_back(dir1.p1() - (dir1.p2() - dir1.p1()));
        ans.push_back(dir2.p1() - (dir2.p2() - dir2.p1()));
    }
    
    for (int i = 1; i < v.size() - 1; i++)
    {
        QPair<QPointF, QPointF> pv = pivots(v[i - 1], v[i], v[i + 1]);
        ans.append(pv.first);
        ans.append(pv.second);
    }
    
    return ans;
}

void bfs(QVector<QVector<int> > *v, int stx, int sty)
{
    QVector<QVector<int> > &lv = *v;
    QQueue<QPair<int, int> > q;
    q.enqueue(qMakePair(stx, sty));
    lv[stx][sty] = 1;
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
                    lv[top.first + i][top.second + j] = 1;
                }
            }
        }
        q.dequeue();
    }
}

void Visualisation::updateVirtualWalls()
{
    QVector<QVector<int> > isVisited(isDiscovered.size(), QVector<int>(isDiscovered[0].size(), 0));
    for (int i = 0; i < isVisited.size(); i++)
    {
        for (int j = 0; j < isVisited[0].size(); j++)
        {
            if (!isDiscovered[i][j])
                isVisited[i][j] = -1;
        }
    }
    for (int i = 0; i < isVisited.size(); i++)
    {
        for (int j = 0; j < isVisited[0].size(); j++)
        {
            if (isVisited[i][j] == 0)
            {
                bfs(&isVisited, i, j);
            }
        }
    }
#ifdef DEBUG
    //qDebug() << isVisited << endl;
    dbgConnComp.clear();
    for (int i = 0; i < isVisited.size(); i++)
    {
        for (int j = 0; j < isVisited[0].size(); j++)
        {
            if (isVisited[i][j] == 1)
            {
                dbgConnComp.push_back(cellSize * QPointF(i, j));
            }
        }
    }
#endif

    QVector<QPointF> result;
    int topx = -1, topy = -1;
    for (int j = 0; j < isVisited[0].size() && topy == -1; j++)
    {
        for (int i = 0; i < isVisited.size() && topx == -1; i++)
        {
            if (isVisited[i][j] == 1)
            {
                topx = i;
                topy = j;
                break;
            }
        }
    }

    int curx = topx, cury = topy;
    int dx[] = {-1, -1, 0, 1, 1,  1,  0, -1};
    int dy[] = {0 ,  1, 1, 1, 0, -1, -1, -1};
    int curDir = 4;

    int finishx, finishy;
    for (int i = 7; i >= 0; i--)
    {
        int nx = curx + dx[i];
        int ny = cury + dy[i];
        if (nx >= 0 && nx < isVisited.size() &&
            ny >= 0 && ny < isVisited[0].size() &&
            isVisited[nx][ny] == 1)
        {
            finishx = nx;
            finishy = ny;
            break;
        }
    }

    while (!(curx == finishx && cury == finishy))
    {
        isVisited[curx][cury] = 0;
#ifdef DEBUG
        qDebug() << "Has visited " << curx << " " << cury << endl;
#endif
        result.append(cellSize * QPointF(curx, cury));

        int c = (curDir + 4) % 8;
        bool foundNew = false;
        for (int i = c; i < 8 && !foundNew; i++)
        {
            int nx = curx + dx[i];
            int ny = cury + dy[i];
            if (nx >= 0 && nx < isVisited.size() &&
                ny >= 0 && ny < isVisited[0].size() &&
                isVisited[nx][ny] == 1)
            {
                curx = nx;
                cury = ny;
                foundNew = true;
                curDir = i;
            }
        }
        for (int i = 0; i < c && !foundNew; i++)
        {
            int nx = curx + dx[i];
            int ny = cury + dy[i];

            if (nx >= 0 && nx < isVisited.size() &&
                ny >= 0 && ny < isVisited[0].size() &&
                isVisited[nx][ny] == 1)
            {
                curx = nx;
                cury = ny;
                foundNew = true;
                curDir = i;
            }
        }
        if (!foundNew)
            break;
    }

    if (result.size() != 0)
    {
        result.append(result.front());
        virtualWalls.append(result);
    }
#ifdef DEBUG
#endif
}


void Visualisation::calculatePath(const QPointF &startPos, const QPointF &targetPos)
{
    QVector<QPointF> points;
    QPointF p00(0, 0), p01(0, height() - 1), p10(width() - 1, 0), p11(width() - 1, height() - 1);//screen corners
    points.push_back(p00);
    points.push_back(p01); 
    points.push_back(p10);
    points.push_back(p11);

    for (int i = 0; i < map.size(); i++)
        points += getPivots(map[i]);
    
    for (int i = 0; i < virtualWalls.size(); i++)
        points += getPivots(virtualWalls[i]);

    points.push_back(startPos);
    points.push_back(targetPos);
    
#ifdef DEBUG
dbgPivots = points;
#endif

    QVector<QLineF> lines;
    lines.push_back(QLineF(p00, p01));
    lines.push_back(QLineF(p00, p10));
    lines.push_back(QLineF(p11, p01));
    lines.push_back(QLineF(p11, p10));
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size() - 1; j++)
        {
            lines.push_back(QLineF(map[i][j], map[i][j + 1]));//solid screen edges
        }
    }

    QHash<QPointF, QVector<QPointF> > graph;//visibilitygraph
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
}

bool fits(const QPointF &p, const QPointF &centre, qreal radius, qreal dirAngle, qreal spanAngle)
{
    dirAngle = fmod(dirAngle, PI() * 2);

    qreal st = dirAngle - spanAngle / 2;
    qreal fn = dirAngle + spanAngle / 2;
#ifdef DEBUG
    //qDebug() << "Start angle: " << st << " " << "Finish angle: " << fn << endl;
#endif
    st = fmod(st, PI() * 2);
    if (st < 0)
        st += 2 * PI();
    fn = fmod(fn, PI() * 2);
    if (fn < 0)
        fn += 2 * PI();
    if (st > fn)
        fn += 2 * PI();
    qreal angle = qAtan2(p.y() - centre.y(), p.x() - centre.x());
    if (angle < 0)
        angle += 2 * PI();
    if (angle < st)
        angle += 2 * PI();
#ifdef DEBUG
    qDebug() << rad2degr(dirAngle) << " Start angle2: " << rad2degr(st) << " " << "Finish angle2: " << rad2degr(fn) << " Angle: " << rad2degr(angle) << endl;
#endif
    
    return (distance(p, centre) < radius && st <= angle && angle <= fn);
}

void Visualisation::discover()
{
    qreal stx = curPos.x() - fovDist, fnx = curPos.x() + fovDist;
    qreal sty = curPos.y() - fovDist, fny = curPos.y() + fovDist;
    int stxp = qMax(0.0, stx / cellSize), fnxp = qMin(qreal(isDiscovered.size() - 1), fnx / cellSize);
    int styp = qMax(0.0, sty / cellSize), fnyp = qMin(qreal(isDiscovered[0].size() - 1), fny / cellSize);
#ifdef DEBUG
    qDebug() << "Stxp " << stxp << " fnxp " << fnxp << " styp " << styp << " fnyp " << fnyp << endl;
#endif 
    for (int i = stxp; i <= fnxp; i++)
    {
        for (int j = styp; j <= fnyp; j++)
        {
            if (!isDiscovered[i][j] && fits(cellSize * QPointF(i, j), curPos, fovDist, curAngle, fovAngle))
            {
                isDiscovered[i][j] = true;
#ifdef DEBUG
                //qDebug() << "Point " << i << " " << j << " discovered" << endl;
#endif
            }
        }
    }
    virtualWalls.clear();
    updateVirtualWalls();
    calculatePath(startPos, targetPos);
    update();
}

void Visualisation::handleKeys()
{
    bool needsDiscover = false;
    if (pressed[Qt::Key_Left])
    {
        curAngle -= rotSpeed;
        needsDiscover = true;
    }
    if (pressed[Qt::Key_Right])
    {
        curAngle += rotSpeed;
        needsDiscover = true;
    }
    if (pressed[Qt::Key_Up])
    {
        QLineF dir(curPos, curPos + moveSpeed * QPointF(qCos(curAngle), qSin(curAngle)));
        QPointF trash;
        bool intersects = false;
        for (int i = 0; i < map.size() && !intersects; i++)
        {
            for (int j = 0; j < map[i].size() - 1 && !intersects; j++)
            {
                if (dir.intersect(QLineF(map[i][j], map[i][j + 1]), &trash) == QLineF::BoundedIntersection)
                {
                    intersects = true;
                }
            }
        }
        if (!intersects)
        {
            curPos += moveSpeed * QPointF(qCos(curAngle), qSin(curAngle));
            needsDiscover = true;
        }
    }
    if (needsDiscover)
    {
        discover();
        update();
    }
    
}

void Visualisation::keyPressEvent(QKeyEvent *e)
{
    pressed[e->key()] = true;
}

void Visualisation::keyReleaseEvent(QKeyEvent *e)
{
    pressed[e->key()] = false;    
}
