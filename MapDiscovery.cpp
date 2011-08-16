#include <QtGui>
#include "MapDiscovery.h"
#include "Visualisation.h"
#include "editor/MapEditor.h"

MapDiscovery::MapDiscovery(QWidget *parent): QWidget(parent), mapEditor(NULL)
{
    setWindowTitle("Map Discovery - Empty map");

    loadMap = new QPushButton("Load...");
    connect(loadMap, SIGNAL(clicked()), this, SLOT(loadFromFile()));
    reloadMapBtn = new QPushButton("Reload map");
    connect(reloadMapBtn, SIGNAL(clicked()), this, SLOT(reloadMap()));
    startMapEditor = new QPushButton("Edit map");
    connect(startMapEditor, SIGNAL(clicked()), this, SLOT(editMap()));

    QVBoxLayout *controls = new QVBoxLayout();
    controls->addWidget(reloadMapBtn);
    controls->addStretch(1);
    controls->addSpacing(20);
    controls->addWidget(startMapEditor);
    controls->addStretch(1);

    vis = new Visualisation(800, 600);
    vis->setMap(QVector<QVector<QPointF> > ());

    QGridLayout *mainLayout = new QGridLayout();
    mainLayout->addWidget(vis, 0, 0);
    mainLayout->addWidget(loadMap, 1, 0);
    mainLayout->addLayout(controls, 0, 1);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(mainLayout);
}

void MapDiscovery::closeEvent(QCloseEvent *)
{
    delete mapEditor;
}

void MapDiscovery::unBlockEditMap()
{
    startMapEditor->setEnabled(true);
    mapEditor = NULL;
}

void MapDiscovery::editMap()
{
    if (mapEditor != NULL)
    {
        return;
    }

#ifdef DEBUG
    qDebug() << "Creating a MapEditor window..." << endl;
#endif
    mapEditor = new MapEditor(800, 600, curMap);
    startMapEditor->setDisabled(true);
    connect(mapEditor, SIGNAL(gonnaDie()), this, SLOT(unBlockEditMap()));
    mapEditor->show();
}

namespace{

QVector<QVector<QPointF> > getMapFromFile(const QString &fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);
    QVector<QVector<QPointF> > m;
    in >> m;
    file.close();
    return m;
}

}

void MapDiscovery::loadFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Open map",
                                                    "",
                                                    "Map files (*.map);;All files (*)");

    if (fileName.isEmpty())
        return;
    else
    {
        curMap = fileName;
        setWindowTitle("Path search - " + fileName);
        QVector<QVector<QPointF> > m = getMapFromFile(fileName);
#ifdef DEBUG
        qDebug() << "File " + fileName + " has been loaded: " << endl << m << endl;
#endif
        vis->setMap(m);
    }
    vis->update();
}

void MapDiscovery::reloadMap()
{
    if (curMap.isEmpty())
        return;
    QVector<QVector<QPointF> > m = getMapFromFile(curMap);
    vis->setMap(m);
#ifdef DEBUG
    qDebug() << "File " + curMap + " has been reloaded: " << endl << m << endl;
#endif
    vis->update();
}
