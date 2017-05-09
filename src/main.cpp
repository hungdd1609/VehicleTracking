#include <QtCore/QCoreApplication>
#include "vehicletrackingserver.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>

static void messageHandler(QtMsgType type, const char *msg)
{
    QString datetime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logFileName=QDateTime::currentDateTime().toString("./log/yyyyMMdd.log");
    QFile syslogFile(logFileName);
    if (syslogFile.open(QFile::Append | QFile::Text)) {
        QTextStream out(&syslogFile);
        out << datetime << " " << msg << "\n";
        syslogFile.close();
    }
}

int main(int argc, char *argv[])
{    
    QCoreApplication a(argc, argv);
    qDebug() << "sizeof(TrainAbsRec)" <<sizeof(TrainAbsRec);
    qDebug() << "sizeof(long)" <<sizeof(long);
    qDebug() << QString::number((double)141/10,'f',2);
    //-/ cai dat logger
    QDir logDir(qApp->applicationDirPath()+"/log");
    logDir.mkpath("./");
    QSettings iniFileSetings(qApp->applicationDirPath() + "/VehicleTracking.ini", QSettings::IniFormat);
    if(iniFileSetings.value("VehicleTrackingServer/LogFile", 0).toInt()>0) {
        qInstallMsgHandler(messageHandler);
    }

    VehicleTrackingServer vehicleTrackingServer;


    return a.exec();
}
