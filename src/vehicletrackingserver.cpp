#include "vehicletrackingserver.h"
#include <QTcpSocket>
#define PREFIX "VehicleTrackingServer:"
#define DATA_PATH "/var/www/html/CadProVTS/data/DATA"
#define REST_TIME 300
#define MIN_PKG 2
#define HeaderAbsWrite 0
#define HeaderRwWrite  1



VehicleTrackingServer::VehicleTrackingServer()
{
    qApp->addLibraryPath(qApp->applicationDirPath() + "/plugins");

    QSettings iniFile(qApp->applicationDirPath() + "/VehicleTracking.ini", QSettings::IniFormat);
    iniFile.beginGroup("VehicleTrackingServer");
    listenPort = iniFile.value("ListenPort",1234).toInt();
    maxPendingConnection = iniFile.value("MaxPendingConnection",1000).toInt();
    dataStorageTime  = iniFile.value("DataStorageTime",60).toInt();
    dataPath = iniFile.value("DataPath", DATA_PATH).toString();
    minRestTime =  iniFile.value("MinRestTime", REST_TIME).toInt();
    minPackage =  iniFile.value("MinPackage", MIN_PKG).toInt();
    Modew = 0;
    CoutBuff=0;

    tcpServer = NULL;
    tcpServer = new QTcpServer();
    tcpServer->setMaxPendingConnections(maxPendingConnection);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(slot_newConnection()));
    if(tcpServer->listen(QHostAddress::Any, listenPort))
        qDebug() << "Listen server started at" << tcpServer->serverAddress().toString() << tcpServer->serverPort();
    else qDebug() << "Can not start listen server at" << tcpServer->serverAddress().toString();


    serverDatabase = new CprTfcDatabase(qApp->applicationDirPath()+"/VehicleTracking.ini", "LocalDatabase","ServerConnection",true);

    //lastVehicleLog = QDateTime::fromString("2017-03-23 14:00:44", "yyyy-MM-dd hh:mm:ss");
    lastVehicleLog = QDateTime::currentDateTime();
    //todo get lasttime of incomplete journey in DB
    QString sqlSelectHanhTrinh =
            QString(" SELECT  hanhtrinh_bienso ,  hanhtrinh_thoigian_batdau , "
                    "   hanhtrinh_kinhdo_batdau , hanhtrinh_vido_batdau ,  hanhtrinh_thoigian_ketthuc , "
                    "   hanhtrinh_kinhdo_ketthuc , hanhtrinh_vido_ketthuc "
                    " FROM  tbl_hanhtrinh "
                    " WHERE hanhtrinh_trangthai = 0 "
                    "   AND hanhtrinh_capdo = 0 "
                    " ORDER BY hanhtrinh_thoigian_batdau DESC LIMIT 1000");

    QSqlQuery queryHanhTrinh;
    if (serverDatabase && serverDatabase->execQuery(sqlSelectHanhTrinh,queryHanhTrinh)) {
        while(queryHanhTrinh.next()) {
            QString key = queryHanhTrinh.value(0).toString();
            if (!mapHanhTrinh.contains(key)) {
                HanhTrinh h;
                h.thoigianBatdau = queryHanhTrinh.value(1).toDateTime();
                h.kinhdoBatdau = queryHanhTrinh.value(2).toDouble();
                h.vidoBatdau = queryHanhTrinh.value(3).toDouble();
                h.thoigianKetthuc = queryHanhTrinh.value(4).toDateTime();
                h.kinhdoKetthuc = queryHanhTrinh.value(5).toDouble();
                h.vidoKetthuc = queryHanhTrinh.value(6).toDouble();

                mapHanhTrinh.insert(key, h);

                if(lastVehicleLog.secsTo(h.thoigianKetthuc) < 0) {
                    lastVehicleLog = h.thoigianKetthuc;
                }
            }
        }
    }else {
        qDebug() << "Get journey error!!!";
        exit(99);
    }

    //test write log
    writeLog("D10H-005", QDateTime::fromString("2017-03-27 15:44:02", "yyyy-MM-dd hh:mm:ss"), QDateTime::fromString("2017-03-27 15:48:02", "yyyy-MM-dd hh:mm:ss"), "data/2017-03-28.log");

    mainTimer = new QTimer(this);
    connect(mainTimer, SIGNAL(timeout()), this, SLOT(slot_mainTimer_timeout()));
    mainTimer->start(1000);
}

