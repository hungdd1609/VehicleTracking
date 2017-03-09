#include <QtCore/QCoreApplication>
#include "vehicletrackingserver.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    VehicleTrackingServer vehicleTrackingServer;

    return a.exec();
}
