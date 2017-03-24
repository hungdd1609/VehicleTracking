#ifndef VEHICLETRACKINGSERVER_H
#define VEHICLETRACKINGSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTimer>
#include "vehicleconnection.h"
#include "cprtfcdatabase.h"

struct VehicleLog{
    //QString bienso;
    QDateTime thoigian;
    double kinhdo;
    double vido;
    double vantocGps;
    double vantocDongho;
    int lytrinh;
    int trangthaiGps;
    int huong;
};
//------------------------------------------------------------------------
struct HanhTrinh{
    QDateTime thoigianBatdau;
    double kinhdoBatdau;
    double vidoBatdau;
    QDateTime thoigianKetthuc;
    double kinhdoKetthuc;
    double vidoKetthuc;
};
//------------------------------------------------------------------------

class VehicleTrackingServer : public QObject
{
    Q_OBJECT
public:    
    VehicleTrackingServer();
private:
    CprTfcDatabase *serverDatabase;
    QTimer *mainTimer;
    QTcpServer *tcpServer;
    int listenPort, maxPendingConnection, dataStorageTime;
    QList <VehicleConnection *>connectionList;
    QMap <QString, QList<VehicleLog> > mapVehicleLog;
    QMap <QString, HanhTrinh> mapHanhTrinh;
    QMap <QString,int> mapDieuKien;
    QDateTime lastVehicleLog;

    void createPartition(QString table);
    bool finishJourney(QString key, VehicleLog v);

signals:

public slots:
    void slot_mainTimer_timeout();
    void slot_newConnection();
    void slot_closeConnection();
};

#endif // VEHICLETRACKINGSERVER_H
