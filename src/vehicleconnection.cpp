#include "vehicleconnection.h"
#include <stdio.h>
#include <string.h>
static const unsigned char bitmap[]={1,2,4,8,16,32,64,128};

typedef const char* railway;// ten tuyen
railway const name_way_flash[max_name_way] =
{
    " THONG NHAT       ",
    " HA NOI-LAO CAI   ",
    " HA NOI-HAI PHONG ",
    " HA NOI-DONG DANG ",
    " LAM THAO-YEN BAI ",
    " YEN VIEN-HA LONG ",
    " YEN BAI-XUAN GIAO"
};
railway const name_way[max_name_way] = {"TH-NH","HN-LC","HN-HP", "HN-DD", "LT-YB", "YV-HL", "YB-XG"};
railway const name_way_mac[max_name_way][max_name_mac] = { // tên mac tàu
                                                           {" SE1 " , " SE3 " , " SE5 " , " SE7 " , " TN1 " , " SE2 " , " SE4 " , " SE6 " , " SE8 " , " TN2 " , " NA1 " , " NA3 " , " VD31"  , " DH41"  , " NA2 " , " NA4 " , " VD32"  , " DH42"  , "THbn1"  , "THbn2"   , " TH1 " , " TH2 ",  " THb1"  , " THb2", "     ", "     ", "     ", "     "},// thong nhat
                                                           {" SP1 " , " SP3 " , " SP5 " , " LC1 " , " LC3 " , " LC5 " , " LC7 " , " YB1 " , " SP2 " , " SP4 " , " SP6 " , " LC2 " , " LC4 "  , " LC6 "  , " LC8 " , " LC10" , " LC9 "  , " YB2 "  , " SP7 "  , " SP8 "   , " TH1 " , " TH2 ",  " THb1"  , " THb2", "     ", "     ", "     ", "     "},
                                                           {" HP1 " , " LP3 " , " LP5 " , " LP7 " , " HP2 " , " LP2 " , " LP6 " , " LP8 " , " TH1 " , " TH2 " , " THb1" , " THb2" , " THd1"  , " THd2"  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" M1  " , " M3  " , " DD3 " , " M2  " , " M4  " , " DD4 " , " TH1 " , " THb1" , " THb2" , " THd1" , " THd2" , "     " , "     "  , "     "  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" TH1 " , " TH2 " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     "  , "     "  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" R157" , " HLr1" , " R158" , " HLr2" , " TH1 " , " TH2 " , " THb1" , " THb2" , " THd1" , " THd2" , "     " , "     " , "     "  , "     "  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" TH1 " , " TH2 " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     "  , "     "  , "     " , "    "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                         };

VehicleConnection::VehicleConnection()
{
    connectionDatabase = NULL;
    state=0;
    plateNumber.clear();
    rawData.clear();
    trainId.clear();
}
VehicleConnection::VehicleConnection(QTcpSocket *socket){
    plateNumber.clear();
    rawData.clear();
    trainId.clear();
    connectionDatabase = NULL;
    moveToThread(this);
    tcpSocket = socket;
    tcpSocket->setParent(0);
    tcpSocket->moveToThread(this);
    lastRecvTime = QDateTime::currentDateTime();
    Sys7bREvIdx=0;
    TotalRevPCk=0;
}
VehicleConnection::~VehicleConnection(){
    qDebug()<<"VehicleConnection::~VehicleConnection()";
}


signed short VehicleConnection::EncodeDataPack(unsigned char inbuf[],unsigned short count)
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
unsigned short VehicleConnection::DecodeDataPack(unsigned char *in2outbuff,unsigned short count)
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
    //    qDebug() << "DecodeDataPack" << count << p_out_mp;
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
void VehicleConnection::Sys7bProcessRevPck(unsigned char* Pck,unsigned short len)
{
    unsigned char PBuff[256];
    unsigned char GpsStates,NumOfSat;
    int eventpck=0;
    unsigned char l;
    QString eTime, trainLabel;
    QDateTime gpsTime;
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

        //get train's data
        memcpy(&TraiRevRec,Pck+1,sizeof(TrainAbsRec));

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
        {
            QString sqlScanDevice = QString("SELECT * FROM tbl_train WHERE train_id = '%1'").arg(trainId);
            QSqlQuery query;
            connectionDatabase->execQuery(query, sqlScanDevice);
            if (!query.next()) {
                qDebug() << "unknow device........";
                break;
            }

            gpsTime.setDate(QDate(TraiRevRec.TimeNow1s.Year + 2000, TraiRevRec.TimeNow1s.Month, TraiRevRec.TimeNow1s.Day));
            gpsTime.setTime(QTime(TraiRevRec.TimeNow1s.Hour, TraiRevRec.TimeNow1s.Min, TraiRevRec.TimeNow1s.Sec, 0));

            GpsStates = TraiRevRec.GpsStatesNumOfSat>>7;
            NumOfSat = TraiRevRec.GpsStatesNumOfSat&0x7F;

            qDebug() << "TrainData" << gpsTime.toString("yyyy-MM-dd hh:mm:ss")
                     << "KmM" << TraiRevRec.KmM
                     << "Latitude" << TraiRevRec.Lat1s
                     << "Longitude" <<TraiRevRec.Long1s
                     << "train label" <<TraiRevRec.TrainLabel
                     << "train name" << TraiRevRec.TrainName
                     << "train height" << TraiRevRec.Height
                     << "gps State" << GpsStates
                     << "num of sat" << NumOfSat;

            //            QString TrainRecStr;
            //            unsigned char i;
            //            TrainRecStr="Speed:";
            //            for(i=0;i<TIME_SEND_DATA_SERVER;i++){
            //                TrainRecStr=TrainRecStr+QString::number(TraiRevRec.SpeedBuff[i])+" ";
            //            }
            //            qDebug() <<TrainRecStr;

            //            TrainRecStr="Presure:";
            //            for(i=0;i<TIME_SEND_DATA_SERVER;i++){
            //                TrainRecStr=TrainRecStr+QString::number(TraiRevRec.PresBuff[i])+" ";
            //            }
            //            qDebug() <<TrainRecStr;

            //insert train's data to tbl_trainlog
            QString sqlInsertTrainLog = QString("INSERT INTO tbl_trainlog ("
                                                "trainlog_trainid, "
                                                "trainlog_latitude, "
                                                "trainlog_longitude, "
                                                "trainlog_time, "
                                                "trainlog_lytrinh, "
                                                "trainlog_speed, "
                                                "trainlog_pressure, "
                                                "trainlog_lastupdate, "
                                                "trainlog_gpsstate, "
                                                "trainlog_numofsat, "
                                                "trainlog_height, "
                                                "trainlog_rawdata) "
                                                "VALUES ('%1', %2, %3, '%4', %5, %6, %7, '%8', %9, %10, '%11', '%12') ")
                    .arg(trainId)
                    .arg(TraiRevRec.Lat1s)
                    .arg(TraiRevRec.Long1s)
                    .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(TraiRevRec.KmM)
                    .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1]))
                    .arg(QString::number(TraiRevRec.PresBuff[TIME_SEND_DATA_SERVER - 1]))
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(GpsStates)
                    .arg(NumOfSat)
                    .arg(TraiRevRec.Height)
                    .arg(rawData);


            // truong hop chua bat may se gui ve 255
            if(TraiRevRec.TrainName == 255 || TraiRevRec.TrainLabel == 255){
                trainLabel = "NULL";
                qDebug() << ">>>>>>> chua bat may";
            }
            else{
                trainLabel = QString(name_way_mac[TraiRevRec.TrainName][TraiRevRec.TrainLabel]).trimmed();
            }
            QString lytrinhView = QString("Km %1 + %2").arg(TraiRevRec.KmM/1000).arg(TraiRevRec.KmM%1000);

            //insert or update into tbl_train
            QString sqlInsertOrUpdateTrain = QString("INSERT INTO tbl_train ("
                                                     "train_id, "
                                                     "train_label, "
                                                     "train_longitude, "
                                                     "train_latitude, "
                                                     "train_time, "
                                                     "train_speed, "
                                                     "train_pressure, "
                                                     "train_lytrinhview, "
                                                     "train_lytrinh, "
                                                     "train_gpsstate, "
                                                     "train_numofsat, "
                                                     "train_height, "
                                                     "train_lastupdate) "
                                                     "VALUES ('%1', '%2', %3, %4, '%5', %6, %7, '%8', %9, %10, %11, '%12', '%13') "
                                                     "ON DUPLICATE KEY UPDATE "
                                                     "train_label = '%2', "
                                                     "train_longitude = %3, "
                                                     "train_latitude = %4, "
                                                     "train_time = '%5', "
                                                     "train_speed = %6, "
                                                     "train_pressure = %7, "
                                                     "train_lytrinhview = '%8', "
                                                     "train_lytrinh = %9, "
                                                     "train_gpsstate = %10, "
                                                     "train_numofsat = %11, "
                                                     "train_height = '%12', "
                                                     "train_lastupdate = '%13'")
                    .arg(trainId)
                    .arg(trainLabel)
                    .arg(TraiRevRec.Long1s)
                    .arg(TraiRevRec.Lat1s)
                    .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1]))
                    .arg(QString::number(TraiRevRec.PresBuff[TIME_SEND_DATA_SERVER - 1]))
                    .arg(lytrinhView)
                    .arg(TraiRevRec.KmM)
                    .arg(GpsStates)
                    .arg(NumOfSat)
                    .arg(TraiRevRec.Height)
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

            if(connectionDatabase && connectionDatabase->startTransaction()){
                if(connectionDatabase->execQuery(sqlInsertTrainLog)
                        && connectionDatabase->execQuery(sqlInsertOrUpdateTrain)){
                    connectionDatabase->doCommit();
                    qDebug() << QString("%1 is updated to tbl_trainlog and tbl_train").arg(trainLabel);
                }
                else{
                    connectionDatabase->doRollback();
                    qDebug() << "can't not update or insert to database";
                }
            }
        }
            break;
        case REC_TRAIN_OVER_SPEED:
        {
            QString sqlScanDevice = QString("SELECT * FROM tbl_train WHERE train_id = '%1'").arg(trainId);
            QSqlQuery query;
            connectionDatabase->execQuery(query, sqlScanDevice);
            if (!query.next()) {
                qDebug() << "unknow device........";
                break;
            }

            memcpy(&EventSpeed,Pck+1,sizeof(Event));
            gpsTime.setDate(QDate(EventSpeed.GpsN.Gps.DateTime.Year + 2000, EventSpeed.GpsN.Gps.DateTime.Month, EventSpeed.GpsN.Gps.DateTime.Day));
            gpsTime.setTime(QTime(EventSpeed.GpsN.Gps.DateTime.Hour, EventSpeed.GpsN.Gps.DateTime.Min, EventSpeed.GpsN.Gps.DateTime.Sec, 0));

            qDebug() << "---over speed---";
            qDebug() << "lat:" << EventSpeed.GpsN.Gps.Lat << "long:" << EventSpeed.GpsN.Gps.Long << "speed:" << EventSpeed.GpsN.Gps.Speed << "time:" << gpsTime.toString("yyyy-MM-dd hh:mm:ss");
            qDebug() << "speed N limit:" << EventSpeed.SpeedNlm << "speed limit:" << EventSpeed.Speedlm;

            QString description = QString("Over speed \n lat:%1; long: %2; speed: %3; speed limit: %4")
                    .arg(EventSpeed.GpsN.Gps.Lat)
                    .arg(EventSpeed.GpsN.Gps.Long)
                    .arg(EventSpeed.GpsN.Gps.Speed)
                    .arg(EventSpeed.Speedlm);
            QString sqlInsertEvent = QString("INSERT INTO tbl_event ("
                                             "event_trainid, "
                                             "event_time, "
                                             "event_type, "
                                             "event_description, "
                                             "event_lastupdate) "
                                             "VALUES ('%1', '%2', %3, '%4', '%5')")
                    .arg(trainId)
                    .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(EVENT_TRAIN_OVER_SPEED)
                    .arg(description)
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

            if(connectionDatabase && connectionDatabase->execQuery(sqlInsertEvent)){
                qDebug() << QString("Train %1 event over speed is inserted").arg(trainId);
            }
        }
            break;
        case REC_TRAIN_CHANGE_SPEED_LIMIT:
        {
            QString sqlScanDevice = QString("SELECT * FROM tbl_train WHERE train_id = '%1'").arg(trainId);
            QSqlQuery query;
            connectionDatabase->execQuery(query, sqlScanDevice);
            if (!query.next()) {
                qDebug() << "unknow device........";
                break;
            }

            memcpy(&EventSpeed,Pck+1,sizeof(Event));
            gpsTime.setDate(QDate(EventSpeed.GpsN.Gps.DateTime.Year + 2000, EventSpeed.GpsN.Gps.DateTime.Month, EventSpeed.GpsN.Gps.DateTime.Day));
            gpsTime.setTime(QTime(EventSpeed.GpsN.Gps.DateTime.Hour, EventSpeed.GpsN.Gps.DateTime.Min, EventSpeed.GpsN.Gps.DateTime.Sec, 0));

            qDebug() << "---change speed limit---";
            qDebug() << "lat:" << EventSpeed.GpsN.Gps.Lat << "long:" << EventSpeed.GpsN.Gps.Long << "speed:" << EventSpeed.GpsN.Gps.Speed << "time:" << gpsTime.toString("yyyy-MM-dd hh:mm:ss");
            qDebug() << "speed N limit:" << EventSpeed.SpeedNlm << "speed limit:" << EventSpeed.Speedlm;

            QString description = QString("Change speed limit \n lat:%1; long: %2; current speed limit: %3; next speed limit: %4")
                    .arg(EventSpeed.GpsN.Gps.Lat)
                    .arg(EventSpeed.GpsN.Gps.Long)
                    .arg(EventSpeed.Speedlm)
                    .arg(EventSpeed.SpeedNlm);
            QString sqlInsertEvent = QString("INSERT INTO tbl_event ("
                                             "event_trainid, "
                                             "event_time, "
                                             "event_type, "
                                             "event_description, "
                                             "event_lastupdate) "
                                             "VALUES ('%1', '%2', %3, '%4', '%5')")
                    .arg(trainId)
                    .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(EVENT_TRAIN_CHANGE_SPEED_LIMIT)
                    .arg(description)
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

            if(connectionDatabase && connectionDatabase->execQuery(sqlInsertEvent)){
                qDebug() << QString("Train %1 event change speed limit is inserted").arg(trainId);
            }
        }
            break;
        }
        break;
    case CMD_BIRDIR_ACK:
        break;
    case CMD_SYS_GET:
    {
        qDebug() <<"CMD_SYS_GET";
        l=DecodeDataPack(Pck,len-3);
        memcpy(&HardDev,Pck,sizeof(DevInfor));
        qDebug() << QString(HardDev.NameDevice) + " " + QString(HardDev.Type);
        trainId = QString(HardDev.NameDevice);

        QString sqlScanDevice = QString("SELECT * FROM tbl_train WHERE train_id = '%1'").arg(trainId);
        QSqlQuery query;
        connectionDatabase->execQuery(query, sqlScanDevice);
        if (!query.next()) {
            tcpSocket ->disconnectFromHost();
            qDebug() << "unknow device...disconected";
        }
    }
        break;
    default:
        qDebug() <<"UnKnowCmd";
        break;
    }
}

