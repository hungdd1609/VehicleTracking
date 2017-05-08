#include "vehicleconnection.h"
#include <stdio.h>
#include <string.h>
#include <QTextCodec>
#define REQ_LIST_LOG_TIMEOUT 20
#define READ_BLOCK 512
#define READ_BLOCK_TIME_OUT 60
#define READ_BLOCK_ZERO_THRESHOLD 5
static const unsigned char bitmap[]={1,2,4,8,16,32,64,128};
#define LOG_PATH "/var/www/html/CadProVTS/download/"

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
                                                           {" SP1 " , " SP3 " , " SP5 " , " LC1 " , " LC3 " , " LC5 " , " LC7 " , " YB1 " , " SP2 " , " SP4 " , " SP6 " , " LC2 " , " LC4 "  , " LC6 "  , " LC8 " , " LC10" , " LC9 "  , " YB2 "  , " SP7 "  , " SP8 "   , " TH1 " , " TH2 ",  " THb1"  , " THb2", " THd1", " THd2", "     ", "     "},
                                                           {" HP1 " , " LP3 " , " LP5 " , " LP7 " , " HP2 " , " LP2 " , " LP6 " , " LP8 " , " TH1 " , " TH2 " , " THb1" , " THb2" , " THd1"  , " THd2"  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" M1  " , " M3  " , " DD3 " , " M2  " , " M4  " , " DD4 " , " TH1 " , " THb1" , " THb2" , " THd1" , " THd2" , "     " , "     "  , "     "  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" TH1 " , " TH2 " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     " , "     "  , "     "  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" R157" , " HLr1" , " R158" , " HLr2" , " TH1 " , " TH2 " , " THb1" , " THb2" , " THd1" , " THd2" , "     " , "     " , "     "  , "     "  , "     " ,"     "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                           {" TH1 " , " TH2 " , " THb1" , " THb2" , " THd1" , " THd2" , "     " , "     " , "     " , "     " , "     " , "     " , "     "  , "     "  , "     " , "    "  , "     "  , "     "  , "     "  , "     "   , "     " , "     " , "     "  , "     ", "     ", "     ", "     ", "     "},
                                                         };

VehicleConnection::VehicleConnection()
{
    connectionDatabase = NULL;
    state=0;
    plateNumber.clear();
    rawData.clear();
}
VehicleConnection::VehicleConnection(QTcpSocket *socket){
    plateNumber.clear();
    rawData.clear();
    connectionDatabase = NULL;
    moveToThread(this);
    tcpSocket = socket;
    tcpSocket->setParent(0);
    tcpSocket->moveToThread(this);
    lastRecvTime = QDateTime::currentDateTime();
    Sys7bREvIdx=0;
    TotalRevPCk=0;
    lastReqLog = QDateTime::currentDateTime();
    dateReqLog = QDateTime::currentDateTime().addDays(-60);
    isReqLog = false;
    isFirstTime = true;
    readFileMng.Id = 0;
    readFileMng.Status = LOG_FILE_DONE;
    readFileMng.zeroCount = 0;
}
VehicleConnection::~VehicleConnection(){
    qDebug()<<"VehicleConnection::~VehicleConnection()";
}

float VehicleConnection::ConvertGPSdegreeToGPSDecimal(long Valua){
    unsigned char degree;
    float min;
    degree = Valua/1000000;
    min = (Valua - degree*1000000)/10000.0;
    min = degree + min/60.0;
    return min;
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
    //        qDebug() << plateNumber << "DecodeDataPack" << count << p_out_mp;
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
    qDebug() << plateNumber << s;
    tcpSocket->write((const char *)PayLoad,PayLoadLen+4);
}


