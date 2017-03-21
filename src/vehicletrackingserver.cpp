#include "vehicletrackingserver.h"
#include <QTcpSocket>
//VehicleTrackingServer::VehicleTrackingServer(QDateTime ibegin, int ithreshold, int ilistenPort)
VehicleTrackingServer::VehicleTrackingServer()
{
    qApp->addLibraryPath(qApp->applicationDirPath() + "/plugins");

    QSettings iniFile(qApp->applicationDirPath() + "/VehicleTracking.ini", QSettings::IniFormat);
    iniFile.beginGroup("VehicleTrackingServer");
    listenPort = iniFile.value("ListenPort",1234).toInt();
    //listenPort = ilistenPort;
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

    mainTimer = new QTimer(this);
    connect(mainTimer, SIGNAL(timeout()), this, SLOT(slot_mainTimer_timeout()));
    mainTimer->start(1000);

    //insert test
    /*
    threshold = ithreshold;
    begin = ibegin;
    QDateTime tmpTime = ibegin;
    int count = 0;
    QString insert =
            " INSERT INTO tbl_trainlog (trainlog_trainid, "
            "      trainlog_latitude, trainlog_longitude, trainlog_time, trainlog_lytrinh, "
            "      trainlog_speed, trainlog_pressure, trainlog_rawdata) "
            " VALUES ";

    while (tmpTime.daysTo(QDateTime::currentDateTime()) > ithreshold ) {
         insert += QString("(1, 1000, 1000, '%1', 10000, 40.4, 36.5, 'abcd')")
                .arg(tmpTime.toString("yyyy-MM-dd hh:mm:ss"));

        count++;

        if(count >=1000) {
            if (serverDatabase && serverDatabase->execQuery(insert)) {
                qDebug() << QString(" 1000 record added: p%1").arg(tmpTime.toString("yyyy-MM-dd hh:mm:ss"));
            }
            count = 0;
            insert = " INSERT INTO tbl_trainlog (trainlog_trainid, "
                    "      trainlog_latitude, trainlog_longitude, trainlog_time, trainlog_lytrinh, "
                    "      trainlog_speed, trainlog_pressure, trainlog_rawdata) "
                    " VALUES ";
        } else {
            insert += ",";
        }
        tmpTime = tmpTime.addSecs(1);
    }
    qDebug() << "---------done!--------------" << listenPort;
    */
}
void VehicleTrackingServer::slot_mainTimer_timeout(){    
    //-/manage backup
    //-/-/check and create partition
    QString getLstPartition = "SELECT partition_name FROM information_schema.partitions WHERE table_name ='tbl_phuongtienlog' ORDER BY partition_name DESC Limit 1";
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
                    QString addPartition = QString("ALTER TABLE tbl_phuongtienlog "
                                                   "ADD PARTITION (PARTITION p%1 VALUES LESS THAN ( TO_DAYS('%2')))")
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
