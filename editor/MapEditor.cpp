#include "MapEditor.h"
#include "EditArea.h"
#include "editortools.h"

MapEditor::MapEditor(int mapwidth, int mapheight, const QString &fileName, QWidget *parent): QWidget(parent)
{
    setWindowTitle("Simple map editor - Empty map");

    loadBtn = new QPushButton("Load...");
    saveBtn = new QPushButton("Save...");
    randomBtn = new QPushButton("Random");
    connect(loadBtn, SIGNAL(clicked()), this, SLOT(loadFromFile()));
    connect(saveBtn, SIGNAL(clicked()), this, SLOT(saveToFile()));
    connect(randomBtn, SIGNAL(clicked()), this, SLOT(generateRandom()));
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(loadBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(randomBtn);

    editArea = new EditArea(this);
    editArea->move(10, 10);
    editArea->setFixedSize(mapwidth, mapheight);
    editArea->setMouseTracking(true);
    editArea->setSnapRadius(25);
    if (!fileName.isEmpty())
    {
        editArea->setMap(getMapFromFile(fileName));
        setWindowTitle("Simple map editor - " + fileName);
        fileSaved = true;
    }
    connect(editArea, SIGNAL(mapChanged()), this, SLOT(mapChanged()));

    snapRadiusSlider = new QSlider(Qt::Horizontal);
    snapRadiusSlider->setMaximum(50);
    snapRadiusSlider->setSliderPosition(25);
    connect(snapRadiusSlider, SIGNAL(sliderMoved(int)), this, SLOT(changeSnapRadius(int)));

    QVBoxLayout *snapLayout = new QVBoxLayout();
    QLabel *lbl = new QLabel("Snap precision:");
    snapLayout->addWidget(lbl);
    snapLayout->addWidget(snapRadiusSlider);

    editMode = new QButtonGroup();
    drawMode = new QRadioButton("Draw lines mode");
    deleteMode = new QRadioButton("Delete lines mode");
    drawMode->setChecked(true);
    editMode->addButton(drawMode);
    editMode->setId(drawMode, 1);
    editMode->addButton(deleteMode);
    editMode->setId(deleteMode, 2);
    connect(editMode, SIGNAL(buttonClicked(int)), this, SLOT(changeEditMode(int)));

    QVBoxLayout *editLayout = new QVBoxLayout();
    //editLayout->addWidget(drawMode);
    //editLayout->addWidget(deleteMode);
    editLayout->addSpacing(10);
    editLayout->addLayout(snapLayout);
    editLayout->addStretch(1);

    QGridLayout *mainLayout = new QGridLayout();
    mainLayout->addWidget(editArea, 0, 0);
    mainLayout->addLayout(btnLayout, 1, 0);
    mainLayout->addLayout(editLayout, 0, 1);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(mainLayout);
}

void MapEditor::closeEvent(QCloseEvent *)
{
    emit gonnaDie();
}

void MapEditor::generateRandom()
{
    setWindowTitle("Simle map editor - random map(unsaved)");
    fileSaved = false;
    QVector<QVector<QPointF> > m;
    for (int i = 0; i < 15; i++)
    {
        QVector<QPointF> l;
        qint32 w = qMax(qrand() % 400, 10);
        qint32 h = qMax(qrand() % 300, 10);
        qint32 px = qrand() % (editArea->width() - w);
        qint32 py = qrand() % (editArea->height() - h);
        for (int j = 0; j < 5; j++)
        {
            l.append(QPointF(px + qrand() % w, py + qrand() % h));
        }
        if (qrand() % 5 == 0)
            l.append(l.front());
        m.append(l);
    }
    editArea->setMap(m);
}

void MapEditor::loadFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Open map",
                                                    "",
                                                    "Map files (*.map);;All files (*)");

    if (fileName.isEmpty())
        return;
    else
    {
        setWindowTitle("Simple map editor - " + fileName);
        QVector<QVector<QPointF> > m = getMapFromFile(fileName);
#ifdef DEBUG
        qDebug() << "File " + fileName + " has been loaded: " << endl << m << endl;
#endif
        editArea->setMap(m);
        fileSaved = true;
    }
    editArea->update();
}


void MapEditor::saveToFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                   "Save map",
                                                   "untitled.map",
                                                   "Map files (*.map);;All files (*)");
    if (fileName.isEmpty())
        return;
    else
    {
        setWindowTitle("Simple map editor - " + fileName);
        QFile file(fileName);
        file.open(QIODevice::WriteOnly);
        QDataStream out(&file);
        out << editArea->getMap();
#ifdef DEBUG
        qDebug() << "File " + fileName + "has been saved: " << endl << editArea->getMap() << endl;
#endif
        file.close();
        fileSaved = true;
    }
}

void MapEditor::mapChanged()
{
#ifdef DEBUG
    qDebug() << "File has changed" << endl;
#endif

    if (fileSaved)
    {
        setWindowTitle(windowTitle() + "(unsaved)");
    }

    fileSaved = false;
}

void MapEditor::changeSnapRadius(int value)
{
    editArea->setSnapRadius(value);
}

void MapEditor::changeEditMode(int id)
{
    editArea->setEditMode(id);
    if (id == 1)
    {
        snapRadiusSlider->setDisabled(false);
    }
    else if (id == 2)
    {
        snapRadiusSlider->setDisabled(true);
    }
}