//--------------------------------------------------------------------------------
void VehicleConnection::Sys7bProcessRevPck(unsigned char* Pck,unsigned short len)
{
    unsigned char PBuff[256];
    unsigned char GpsStates,NumOfSat;
    unsigned char l;
    QString eTime, vehicleLabel, TrainRecStr;
    QString longitude, latitude;
    QDateTime gpsTime, lastTime, current;
    PBuff[0]=128;
    TotalRevPCk++;

    switch(Pck[len-2]){
    case CMD_DEBUG:
        qDebug() << plateNumber <<"CMD_DEBUG";
        break;
    case CMD_DEV_EVENT:
        SendPck(PBuff,1,CMD_BIRDIR_ACK,Pck[len-3]);
        l=DecodeDataPack(Pck,len-3);
        current = QDateTime::currentDateTime();
        switch(Pck[0]&0x3F){
        case REC_GPS_ABS://Ban ghi Gps tuyet doi
            qDebug() << plateNumber <<"Ban ghi tuyet doi";
            break;
        case REC_GPS_DIF://Ban ghi GPS tuong doi
            qDebug() << plateNumber <<"Ban ghi tuong doi";
            break;
        case REC_USER_SIGNIN://Ban ghi su kien nguoi dang nhap
            memcpy(&Usr,Pck,l);
            memcpy(&Usr,Pck+1,l);
            eTime.sprintf("%d/%d/%d %d:%d:%d",Usr.GPS.Gps.DateTime.Day,Usr.GPS.Gps.DateTime.Month,Usr.GPS.Gps.DateTime.Year,Usr.GPS.Gps.DateTime.Hour,Usr.GPS.Gps.DateTime.Min,Usr.GPS.Gps.DateTime.Sec);
            //                              qDebug() << plateNumber <<eTime+"->"+QString(Usr.Hoten)+" "+QString(Usr.Giaypheplaixe)+" Singin");
            break;
        case REC_USER_SIGNOUT://Ban ghi su kien nguoi dang xuat
            memcpy(&Usr,Pck+1,l);
            eTime.sprintf("%d/%d/%d %d:%d:%d",Usr.GPS.Gps.DateTime.Day,Usr.GPS.Gps.DateTime.Month,Usr.GPS.Gps.DateTime.Year,Usr.GPS.Gps.DateTime.Hour,Usr.GPS.Gps.DateTime.Min,Usr.GPS.Gps.DateTime.Sec);
            break;
        case REC_USER_OVERTIME://Qua thoi gian
            qDebug() << plateNumber <<"Qua thoi gian";
            break;
        case REC_VEHICLE_STOP:// dung
            qDebug() << plateNumber <<"Dung xe";
            break;
        case REC_VEHICLE_RUN:
            qDebug() << plateNumber <<"Do xe";
            break;
        case REC_DIRVER_OVER_DAY:
            qDebug() << plateNumber <<"Lai qua ngay";
            break;
        case REC_DIRVER_OVER_SPEED:
            qDebug() << plateNumber <<"Qua van toc";
            break;
        case REC_TRAIN:
        {
            if(plateNumber.isEmpty() || plateNumber.isEmpty()){
                qDebug() << plateNumber << "break! plate number is empty";
                break;
            }

            QString sqlScanDevice = QString("SELECT * FROM tbl_phuongtien WHERE phuongtien_bienso = '%1'").arg(plateNumber);
            QSqlQuery query;
            connectionDatabase->execQuery(query, sqlScanDevice);
            if (query.next()) {
                lastTime = query.value(9).toDateTime();
            }

            //get train's data
            memcpy(&TraiRevRec,Pck+1,sizeof(TrainAbsRec));
            QByteArray trainRev((char*)Pck+1,sizeof(TrainAbsRec));
            QString cData = trainRev.toHex();


            longitude = QString::number(ConvertGPSdegreeToGPSDecimal(TraiRevRec.Long1s),'f',9);
            latitude = QString::number(ConvertGPSdegreeToGPSDecimal(TraiRevRec.Lat1s),'f',9);

            gpsTime.setDate(QDate(TraiRevRec.TimeNow1s.Year + 2000, TraiRevRec.TimeNow1s.Month, TraiRevRec.TimeNow1s.Day));
            gpsTime.setTime(QTime(TraiRevRec.TimeNow1s.Hour, TraiRevRec.TimeNow1s.Min, TraiRevRec.TimeNow1s.Sec, 0));

            GpsStates = TraiRevRec.GpsStatesNumOfSat>>7;
            NumOfSat = TraiRevRec.GpsStatesNumOfSat&0x7F;

            // truong hop chua bat may se gui ve 255
            if(TraiRevRec.TrainName == 255 || TraiRevRec.TrainLabel == 255){
                qDebug() << plateNumber << "TraiRevRec.TrainName == 255 || TraiRevRec.TrainLabel == 255";
                break;
            }
            else{
                vehicleLabel = QString(name_way_mac[TraiRevRec.TrainName][TraiRevRec.TrainLabel]).trimmed();
            }

            qDebug() << plateNumber << "TrainData" << gpsTime.toString("yyyy-MM-dd hh:mm:ss")
                     << "Train id" << plateNumber
                     << "label" << vehicleLabel << TraiRevRec.TrainName << TraiRevRec.TrainLabel
                     << "KmM" << TraiRevRec.KmM
                     << "Heading" << TraiRevRec.Heading
                     << "Latitude" << TraiRevRec.Lat1s
                     << "Longitude" <<TraiRevRec.Long1s
                     << "train label" <<TraiRevRec.TrainLabel
                     << "train name" << TraiRevRec.TrainName
                     << "train height" << TraiRevRec.Height
                     << "gps State" << GpsStates
                     << "num of sat" << NumOfSat;

            //insert train's data to tbl_phuongtienlog
            if(Insert2PhuongTienLog(vehicleLabel, gpsTime, longitude, latitude, GpsStates, cData, "tbl_phuongtienlog")) {
                qDebug() << plateNumber << QString("%1 is inserted to tbl_phuongtienlog").arg(vehicleLabel);
            }

            //insert or update into tbl_train
            qDebug() << plateNumber << "1---" << lastTime << "2---" << current;
            if(lastTime.secsTo(gpsTime) < 0
                    || !(lastTime.secsTo(current) > PERIOD_TIME)
                    || lastTime.secsTo(current) < 0){
                qDebug() << plateNumber << "thoi gian khong hop le";
                break;
            } else if(Insert2PhuongTien(gpsTime,vehicleLabel, longitude, latitude, GpsStates, cData)){
                qDebug() << plateNumber << QString("%1 is inserted to tbl_phuongtien").arg(vehicleLabel);
            }
        }
            break;
        case REC_TRAIN_OVER_SPEED:
        {
            QString sqlScanDevice = QString("SELECT * FROM tbl_phuongtien WHERE phuongtien_bienso = '%1'").arg(plateNumber);
            QSqlQuery query;
            connectionDatabase->execQuery(query, sqlScanDevice);
            if (!query.next()) {
                qDebug() << plateNumber << "unknow vehicle........";
                break;
            }

            memcpy(&EventSpeed,Pck+1,sizeof(Event));
            gpsTime.setDate(QDate(EventSpeed.GpsN.Gps.DateTime.Year + 2000, EventSpeed.GpsN.Gps.DateTime.Month, EventSpeed.GpsN.Gps.DateTime.Day));
            gpsTime.setTime(QTime(EventSpeed.GpsN.Gps.DateTime.Hour, EventSpeed.GpsN.Gps.DateTime.Min, EventSpeed.GpsN.Gps.DateTime.Sec, 0));

            qDebug() << plateNumber << "---over speed---";
            qDebug() << plateNumber << "lat:" << EventSpeed.GpsN.Gps.Lat << "long:" << EventSpeed.GpsN.Gps.Long << "speed:" << EventSpeed.GpsN.Gps.Speed << "time:" << gpsTime.toString("yyyy-MM-dd hh:mm:ss");
            qDebug() << plateNumber << "speed N limit:" << EventSpeed.SpeedNlm << "speed limit:" << EventSpeed.Speedlm;

            QString description = QString("Over speed \n lat:%1; long: %2; speed: %3; speed limit: %4")
                    .arg(EventSpeed.GpsN.Gps.Lat)
                    .arg(EventSpeed.GpsN.Gps.Long)
                    .arg(EventSpeed.GpsN.Gps.Speed)
                    .arg(EventSpeed.Speedlm);
            QString sqlInsertEvent = QString("INSERT INTO tbl_event ("
                                             "event_bienso, "
                                             "event_time, "
                                             "event_type, "
                                             "event_description, "
                                             "event_lastupdate) "
                                             "VALUES ('%1', '%2', %3, '%4', '%5')")
                    .arg(plateNumber)
                    .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(EVENT_TRAIN_OVER_SPEED)
                    .arg(description)
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

            if(connectionDatabase && connectionDatabase->execQuery(sqlInsertEvent)){
                qDebug() << plateNumber << QString("vehicle %1 event over speed is inserted").arg(plateNumber);
            }
        }
            break;
        case REC_TRAIN_CHANGE_SPEED_LIMIT:
        {
            QString sqlScanDevice = QString("SELECT * FROM tbl_phuongtien WHERE phuongtien_bienso = '%1'").arg(plateNumber);
            QSqlQuery query;
            connectionDatabase->execQuery(query, sqlScanDevice);
            if (!query.next()) {
                qDebug() << plateNumber << "unknow vehicle........";
                break;
            }

            memcpy(&EventSpeed,Pck+1,sizeof(Event));
            gpsTime.setDate(QDate(EventSpeed.GpsN.Gps.DateTime.Year + 2000, EventSpeed.GpsN.Gps.DateTime.Month, EventSpeed.GpsN.Gps.DateTime.Day));
            gpsTime.setTime(QTime(EventSpeed.GpsN.Gps.DateTime.Hour, EventSpeed.GpsN.Gps.DateTime.Min, EventSpeed.GpsN.Gps.DateTime.Sec, 0));

            qDebug() << plateNumber << "---change speed limit---";
            qDebug() << plateNumber << "lat:" << EventSpeed.GpsN.Gps.Lat << "long:" << EventSpeed.GpsN.Gps.Long << "speed:" << EventSpeed.GpsN.Gps.Speed << "time:" << gpsTime.toString("yyyy-MM-dd hh:mm:ss");
            qDebug() << plateNumber << "speed N limit:" << EventSpeed.SpeedNlm << "speed limit:" << EventSpeed.Speedlm;

            QString description = QString("Change speed limit \n lat:%1; long: %2; current speed limit: %3; next speed limit: %4")
                    .arg(EventSpeed.GpsN.Gps.Lat)
                    .arg(EventSpeed.GpsN.Gps.Long)
                    .arg(EventSpeed.Speedlm)
                    .arg(EventSpeed.SpeedNlm);
            QString sqlInsertEvent = QString("INSERT INTO tbl_event ("
                                             "event_bienso, "
                                             "event_time, "
                                             "event_type, "
                                             "event_description, "
                                             "event_lastupdate) "
                                             "VALUES ('%1', '%2', %3, '%4', '%5')")
                    .arg(plateNumber)
                    .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
                    .arg(EVENT_TRAIN_CHANGE_SPEED_LIMIT)
                    .arg(description)
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

            if(connectionDatabase && connectionDatabase->execQuery(sqlInsertEvent)){
                qDebug() << plateNumber << QString("vehicle %1 event change speed limit is inserted").arg(plateNumber);
            }
        }
            break;
        }
        break;
    case CMD_BIRDIR_ACK:
        break;
    case CMD_SYS_GET:
        qDebug() << plateNumber <<"CMD_SYS_GET";
        l=DecodeDataPack(Pck,len-3);
        memcpy(&HardDev,Pck,sizeof(DevInfor));
        qDebug() << plateNumber << QString(HardDev.NameDevice) + " " + QString(HardDev.Type);
        plateNumber = QString(HardDev.NameDevice).trimmed();
        break;
    case CMD_FILE_MNG:
        qDebug() << plateNumber << "CMD_FILE_MNG RETURN";
        l=DecodeDataPack(Pck,len-3);
        switch(Pck[len-3]&0x3F){
        case CmdGprsFMngCreateF://trả về của lệnh tao file, nếu thành công
            qDebug() << plateNumber << "CmdGprsFMngCreateF";
            break;
        case CmdGprsFMngWriteF://trả về của lệnh ghi file, có độ dài của file đã được ghi.
            qDebug() << plateNumber << "CmdGprsFMngWriteF";
            //            RfMng.WaitFlg=0;
            //            memcpy(&tmp,Pck+1,4);
            //            memcpy(&RfMng.LastWritePos,Pck+1,4);
            //            as.sprintf("%d",tmp);
            //            qDebug() << "EndPosOfFile="+as);

            break;
        case CmdGprsFMngReadF://trả về của lệnh đọc file
        {
            memcpy(&lr,Pck+1,2);
            qDebug() << plateNumber << "CmdGprsFMngReadF Len="+ QString::number(lr) << "+++++++++++++++++++++";
            QByteArray buf((char*)Pck+3,lr);

            qDebug() << plateNumber << "CmdGprsFMngReadF" << "copy done!";
            //ghi xuong file
            if(lr > 0) {
                qDebug() << plateNumber << "write to " << readFileMng.Path;
                readFileMng.Block += lr;

                pw = fopen(readFileMng.Path.toStdString().c_str(),"a+");
                if(pw != NULL){
                    fwrite(buf, sizeof(unsigned char), lr, pw);
                    fclose(pw);
                }
                qDebug() << plateNumber << "write to " << readFileMng.Name << lr << "bytes";
            } else {
                readFileMng.zeroCount += 1;
                if (readFileMng.zeroCount > READ_BLOCK_ZERO_THRESHOLD) {
                    qDebug() << plateNumber << "Can't read file: " << readFileMng.Name << readFileMng.Size << readFileMng.Block;
                    readFileMng.Status = LOG_FILE_ERROR;
                    readFileMng.EndRead = QDateTime::currentDateTime();
                    if(UpdateLogFile(readFileMng.Status,
                                     readFileMng.StartRead,
                                     readFileMng.EndRead,
                                     readFileMng.Size,
                                     readFileMng.Block,
                                     readFileMng.Path,
                                     readFileMng.Id)) {
                        qDebug() << plateNumber << "File log updated!" << readFileMng.Id ;
                    }

                    readFileMng.zeroCount = 0;
                    break;
                }
            }

            //lr < READ_BLOCK ||
            if(readFileMng.Block >= readFileMng.Size) {
                //update DB ket thuc file
                qDebug() << plateNumber << "End file:" << readFileMng.Name;

                if (readFileMng.Block == readFileMng.Size) {
                    qDebug() << plateNumber << "Read file done: " << readFileMng.Name << readFileMng.Size << readFileMng.Block;
                    readFileMng.Status = LOG_FILE_DONE;
                } else {
                    qDebug() << plateNumber << "Read file error: " << readFileMng.Name << readFileMng.Size << readFileMng.Block;
                    readFileMng.Status = LOG_FILE_ERROR;
                }
            }

            readFileMng.EndRead = QDateTime::currentDateTime();
            if(UpdateLogFile(readFileMng.Status,
                             readFileMng.StartRead,
                             readFileMng.EndRead,
                             readFileMng.Size,
                             readFileMng.Block,
                             readFileMng.Path,
                             readFileMng.Id)) {
                qDebug() << plateNumber << "File log updated!" << readFileMng.Id ;
            }

            if(readFileMng.Block < readFileMng.Size) {
                // doc tiep
                qDebug() << plateNumber << "Read continue " << readFileMng.Name << readFileMng.Block;
                GprsFmngReadBlk(readFileMng.Name, readFileMng.Block, READ_BLOCK);
            } else {
                readFileMng.Id = 0;
            }

        }
            break;

        case CmdGprsFMngChangeFileName:
            qDebug() << plateNumber << "CmdGprsFMngChangeFileName";
            break;
        case CmdGprsGetFileLen://lấy độ dài của file
            u_int32_t tmp;
            memcpy(&tmp,Pck+1,4);
            readFileMng.Size = tmp;
            qDebug() << plateNumber << "CmdGprsGetFileLen=" << readFileMng.Size;

            if(readFileMng.Status == LOG_FILE_SIZE_READING) {
                //-/insert to DB file size
                QString sqlUpdateFileSize =
                        QString(" UPDATE tbl_filelog "
                                " SET filelog_size = %1, filelog_trangthai = %2 "
                                " WHERE filelog_id = %3")
                        .arg(QString::number(readFileMng.Size))
                        .arg(LOG_FILE_SIZE_READ)
                        .arg(QString::number(readFileMng.Id));

                if(connectionDatabase && connectionDatabase->execQuery(sqlUpdateFileSize)) {
                    readFileMng.Status = LOG_FILE_SIZE_READ;
                } else {

                }
            }

            break;

        case CmdGprsFileDelete://xóa file thành công
            qDebug() << plateNumber << "CmdGprsFileDelete";
            break;

        case CmdGprsGetFileListByTime:
            qDebug() << plateNumber << "CmdGprsGetFileListByTime" << l;
            filelenpos=0;

            while((filelenpos<512)&&(filelenpos+1<l)){
                if(Pck[filelenpos+1]!=0){
                    QString tmpfName= QString((char*)Pck+1+filelenpos);
                    //-/insert vao bang ten log
                    QDateTime logTime = QDateTime::fromString(tmpfName.left(tmpfName.lastIndexOf(".")), "yyMMddhhmm").addYears(100);
                    qDebug() << plateNumber << tmpfName << logTime.toString("yyyy-MM-dd hh:mm:ss");

                    QString sqlcheck =
                            QString(" SELECT * from tbl_filelog "
                                    " WHERE filelog_bienso = '%1' AND filelog_thoigian = '%2'")
                            .arg(plateNumber)
                            .arg(logTime.toString("yyyy-MM-dd hh:mm:ss"));
                    QSqlQuery queryCheck;
                    if(connectionDatabase && connectionDatabase->execQuery(sqlcheck,queryCheck)) {

                         qDebug() << plateNumber << "queryCheck.size()" << queryCheck.size();
                        if(queryCheck.size() == 0) {
                            QString sqlInsertFileLog =
                                    QString(" INSERT INTO tbl_filelog(filelog_bienso, filelog_thoigian, filelog_ten) "
                                            " VALUES ('%1', '%2', '%3') "
                                            " ON DUPLICATE KEY UPDATE "
                                            " filelog_ten = '%3' ")
                                    .arg(plateNumber)
                                    .arg(logTime.toString("yyyy-MM-dd hh:mm:ss"))
                                    .arg(tmpfName);
                            if(connectionDatabase && connectionDatabase->execQuery(sqlInsertFileLog)){

                            } else {
                                qDebug() << plateNumber << tmpfName << "insert to DB err!!";
                            }
                        }
                    }

                    filelenpos+=16;
                }else{
                    break;
                }
            }

            dateReqLog = dateReqLog.addDays(2);
            if(dateReqLog.daysTo(QDateTime::currentDateTime()) < 0){
                dateReqLog = QDateTime::currentDateTime();
            }
            isReqLog = true;

            break;

        default:
            break;
        }
        break;
    case CMD_L7B_MNG:
        l=DecodeDataPack(Pck,len-3);
        switch(Pck[len-3]&0x3F){
        case L7B_CMD_GET_LOG:
            qDebug() << "CMD_SYS_L7MNG GET LOG RETURN";
            switch(Pck[0]&0x3F){
            case REC_GPS_ABS://Ban ghi Gps tuyet doi
                qDebug() << "Ban ghi tuyet doi";
                break;
            case REC_GPS_DIF://Ban ghi GPS tuong doi
                qDebug() << "Ban ghi tuong doi";
                break;
            case REC_USER_SIGNIN://Ban ghi su kien nguoi dang nhap
                memcpy(&Usr,Pck,l);
                memcpy(&Usr,Pck+1,l);
                eTime.sprintf("%d/%d/%d %d:%d:%d",Usr.GPS.Gps.DateTime.Day,Usr.GPS.Gps.DateTime.Month,Usr.GPS.Gps.DateTime.Year,Usr.GPS.Gps.DateTime.Hour,Usr.GPS.Gps.DateTime.Min,Usr.GPS.Gps.DateTime.Sec);
                qDebug() << eTime+"->"+ QString(Usr.Hoten)+" "+QString(Usr.Giaypheplaixe)+" Singin";
                break;
            case REC_USER_SIGNOUT://Ban ghi su kien nguoi dang xuat
                memcpy(&Usr,Pck+1,l);
                eTime.sprintf("%d/%d/%d %d:%d:%d",Usr.GPS.Gps.DateTime.Day,Usr.GPS.Gps.DateTime.Month,Usr.GPS.Gps.DateTime.Year,Usr.GPS.Gps.DateTime.Hour,Usr.GPS.Gps.DateTime.Min,Usr.GPS.Gps.DateTime.Sec);
                qDebug() << eTime+"->"+QString(Usr.Hoten)+" "+QString(Usr.Giaypheplaixe)+" Singout";
                break;
            case REC_USER_OVERTIME://Qua thoi gian
                qDebug() << "L7B_CMD_GET_LOG>Qua thoi gian";
                break;
            case REC_VEHICLE_STOP:// dung
                qDebug() << "L7B_CMD_GET_LOG>Dung xe";
                break;
            case REC_VEHICLE_RUN:
                qDebug() << "L7B_CMD_GET_LOG>Do xe";
                break;
            case REC_DIRVER_OVER_DAY:
                qDebug() << "L7B_CMD_GET_LOG>Lai qua ngay";
                break;
            case REC_DIRVER_OVER_SPEED:
                qDebug() << "L7B_CMD_GET_LOG>Qua van toc";
                break;
            case REC_TRAIN:
                qDebug() << "L7B_CMD_GET_LOG>Ban ghi tau hoa";

                memcpy(&TraiRevRec,Pck+1,sizeof(TrainAbsRec));
                QByteArray trainRev((char*)Pck+1,sizeof(TrainAbsRec));
                QString cData = trainRev.toHex();

                TrainRecStr.sprintf("Train:%d ->%d/%d/%d %d:%d:%d Lat %ld Long %ld Km %d M%d Height%d Gps%d VLim:%d Heading:%d",TraiRevRec.TrainLabel,TraiRevRec.TimeNow1s.Day,TraiRevRec.TimeNow1s.Month,TraiRevRec.TimeNow1s.Year,TraiRevRec.TimeNow1s.Hour,TraiRevRec.TimeNow1s.Min,TraiRevRec.TimeNow1s.Sec,TraiRevRec.Lat1s,TraiRevRec.Long1s,TraiRevRec.KmM/1000,TraiRevRec.KmM%1000,TraiRevRec.Height,TraiRevRec.GpsStatesNumOfSat,TraiRevRec.LimitSpeed,TraiRevRec.Heading);
                qDebug() << plateNumber << TrainRecStr;

                if(TraiRevRec.TrainName == 255 || TraiRevRec.TrainLabel == 255){
                    qDebug() << plateNumber << "TraiRevRec.TrainName == 255 || TraiRevRec.TrainLabel == 255";
                    break;
                }
                else{
                    vehicleLabel = QString(name_way_mac[TraiRevRec.TrainName][TraiRevRec.TrainLabel]).trimmed();
                }

                //insert train's data to tbl_phuongtienlog
                longitude = QString::number(ConvertGPSdegreeToGPSDecimal(TraiRevRec.Long1s),'f',9);
                latitude = QString::number(ConvertGPSdegreeToGPSDecimal(TraiRevRec.Lat1s),'f',9);

                gpsTime.setDate(QDate(TraiRevRec.TimeNow1s.Year + 2000, TraiRevRec.TimeNow1s.Month, TraiRevRec.TimeNow1s.Day));
                gpsTime.setTime(QTime(TraiRevRec.TimeNow1s.Hour, TraiRevRec.TimeNow1s.Min, TraiRevRec.TimeNow1s.Sec, 0));

                if(Insert2PhuongTienLog(vehicleLabel, gpsTime, longitude, latitude, GpsStates, cData, "tbl_phuongtienlog2")) {
                    qDebug() << plateNumber << QString("%1 is inserted to tbl_phuongtienlog").arg(vehicleLabel);
                }
                break;
            }
            break;
        case L7B_CMD_EARSE:
            qDebug() << "CMD_SYS_L7MNG EARSE RETURN";
            break;
        default:
            qDebug() << "CMD_SYS_L7MNG UNKNOW";
            break;
        }
        break;

    default:
        qDebug() << plateNumber <<"UnKnowCmd";
        break;
    }
}
//---------------------------------------------------------
bool VehicleConnection::Insert2PhuongTien(QDateTime gpsTime, QString vehicleLabel, QString longitude, QString latitude, unsigned char GpsStates, QString cData ){
    QString sqlInsertOrUpdateTrain = QString("INSERT INTO tbl_phuongtien ("
                                             "phuongtien_imei, "
                                             "phuongtien_loaiphuongtien, "
                                             "phuongtien_tochuc, "
                                             "phuongtien_bienso, "
                                             "phuongtien_laichinh, "
                                             "phuongtien_tuyenduong, "
                                             "phuongtien_machuyen, "
                                             "phuongtien_phulai, "
                                             "phuongtien_thoigian, "
                                             "phuongtien_kinhdo, "
                                             "phuongtien_vido, "
                                             "phuongtien_vantoc_gps, "
                                             "phuongtien_vantoc_banhxe, "
                                             "phuongtien_vantoc_dongho, "
                                             "phuongtien_vantoc_dongco, "
                                             "phuongtien_hanchetocdo, "
                                             "phuongtien_lytrinh, "
                                             "phuongtien_apsuat, "
                                             "phuongtien_huong, "
                                             "phuongtien_trangthaigps, "
                                             "phuongtien_extdata) "
                                             "VALUES (%1, %2, %3, '%4', %5, '%6', '%7', %8, '%9', %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, '%21') "
                                             "ON DUPLICATE KEY UPDATE "
                                             "phuongtien_imei = %1, "
                                             "phuongtien_loaiphuongtien = %2, "
                                             "phuongtien_laichinh = %5, "
                                             "phuongtien_tuyenduong = '%6', "
                                             "phuongtien_machuyen = '%7', "
                                             "phuongtien_phulai = %8, "
                                             "phuongtien_thoigian = '%9', "
                                             "phuongtien_kinhdo = %10, "
                                             "phuongtien_vido = %11, "
                                             "phuongtien_vantoc_gps = %12, "
                                             "phuongtien_vantoc_banhxe = %13, "
                                             "phuongtien_vantoc_dongho = %14, "
                                             "phuongtien_vantoc_dongco = %15, "
                                             "phuongtien_hanchetocdo = %16, "
                                             "phuongtien_lytrinh = %17, "
                                             "phuongtien_apsuat = %18, "
                                             "phuongtien_huong = %19, "
                                             "phuongtien_trangthaigps = %20, "
                                             "phuongtien_extdata = '%21'")
            .arg("' '") //imei
            .arg(TYPE_TRAIN)
            .arg(1) //to chuc
            .arg(plateNumber) //bien so
            .arg(1) //lai chinh
            .arg(TraiRevRec.TrainName + 1) //tuyen duong
            .arg(vehicleLabel) //ma chuyen
            .arg(1) //phu lai
            .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(longitude)
            .arg(latitude)
            .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1])) //van toc gps
            .arg(QString::number(TraiRevRec.WheelSpeed[TIME_SEND_DATA_SERVER - 1])) //van toc banh xe
            .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1])) //van toc dong ho
            .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1])) //van toc dong co
            .arg(TraiRevRec.LimitSpeed) //gioi han toc do
            .arg(TraiRevRec.KmM) //ly trinh
            .arg(QString::number(TraiRevRec.PresBuff[TIME_SEND_DATA_SERVER - 1])) //ap suat
            .arg(QString::number(TraiRevRec.Heading)) //huong
            .arg(GpsStates)
            .arg(cData);

    if(connectionDatabase && connectionDatabase->execQuery(sqlInsertOrUpdateTrain)){
        return true;
    }
    return false;
}

