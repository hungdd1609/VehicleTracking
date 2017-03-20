#ifndef CPRTFCDATABASE_H
#define CPRTFCDATABASE_H
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QDateTime>
#include <QTimer>
#include <QSettings>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#define fieldByName(query, name) query.value(query.record().indexOf(name))

#ifdef USE_REAL_MYSQL_CONNECTION
#include <mysql/mysql.h>
#include "qsql_mysql.h"
#endif

#define MESSAGE_PARSER_NONE  0
#define MESSAGE_PARSER_OK    1
#define MESSAGE_PARSER_ERROR 2

#define SERVER_STARTSUCCESS     1
#define SERVER_HASERRORS        2

#define BANK_PARTNER            1
#define ETC_PARTNER             2

#define LAN_THU_PHI_MO          0
#define LAN_VAO_THU_PHI_KIN     1
#define LAN_RA_THU_PHI_KIN      2

#define START_OF_BANK_MESSAGE   "<ISO8583>"
#define END_OF_BANK_MESSAGE   "</ISO8583>"

#define START_OF_STATION_MESSAGE   "<STATIONMSG>"
#define END_OF_STATION_MESSAGE   "</STATIONMSG>"
#define START_OF_MESSAGE_DATA   "Data:"

#define PING_REQUEST_HEADER   "PING_REQUEST"
#define SOATVE_REQUEST_HEADER   "SOATVE_REQUEST"
#define THEETC_REQUEST_HEADER   "THEETC_REQUEST"
#define TAIKHOAN_REQUEST_HEADER "TAIKHOAN_REQUEST"
#define LUOTVAO_REQUEST_HEADER "LUOTVAO_REQUEST"
#define LUOTRA_REQUEST_HEADER "LUOTRA_REQUEST"
#define CATRAM_REQUEST_HEADER "CATRAM_REQUEST"
#define THIETBI_REQUEST_HEADER "THIETBI_REQUEST"
#define LOAIVE_REQUEST_HEADER "LOAIVE_REQUEST"
#define TRAM_REQUEST_HEADER "TRAM_REQUEST"
#define LOAIXE_REQUEST_HEADER "LOAIXE_REQUEST"
#define XEVAO_REQUEST_HEADER "XEVAO_REQUEST"
#define BANGXE_REQUEST_HEADER "BANGXE_REQUEST"
#define ETCPASS_REQUEST_HEADER "ETCPASS_REQUEST"

#define PING_RESPONSE_HEADER   "PING_RESPONSE"
#define SOATVE_RESPONSE_HEADER   "SOATVE_RESPONSE"
#define THEETC_RESPONSE_HEADER   "THEETC_RESPONSE"
#define TAIKHOAN_RESPONSE_HEADER "TAIKHOAN_RESPONSE"
#define LUOTVAO_RESPONSE_HEADER "LUOTVAO_RESPONSE"
#define LUOTRA_RESPONSE_HEADER "LUOTRA_RESPONSE"
#define CATRAM_RESPONSE_HEADER "CATRAM_RESPONSE"
#define THIETBI_RESPONSE_HEADER "THIETBI_RESPONSE"
#define TRAM_RESPONSE_HEADER "TRAM_RESPONSE"
#define LOAIXE_RESPONSE_HEADER "LOAIXE_RESPONSE"
#define LOAIVE_RESPONSE_HEADER "LOAIVE_RESPONSE"
#define XEVAO_RESPONSE_HEADER "XEVAO_RESPONSE"
#define ETCPASS_RESPONSE_HEADER "ETCPASS_RESPONSE"

#define DATABASE_OPEN       1
#define DATABASE_ERROR      2
#define EXEC_SQL_ERROR      3

struct CprMessage{
    int ReqId;
    QDateTime ReqTime;
    QString From;
    QString To;
    QString CheckSum;
    QString Command;
    QString MessageData;
    CprMessage(){
        ReqId = 0;
        ReqTime =QDateTime();
        From.clear();
        To.clear();
        CheckSum.clear();
        MessageData;
    }
};
struct CprMessageIndex{
    int ReqId;
    QString ReqCommand;
    QString ReqKey;
    QDateTime ReqTime;
    QString ReqData;
    CprMessageIndex(){
        ReqId = 0;
        ReqCommand.clear();
        ReqKey.clear();
        ReqTime =QDateTime();
        ReqData.clear();
    }
    CprMessageIndex(const int &id, const QString &cmd, const QString &key, const QDateTime &time=QDateTime::currentDateTime(), const QString &reqData=""){
        ReqId = id;
        ReqCommand = cmd;
        ReqKey = key;
        ReqTime = time;
        ReqData = reqData;
    }
};
class CprTfcDatabase:   public QObject
{
    Q_OBJECT
public:
    CprTfcDatabase();
    CprTfcDatabase(QString iniFile, QString iniSection, QString aliasBaseName, bool forceOpen=true);
    ~CprTfcDatabase();
    bool execQuery(const QString &str);
    bool execQuery(QSqlQuery &query, const QString &str);
    bool execQuery(const QString &str, QSqlQuery &query){
        return execQuery(query, str);
    }
    bool startTransaction();
    bool doCommit();
    bool doRollback();
    int openDatabase(bool force=false);
    bool isOpen(){return opened;}
    QString getSqlErrorString(){return sqlErrorString;}
private:
    QSqlDatabase localDatabase;
    QSqlQuery localQuery;
    QString IniFile, IniSection, AliasName, SqlDriverName;
    QDateTime lastOpenDatabase;
    int tryOpenDatabaseTime;
    int debugLog;
    QString debugFileName;
    int slowQueryLog;
#ifdef USE_REAL_MYSQL_CONNECTION
    MYSQL *myHandle;
    QMYSQLDriver *myDriver;
#endif
    QString sqlErrorString;
    QString databaseErrorString;
    int sqlState;
    int databaseState;
    bool opened;
    static int connectionCount;
    void writeDebug(const QString &msg);
signals:
    void sqlError(int errorCode, QString errorString);
};
QString getHeaderValue(const QString &header, const QString &field);
int messageParser(QString &recvData, CprMessage &message);
int checkMessag(CprMessage &message, const QString &publicKey);
int makeMessageData(QString &messageData, const QString &sendCommand, const QString &sendData, const int &ReqId, const QDateTime &ReqTime, const QString &From, const QString &To, const QString &PublicKey);
int makeMessageData(QString &messageData, const CprMessage &sendMessage, const QString &PublicKey);

#endif // CprTfcDatabase_H
