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
    //-/ cai dat logger
    QDir logDir(qApp->applicationDirPath()+"/log");
    logDir.mkpath("./");
    QSettings iniFileSetings(qApp->applicationDirPath() + "/VehicleTracking.ini", QSettings::IniFormat);
    if(iniFileSetings.value("VehicleTrackingServer/LogFile", 0).toInt()>0) {
        qInstallMsgHandler(messageHandler);
    }

    //VehicleTrackingServer vehicleTrackingServer(QDateTime::fromString(QDateTime::currentDateTime().toString("yyyy-MM-dd"), "yyyy-MM-dd").addDays(-60), 0, 1235);
    VehicleTrackingServer vehicleTrackingServer;


    return a.exec();
}
