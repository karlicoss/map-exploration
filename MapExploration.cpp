#include <QtGui>
#include "MapExploration.h"
#include "Visualisation.h"
#include "editor/MapEditor.h"

MapExploration::MapExploration(int vwidth_, int vheight_, QWidget *parent):
    QWidget(parent),
    name("Map exploration"),
    mapEditor(NULL),
    visualisation(NULL),
    loadMapBtn(new QPushButton("Load...")),
    reloadMapBtn(new QPushButton("Reload map")),
    startMapEditorBtn(new QPushButton("Edit map")),
    pauseVisualisationBtn(new QPushButton("Pause visualisation")),
    toggleManualControlBtn(new QPushButton("Toggle manual control")),
    vwidth(vwidth_), vheight(vheight_)
{
    setWindowTitle(name + " - " + "Empty map");


    connect(loadMapBtn, SIGNAL(clicked()), this, SLOT(loadFromFile()));
    connect(reloadMapBtn, SIGNAL(clicked()), this, SLOT(reloadMap()));
    connect(startMapEditorBtn, SIGNAL(clicked()), this, SLOT(editMap()));

    QVBoxLayout *mapControls = new QVBoxLayout();
    mapControls->addWidget(loadMapBtn);
    mapControls->addWidget(reloadMapBtn);
    mapControls->addWidget(startMapEditorBtn);
    mapControls->addStretch(1);

    QHBoxLayout *visControls = new QHBoxLayout();
    visControls->addWidget(pauseVisualisationBtn);
    visControls->addWidget(toggleManualControlBtn);
    visControls->addStretch(1);

    mainLayout = new QGridLayout();
    setVisualisation(new Visualisation(vwidth, vheight, QVector<QVector<QPointF> > ()));
    mainLayout->addLayout(visControls, 1, 0);
    mainLayout->addLayout(mapControls, 0, 1);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(mainLayout);
}

void MapExploration::closeEvent(QCloseEvent *)
{
    delete mapEditor;
}

void MapExploration::unBlockEditMap()
{
    startMapEditorBtn->setEnabled(true);
    mapEditor = NULL;
}

void MapExploration::editMap()
{
    if (mapEditor != NULL)
    {
        return;
    }
#ifdef DEBUG
    qDebug() << "Creating a MapEditor window..." << endl;
#endif
    mapEditor = new MapEditor(vwidth, vheight, curMap);
    startMapEditorBtn->setDisabled(true);
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

void MapExploration::loadFromFile()
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
        setWindowTitle(name + " - " + fileName);
        QVector<QVector<QPointF> > m = getMapFromFile(fileName);
#ifdef DEBUG
        qDebug() << "File " + fileName + " has been loaded: " << endl << m << endl;
#endif
        setVisualisation(new Visualisation(vwidth, vheight, m));
    }
}

void MapExploration::reloadMap()
{
    if (curMap.isEmpty())
        return;
    QVector<QVector<QPointF> > m = getMapFromFile(curMap);
    setVisualisation(new Visualisation(vwidth, vheight, m));
#ifdef DEBUG
    qDebug() << "File " + curMap + " has been reloaded: " << endl << m << endl;
#endif
}

void MapExploration::setVisualisation(Visualisation *newvis)
{
    if (visualisation != NULL)
    {
        mainLayout->removeWidget(visualisation);
        delete visualisation;
    }
    visualisation = newvis;
    mainLayout->addWidget(visualisation, 0, 0);
    connect(pauseVisualisationBtn, SIGNAL(clicked()), visualisation, SLOT(togglePause()));
    connect(toggleManualControlBtn, SIGNAL(clicked()), visualisation, SLOT(toggleManualControl()));
}
