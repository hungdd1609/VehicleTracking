#include "vehicletrackingserver.h"
#include <QTcpSocket>
#define PREFIX "VehicleTrackingServer:"
#define DATA_PATH "DATA"
#define REST_TIME 300

VehicleTrackingServer::VehicleTrackingServer()
{
    qApp->addLibraryPath(qApp->applicationDirPath() + "/plugins");

    QSettings iniFile(qApp->applicationDirPath() + "/VehicleTracking.ini", QSettings::IniFormat);
    iniFile.beginGroup("VehicleTrackingServer");
    listenPort = iniFile.value("ListenPort",1234).toInt();
    maxPendingConnection = iniFile.value("MaxPendingConnection",1000).toInt();
    dataStorageTime  = iniFile.value("DataStorageTime",60).toInt();

    tcpServer = NULL;
    tcpServer = new QTcpServer();
    tcpServer->setMaxPendingConnections(maxPendingConnection);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(slot_newConnection()));
    if(tcpServer->listen(QHostAddress::Any, listenPort))
        qDebug() << "Listen server started at" << tcpServer->serverAddress().toString() << tcpServer->serverPort();
    else qDebug() << "Can not start listen server at" << tcpServer->serverAddress().toString();


    serverDatabase = new CprTfcDatabase(qApp->applicationDirPath()+"/VehicleTracking.ini", "LocalDatabase","ServerConnection",true);

    lastVehicleLog = QDateTime::fromString("2017-03-23 14:00:44", "yyyy-MM-dd hh:mm:ss");
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
            if(mapHanhTrinh[tmpKey].thoigianKetthuc.secsTo(tmpVlog.thoigian) >  REST_TIME) {
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

                        if(mapDieuKien[tmpKey] >= 3) {
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
                    if(mapDieuKien[tmpKey] <= -3 && mapHanhTrinh.contains(tmpKey)){
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


bool VehicleTrackingServer::finishJourney(QString key, VehicleLog v){
    qDebug() << "ket thuc hanh trinh" << key << mapHanhTrinh[key].thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss");

    //-/tao file data
    QString sqlSelectData =
            QString(" SELECT phuongtienlog_extdata "
                    " FROM tbl_phuongtienlog "
                    " WHERE  `phuongtienlog_bienso` =  '%1' "
                    "   AND  `phuongtienlog_thoigian` <=  '%2' "
                    "   AND  `phuongtienlog_thoigian` >=  '%3' ")
            .arg(key)
            .arg(v.thoigian.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(mapHanhTrinh[key].thoigianBatdau.toString("yyyy-MM-dd hh:mm:ss"));
    QSqlQuery queryData;
    if(serverDatabase && serverDatabase->execQuery(sqlSelectData,queryData)) {

        QString fileName = QString("%1/%2/data_%3_%4_%5")
                .arg(DATA_PATH)
                .arg(key)
                .arg(mapHanhTrinh[key].thoigianBatdau.toString("yyyyMMddhhmmss"))
                .arg(v.thoigian.toString("yyyyMMddhhmmss"))
                .arg(QString::number(QDateTime::currentMSecsSinceEpoch()));

        qDebug() << fileName;

        QString filePath = fileName.left(fileName.lastIndexOf("/"));
        QDir dir(filePath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QFile file(fileName);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);

        if(queryData.size() > 0) {
            while(queryData.next()){
                out << queryData.value(0).toByteArray() << endl;
            }
        }

        file.close();


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
