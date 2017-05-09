#ifndef VEHICLECONNECTION_H
#define VEHICLECONNECTION_H

#include <QThread>
#include <QTcpSocket>
#include <QTimer>
#include "cprtfcdatabase.h"

//----------------------------------------------------------
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(4)     /* set alignment to 1 byte boundary */

#define SYS7B_REV_BUFF_SIZE 10240
#define TIME_TRAIN_SEND_DATA_SERVER 20
#define MAXDATAPACKSIZE 256
#define TIME_CAR_SEND_DATA_SERVER 30
//#define CMD_DEBUG 0
//#define CMD_DEV_EVENT 1
//#define CMD_BIRDIR_ACK 2
//#define CMD_SYS_GET 3

#define max_name_way        7
#define max_name_mac        28 // tuyến thống nhất

#define TYPE_CAR 1
#define TYPE_TRAIN 2

#define PERIOD_TIME 30



//-----------------------------------------------------------------------------
struct SDateTime{
    unsigned char Hour;
    unsigned char Min;
    unsigned char Sec;
    unsigned char Day;
    unsigned char Month;
    unsigned char Year;
};
//------------------------------------------------------------------------------
struct GpsInt{
    unsigned int Lat;
    unsigned int Long;
    short Speed;
    SDateTime DateTime;
};
//------------------------------------------------------------------------------
struct GpsMng{
    GpsInt Gps;
    unsigned char GpsStates;// trang thai gps
    unsigned char NumOfSat;//so ve tinh
    short Height; //cao do
};
//------------------------------------------------------------------------------
//struct UserInfor{
//    char uid[24];
//    char Giaypheplaixe[16];
//    char Hoten[44];
//    char Thongtinkhac[10];
//    char States;
//    char Event;
//    int TimerDriver; // thoi gian lai xe lien tuc
//    GpsMng GPS;
//};
struct User{ // dang nhap+ dang xuat
    char uid[24];
    char Giaypheplaixe[17];
    char Hoten[44];
    char States;
    char Event;
    int TimerDriver; // thoi gian lai xe lien tuc
    GpsMng GPS;
};
//------------------------------------------------------------------------------
struct DevInfor{
    char NameDevice[20]; // tên thiet bi
    char Type[20];// seri
};
//------------------------------------------------------------------------------
struct Event{// su kien ve van toc
    GpsMng GpsN;
    unsigned long Kmm;
    unsigned char Speedlm;
    unsigned char SpeedNlm;
};
//------------------------------------------------------------------------------
struct TrainAbsRec{
    unsigned int KmM;
    unsigned char TrainName;// id tuyen tau
    unsigned char TrainLabel;// id mac tau
    unsigned char GpsStatesNumOfSat;// trang thai gps bit 8, so ve tinh bit tu 7-1
    unsigned char LimitSpeed;// toc do cong lenh
    unsigned int Long1s;
    unsigned int Lat1s;
    SDateTime TimeNow1s;
    short GpsSpeed1s;
    short DtSpeed1s;
    short Height;           // do cao
    short Heading;/// Dregree
    signed char SpeedBuff[TIME_TRAIN_SEND_DATA_SERVER-1];
    unsigned char PresBuff[TIME_TRAIN_SEND_DATA_SERVER];
    signed char WheelSpeed[TIME_TRAIN_SEND_DATA_SERVER-1];// tốc độ đồng trục
};
struct CarAbsRec{
    GpsMng Gps1sStart;
    unsigned char Speed[TIME_CAR_SEND_DATA_SERVER];
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
//------------------------------------------------------------------------------
enum EventType{
    EVENT_GPS_ABS=0,//Ban ghi Gps tuyet doi
    EVENT_GPS_DIF,//Ban ghi GPS tuong doi
    EVENT_USER_SIGNIN,//Ban ghi su kien nguoi dang nhap
    EVENT_USER_SIGNOUT,//Ban ghi su kien nguoi dang xuat
    EVENT_USER_OVERTIME,//Qua thoi gian
    EVENT_VEHICLE_STOP,// dung
    EVENT_VEHICLE_RUN,
    EVENT_DIRVER_OVER_DAY,
    EVENT_DIRVER_OVER_SPEED,
    EVENT_TRAIN,
    EVENT_TRAIN_OVER_SPEED,
    EVENT_TRAIN_CHANGE_SPEED_LIMIT,
};
#pragma pack(pop)   /* restore original alignment from stack */
//-------------------------------------------------------------------------------
typedef struct ReadFileMng{
    int Id;
    QString Name;
    QString Path;
    int Status;
    int Size;
    int Block;
    unsigned char* Buff;
    QDateTime StartRead;
    QDateTime EndRead;
    int zeroCount;
}ReadFileMng;

//-------------------------------------------------------------------------------

enum CPR_CMD{
        CMD_DEBUG,
        CMD_DEV_EVENT,
        CMD_BIRDIR_ACK,
        CMD_SYS_GET,
        CMD_SYS_SET,
        CMD_L7B_MNG,
        CMD_FILE_MNG,
};

enum GprsFMngCmd{
    CmdGprsFMngCreateF,
    CmdGprsFMngWriteF,
    CmdGprsFMngReadF,
    CmdGprsFMngChangeFileName,
    CmdGprsGetFileLen,
    CmdGprsFileDelete,
    CmdGprsGetFileListByTime,
};

//-------------------------------------------------------------------------------
enum L7B_CMD_SET{
    L7B_CMD_EARSE=0,
    L7B_CMD_GET_LOG,
};
//-------------------------------------------------------------------------------
enum READ_LOG_FILE_STATUS{
    LOG_FILE_REQUEST_READ = 1,
    LOG_FILE_SIZE_READING = 2,
    LOG_FILE_SIZE_READ = 3,
    LOG_FILE_PROCESSING = 4,
    LOG_FILE_DONE = -1,
    LOG_FILE_ERROR = -2
};
//

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
    QTimer *requestLogTimer;
    QTcpSocket *tcpSocket;
    QDateTime lastRecvTime;
    QString plateNumber;
    QString rawData;
    int state;
    unsigned int TotalRevPCk;
    unsigned char Sys7bRevBuff[SYS7B_REV_BUFF_SIZE];
    unsigned short Sys7bREvIdx,lr,filelenpos;
    Event EventSpeed;
    TrainAbsRec TraiRevRec;
    CarAbsRec CarRevRec;
    User Usr;
    DevInfor HardDev;

