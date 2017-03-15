#ifndef VEHICLECONNECTION_H
#define VEHICLECONNECTION_H

#include <QThread>
#include <QTcpSocket>
#include <QTimer>
#include "cprtfcdatabase.h"

//----------------------------------------------------------
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(4)     /* set alignment to 1 byte boundary */

#define SYS7B_REV_BUFF_SIZE 194
#define TIME_SEND_DATA_SERVER 20
#define MAXDATAPACKSIZE 256
#define CMD_DEBUG 0
#define CMD_DEV_EVENT 1
#define CMD_BIRDIR_ACK 2
#define CMD_SYS_GET 3

struct SDateTime{
    unsigned char Hour;
    unsigned char Min;
    unsigned char Sec;
    unsigned char Day;
    unsigned char Month;
    unsigned char Year;
};

struct GpsInt{
    unsigned int Lat;
    unsigned int Long;
    unsigned char Speed;
    SDateTime DateTime;
};

struct UserInfor{
    char uid[24];
    char Giaypheplaixe[16];
    char Hoten[44];
    char Thongtinkhac[10];
    char States;
    char Event;
    int TimerDriver; // thoi gian lai xe lien tuc
    GpsInt GPS;
};
struct DevInfor{
    char NameDevice[20]; // tên thiet bi
    char Type[20];// seri
};
//---------------------------------------------------------------
struct Event{// su kien ve van toc
    GpsInt GpsN;
    unsigned char Speedlm;
    unsigned char SpeedNlm;
};
//------------------------------------------------------------------------------
typedef struct TrainAbsRec{
    unsigned int KmM;
    unsigned char TrainName;// id tuyen tau
    unsigned char TrainLabel;// id mac tau
    unsigned int Long1s;
    unsigned int Lat1s;
    SDateTime TimeNow1s;
    unsigned char SpeedBuff[TIME_SEND_DATA_SERVER];
    unsigned char PresBuff[TIME_SEND_DATA_SERVER];
};
//------------------------------------------------------------------------------
enum LogRecType{
    REC_GPS_ABS=0,//Ban ghi Gps tuyet doi
    REC_GPS_DIF,//Ban ghi GPS tuong doi
    REC_USER_SIGNIN,//Ban ghi su kien nguoi dang nhap
    REC_USER_SIGNOUT,//Ban ghi su kien nguoi dang xuat
    REC_USER_OVERTIME,//Qua thoi gian
    REC_VEHICLE_STOP,// dung
    REC_VEHICLE_RUN,
    REC_DIRVER_OVER_DAY,
    REC_DIRVER_OVER_SPEED,
    REC_TRAIN,
    REC_TRAIN_OVER_SPEED,
    REC_TRAIN_CHANGE_SPEED_LIMIT,
};
#pragma pack(pop)   /* restore original alignment from stack */


class VehicleConnection : public QThread
{
    Q_OBJECT
public:
    VehicleConnection();
    VehicleConnection(QTcpSocket *socket);
    QDateTime getLastRecvTime(){return lastRecvTime;}
    ~VehicleConnection();
private:
    CprTfcDatabase *connectionDatabase;
    QTimer *connectionTimer;
    QTimer *requestInfoTimer;
    QTcpSocket *tcpSocket;
    QDateTime lastRecvTime;
    QString plateNumber;
    unsigned short Sys7bREvIdx;
    unsigned int TotalRevPCk;
    int state;
    unsigned char SendPck(unsigned char* PayLoad,unsigned short PayLoadLen,unsigned char Cmd,unsigned char PckIdx);
    void Sys7bProcessRevPck(unsigned char* Pck,unsigned short len);
    void Sys7bInput(unsigned char data);
    signed short EncodeDataPack(unsigned char *inbuf,unsigned short count);
    unsigned short DecodeDataPack(unsigned char *in2outbuff,unsigned short count);

    unsigned char Sys7bRevBuff[SYS7B_REV_BUFF_SIZE];
    Event EventSpeed;
    TrainAbsRec TraiRevRec;
    UserInfor Usr;
    DevInfor HardDev;

private slots:
    void slot_connectionTimer_timeout();
    void slot_readyRead();
    void slot_socketDisconnected();
    void slot_socketError(QAbstractSocket::SocketError socketError);
    void slot_socketDestroyed();
//    void slot_requestInfoTimer();
protected:
    void run();
};

#endif // VEHICLECONNECTION_H
