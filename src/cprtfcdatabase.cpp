#include "cprtfcdatabase.h"
QString getHeaderValue(const QString &header, const QString &field){
    int start = header.indexOf(field + "<", 0, Qt::CaseInsensitive);
    if(start>=0){
        int end = header.indexOf(">",start+field.length()+1);
        if(end>start){
            return header.mid(start+field.length()+1, end-start-field.length()-1);
        }
    }
    return "";
}
int msecsToDateTime(const QDateTime &startTime, const QDateTime &endTime){
#if QT_VERSION >= 0x040700
    return startTime.msecsTo(endTime);
#else
    return startTime.daysTo(endTime)*86400000 + startTime.time().msecsTo(endTime.time());
#endif
}
QString passwordDecode(const QString &username, const QString &passwordKey, const QString &privateKey){
    quint64 offsetValue=137;
    int i, numPack;
    quint64 packDatas[10];
    unsigned char passwordData[255];

    for(i=0;i<privateKey.toAscii().size();i++)
        offsetValue=offsetValue+(1<<i)*uchar(privateKey.toAscii().data()[i]);
    offsetValue&=0xFFFFFF;
    numPack=(passwordKey.toAscii().size()-1)/16+1;
    QByteArray passwordHexArr = passwordKey.toAscii();
    for(i=passwordHexArr.size();i<numPack*16;i++)
        passwordHexArr.append("0");
    QByteArray binArr=QByteArray::fromHex(passwordHexArr);
    memcpy(packDatas, binArr.data(), binArr.size());
    for(i=0;i<numPack;i++){
        packDatas[i]=packDatas[i]/offsetValue;
    }
    for(i=0;i<numPack*5;i++){
        if(i%5)
            passwordData[i]=(packDatas[i/5]>>((i%5)*8))&0xFF;
        else
            passwordData[i]=packDatas[i/5]&0xFF;
    }
    QByteArray resArr;
    for(i=0;i<numPack*5;i++){
        if(i<username.toUtf8().size()){
            if(uchar(username.toUtf8().data()[i])>passwordData[i])
                resArr.append(256+passwordData[i]-uchar(username.toUtf8().data()[username.toUtf8().size()-1-i]));
            else
                resArr.append(uchar(passwordData[i]-uchar(username.toUtf8().data()[username.toUtf8().size()-1-i])));
        }else
            resArr.append(uchar(passwordData[i]));
    }
    return QString::fromUtf8(resArr);
}

int messageParser(QString &recvData, CprMessage &message){
    int start = recvData.indexOf((QString)START_OF_STATION_MESSAGE,0,Qt::CaseInsensitive);
    int end = start==-1?-1:recvData.indexOf((QString)END_OF_STATION_MESSAGE,0,Qt::CaseInsensitive);
    if (start!=-1&&end!=-1){
        int startCommand=recvData.indexOf((QString)START_OF_MESSAGE_DATA, start, Qt::CaseInsensitive);
        if(start<end && startCommand < end){
            QString header = recvData.mid(start +((QString)START_OF_MESSAGE_DATA).length(), startCommand - start);
            message.Command = getHeaderValue(header, "Command");
            message.ReqId = getHeaderValue(header, "ReqId").toInt();
            message.ReqTime = QDateTime::fromString(getHeaderValue(header, "ReqTime"), "yyyy-MM-dd hh:mm:ss");
            message.From = getHeaderValue(header, "From");
            message.To = getHeaderValue(header, "To");
            message.CheckSum = getHeaderValue(header, "CheckSum");
            message.MessageData = recvData.mid(startCommand+((QString)START_OF_MESSAGE_DATA).length(),
                                                   end - startCommand-((QString)START_OF_MESSAGE_DATA).length());
            recvData.remove(0, end + ((QString)END_OF_STATION_MESSAGE).length());
            return MESSAGE_PARSER_OK;
        }else{
            return MESSAGE_PARSER_ERROR;
        }
    }
    return MESSAGE_PARSER_NONE;
}
int makeMessageData(QString &messageData, const QString &sendCommand, const QString &sendData, const int &ReqId, const QDateTime &ReqTime, const QString &From, const QString &To, const QString &CheckSum){
    messageData.clear();
    messageData.append(START_OF_STATION_MESSAGE);
    if(!sendCommand.isEmpty())
        messageData.append(QString("Command<%1>").arg(sendCommand));
    else
        return MESSAGE_PARSER_ERROR;
    if(ReqId>0)
        messageData.append(QString("ReqId<%1>").arg(ReqId));
    if(ReqTime.isValid())
        messageData.append(QString("ReqTime<%1>").arg(ReqTime.toString("yyyy-MM-dd hh:mm:ss")));
    if(!From.isEmpty())
        messageData.append(QString("From<%1>").arg(From));
    if(!To.isEmpty())
        messageData.append(QString("To<%1>").arg(To));
    if(!To.isEmpty())
        messageData.append(QString("CheckSum<%1>").arg(CheckSum));
    messageData.append(START_OF_MESSAGE_DATA);
    messageData.append(sendData);
    messageData.append(END_OF_STATION_MESSAGE);
    return MESSAGE_PARSER_OK;
}

