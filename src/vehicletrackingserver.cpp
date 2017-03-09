#include "vehicletrackingserver.h"
#include <QTcpSocket>
VehicleTrackingServer::VehicleTrackingServer()
{
    qApp->addLibraryPath(qApp->applicationDirPath() + "/plugins");

    QSettings iniFile(qApp->applicationDirPath() + "/VehicleTracking.ini", QSettings::IniFormat);
    iniFile.beginGroup("VehicleTrackingServer");
    listenPort = iniFile.value("ListenPort",1234).toInt();
    maxPendingConnection = iniFile.value("MaxPendingConnection",1000).toInt();

    tcpServer = NULL;
    tcpServer = new QTcpServer();
    tcpServer->setMaxPendingConnections(maxPendingConnection);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(slot_newConnection()));
    if(tcpServer->listen(QHostAddress::Any, listenPort))
        qDebug() << "Listen server started at" << tcpServer->serverAddress().toString() << tcpServer->serverPort();
    else qDebug() << "Can not start listen server at" << tcpServer->serverAddress().toString();

    mainTimer = new QTimer(this);
    connect(mainTimer, SIGNAL(timeout()), this, SLOT(slot_mainTimer_timeout()));
    mainTimer->start(1000);


}
void VehicleTrackingServer::slot_mainTimer_timeout(){

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
