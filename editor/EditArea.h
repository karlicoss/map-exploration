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
    void setEditMode(int);
    void setMap(const QVector<QVector<QPointF> > &);
    QVector<QVector<QPointF> > getMap() const;

signals:
    void mapChanged();

private:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

    void snapPoints(const QPointF &, const QPointF &);
    bool canSnap(const QPointF &, const QPointF &);

    QVector<QVector<QPointF> > map;
    QPointF startPos, lastPos;
    qreal snapRadius;
    int mode;
    bool buttonPressed;
};

#endif
