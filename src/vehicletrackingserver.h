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
    VehicleTrackingServer(QDateTime begin, int threshold, int listenPort);
    VehicleTrackingServer();
private:
    CprTfcDatabase *serverDatabase;
    QTimer *mainTimer;
    QTcpServer *tcpServer;
    int listenPort, maxPendingConnection, dataStorageTime;
    QList <VehicleConnection *>connectionList;

    //for insert test data
    QDateTime begin;
    int threshold;

signals:

public slots:
    void slot_mainTimer_timeout();
    void slot_newConnection();
    void slot_closeConnection();
};

#endif // VEHICLETRACKINGSERVER_H
