#include <QFile>
#include <QDataStream>

#include "editortools.h"

QVector<QVector<QPointF> > getMapFromFile(const QString &fileName)//assuming the file already exists
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);
    QVector<QVector<QPointF> > m;
    in >> m;
    file.close();
    return m;
}

