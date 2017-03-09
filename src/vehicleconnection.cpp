#include "vehicleconnection.h"

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

void VehicleConnection::slot_connectionTimer_timeout(){
    if(lastRecvTime.secsTo(QDateTime::currentDateTime())>30)
        tcpSocket->disconnectFromHost();
}

void VehicleConnection::slot_readyRead(){
    lastRecvTime = QDateTime::currentDateTime();
    qDebug()<< "VehicleConnection::slot_readyRead()";
    if(tcpSocket->canReadLine()){
        QString str = tcpSocket->readLine();
        QSqlQuery query;
        if(connectionDatabase && connectionDatabase->execQuery(query, str)){
            qDebug()<<"query Size" << query.size();
            if(query.next())
                qDebug()<<"query Value" << query.value(0).toString();
        }
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
    tcpSocket->write("HELLO\r\n");

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