int makeMessageData(QString &messageData, const CprMessage &sendMessage, const QString &PublicKey){
    return makeMessageData(messageData, sendMessage.Command, sendMessage.MessageData, sendMessage.ReqId, sendMessage.ReqTime, sendMessage.From, sendMessage.To, PublicKey);
}
int CprTfcDatabase::connectionCount = 0;
CprTfcDatabase::CprTfcDatabase()
{
#ifdef USE_REAL_MYSQL_CONNECTION
    myHandle = NULL;
    myDriver=NULL;
#endif
}
CprTfcDatabase::CprTfcDatabase(QString iniFile, QString iniSection, QString aliasBaseName, bool forceOpen){
    databaseState = 0;
#ifdef USE_REAL_MYSQL_CONNECTION
    myHandle = NULL;
    myDriver=NULL;
#endif
    IniFile=iniFile;
    tryOpenDatabaseTime = 5;
    opened = false;
    IniSection = iniSection;
    AliasName = aliasBaseName + QString::number(connectionCount++);
    lastOpenDatabase = QDateTime();
    QSettings iniFileSetings(IniFile, QSettings::IniFormat);
    iniFileSetings.beginGroup(iniSection);
    debugLog = iniFileSetings.value("DebugLog",1).toInt();
    debugFileName = iniFileSetings.value("DebugFile","").toString();
    slowQueryLog = iniFileSetings.value("SlowQueryLog",0).toInt();
    debugFileName.replace("<Alias>",aliasBaseName);
    tryOpenDatabaseTime = iniFileSetings.value(iniSection + "/" + "TryOpenDatabaseTime",5).toInt();
    iniFileSetings.endGroup();
    if(tryOpenDatabaseTime<1)
        tryOpenDatabaseTime=1;
    if(tryOpenDatabaseTime>300)
        tryOpenDatabaseTime=300;
    if(forceOpen)
        openDatabase();
}
void CprTfcDatabase::writeDebug(const QString &msg){
    if (debugFileName.length()>0) {
        QString datetimeFile = QDateTime::currentDateTime().toString(debugFileName);
        QFile debugLogFile(datetimeFile);
        if (debugLogFile.open(QFile::Append | QFile::Text)) {
            QTextStream out(&debugLogFile);
            out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " " << msg << "\n";
            debugLogFile.close();
        }
        return;
    }else
        qDebug() << msg;
}
int CprTfcDatabase::openDatabase(bool force){
    if(lastOpenDatabase>QDateTime::currentDateTime())
        lastOpenDatabase=QDateTime::currentDateTime();
    if(!force && lastOpenDatabase.isValid() && lastOpenDatabase.secsTo(QDateTime::currentDateTime())<tryOpenDatabaseTime){
        return 0;
    }
    opened = false;
    if(localDatabase.isValid() && localDatabase.isOpen()){
        QSqlQuery pingQuery("select 1", localDatabase);
        if(pingQuery.isActive()){
            databaseErrorString.clear();;
            opened = true;
            if(databaseState != 1){
                databaseState = 1;
                emit sqlError(DATABASE_OPEN, QString("HostName=%1, UserName=%2, DatabaseName=%3, AliasName=%4").arg(localDatabase.hostName()).arg(localDatabase.userName()).arg(localDatabase.databaseName()).arg(AliasName));
                if(debugLog&1)
                    writeDebug("Open DatabaseName = " + localDatabase.databaseName() + ",HostName = " + localDatabase.hostName() + ",AliasName =" + AliasName + ",Ok");
            }
            lastOpenDatabase = QDateTime::currentDateTime();
            return 2;
        }
    }
    QSettings iniFileSetings(IniFile, QSettings::IniFormat);
    iniFileSetings.beginGroup(IniSection);
    tryOpenDatabaseTime = iniFileSetings.value("TryOpenDatabaseTime",5).toInt();
    debugLog = iniFileSetings.value("DebugLog",1).toInt();
    slowQueryLog = iniFileSetings.value("SlowQueryLog",0).toInt();
    if(tryOpenDatabaseTime<1)
        tryOpenDatabaseTime=1;
    if(tryOpenDatabaseTime>300)
        tryOpenDatabaseTime=300;
    SqlDriverName =iniFileSetings.value("SqlDriverName","QMYSQL").toString();
    if(SqlDriverName == "QMYSQL" && !QSqlDatabase::isDriverAvailable("QMYSQL")){
        qDebug() << "Can't load QMYSQL driver ::exit(1)";
        ::exit(1);
    }

    if(SqlDriverName == "QMYSQL" && QSqlDatabase::isDriverAvailable("QMYSQL")){
        if(!lastOpenDatabase.isValid())
            localDatabase=QSqlDatabase::addDatabase("QMYSQL",AliasName);
        lastOpenDatabase = QDateTime::currentDateTime();
        localDatabase.setHostName(iniFileSetings.value("HostName","localhost").toString());
        localDatabase.setPort(iniFileSetings.value("Port",3306).toInt());
        localDatabase.setDatabaseName(iniFileSetings.value("DatabaseName","CadProTFC").toString());
        localDatabase.setUserName(iniFileSetings.value("UserName","app").toString());
        localDatabase.setPassword( passwordDecode(localDatabase.userName(), iniFileSetings.value("Password","cadpro").toString(), "CadProTFC"));
        iniFileSetings.endGroup();
        if(!localDatabase.open()){
            databaseErrorString = localDatabase.lastError().text();
            if(debugLog&1)
                writeDebug("Open DatabaseName = " + localDatabase.databaseName() + ",HostName = " + localDatabase.hostName() + ",AliasName =" + AliasName + ",Error :" + databaseErrorString);
            if(databaseState != -1){
                databaseState = -1;
                emit sqlError(DATABASE_ERROR, databaseErrorString);
            }
            return -1;
        }
        localQuery = QSqlQuery(localDatabase);
    }
#ifdef USE_REAL_MYSQL_CONNECTION
    if(SqlDriverName == "MYSQL"){
        lastOpenDatabase = QDateTime::currentDateTime();
        QString HostName = iniFileSetings.value("HostName","localhost").toString();
        int Port = iniFileSetings.value("Port",3306).toInt();
        QString DatabaseName = iniFileSetings.value("DatabaseName","CadProTFC").toString();
        QString UserName = iniFileSetings.value("UserName","app").toString();
        QString Password = passwordDecode(UserName, iniFileSetings.value("Password","cadpro").toString(), "CadProTFC");
        int connect_timeout = iniFileSetings.value("MYSQL_CONNECT_TIMEOUT",10).toInt();
        int read_timeout = iniFileSetings.value("MYSQL_READ_TIMEOUT",10).toInt();
        int write_timeout = iniFileSetings.value("MYSQL_WRITE_TIMEOUT",10).toInt();        
        iniFileSetings.endGroup();

        if(myHandle){
            mysql_options((MYSQL *)myHandle,  MYSQL_OPT_CONNECT_TIMEOUT,  (const char *)&connect_timeout);
            mysql_options((MYSQL *)myHandle,  MYSQL_OPT_READ_TIMEOUT,  (const char *)&read_timeout);
            mysql_options((MYSQL *)myHandle,  MYSQL_OPT_WRITE_TIMEOUT,  (const char *)&write_timeout);
            if(!mysql_real_connect ( (MYSQL *)myHandle, HostName.toAscii().data(), UserName.toAscii().data(), Password.toAscii().data(), DatabaseName.toAscii().data(), Port, NULL, 0)){
                databaseErrorString = mysql_error((MYSQL *)myHandle);
            }else
                if (mysql_select_db((MYSQL *)myHandle, DatabaseName.toAscii().data())) {
                    databaseErrorString = mysql_error((MYSQL *)myHandle);
                }
        }else{
            MYSQL *connection = mysql_init(NULL);
            if (connection != NULL) {
                mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT,  (const char *)&connect_timeout);
                mysql_options(connection, MYSQL_OPT_READ_TIMEOUT,  (const char *)&read_timeout);
                mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT,  (const char *)&write_timeout);
                if(!mysql_real_connect ( connection, HostName.toAscii().data(), UserName.toAscii().data(), Password.toAscii().data(), DatabaseName.toAscii().data(), Port, NULL, 0)){
                    databaseErrorString = mysql_error(connection);
                    mysql_close(connection);
                    connection = NULL;
                }else
                    if (mysql_select_db(connection, DatabaseName.toAscii().data())) {
                        databaseErrorString = mysql_error(connection);
                        mysql_close(connection);
                        connection = NULL;
                    }
                if(connection)
                    mysql_set_character_set(connection, "utf8");
            }
            myHandle = connection;
            if (myHandle != NULL) {
                myDriver = new QMYSQLDriver((MYSQL *)myHandle);
                localDatabase = QSqlDatabase::addDatabase((QMYSQLDriver *)myDriver, AliasName);
                localDatabase.setHostName(HostName);
                localDatabase.setDatabaseName(DatabaseName);
                localDatabase.setUserName(UserName);
                localDatabase.setPassword(Password);
                localQuery = QSqlQuery(localDatabase);
            }else{
                if(debugLog&1)
                    writeDebug("Open DatabaseName = " + localDatabase.databaseName() + ",HostName = " + localDatabase.hostName() + ",AliasName =" + AliasName + ",Error :" + databaseErrorString);
                if(databaseState != -1){
                    databaseState = -1;
                    emit sqlError(DATABASE_ERROR, databaseErrorString);
                }
                return -1;
            }
        }
    }
