#include "vehicleconnection.h"
#include <stdio.h>
#include <string.h>

VehicleConnection::VehicleConnection()
{
    connectionDatabase = NULL;
    state=0;
    plateNumber.clear();
}
VehicleConnection::VehicleConnection(QTcpSocket *socket){
    plateNumber.clear();
    connectionDatabase = NULL;
    moveToThread(this);
    tcpSocket = socket;
    tcpSocket->setParent(0);
    tcpSocket->moveToThread(this);
    lastRecvTime = QDateTime::currentDateTime();
}
VehicleConnection::~VehicleConnection(){
    qDebug()<<"VehicleConnection::~VehicleConnection()";
}

//----------------------------------------------------------
#define SYS7B_REV_BUFF_SIZE 194
unsigned char Sys7bRevBuff[SYS7B_REV_BUFF_SIZE];
typedef struct SDateTime{
    unsigned char Hour;
    unsigned char Min;
    unsigned char Sec;
    unsigned char Day;
    unsigned char Month;
    unsigned char Year;
}SDateTime;
typedef struct GpsInt{
    long Lat;
    long Long;
    unsigned char Speed;
    SDateTime DateTime;
}GpsInt;
typedef struct UserInfor{
    char uid[24];
    char Giaypheplaixe[16];
    char Hoten[44];
    char Thongtinkhac[10];
    char States;
    char Event;
    int TimerDriver; // thoi gian lai xe lien tuc
    GpsInt GPS;
}UserInfor;
typedef struct DevInfor{
    char NameDevice[20]; // tên thiet bi
    char Type[20];// seri
}DevInfor;
//---------------------------------------------------------------
typedef struct Event{// su kien ve van toc
    GpsInt GpsN;
    unsigned char Speedlm;
    unsigned char SpeedNlm;
}Event;
Event EventSpeed;
//------------------------------------------------------------------------------
#define TIME_SEND_DATA_SERVER 20
typedef struct TrainAbsRec{
    unsigned long KmM;
    unsigned char TrainName;// id tuyen tau
    unsigned char TrainLabel;// id mac tau
    long Long1s;
    long Lat1s;
    SDateTime TimeNow1s;
    unsigned char SpeedBuff[TIME_SEND_DATA_SERVER];
    unsigned char PresBuff[TIME_SEND_DATA_SERVER];
}TrainAbsRec;
TrainAbsRec TraiRevRec;
//------------------------------------------------------------------------------
UserInfor Usr;
//------------------------------------------------------------------------------
#define CMD_DEBUG 0
#define CMD_DEV_EVENT 1
#define CMD_BIRDIR_ACK 2
#define CMD_SYS_GET 3
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
}LogRecType;