    ReadFileMng readFileMng;
    FILE *pw;



    QDateTime dateReqLog;
    QDateTime lastReqLog;
    bool isReqLog;
    bool isFirstTime;
    QString logPath;

    bool Insert2PhuongTienLog(QString machuyen, QDateTime gpsTime, QString longitude, QString latitude, unsigned char GpsStates, QString  cData, QString table);
    bool Insert2PhuongTien(QDateTime gpsTime, QString vehicleLabel, QString longitude, QString latitude, unsigned char GpsStates, QString cData );
    bool UpdateLogFile(int trangthai, QDateTime batdau, QDateTime ketthuc, int size, int block, QString duongdan, int id);
    bool UpdateCommand(int id, QDateTime sent, int status);
    void GetListFileLogBytime(QDateTime Start,QDateTime End,QString Filter);
    void GetLogByTime(QDateTime qStart,QDateTime qEnd);
    void GetFileSize(QString FileName);
    void GprsFmngReadBlk(QString FileName, int StartAddr, int Len);
    void Sys7bProcessRevPck(unsigned char* Pck,unsigned short len);
    void Sys7bInput(unsigned char data);
    float ConvertGPSdegreeToGPSDecimal(long input);
    signed short EncodeDataPack(unsigned char *inbuf,unsigned short count);
    unsigned short DecodeDataPack(unsigned char *in2outbuff,unsigned short count);
    unsigned char SendPck(unsigned char* PayLoad,unsigned short PayLoadLen,unsigned char Cmd,unsigned char PckIdx);
    SDateTime QDateTime2SDateTime(QDateTime);

private slots:
    void slot_connectionTimer_timeout();
    void slot_readyRead();
    void slot_socketDisconnected();
    void slot_socketError(QAbstractSocket::SocketError socketError);
    void slot_socketDestroyed();
    void slot_requestInfoTimer();
    void slot_requestLogTimer();
protected:
    void run();
};

#endif // VEHICLECONNECTION_H
