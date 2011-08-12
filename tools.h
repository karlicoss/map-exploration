#ifndef TOOLS_H
#define TOOLS_H

#include <QtGlobal>
#include <QPointF>

uint qHash(const QPointF &p);

qreal distance(const QPointF &a, const QPointF &b);

qreal PI();

qreal rad2degr(qreal);
qreal degr2rad(qreal);

#endif // TOOLS_H