//----------------------------------------------------------
DevInfor HardDev;
static const unsigned char bitmap[]={1,2,4,8,16,32,64,128};
#define MAXDATAPACKSIZE 256
signed short EncodeDataPack(unsigned char inbuf[],unsigned short count)
{
    unsigned char outbuf[MAXDATAPACKSIZE];//max 64 byte dadta-> so luong byte =9*8+3//
    register unsigned short tmp,dataidx=0,out_idx,i,j,rem,quot;
    if(count==1){//neu goi chi co 1 byte, thi bo bit 7 di
        inbuf[0]=inbuf[0]|0x80;//
        return 1;
    }
    if(count>(MAXDATAPACKSIZE*7)/8-1)return -1;
    quot=count/7;
    rem=count%7;//lay so luong chan cua x*7 byte
    out_idx=0;//bat dau doan du lieu tu byte thu 2
    for(i=0;i<quot;i++){
        tmp=0;
        for(j=0;j<7;j++){
            if(inbuf[dataidx]&0x80) tmp|=bitmap[j];//kem tra de set bit tuong ung
            outbuf[out_idx]=inbuf[dataidx]&0x7F;//lay du lieu vao bo dem
            out_idx++;//tang con tro du lieu ra ke tiep
            dataidx++;  //tang con tro du lieu vao
        }
        outbuf[out_idx]=tmp;//lay byte giu 7 bit msb cua 7 byte du lieu truoc
        out_idx++;//tang con tro bo dem ra
    } // het phan chan 7 cua du lieu
    tmp=0; //den phan le cua lieu
    if(rem>0){
        for(j=0;j<rem;j++){ //kiem tra tung byte cua phan le
            if(inbuf[dataidx]&0x80) tmp|=bitmap[j];// dat bit map tuong ung
            outbuf[out_idx]=inbuf[dataidx]&0x7F; //lay du lieu
            out_idx++;//dia chi ra tiep the
            dataidx++;// dia chi vao tiep theo
        }
        outbuf[out_idx]=tmp; //byte cuoi cung cua du lieu ra
        out_idx++;     //gan check sum cho du lieu ra
    }
    outbuf[0]|=0x80;  //Bat co dau frame
    memcpy(inbuf,outbuf,out_idx);
    return out_idx;
}
//------------------------------------------------------------------------------
unsigned short DecodeDataPack(unsigned char in2outbuff[],unsigned short count)
{
    register unsigned short i,j,p_out_mp=0;
    register unsigned short p_out_dp,quot,rem;
    in2outbuff[0]&=0x7F;
    if(count==1){
        return 1;
    }
    p_out_dp=0;
    quot=count/8;
    rem=count%8;
    for(i=0;i<quot;i++){
        for(j=0;j<7;j++){
            if(in2outbuff[p_out_dp+7]&bitmap[j]){
                in2outbuff[p_out_mp]= in2outbuff[p_out_dp+j]|0x80;
            }else{
                in2outbuff[p_out_mp]= in2outbuff[p_out_dp+j];
            }
            p_out_mp++;
        }
        p_out_dp+=8;
    }
    for(j=0;j<rem-1;j++){
        if(in2outbuff[rem+p_out_dp-1]&bitmap[j]){
            in2outbuff[p_out_mp]= in2outbuff[p_out_dp+j]|0x80;
        }else{
            in2outbuff[p_out_mp]= in2outbuff[p_out_dp+j];
        }
        p_out_mp++;
    }
    return p_out_mp;
};
//-------------------------------------------------------------------------
unsigned char VehicleConnection::SendPck(unsigned char* PayLoad,unsigned short PayLoadLen,unsigned char Cmd,unsigned char PckIdx)
{
    unsigned short Pos,crc,i;
    static unsigned int TotalSendPck=0;
    TotalSendPck++;
    crc=PayLoad[0];
    Pos=PayLoadLen;
    PayLoad[Pos]=PckIdx;
    PayLoad[Pos+1]=Cmd;//
    for(i=0;i<PayLoadLen+2;i++)crc^=PayLoad[i];
    PayLoad[Pos+2]=crc&0x7F;
    PayLoad[Pos+3]=0xFF;
    QString s="";
    for(i=0;i<PayLoadLen+4;i++){
        // s=s+ IntToHex(PayLoad[i],2)+" ";
        s=s+ QString::number(PayLoad[i],16).prepend("0x")+" ";
    }
    qDebug() << s;
    tcpSocket->write((const char *)PayLoad,PayLoadLen+4);
}


