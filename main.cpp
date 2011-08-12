#include <QtGui>
#include "MapDiscovery.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
   
    MapDiscovery *p = new MapDiscovery();
    p->show();

    return app.exec();
}
