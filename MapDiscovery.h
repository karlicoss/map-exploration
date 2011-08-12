#ifndef MAPDISCOVERY_H
#define MAPDISCOVERY_H

#include <QtGui>
#include "Visualisation.h"
#include "editor/MapEditor.h"

class MapDiscovery: public QWidget
{
    Q_OBJECT

public slots:
    void loadFromFile();
    void reloadMap();
    void editMap();
    void unBlockEditMap();

public:
    MapDiscovery(QWidget *parent = NULL);
   
private:
    void closeEvent(QCloseEvent *);

    QPushButton *loadMap, *reloadMapBtn, *startMapEditor;
    MapEditor *mapEditor;
    Visualisation *vis;
    QString curMap;
};

#endif