#endif
    if(SqlDriverName == "QMYSQL")
        databaseErrorString = localDatabase.lastError().text();
    if(localDatabase.isOpen()){
        QSqlQuery pingQuery("select 1", localDatabase);
        if(pingQuery.isActive()){
            if(debugLog&1)
                writeDebug("Open DatabaseName = " + localDatabase.databaseName() + ",HostName = " + localDatabase.hostName() + ",AliasName =" + AliasName + ",Ok");
            databaseErrorString.clear();
            opened = true;
            if(databaseState != 1){
                databaseState = 1;
                emit sqlError(DATABASE_OPEN, QString("HostName=%1, UserName=%2, DatabaseName=%3, AliasName=%4").arg(localDatabase.hostName()).arg(localDatabase.userName()).arg(localDatabase.databaseName()).arg(AliasName));
            }
            return 1;
        }else{
            if(debugLog&1)
                writeDebug("Open DatabaseName = " + localDatabase.databaseName() + ",HostName = " + localDatabase.hostName() + ",AliasName =" + AliasName + ",Error :" + databaseErrorString);
        }
    }
    if(databaseState != -1){
        databaseState = -1;
        emit sqlError(DATABASE_ERROR, databaseErrorString);
    }
    return -1;
}
CprTfcDatabase::~CprTfcDatabase(){
    if(debugLog&1)
        qDebug() << "~CprTfcDatabase" << AliasName;
    localQuery.clear();
    localDatabase.close();
    QSqlDatabase::removeDatabase(AliasName);
}

