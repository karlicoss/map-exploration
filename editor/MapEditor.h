#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <QtGui>
#include "EditArea.h"

class MapEditor: public QWidget
{
    Q_OBJECT

public slots:
    void saveToFile();
    void loadFromFile();
    void changeSnapRadius(int value);
    void generateRandom();
    void mapChanged();

signals:
    void gonnaDie();

public:
    MapEditor(int mapwidth, int mapheight, const QString &, QWidget *parent = NULL);

private:
    void closeEvent(QCloseEvent *);

    EditArea *editArea;
    QPushButton *loadBtn, *saveBtn, *randomBtn;
    QSlider *snapRadiusSlider;

    bool fileSaved;
};

#endif
