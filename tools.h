#ifndef TOOLS_H
#define TOOLS_H

#include <QtGlobal>
#include <QPointF>

uint qHash(const QPointF &p);

qreal distance(const QPointF &a, const QPointF &b);

qreal PI();

#endif // TOOLS_H
