#ifndef EDITAREA_H
#define EDITAREA_H

#include <QtGui>
#include <QtGlobal>

class EditArea: public QWidget
{
    Q_OBJECT

public: 
    EditArea(QWidget *parent = NULL);

    void setSnapRadius(int);
    void setMap(const QVector<QVector<QPointF> > &);
    QVector<QVector<QPointF> > getMap() const;

signals:
    void mapChanged();

private:
    enum EditMode
    {
        noMode,
        drawMode,
        deleteMode
    };

    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

    void snapPoints(const QPointF &, const QPointF &);
    bool canSnap(const QPointF &, const QPointF &) const;

    QVector<QVector<QPointF> > map;
    QPointF startPos, lastPos;
    qreal snapRadius;
    EditMode mode;
};

#endif
