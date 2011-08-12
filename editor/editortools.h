#ifndef EDITORTOOLS_H
#define EDITORTOOLS_H

#include <QVector>
#include <QString>
#include <QPointF>

QVector<QVector<QPointF> > getMapFromFile(const QString &fileName);

#endif // EDITORTOOLS_H