//---------------------------------------------------------
bool VehicleConnection::Insert2PhuongTienLog(QString machuyen, QDateTime gpsTime, QString longitude, QString latitude, unsigned char GpsStates, QString  cData, QString table){
    QString sqlInsertTrainLog = QString("INSERT INTO "+ table + " ("
                                        "phuongtienlog_bienso, "
                                        "phuongtienlog_thoigian, "
                                        "phuongtienlog_kinhdo, "
                                        "phuongtienlog_vido, "
                                        "phuongtienlog_vantoc_gps, "
                                        "phuongtienlog_vantoc_banhxe, "
                                        "phuongtienlog_vantoc_dongho, "
                                        "phuongtienlog_vantoc_dongco, "
                                        "phuongtienlog_hanchetocdo, "
                                        "phuongtienlog_apsuat, "
                                        "phuongtienlog_lytrinh, "
                                        "phuongtienlog_huong, "
                                        "phuongtienlog_trangthaigps, "
                                        "phuongtienlog_extdata, "
                                        "phuongtienlog_machuyen ) "
                                        "VALUES ('%1', '%2', %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, '%14', '%15') ")
            .arg(plateNumber)
            .arg(gpsTime.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(longitude)
            .arg(latitude)
            .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1])) //van toc gps
            .arg(QString::number(TraiRevRec.WheelSpeed[TIME_SEND_DATA_SERVER - 1]))
            .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1])) //van toc dong ho
            .arg(QString::number(TraiRevRec.SpeedBuff[TIME_SEND_DATA_SERVER - 1]))
            .arg(TraiRevRec.LimitSpeed)
            .arg(QString::number(TraiRevRec.PresBuff[TIME_SEND_DATA_SERVER - 1]))
            .arg(TraiRevRec.KmM)
            .arg(QString::number(TraiRevRec.Heading)) //huong
            .arg(GpsStates)
            .arg(cData)
            .arg(machuyen);

    if(connectionDatabase->execQuery(sqlInsertTrainLog)){
        return true;
    }
    return false;
}

