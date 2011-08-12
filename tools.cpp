#include "tools.h"

#include <QPair>
#include <QHash>
#include <QtCore/qmath.h>

uint qHash(const QPointF &p)
{
    return qHash(QPair<qint64, qint64>(p.x(), p.y()));
}

qreal distance(const QPointF &a, const QPointF &b)
{
    QPointF c = b - a;
    return qSqrt(c.x() * c.x() + c.y() * c.y());
}

qreal PI()
{
    return 4 * qAtan(1);
}

qreal rad2degr(qreal rad)
{
    return rad / PI() * 180;
}

qreal degr2rad(qreal degr)
{
    return degr / 180 * PI();
}