//--------------------------------------------------------------------------------
unsigned char PBuff[256];
int eventpck=0;
void VehicleConnection::Sys7bProcessRevPck(unsigned char* Pck,unsigned short len)
{
    unsigned char l,i;
    static unsigned int TotalRevPCk=0;
    //AnsiString eTime,TrainRecStr;
    QString eTime,TrainRecStr;
    PBuff[0]=128;
    TotalRevPCk++;

    switch(Pck[len-2]){
    case CMD_DEBUG:
        qDebug() <<"CMD_DEBUG";
        break;
    case CMD_DEV_EVENT:
        SendPck(PBuff,1,CMD_BIRDIR_ACK,Pck[len-3]);
        //eventpck++;
        l=DecodeDataPack(Pck,len-3);
        switch(Pck[0]&0x3F){
        case REC_GPS_ABS://Ban ghi Gps tuyet doi
            qDebug() <<"Ban ghi tuyet doi";
            break;
        case REC_GPS_DIF://Ban ghi GPS tuong doi
            qDebug() <<"Ban ghi tuong doi";
            break;
        case REC_USER_SIGNIN://Ban ghi su kien nguoi dang nhap
            memcpy(&Usr,Pck,l);
            memcpy(&Usr,Pck+1,l);
            eTime.sprintf("%d/%d/%d %d:%d:%d",Usr.GPS.DateTime.Day,Usr.GPS.DateTime.Month,Usr.GPS.DateTime.Year,Usr.GPS.DateTime.Hour,Usr.GPS.DateTime.Min,Usr.GPS.DateTime.Sec);
            //                              qDebug() <<eTime+"->"+AnsiString(Usr.Hoten)+" "+AnsiString(Usr.Giaypheplaixe)+" Singin");
            break;
        case REC_USER_SIGNOUT://Ban ghi su kien nguoi dang xuat
            memcpy(&Usr,Pck+1,l);
            eTime.sprintf("%d/%d/%d %d:%d:%d",Usr.GPS.DateTime.Day,Usr.GPS.DateTime.Month,Usr.GPS.DateTime.Year,Usr.GPS.DateTime.Hour,Usr.GPS.DateTime.Min,Usr.GPS.DateTime.Sec);
            break;
        case REC_USER_OVERTIME://Qua thoi gian
            qDebug() <<"Qua thoi gian";
            break;
        case REC_VEHICLE_STOP:// dung
            qDebug() <<"Dung xe";
            break;
        case REC_VEHICLE_RUN:
            qDebug() <<"Do xe";
            break;
        case REC_DIRVER_OVER_DAY:
            qDebug() <<"Lai qua ngay";
            break;
        case REC_DIRVER_OVER_SPEED:
            qDebug() <<"Qua van toc";
            break;
        case REC_TRAIN:
            memcpy(&TraiRevRec,&Pck+1,sizeof(TrainAbsRec));
            qDebug() << "Ban ghi tau hoa";
            TrainRecStr.sprintf("Train:%d ->%d/%d/%d %d:%d:%d Lat%ld Long%ld Km %d M%d",TraiRevRec.TrainLabel,TraiRevRec.TimeNow1s.Day,TraiRevRec.TimeNow1s.Month,TraiRevRec.TimeNow1s.Year,TraiRevRec.TimeNow1s.Hour,TraiRevRec.TimeNow1s.Min,TraiRevRec.TimeNow1s.Sec,TraiRevRec.Lat1s,TraiRevRec.Long1s,TraiRevRec.KmM/1000,TraiRevRec.KmM%1000);
            qDebug() <<TrainRecStr;
            TrainRecStr="Speed:";
            for(i=0;i<TIME_SEND_DATA_SERVER;i++){
                //TrainRecStr=TrainRecStr+IntToStr(TraiRevRec.SpeedBuff[i])+",";
                TrainRecStr=TrainRecStr+QString::number(TraiRevRec.SpeedBuff[i])+" ";
            }
            qDebug() <<TrainRecStr;

            TrainRecStr="Presure:";
            for(i=0;i<TIME_SEND_DATA_SERVER;i++){
                //TrainRecStr=TrainRecStr+IntToStr(TraiRevRec.PresBuff[i])+",";
                TrainRecStr=TrainRecStr+QString::number(TraiRevRec.PresBuff[i])+" ";
            }
            qDebug() <<TrainRecStr;
            break;
        }

        break;
    case CMD_BIRDIR_ACK:
        break;
    case CMD_SYS_GET:
        qDebug() <<"CMD_SYS_GET";
        l=DecodeDataPack(Pck,len-3);
        memcpy(&HardDev,Pck,sizeof(DevInfor));
        qDebug() << QString(HardDev.NameDevice) + " " + QString(HardDev.Type);
        break;
    default:
        qDebug() <<"UnKnowCmd";
        break;
    }
}

