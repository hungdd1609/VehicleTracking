#ifndef VEHICLETRACKINGSERVER_H
#define VEHICLETRACKINGSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTimer>
#include "vehicleconnection.h"
#include "cprtfcdatabase.h"
class VehicleTrackingServer : public QObject
{
    Q_OBJECT
public:
    VehicleTrackingServer();
private:
    CprTfcDatabase *serverDatabase;
    QTimer *mainTimer;
    QTcpServer *tcpServer;
    int listenPort, maxPendingConnection;
    QList <VehicleConnection *>connectionList;

signals:

public slots:
    void slot_mainTimer_timeout();
    void slot_newConnection();
    void slot_closeConnection();
};

#endif // VEHICLETRACKINGSERVER_H
