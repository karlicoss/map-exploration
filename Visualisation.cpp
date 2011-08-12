#include <QtGui>

#include <queue>
#include <cmath>

#include "Visualisation.h"
#include "tools.h"

Visualisation::Visualisation(int width_, int height_, QWidget *parent):
    QWidget(parent),
    curPos(0, 0), curAngle(0.0),
    pivotOffset(10.0),
    cellSize(10.0),
    fovDist(100.0), fovAngle(90.0 / 180.0 * 3.14),
    moveSpeed(10.0), rotSpeed(0.2)
{
   setFixedSize(width_, height_);
    setFocusPolicy(Qt::StrongFocus);
   int cellsx = width() / cellSize + 2;
   int cellsy = height() / cellSize + 2;
   isDiscovered = QVector<QVector<bool> > (cellsx, QVector<bool> (cellsy, false));
   discover();
}

void Visualisation::mousePressEvent(QMouseEvent *e)
{
    curPos = e->pos();
    discover();
    update();
    /*if (e->button() == Qt::LeftButton)
    {
        startPos = e->pos();
    }
    else if (e->button() == Qt::RightButton)
    {
        targetPos = e->pos();
    }
    restart();
    */
}

void Visualisation::paintEvent(QPaintEvent *)
{
    QPainter p(this);

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
    p.setBrush(Qt::black);
    for (int i = 0; i < isDiscovered.size(); i++)
    {
        for (int j = 0; j < isDiscovered[0].size(); j++)
        {
            if (!isDiscovered[i][j])
            {
                p.drawEllipse(cellSize * QPointF(i, j), cellSize, cellSize); //or cellSize / 2 ?
            }
        }
    }
    
    
#ifdef DEBUG
    p.setPen(Qt::red);
    p.setBrush(Qt::red);
    for (qreal i = 0; i < width(); i += 2 * cellSize)
        for (qreal j = 0; j < height(); j += 2 * cellSize)
            p.drawEllipse(QPointF(i, j), 1, 1);
   
    p.setPen(Qt::magenta);
    p.setBrush(Qt::transparent);
    qreal pi = 4 * qAtan(1);
    qreal rad2qtdegr = 1.0 / pi * 180 * 16;
    p.drawPie(QRectF(curPos - QPointF(fovDist, fovDist), curPos + QPointF(fovDist, fovDist)), (-curAngle - fovAngle / 2) * rad2qtdegr, fovAngle * rad2qtdegr);
    /*
    for (int i = 0; i < dbgPivots.size(); i++)
        p.drawEllipse(dbgPivots[i], 1, 1);
        */
#endif
}

void Visualisation::setMap(QVector<QVector<QPointF> > m)
{
    map = m;
    restart();
}

void Visualisation::restart()
{
    calculatePath();
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

void Visualisation::calculatePath()
{
    QVector<QPointF> points;
    QPointF p00(0, 0), p01(0, height() - 1), p10(width() - 1, 0), p11(width() - 1, height() - 1);//screen corners
    points.push_back(p00);
    points.push_back(p01); 
    points.push_back(p10);
    points.push_back(p11);

    for (int i = 0; i < map.size(); i++)
        points += getPivots(map[i]);
    
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
            lines.push_back(QLineF(map[i][j], map[i][j + 1]));
        }
    }

    QHash<QPointF, QVector<QPointF> > graph;
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
    double rad2degr = 1.0 / PI() * 180;
    dirAngle = fmod(dirAngle, PI() * 2);
    //if (dirAngle
    qreal st = dirAngle - spanAngle / 2;
    qreal fn = dirAngle + spanAngle / 2;
#ifdef DEBUG
    //qDebug() << "Start angle: " << st << " " << "Finish angle: " << fn << endl;
#endif
    //st = fmod(st, PI() * 2);
    //if (st < 0)
    //    st += 2 * PI();
    //fn = fmod(fn, PI() * 2);
    //if (fn < 0)
    //    fn += 2 * PI();
    qreal angle = qAtan2(p.y() - centre.y(), p.x() - centre.x());//, p.y() - centre.y());
    //if (angle < 0)
    //    angle += 2 * PI();
#ifdef DEBUG
   // qDebug() << "Start angle2: " << st * rad2degr << " " << "Finish angle2: " << fn * rad2degr << " " << " Angle: " << angle * rad2degr << endl;
#endif
    
    return (distance(p, centre) < radius && st <= angle && angle <= fn);
}

void Visualisation::discover()
{
    qreal stx = curPos.x() - fovDist, fnx = curPos.x() + fovDist;
    qreal sty = curPos.y() - fovDist, fny = curPos.y() + fovDist;
    int stxp = qMax(0.0, stx / cellSize), fnxp = qMin(width() / cellSize, fnx / cellSize);
    int styp = qMax(0.0, sty / cellSize), fnyp = qMin(height() / cellSize, fny / cellSize);
#ifdef DEBUG
    qDebug() << "Stxp " << stxp << " fnxp " << fnxp << " styp " << styp << " fnyp " << fnyp << endl;
#endif 
    for (int i = stxp; i <= fnxp; i++)
    {
        for (int j = styp; j <= fnyp; j++)
        {
            if (!isDiscovered[i][j] && fits(QPointF(i * cellSize, j * cellSize), curPos, fovDist, curAngle, fovAngle))
            {
                isDiscovered[i][j] = true;
#ifdef DEBUG
                //qDebug() << "Point " << i << " " << j << " discovered" << endl;
#endif
            }
        }
    }
}

void Visualisation::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Left)
    {
        curAngle -= rotSpeed;
    }
    else if (e->key() == Qt::Key_Right)
    {
        curAngle += rotSpeed;
    }
    else if (e->key() == Qt::Key_Up)
    {
        curPos += moveSpeed * QPointF(qCos(curAngle), qSin(curAngle));
    }
    discover();
    update();
}
