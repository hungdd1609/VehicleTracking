#ifndef VEHICLECONNECTION_H
#define VEHICLECONNECTION_H

#include <QThread>
#include <QTcpSocket>
#include <QTimer>

#include "cprtfcdatabase.h"
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
    QTcpSocket *tcpSocket;
    QDateTime lastRecvTime;
    QString plateNumber;
    int state;
    unsigned char SendPck(unsigned char* PayLoad,unsigned short PayLoadLen,unsigned char Cmd,unsigned char PckIdx);
    void Sys7bProcessRevPck(unsigned char* Pck,unsigned short len);
    void Sys7bInput(unsigned char data);


private slots:
    void slot_connectionTimer_timeout();
    void slot_readyRead();
    void slot_socketDisconnected();
    void slot_socketError(QAbstractSocket::SocketError socketError);
    void slot_socketDestroyed();
protected:
    void run();
};

#endif // VEHICLECONNECTION_H