void VehicleTrackingServer::slot_mainTimer_timeout(){
    qDebug() << "---------------------------------------------------";
    createPartition("tbl_phuongtienlog");
    createPartition("tbl_event");

    //-/ tach hanh trinh
    //-/-/scan log
    QString sqlScanVehicleLog  =
            QString(" SELECT  phuongtienlog_bienso, phuongtienlog_thoigian, phuongtienlog_kinhdo, "
                    "      phuongtienlog_vido, phuongtienlog_vantoc_gps, phuongtienlog_vantoc_dongho, "
                    "      phuongtienlog_lytrinh, phuongtienlog_huong, phuongtienlog_trangthaigps "
                    " FROM tbl_phuongtienlog "
                    " WHERE phuongtienlog_thoigian > '%1' AND phuongtienlog_thoigian < '%2' "
                    " ORDER BY phuongtienlog_thoigian ASC limit 1000;")
            .arg(lastVehicleLog.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    QSqlQuery queryScanVehicleLog;
    if (serverDatabase && serverDatabase->execQuery(sqlScanVehicleLog, queryScanVehicleLog)) {
        while (queryScanVehicleLog.next()) {
            VehicleLog tmpLog;
            QString tmpBienso = queryScanVehicleLog.value(0).toString();
            tmpLog.thoigian = queryScanVehicleLog.value(1).toDateTime();
            tmpLog.kinhdo = queryScanVehicleLog.value(2).toDouble();
            tmpLog.vido = queryScanVehicleLog.value(3).toDouble();
            tmpLog.vantocGps = queryScanVehicleLog.value(4).toDouble();
            tmpLog.vantocDongho = queryScanVehicleLog.value(5).toDouble();
            tmpLog.lytrinh = queryScanVehicleLog.value(6).toInt();
            tmpLog.huong = queryScanVehicleLog.value(7).toInt();
            tmpLog.trangthaiGps = queryScanVehicleLog.value(8).toInt();
            lastVehicleLog = tmpLog.thoigian;

            //-/-/them vao map
            if(!mapVehicleLog.contains(tmpBienso) || mapVehicleLog.isEmpty()){
                QList<VehicleLog> vLog;
                vLog.append(tmpLog);
                mapVehicleLog.insert(tmpBienso, vLog);
                qDebug() << "add new key: "  << tmpBienso;
            } else {
                mapVehicleLog[tmpBienso].append(tmpLog);
                //listLog.append(tmpLog);
                qDebug() << "add to value to key: " << tmpBienso;
            }
        }


        QMap<QString, QList<VehicleLog> >::const_iterator i;
        for (i = mapVehicleLog.constBegin(); i != mapVehicleLog.constEnd(); ++i) {
            qDebug() << i.key() << i.value().size();
            //            QList<VehicleLog>::iterator j;
            //            QList<VehicleLog> vehicleLog = i.value();
            //            for(j = vehicleLog.begin(); j != vehicleLog.end(); j++) {
            //                qDebug() << QString("time=%1, kinhdo=%2, vido=%3, lytrinh=%4")
            //                            .arg((*j).thoigian.toString("yyyy-MM-dd hh:mm:ss"))
            //                            .arg((*j).kinhdo)
            //                            .arg((*j).vido)
            //                            .arg((*j).lytrinh);
            //            }
        }
    } else{
        qDebug() << PREFIX << "Scan VehicleLog error!";
    }

    //-/-/Tach hanh trinh bac 0
    QStringList listKey = mapVehicleLog.keys();
    for(int i = 0; i<listKey.size(); i++) {
        QString tmpKey = listKey[i];

        VehicleLog tmpVlog,lastStatus;
        tmpVlog = lastStatus = mapVehicleLog[tmpKey].first();

        //-/-/-/ neu hanh trinh cu cach qua xa thi ngat'
        if(mapHanhTrinh.contains(tmpKey)) {
            if(mapHanhTrinh[tmpKey].thoigianKetthuc.secsTo(tmpVlog.thoigian) >  minRestTime) {
                //-/-/-/-/ ket thuc hanh trinh cu
                VehicleLog v;
                v.thoigian = mapHanhTrinh[tmpKey].thoigianKetthuc;
                v.kinhdo = mapHanhTrinh[tmpKey].kinhdoKetthuc;
                v.vido = mapHanhTrinh[tmpKey].vidoKetthuc;
                finishJourney(tmpKey,v);
            }
        }

        while(!mapVehicleLog[tmpKey].isEmpty()) {
            tmpVlog = mapVehicleLog[tmpKey].first();

            if (tmpVlog.vantocDongho > 0 || tmpVlog.vantocGps >0) {
                if(mapHanhTrinh.contains(tmpKey)) {

                    // cap nhat lai map dieu kien
                    if(mapDieuKien.contains(tmpKey)){
                        mapDieuKien[tmpKey] = 0;
                    } else {
                        mapDieuKien.insert(tmpKey,0);
                    }

                    if(mapHanhTrinh[tmpKey].thoigianKetthuc.secsTo(tmpVlog.thoigian) > 0 ) {
                        qDebug() << "cap nhat lai" << tmpKey << mapHanhTrinh[tmpKey].thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss");
                        mapHanhTrinh[tmpKey].thoigianKetthuc = tmpVlog.thoigian;
                        mapHanhTrinh[tmpKey].kinhdoKetthuc = tmpVlog.kinhdo;
                        mapHanhTrinh[tmpKey].vidoKetthuc = tmpVlog.vido;

                        // cap nhat lai hanh trinh trong db
                        QString sqlUpdateHanhTrinh =
                                QString(" UPDATE  tbl_hanhtrinh "
                                        " SET  hanhtrinh_thoigian_ketthuc =  '%1', "
                                        "   hanhtrinh_kinhdo_ketthuc = %2, "
                                        "   hanhtrinh_vido_ketthuc = %3, "
                                        "   hanhtrinh_capdo = 0, "
                                        "   hanhtrinh_trangthai = 0 "
                                        " WHERE hanhtrinh_bienso =  '%4' "
                                        "   AND hanhtrinh_thoigian_batdau =  '%5'")
                                .arg(mapHanhTrinh[tmpKey].thoigianKetthuc.toString("yyyy-MM-dd hh:mm:ss"))
                                .arg(QString::number(mapHanhTrinh[tmpKey].kinhdoKetthuc,'f',6))
                                .arg(QString::number(mapHanhTrinh[tmpKey].vidoKetthuc,'f', 6))
                                .arg(tmpKey)
                                .arg(mapHanhTrinh[tmpKey].thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss"));

                        if(serverDatabase) serverDatabase->execQuery(sqlUpdateHanhTrinh);
                    }
                } else {
                    //=> khong co key -> kiem tra tao hanh trinh moi
                    //if(lastStatus.vantocDongho==0 && lastStatus.vantocGps==0) {

                    //-/ ?????? kiem tra van toc trong bao lau de tao hanh trinh moi???
                    if(!mapDieuKien.contains(tmpKey)){
                        mapDieuKien.insert(tmpKey,1);
                    } else {
                        mapDieuKien[tmpKey] = mapDieuKien[tmpKey] + 1;

                        if(mapDieuKien[tmpKey] >= minPackage) {
                            //-/ thoi gian lon hon hanh trinh moi nhat thi dc tao hanh trinh moi
                            QString sqlLastTime =
                                    QString(" SELECT COALESCE(max(hanhtrinh_thoigian_ketthuc) ,'2000-01-01 00:00:00') "
                                            " FROM tbl_hanhtrinh "
                                            " WHERE hanhtrinh_bienso = '%1'").arg(tmpKey);
                            QSqlQuery queryLastTime;
                            if(serverDatabase && serverDatabase->execQuery(sqlLastTime,queryLastTime)) {
                                queryLastTime.next();
                                QDateTime lastTime = queryLastTime.value(0).toDateTime();

                                if(lastTime.secsTo(tmpVlog.thoigian) > 0) {
                                    qDebug() << "tao moi " << tmpKey << tmpVlog.thoigian.toString("yyyy-MM-dd hh:mm:ss");
                                    HanhTrinh hanhTrinh;
                                    hanhTrinh.thoigianBatdau = tmpVlog.thoigian;
                                    hanhTrinh.kinhdoBatdau = tmpVlog.kinhdo;
                                    hanhTrinh.vidoBatdau = tmpVlog.vido;
                                    hanhTrinh.thoigianKetthuc = tmpVlog.thoigian;
                                    hanhTrinh.kinhdoKetthuc = tmpVlog.kinhdo;
                                    hanhTrinh.vidoKetthuc = tmpVlog.vido;
                                    mapHanhTrinh.insert(tmpKey,hanhTrinh);

                                    QString sqlInsertHanhTrinh =
                                            QString(" INSERT INTO  tbl_hanhtrinh(hanhtrinh_bienso ,  hanhtrinh_thoigian_batdau , "
                                                    "   hanhtrinh_kinhdo_batdau ,  hanhtrinh_vido_batdau , hanhtrinh_thoigian_ketthuc , "
                                                    "   hanhtrinh_kinhdo_ketthuc , hanhtrinh_vido_ketthuc ,  hanhtrinh_capdo , "
                                                    "   hanhtrinh_trangthai, hanhtrinh_tuyenduong_id, hanhtrinh_machuyen ) "
                                                    " VALUES ( '%1',  '%2', %3, %4, '%5', %6, %7, 0, 0, "
                                                    "           (SELECT  phuongtien_tuyenduong FROM `tbl_phuongtien` WHERE phuongtien_bienso = '%1' limit 1), "
                                                    "           (SELECT  phuongtien_machuyen FROM `tbl_phuongtien` WHERE phuongtien_bienso = '%1' limit 1))")
                                            .arg(tmpKey)
                                            .arg(hanhTrinh.thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss"))
                                            .arg(QString::number(hanhTrinh.kinhdoBatdau,'f',6))
                                            .arg(QString::number(hanhTrinh.vidoBatdau,'f',6))
                                            .arg(hanhTrinh.thoigianKetthuc.toString("yyyy-MM-dd hh:mm:ss"))
                                            .arg(QString::number(hanhTrinh.kinhdoKetthuc,'f',6))
                                            .arg(QString::number(hanhTrinh.vidoKetthuc,'f',6));

                                    if(serverDatabase) serverDatabase->execQuery(sqlInsertHanhTrinh);

                                    //                                    mapDieuKien[tmpKey] = 0;
                                }
                            }
                        }
                    }
                    //}
                }
            } else {
                // => 2 loai van toc deu = 0 -> kiem tra ket thuc hanh trinh cu

                if(mapDieuKien.contains(tmpKey)) {
                    mapDieuKien[tmpKey] = mapDieuKien[tmpKey] - 1;

                    //if((lastStatus.vantocDongho > 0 || lastStatus.vantocGps >0) && mapHanhTrinh.contains(tmpKey)){
                    if(mapDieuKien[tmpKey] <= -minPackage && mapHanhTrinh.contains(tmpKey)){
                        //cap nhat lai DB hanh trinh vua thuc hien
                        if (finishJourney(tmpKey,tmpVlog)) {
                            mapDieuKien.remove(tmpKey);
                        }
                    }
                }

            }
            lastStatus = tmpVlog;
            mapVehicleLog[tmpKey].pop_front();
        }

        if(mapVehicleLog[tmpKey].size() == 0) {
            mapVehicleLog.remove(tmpKey);
        }
    }
}

void VehicleTrackingServer::WriteBuffLog(GsThOldLogRec GsPosLog, QString NameFile){
    switch(Modew){
    case HeaderAbsWrite:
        ConvertToOldBuff(&GsPosLog, WrBuff,NULL);
        CoutBuff = 16;
        Modew = HeaderRwWrite;
        break;
    case HeaderRwWrite:
        if(CoutBuff <= 256){
            ConvertToOldBuff(&GsPosLog, NULL, WrBuff + CoutBuff);
            CoutBuff += 6;
            if(CoutBuff==256){
                pw = fopen(NameFile.toStdString().c_str(),"a+");
                if(pw != NULL){
                    //fwrite(pw, WrBuff, 256);
                    fwrite(WrBuff, sizeof(unsigned char), 256, pw);
                    fclose(pw);
                }
                CoutBuff = 0;
                Modew = HeaderAbsWrite;
            }
        }
        break;
    default:
        break;
    }
}

void VehicleTrackingServer::ConvertToOldBuff(GsThOldLogRec* Rec,unsigned char OutBuff16[],unsigned char Buff6[]){
    int LyTrinhM;
    static GsThOldLogRec FirstBlogRec;
    if(OutBuff16!=NULL){
        FirstBlogRec=*Rec;
        OutBuff16[0]=Rec->DateTime.Day;
        OutBuff16[1]=(Rec->DateTime.Month&0x0F)|(Rec->IdTuyen<<4);
        OutBuff16[2]=(Rec->Rundirection&1);//Need more

        LyTrinhM=Rec->Km*1000+Rec->m;
        OutBuff16[3]=LyTrinhM>>16;
        OutBuff16[4]=LyTrinhM>>8;
        OutBuff16[5]=LyTrinhM;

        OutBuff16[6]=Rec->Altitude/256;
        OutBuff16[7]=Rec->Altitude&255;
        int h,l;
        //--------------------------------------
        h=Rec->Lat/10000;
        l=Rec->Lat%10000;
        OutBuff16[8]=h>>8;
        OutBuff16[9]=h&0xFF;
        OutBuff16[10]=l>>8;
        OutBuff16[11]=l&0xFF;
        //----------------------------------------------
        h=Rec->Long/10000;
        l=Rec->Long%10000;
        OutBuff16[12]=h>>8;
        OutBuff16[13]=h&0xFF;
        OutBuff16[14]=l>>8;
        OutBuff16[15]=l&0xFF;
    }
    //-----------------------------------------------------------------
    if(Buff6!=NULL){
        Buff6[1]=(Rec->DateTime.Hour<<3)|(Rec->DateTime.Min>>3);
        //Buff6[0]=(Rec->DateTime.Min<<5)|(Rec->DateTime.Sec/2);
        Buff6[0]=(Rec->DateTime.Min<<5) | (Rec->DateTime.Sec/2);
        unsigned char Speed;
        Speed=Rec->Speed*10;
        Buff6[3]=Speed&0xFF;
        Buff6[2]=(Speed>>8)&0x03;//|(Rec->Presure<<2);
        signed short mm=(Rec->Km*1000+Rec->m)-(FirstBlogRec.Km*1000+FirstBlogRec.m);
        memcpy(Buff6+4,&mm,2);
    }
}

bool VehicleTrackingServer::writeLog(QString key, QDateTime begin, QDateTime end, QString fileName){
    QString sqlSelectData =
            QString(" SELECT phuongtienlog_extdata "
                    " FROM tbl_phuongtienlog "
                    " WHERE  `phuongtienlog_bienso` =  '%1' "
                    "   AND  `phuongtienlog_thoigian` <=  '%2' "
                    "   AND  `phuongtienlog_thoigian` >=  '%3' ")
            .arg(key)
            .arg(end.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(begin.toString("yyyy-MM-dd hh:mm:ss"));
    QSqlQuery queryData;

    if(serverDatabase && serverDatabase->execQuery(sqlSelectData,queryData)) {

        QString filePath = fileName.left(fileName.lastIndexOf("/"));
        QDir dir(filePath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }


        if(queryData.size() > 1) {
            if(queryData.next()) {
                QByteArray input = QByteArray::fromHex(queryData.value(0).toString().toAscii());
                memcpy(&TraiRevRec,input,sizeof(TrainAbsRec));
            }


            while(queryData.next()){
                TrainAbsRec nextState;
                QByteArray nextInput = QByteArray::fromHex(queryData.value(0).toString().toAscii());
                memcpy(&nextState,nextInput,sizeof(TrainAbsRec));

                long longDif = nextState.Long1s - TraiRevRec.Long1s;
                long latDif = nextState.Lat1s - TraiRevRec.Lat1s;

                QDateTime gpsTime;
                gpsTime.setDate(QDate(TraiRevRec.TimeNow1s.Year + 2000, TraiRevRec.TimeNow1s.Month, TraiRevRec.TimeNow1s.Day));
                gpsTime.setTime(QTime(TraiRevRec.TimeNow1s.Hour, TraiRevRec.TimeNow1s.Min, TraiRevRec.TimeNow1s.Sec, 0));


                int tmpKmM = TraiRevRec.KmM;

                for(int i = 0; i < TIME_SEND_DATA_SERVER; i++){
                    int s = i>0?TraiRevRec.SpeedBuff[i-1]/3.6:0;
                    tmpKmM += s;

                    GsPosLog.Km = tmpKmM / 1000;
                    GsPosLog.m =  tmpKmM % 1000;
                    GsPosLog.Rundirection = 0;
                    GsPosLog.Presure = TraiRevRec.PresBuff[i];
                    GsPosLog.Speed = TraiRevRec.SpeedBuff[i];
                    GsPosLog.Long = TraiRevRec.Long1s + longDif*i/20;
                    GsPosLog.Lat = TraiRevRec.Lat1s + latDif*i/20;

                    TraiRevRec.TimeNow1s.Year = gpsTime.addSecs(i).toString("yyyy").toShort() - 2000;
                    TraiRevRec.TimeNow1s.Month = gpsTime.addSecs(i).toString("MM").toShort();
                    TraiRevRec.TimeNow1s.Day = gpsTime.addSecs(i).toString("dd").toShort();
                    TraiRevRec.TimeNow1s.Hour = gpsTime.addSecs(i).toString("hh").toShort();
                    TraiRevRec.TimeNow1s.Min = gpsTime.addSecs(i).toString("mm").toShort();
                    TraiRevRec.TimeNow1s.Sec = gpsTime.addSecs(i).toString("ss").toShort();
                    GsPosLog.DateTime = TraiRevRec.TimeNow1s;

                    WriteBuffLog(GsPosLog, fileName);
                }

                TraiRevRec = nextState;
            }
        }
    } else {
        return false;
    }
    return true;
}

bool VehicleTrackingServer::finishJourney(QString key, VehicleLog v){
    qDebug() << "ket thuc hanh trinh" << key << mapHanhTrinh[key].thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss");

    QString fileName = QString("%1/%2/data_%3_%4_%5")
            .arg(dataPath)
            .arg(key)
            .arg(mapHanhTrinh[key].thoigianBatdau.toString("yyyyMMddhhmmss"))
            .arg(v.thoigian.toString("yyyyMMddhhmmss"))
            .arg(QString::number(QDateTime::currentMSecsSinceEpoch()));

    qDebug() << fileName;

    if(writeLog(key, mapHanhTrinh[key].thoigianBatdau, v.thoigian, fileName)) {
        //-/update hanh trinh ve trang thai ket thuc
        QString sqlKetThucHanhTrinh =
                QString(" UPDATE  tbl_hanhtrinh "
                        " SET  hanhtrinh_thoigian_ketthuc =  '%1', "
                        "   hanhtrinh_kinhdo_ketthuc = %2, "
                        "   hanhtrinh_vido_ketthuc = %3, "
                        "   hanhtrinh_capdo = 0, "
                        "   hanhtrinh_trangthai = 1, "
                        "   hanhtrinh_datafile = '%4'"
                        " WHERE hanhtrinh_bienso =  '%5' "
                        "   AND hanhtrinh_thoigian_batdau =  '%6'")
                .arg(v.thoigian.toString("yyyy-MM-dd hh:mm:ss"))
                .arg(QString::number(v.kinhdo,'f',6))
                .arg(QString::number(v.vido,'f',6))
                .arg(fileName)
                .arg(key)
                .arg(mapHanhTrinh[key].thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss"));

        if(serverDatabase && serverDatabase->execQuery(sqlKetThucHanhTrinh)) {
            //-/xoa hanh trinh ra khoi map
            mapHanhTrinh.remove(key);
            qDebug() << "mapHanhTrinh -> remove:" << key;
        } else {
            return false;
        }
    } else {
        qDebug() << "ERROR : can not finish journey" << key << mapHanhTrinh[key].thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss");
        return false;
    }

    return true;
}




void VehicleTrackingServer::createPartition(QString table){
    //-/manage backup
    //-/-/check and create partition
    QString getLstPartition = QString("SELECT partition_name FROM information_schema.partitions WHERE table_name ='%1' ORDER BY partition_name DESC Limit 1")
            .arg(table);
    QSqlQuery qLstPartition;

    if(serverDatabase && serverDatabase->execQuery(qLstPartition,getLstPartition)) {

        if(qLstPartition.next()){
            QString lstPartitionName = qLstPartition.value(0).toString().right(8);
            QDateTime lstPartitionDate = QDateTime::fromString(lstPartitionName, "yyyyMMdd");

            if (lstPartitionDate.daysTo(QDateTime::currentDateTime()) > -1) {
                if (lstPartitionDate.daysTo(QDateTime::currentDateTime()) > dataStorageTime) {
                    //-/-/-/create partition: from (current day - dataStorageTime) to (current day + 1)
                    lstPartitionDate = QDateTime::currentDateTime().addDays(-dataStorageTime);
                } else {
                    //-/-/-/create partition: from (lastPartition + 1 ) to (current day + 1)
                    lstPartitionDate = lstPartitionDate.addDays(1);
                }


                while (lstPartitionDate.daysTo(QDateTime::currentDateTime()) >= -1 ) {
                    QString addPartition = QString("ALTER TABLE %1 "
                                                   "ADD PARTITION (PARTITION p%2 VALUES LESS THAN ( TO_DAYS('%3')))")
                            .arg(table)
                            .arg(lstPartitionDate.toString("yyyyMMdd"))
                            .arg(lstPartitionDate.addDays(1).toString("yyyy-MM-dd hh:mm:ss"));
                    if (serverDatabase && serverDatabase->execQuery(addPartition)) {
                        qDebug() << QString(" Partition added: p%1").arg(lstPartitionDate.toString("yyyyMMdd"));
                    }
                    lstPartitionDate = lstPartitionDate.addDays(1);
                }
            }
        }
    }

    //-/-/clear expired data
}

void VehicleTrackingServer::slot_newConnection(){
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    if(socket)
        qDebug() << "newConnection" << socket->peerAddress().toString();
    VehicleConnection *vehicleConnection = new VehicleConnection(socket);
    if(vehicleConnection){
        connect(vehicleConnection, SIGNAL(finished()), this, SLOT(slot_closeConnection()));
        connectionList.append(vehicleConnection);
        vehicleConnection->start();
    }

}
void VehicleTrackingServer::slot_closeConnection(){
    qDebug() << "VehicleTrackingServer::slot_closeConnection()";
    VehicleConnection *vehicleConnection = qobject_cast<VehicleConnection *>(sender());
    for(int i=connectionList.size()-1;i>=0;i--){
        if(vehicleConnection == connectionList[i]){
            qDebug() << "closeConnection" << i << connectionList.size();
            connectionList.removeAt(i);
            delete vehicleConnection;
        }
    }
}
