#ifndef MAPEXPLORATION_H
#define MAPEXPLORATION_H

#include <QtGui>
#include "Visualisation.h"
#include "editor/MapEditor.h"

class MapExploration: public QWidget
{
    Q_OBJECT

public slots:
    void loadFromFile();
    void reloadMap();
    void editMap();
    void unBlockEditMap();

public:
    MapExploration(QWidget *parent = NULL);
   
private:
    void closeEvent(QCloseEvent *);

    QPushButton *loadMap, *reloadMapBtn, *startMapEditor;
    MapEditor *mapEditor;
    Visualisation *vis;
    QString curMap;
};

#endif //MAPEXPLORATION_H
