#include <QtGui>
#include "MapExploration.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
   
    MapExploration *p = new MapExploration(700, 700);
    p->show();

    return app.exec();
}
