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
    //-/ cai dat logger
    QDir logDir(qApp->applicationDirPath()+"/log");
    logDir.mkpath("./");
    //qInstallMsgHandler(messageHandler);

    //VehicleTrackingServer vehicleTrackingServer(QDateTime::fromString(QDateTime::currentDateTime().toString("yyyy-MM-dd"), "yyyy-MM-dd").addDays(-60), 0, 1235);
    VehicleTrackingServer vehicleTrackingServer;

    return a.exec();
}