//---------------------------------------------------------

void VehicleConnection::Sys7bInput(unsigned char data)
{
    static unsigned short Sys7bREvIdx=0;
    unsigned char crc;
    unsigned short i;
    if(Sys7bREvIdx<SYS7B_REV_BUFF_SIZE){
        if(data==255){//nhan duoc byte bao hieu ket thuc goi tin
            crc=Sys7bRevBuff[0];
            for(i=0;i<Sys7bREvIdx-1;i++){
                crc^=Sys7bRevBuff[i];
            }
            crc&=0x7F;
            if(crc==Sys7bRevBuff[Sys7bREvIdx-1]){
                Sys7bProcessRevPck(Sys7bRevBuff,Sys7bREvIdx);
            }else{
                qDebug() << "PackCrcError";
            }
        }else{
            if(data>=128){
                Sys7bREvIdx=0;
            }
        }
        Sys7bRevBuff[Sys7bREvIdx]=data;
        Sys7bREvIdx++;
    }else{
        Sys7bREvIdx=0;
    }
}
//----------------------------------------------------------



void VehicleConnection::slot_connectionTimer_timeout(){
    if(lastRecvTime.secsTo(QDateTime::currentDateTime())>30)
        tcpSocket->disconnectFromHost();
}

void VehicleConnection::slot_readyRead(){
    lastRecvTime = QDateTime::currentDateTime();
    qDebug()<< "VehicleConnection::slot_readyRead()";
    if(tcpSocket->canReadLine()){
        QByteArray input = tcpSocket->readAll();

        QString s="";
        for(int i=0; i < input.length(); i++) {
            Sys7bInput(input.at(i));
            s=s+ QString(input.at(i)) + "";
        }
        qDebug() << s;


        //        QSqlQuery query;
        //        if(connectionDatabase && connectionDatabase->execQuery(query, str)){
        //            qDebug()<<"query Size" << query.size();
        //            if(query.next())
        //                qDebug()<<"query Value" << query.value(0).toString();
        //        }
    }
}

void VehicleConnection::slot_socketDisconnected(){
    qDebug()<< "VehicleConnection::slot_socketDisconnected()";
    connectionTimer->stop();
    tcpSocket = NULL;
    exit(0);
}

void VehicleConnection::slot_socketError(QAbstractSocket::SocketError socketError){
    qDebug()<< "VehicleConnection::slot_socketError()";
}

void VehicleConnection::slot_socketDestroyed(){
    qDebug()<< "VehicleConnection::slot_socketDestroyed()";
}
void VehicleConnection::run()
{
    connect(tcpSocket, SIGNAL(readyRead()),this, SLOT(slot_readyRead()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(slot_socketDisconnected()));
    connect(tcpSocket, SIGNAL(disconnected()), tcpSocket, SLOT(deleteLater()));
    connect(tcpSocket, SIGNAL(destroyed()), this, SLOT(slot_socketDestroyed()));

    unsigned char PSBuff[10]={0x80};
    qDebug() << "i'm server";
    SendPck(PSBuff,1,CMD_SYS_GET,0);
    qDebug() << "what 's your name ?";

    connectionTimer = new QTimer(this);
    connect(connectionTimer, SIGNAL(timeout()), this, SLOT(slot_connectionTimer_timeout()));
    connectionTimer->start(1000);
    connectionDatabase = new CprTfcDatabase(qApp->applicationDirPath()+"/VehicleTracking.ini", "LocalDatabase","VehicleConnection");
    exec()  ;
    if(connectionDatabase){
        delete connectionDatabase;
        connectionDatabase = NULL;
    }
}