//---------------------------------------------------------

void VehicleConnection::Sys7bInput(unsigned char data)
{
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
    if(lastRecvTime.secsTo(QDateTime::currentDateTime())>64){
        tcpSocket->disconnectFromHost();
        qDebug() << "VehicleConnection::slot_connectionTimer_timeout() tcpSocket->disconnectFromHost();";
    }
}

void VehicleConnection::slot_readyRead(){
    lastRecvTime = QDateTime::currentDateTime();
    qDebug()<< "VehicleConnection::slot_readyRead()";
    QByteArray input = tcpSocket->readAll();
    for(int i=0; i < input.length(); i++) {
        Sys7bInput((unsigned char)input.at(i));
    }
    rawData = input.toHex();
    //    qDebug() << "RecvData" << input.size() << rawData;
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

void VehicleConnection::slot_requestInfoTimer(){
    unsigned char PSBuff[10]={0x80};
    qDebug() << "15P <<<<------------------";
    SendPck(PSBuff,1,CMD_SYS_GET,0);
    qDebug() << "what 's your name ?";
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

    //hoi ten sau 15p
    requestInfoTimer = new QTimer(this);
    connect(requestInfoTimer, SIGNAL(timeout()), this, SLOT(slot_requestInfoTimer()));
    requestInfoTimer->start(900000);

    connectionDatabase = new CprTfcDatabase(qApp->applicationDirPath()+"/VehicleTracking.ini", "LocalDatabase","VehicleConnection");
    exec()  ;
    if(connectionDatabase){
        delete connectionDatabase;
        connectionDatabase = NULL;
    }
}


