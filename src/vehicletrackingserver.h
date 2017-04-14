#ifndef VEHICLETRACKINGSERVER_H
#define VEHICLETRACKINGSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTimer>
#include "vehicleconnection.h"
#include <QFile>
#include "cprtfcdatabase.h"

struct VehicleLog{
    //QString bienso;
    int id;
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
typedef struct GsThOldLogRec{
    unsigned short Km;
    unsigned short m;
    int Rundirection;
    SDateTime DateTime;
    unsigned char IdTuyen;
    unsigned char IdMacTau;
    unsigned char Vps;
    unsigned char VDauTruc;
    unsigned short Altitude;//cao do
    long Lat;
    long Long;
    unsigned char Presure;
    unsigned char Speed;
}GsThOldLogRec;
//------------------------------------------------------------------------------
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
    QString dataPath;
    int minRestTime; //seconds
    int minPackage;

    GsThOldLogRec GsPosLog;
    TrainAbsRec TraiRevRec;
    unsigned char Modew;
    unsigned char WrBuff[260];
    char NameFile[25];
    FILE *pw;
    short CoutBuff;




    void createPartition(QString table);
    bool finishJourney(QString key, VehicleLog v);
    bool writeLog(QString key, QDateTime begin, QDateTime end, QString fileName);
    void ConvertToOldBuff(GsThOldLogRec* Rec,unsigned char OutBuff16[],unsigned char Buff6[]);
    void WriteBuffLog(GsThOldLogRec GsPosLog, QString NameFile);

    void scanNewLog();
    void splitJourneyOnline();
    void updateHanhTrinhId(QString key, QDateTime batdau, int phuongtienlogid);

signals:

public slots:
    void slot_mainTimer_timeout();
    void slot_newConnection();
    void slot_closeConnection();
};

#endif // VEHICLETRACKINGSERVER_H