//---------------------------------------------------------

bool VehicleConnection::UpdateLogFile(int trangthai, QDateTime batdau, QDateTime ketthuc, int size, int block, QString duongdan, int id) {

    QString sqlFinishRead = QString(
                " UPDATE tbl_filelog "
                " SET filelog_trangthai = %1, "
                "   filelog_doc_batdau = '%2', "
                "   filelog_doc_ketthuc ='%3', "
                "   filelog_size = %4,"
                "   filelog_block = %5, "
                "   filelog_duongdan = '%6' "
                " WHERE  filelog_id = %7")
            .arg(QString::number(trangthai))
            .arg(batdau.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(ketthuc.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(QString::number(size))
            .arg(QString::number(block))
            .arg(duongdan)
            .arg(QString::number(id));

    if(connectionDatabase && connectionDatabase->execQuery(sqlFinishRead)) return true;

    return false;
}

bool VehicleConnection::UpdateCommand(int id, QDateTime sent, int status)
{
    //update tbl_command
    QString sqlUpdateCommand =
            QString(" UPDATE tbl_command "
                    " SET command_time_sent='%1', command_status=%2 "
                    " WHERE command_id = %3")
            .arg(sent.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(QString::number(status))
            .arg(QString::number(id));
    if(connectionDatabase && connectionDatabase->execQuery(sqlUpdateCommand)){
        return true;
    }
    return false;
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
                qDebug() << plateNumber << "PackCrcError";
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

SDateTime VehicleConnection::QDateTime2SDateTime(QDateTime input){
    SDateTime output;
    output.Year = input.toString("yy").toShort();
    output.Month = input.toString("MM").toShort();
    output.Day = input.toString("dd").toShort();
    output.Hour = input.toString("hh").toShort();
    output.Min = input.toString("mm").toShort();
    output.Sec = input.toString("ss").toShort();

    return output;
}

//----------------------------------------------------------

void VehicleConnection::GetLogByTime(QDateTime qStart,QDateTime qEnd){
    qDebug() << plateNumber << "GetLogByTime" << qStart.toString("yyyy-MM-dd hh:mm:ss") << "->" << qEnd.toString("yyyy-MM-dd hh:mm:ss") ;

    SDateTime Start = QDateTime2SDateTime(qStart);
    SDateTime End = QDateTime2SDateTime(qEnd);

    unsigned char PBuff[128]={L7B_CMD_GET_LOG};
    memcpy(PBuff+1,&Start,sizeof(Start));
    memcpy(PBuff+1+sizeof(Start),&End,sizeof(End));
    int l=EncodeDataPack(PBuff,17);

    SendPck(PBuff,l,CMD_L7B_MNG,0);

}

//----------------------------------------------------------

void VehicleConnection::GetListFileLogBytime(QDateTime qStart,QDateTime qEnd,QString qFilter){
    qDebug() << plateNumber << "GetListFileLogBytime" << qStart.toString("yyyy-MM-dd hh:mm:ss") << "->" << qEnd.toString("yyyy-MM-dd hh:mm:ss") << qFilter;

    SDateTime Start = QDateTime2SDateTime(qStart);
    SDateTime End = QDateTime2SDateTime(qEnd);

    //    qDebug() << plateNumber << Start.Day << Start.Month << Start.Year << Start.Hour << Start.Min << Start.Sec;
    //    qDebug() << plateNumber << End.Day << End.Month << End.Year << End.Hour << End.Min << End.Sec;


    unsigned char PBuff[128]={CmdGprsGetFileListByTime};
    memcpy(PBuff+1,&Start,sizeof(Start));
    memcpy(PBuff+1+sizeof(Start),&End,sizeof(End));
    strcpy((char*)PBuff+1+sizeof(Start)*2, qFilter.toStdString().c_str());

    int l=EncodeDataPack(PBuff,sizeof(Start)*2+5+qFilter.length());

    SendPck(PBuff,l,CMD_FILE_MNG,0);
    qDebug() << plateNumber << "hex" << QByteArray((char*)PBuff).toHex();

}

//----------------------------------------------------------

void VehicleConnection::GetFileSize(QString FileName){
    unsigned char PBuff[1024];
    memset(PBuff,0,1024);
    PBuff[0]=CmdGprsGetFileLen;
    strcpy((char*)PBuff+1,FileName.toStdString().c_str());
    int l=EncodeDataPack(PBuff,17);

    SendPck(PBuff,l,CMD_FILE_MNG,0);
}

//----------------------------------------------------------

void VehicleConnection::GprsFmngReadBlk(QString FileName, int StartAddr, int Len)
{    
    unsigned char PBuff[1024];
    memset(PBuff,0,1024);
    PBuff[0]=0x80|CmdGprsFMngReadF;
    strcpy((char *)PBuff+1,FileName.toStdString().c_str());
    memcpy(PBuff+17,&StartAddr,4);
    memcpy(PBuff+21,&Len,2);
    int l=EncodeDataPack(PBuff,25);
    SendPck(PBuff,l,CMD_FILE_MNG,0);
    qDebug() << plateNumber << "getblock";
}

//----------------------------------------------------------

void VehicleConnection::slot_connectionTimer_timeout(){
    if(lastRecvTime.secsTo(QDateTime::currentDateTime())>64){
        tcpSocket->disconnectFromHost();
        qDebug() << plateNumber << "VehicleConnection::slot_connectionTimer_timeout() tcpSocket->disconnectFromHost();";
    }

    //-/ask name
    unsigned char PSBuff[10]={0x80};
    qDebug() << plateNumber << "15P <<<<------------------";
    SendPck(PSBuff,1,CMD_SYS_GET,0);
    qDebug() << plateNumber << "what 's your name ?";

    //-/ask list log file
    if(isFirstTime) {
        GetListFileLogBytime(QDateTime::currentDateTime().addDays(-1), QDateTime::currentDateTime(), ".log");
        isFirstTime = false;
        lastReqLog = QDateTime::currentDateTime();
    } else {
        if (isReqLog || lastReqLog.secsTo(QDateTime::currentDateTime())>REQ_LIST_LOG_TIMEOUT){
            if((dateReqLog.daysTo(QDateTime::currentDateTime()) > 0) ||
                    (dateReqLog.daysTo(QDateTime::currentDateTime()) == 0 && lastReqLog.secsTo(QDateTime::currentDateTime())>REQ_LIST_LOG_TIMEOUT)) {
                GetListFileLogBytime(dateReqLog, dateReqLog.addDays(1), ".log");
                isReqLog = false;
                lastReqLog = QDateTime::currentDateTime();
            }
        }
    }
}

void VehicleConnection::slot_readyRead(){
    lastRecvTime = QDateTime::currentDateTime();
    qDebug()<< "VehicleConnection::slot_readyRead()";
    QByteArray input = tcpSocket->readAll();

    for(int i=0; i < input.length(); i++) {
        Sys7bInput((unsigned char)input.at(i));
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

void VehicleConnection::slot_requestInfoTimer(){    
    //down file
    //-/ Query downfile
    if (readFileMng.Id == 0 || readFileMng.Status == LOG_FILE_DONE || readFileMng.Status == LOG_FILE_ERROR) {
        QString sqlChooseFile =
                QString(" SELECT filelog_id, filelog_bienso, filelog_thoigian, filelog_ten, "
                        "   filelog_trangthai, filelog_doc_batdau, filelog_doc_ketthuc, "
                        "   filelog_size, filelog_block, filelog_duongdan, filelog_lastupdate "
                        " FROM tbl_filelog "
                        " WHERE filelog_trangthai > 0 AND filelog_bienso = '%1' "
                        " ORDER BY filelog_trangthai DESC, filelog_thoigian ASC LIMIT 1 ").arg(plateNumber);

        QSqlQuery queryLogFile;
        if(connectionDatabase && connectionDatabase->execQuery(sqlChooseFile, queryLogFile)){
            if(queryLogFile.next()) {
                readFileMng.Id = queryLogFile.value(0).toInt();
                readFileMng.Name = queryLogFile.value(3).toString();
                readFileMng.Status = queryLogFile.value(4).toInt();
                readFileMng.StartRead = queryLogFile.value(5).toDateTime();
                readFileMng.EndRead = queryLogFile.value(6).toDateTime();
                readFileMng.Size = queryLogFile.value(7).toInt();
                readFileMng.Block = queryLogFile.value(8).toInt();
                readFileMng.Path = queryLogFile.value(9).toString();
                readFileMng.zeroCount = 0;

                qDebug() << plateNumber << readFileMng.Name << "Status" << readFileMng.Status;
            }
        }
    } else {
        if (readFileMng.Status == LOG_FILE_REQUEST_READ) {
            //-/-/ mkdir
            logPath = LOG_PATH + plateNumber + "/";
            QDir dir(logPath);
            if (!dir.exists()) {
                dir.mkpath(".");
            }

            //-/-/ 1 -> lay file size
            readFileMng.Status = LOG_FILE_SIZE_READING;
            readFileMng.Path = logPath + readFileMng.Name;
            if(UpdateLogFile(readFileMng.Status,
                             readFileMng.StartRead,
                             QDateTime::currentDateTime(),
                             readFileMng.Size,
                             readFileMng.Block,
                             readFileMng.Path,
                             readFileMng.Id)){
                GetFileSize(readFileMng.Name);
            }
        } else if(readFileMng.Status == LOG_FILE_SIZE_READING) {
            if (readFileMng.EndRead.secsTo(QDateTime::currentDateTime()) >= READ_BLOCK_TIME_OUT) {
                if(UpdateLogFile(readFileMng.Status,
                                 readFileMng.StartRead,
                                 QDateTime::currentDateTime(),
                                 readFileMng.Size,
                                 readFileMng.Block,
                                 readFileMng.Path,
                                 readFileMng.Id)){
                    GetFileSize(readFileMng.Name);
                }
            }
        } else if(readFileMng.Status == LOG_FILE_SIZE_READ){
            //-/-/ 2 -> doc file
            readFileMng.Status = LOG_FILE_PROCESSING;
            readFileMng.StartRead =QDateTime::currentDateTime();
            readFileMng.EndRead = QDateTime::currentDateTime();

            if(UpdateLogFile(readFileMng.Status,
                             readFileMng.StartRead,
                             readFileMng.EndRead,
                             readFileMng.Size,
                             readFileMng.Block,
                             readFileMng.Path,
                             readFileMng.Id)){
                qDebug() << plateNumber << "get first block of" << readFileMng.Name;
                GprsFmngReadBlk(readFileMng.Name, readFileMng.Block, READ_BLOCK);
            }
        } else if(readFileMng.Status == LOG_FILE_PROCESSING){
            if(readFileMng.EndRead.secsTo(QDateTime::currentDateTime()) >= READ_BLOCK_TIME_OUT) {
                readFileMng.EndRead = QDateTime::currentDateTime();
                if(UpdateLogFile(readFileMng.Status,
                                 readFileMng.StartRead,
                                 readFileMng.EndRead,
                                 readFileMng.Size,
                                 readFileMng.Block,
                                 readFileMng.Path,
                                 readFileMng.Id)){
                    qDebug() << plateNumber << "get block begin at " << readFileMng.Block << readFileMng.Name;
                    GprsFmngReadBlk(readFileMng.Name, readFileMng.Block, READ_BLOCK);
                }
            }
        }
    }
}

void VehicleConnection::slot_requestLogTimer(){

    qDebug() << plateNumber << "slot_requestLogTimer >>>>>>>>>>>>>>>>>>>>>>>>>>>";
    //get log by time
    //-/ get command
    QString sqlQueryCommand =
            QString(" SELECT command_id,command_time, command_bienso, command_time_from, "
                    "   command_time_to, command_time_sent, command_status "
                    " FROM tbl_command WHERE command_bienso = '%1' AND command_status >=0 "
                    " ORDER BY command_time ASC, command_status DESC;")
            .arg(plateNumber);

    QSqlQuery queryCommand;
    if(connectionDatabase && connectionDatabase->execQuery(sqlQueryCommand, queryCommand)){
        qDebug() << plateNumber << queryCommand.size();
        while(queryCommand.next()){
            int cmId = queryCommand.value(0).toInt();
            QDateTime cmTimeFrom = queryCommand.value(3).toDateTime();
            QDateTime cmTimeTo = queryCommand.value(4).toDateTime();
            QDateTime cmTimeSent = queryCommand.value(5).toDateTime();
            int cmStatus = queryCommand.value(6).toInt();

            if(cmStatus == 0) {
                cmTimeSent = cmTimeFrom;
                cmStatus = 1;
            } else if(cmStatus == 1) {
                if (cmTimeSent.secsTo(cmTimeFrom) > 0) {
                    cmStatus = -2;
                }
            }

            while (cmStatus != -2 && cmTimeSent.secsTo(cmTimeTo) > 0) {
                GetLogByTime(cmTimeSent.addSecs(60),cmTimeSent.addSecs(360));
                cmTimeSent = cmTimeSent.addSecs(360);

                if (UpdateCommand(cmId, cmTimeSent, cmStatus)) {
                    qDebug() << plateNumber << "update command sent" << cmTimeSent;
                }

                sleep(1);
            }

            if(cmStatus == 1) {
                cmStatus = -1;
            }

            if(UpdateCommand(cmId, cmTimeSent, cmStatus)){
                qDebug() << plateNumber << "update command done" << cmId ;
            }
        }
    }
}



void VehicleConnection::run()
{
    connect(tcpSocket, SIGNAL(readyRead()),this, SLOT(slot_readyRead()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(slot_socketDisconnected()));
    connect(tcpSocket, SIGNAL(disconnected()), tcpSocket, SLOT(deleteLater()));
    connect(tcpSocket, SIGNAL(destroyed()), this, SLOT(slot_socketDestroyed()));

    unsigned char PSBuff[10]={0x80};
    qDebug() << plateNumber << "i'm server";
    SendPck(PSBuff,1,CMD_SYS_GET,0);
    qDebug() << plateNumber << "what 's your name ?";

    connectionTimer = new QTimer(this);
    connect(connectionTimer, SIGNAL(timeout()), this, SLOT(slot_connectionTimer_timeout()));
    connectionTimer->start(30000);

    //hoi ten sau 15p
    requestInfoTimer = new QTimer(this);
    connect(requestInfoTimer, SIGNAL(timeout()), this, SLOT(slot_requestInfoTimer()));
    requestInfoTimer->start(10000);

    //get log by time
    requestLogTimer = new QTimer(this);
    connect(requestLogTimer, SIGNAL(timeout()),this, SLOT(slot_requestLogTimer()));
    requestLogTimer->start(5000);

    connectionDatabase = new CprTfcDatabase(qApp->applicationDirPath()+"/VehicleTracking.ini", "LocalDatabase","VehicleConnection");

    exec()  ;
    if(connectionDatabase){
        delete connectionDatabase;
        connectionDatabase = NULL;
    }
}