bool CprTfcDatabase::startTransaction() {
    localQuery.clear();
    if (localDatabase.isOpen()) {
        return localDatabase.transaction();
    }
    return false;
}

bool CprTfcDatabase::doCommit() {
    localQuery.clear();
    if (localDatabase.isOpen()) {
        return localDatabase.commit();
    }
    return false;
}

bool CprTfcDatabase::doRollback() {
    localQuery.clear();
    if (localDatabase.isOpen()) {
        return localDatabase.rollback();
    }
    return false;
}

bool CprTfcDatabase::execQuery(const QString& str)
{
    localQuery.clear();
    sqlState=0;
    if (!localDatabase.isOpen() && openDatabase(true)!=1)
        return false;
    localQuery = QSqlQuery(localDatabase);
    QDateTime startQuery = QDateTime::currentDateTime();
    if (localQuery.exec(str)){
        sqlState=1;
        if(slowQueryLog){
            int queryTime = msecsToDateTime(startQuery, QDateTime::currentDateTime());
            if(queryTime>queryTime)
                writeDebug(QString("Exec Time[%1]Query[%2]").arg(queryTime).arg(str));
        }
        if(debugLog&2)
            writeDebug(QString("Exec Query[%1]RecordCout[%2]").arg(str).arg(localQuery.size()));
        sqlErrorString.clear();
        return true;
    }else{
        sqlState = -1;        
        if(openDatabase(true)==1){
            startQuery = QDateTime::currentDateTime();
            if (localQuery.exec(str)){
                sqlState=1;
                if(slowQueryLog){
                    int queryTime = msecsToDateTime(startQuery, QDateTime::currentDateTime());
                    if(queryTime>queryTime)
                        writeDebug(QString("Exec Time[%1]Query[%2]").arg(queryTime).arg(str));
                }
                if(debugLog&2)
                    writeDebug(QString("Exec Query[%1]RecordCout[%2]").arg(str).arg(localQuery.size()));
                sqlErrorString.clear();
                return true;
            }
        }else
            sqlState=0;
        if(debugLog&1)
            writeDebug(QString("Exec Query[%1], error[%2], host[%3], database[%4], alias[%5]").arg(str).arg(localQuery.lastError().text()).arg(localDatabase.hostName()).arg(localDatabase.databaseName()).arg(AliasName));
        sqlErrorString = localQuery.lastError().text();
        emit sqlError(EXEC_SQL_ERROR, sqlErrorString);
        return false;
    }
}
bool CprTfcDatabase::execQuery(QSqlQuery &query, const QString &str)
{
    sqlState=0;
    if (!localDatabase.isOpen() && openDatabase(true)!=1)
        return false;
    query = QSqlQuery(localDatabase);
    QDateTime startQuery = QDateTime::currentDateTime();
    if (query.exec(str)){
        sqlState=1;
        if(slowQueryLog){
            int queryTime = msecsToDateTime(startQuery, QDateTime::currentDateTime());
            if(queryTime>queryTime)
                writeDebug(QString("Exec Time[%1]Query[%2]").arg(queryTime).arg(str));
        }
        if(debugLog&2)
            writeDebug(QString("Exec Query[%1]RecordCout[%2]").arg(str).arg(localQuery.size()));
        sqlErrorString.clear();
        return true;
    }else{
        sqlState = -1;
        if(openDatabase(true)==1){
            startQuery = QDateTime::currentDateTime();
            if (query.exec(str)){
                sqlState=1;
                if(slowQueryLog){
                    int queryTime = msecsToDateTime(startQuery, QDateTime::currentDateTime());
                    if(queryTime>queryTime)
                        writeDebug(QString("Exec Time[%1]Query[%2]").arg(queryTime).arg(str));
                }
                if(debugLog&2)
                    writeDebug(QString("Exec Query[%1]RecordCout[%2]").arg(str).arg(localQuery.size()));
                sqlErrorString.clear();
                return true;
            }
        }else
            sqlState=0;
        if(debugLog&1)
            writeDebug(QString("Exec Query[%1], error[%2], host[%3], database[%4], alias[%5]").arg(str).arg(query.lastError().text()).arg(localDatabase.hostName()).arg(localDatabase.databaseName()).arg(AliasName));
        sqlErrorString = query.lastError().text();
        emit sqlError(EXEC_SQL_ERROR, sqlErrorString);
        return false;
    }
}


