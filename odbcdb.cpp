#include "odbcdb.h"


//======================================================================
OdbcDb::OdbcDb(QString odbcName,QString user, QString pass)
{
    logger=Logger::Instance();
    this->odbcName=odbcName;
    this->user=user;
    this->pass=pass;
    counter=0;
}
//=======================================================================
OdbcDb::~OdbcDb()
{
}
//=======================================================================
QString OdbcDb::GetNextName()
{
    QString res;
    counterMutex.lock();
    res.sprintf("%u",counter);
    counter=(counter+1) % 1000000;
    counterMutex.unlock();
    return res;
}
//=======================================================================
bool OdbcDb::ExecQuery(QString query)
{
    bool res=true;
    QString connectionName=GetNextName();


    {
        // start of the block where the db object lives

        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);

        db.setDatabaseName(odbcName);
        db.setUserName(user);
        db.setPassword(pass);

        if (db.open())
        {
            QSqlQuery sqlQuery(db);
            sqlQuery.exec(query);

            if (sqlQuery.lastError().isValid())
            {
                logger->AddLog("ODBC: Ошибка "+sqlQuery.lastError().text(),Qt::red);
                res=false;
            }

        }
        else
        {
            logger->AddLog("ODBC: Ошибка "+db.lastError().text(),Qt::red);
            res=false;
        }
        if (res) logger->AddLog("ODBC: Выполнено :"+query,Qt::darkGreen);
        db.close();
    } // end of the block where the db object lives, it will be destroyed here

    QSqlDatabase::removeDatabase(connectionName);

    return res;
}
//=======================================================================
void OdbcDb::AddAlarm2DB(QString what)
{
    QString strDateTime;
    QDateTime dt;
    dt=QDateTime::currentDateTime();
    strDateTime.sprintf("%.2u.%.2u.%.4u %.2u:%.2u:%.2u.%.3u",
                        dt.date().day(),dt.date().month(),dt.date().year(),
                        dt.time().hour(),dt.time().minute(),dt.time().second(),dt.time().msec());


    ExecQuery("INSERT INTO ALARMS(DT,ALARMTEXT) VALUES('" + strDateTime + "', '" + what + "')");


}
//============================================================================
void OdbcDb::AddEvent2DB(QDateTime dt, QString object, QString type, QString what)
{
    QString strDateTime;
    strDateTime.sprintf("%.2u.%.2u.%.4u %.2u:%.2u:%.2u.%.3u",
                        dt.date().day(),dt.date().month(),dt.date().year(),
                        dt.time().hour(),dt.time().minute(),dt.time().second(),dt.time().msec());


    ExecQuery("INSERT INTO EVENTS(DT,EVENTOBJECT,EVENTTYPE,EVENTTEXT) VALUES('" + strDateTime + "', '"+object+"', '" + type +"', '"+what + "')");

}
//============================================================================
bool OdbcDb::ReadLastEventsFromDB(QVector<QString> &vect_dt, QVector<QString> &vect_type, QVector<QString> &vect_text, uint maxcount)
{
    bool res=true;
    QString connectionName=GetNextName();


    {
        // start of the block where the db object lives

        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);

        db.setDatabaseName(odbcName);
        db.setUserName(user);
        db.setPassword(pass);

        QString query;
        query="SELECT FIRST "+QString::number(maxcount)+" * FROM EVENTS ORDER BY DT DESC";

        if (db.open())
        {
            QSqlQuery sqlQuery(db);

            sqlQuery.exec(query);

            if (sqlQuery.lastError().isValid())
            {
                logger->AddLog("ODBC: Ошибка "+sqlQuery.lastError().text(),Qt::red);
                res=false;
            }

            if (res)
            {
                QSqlRecord rec = sqlQuery.record();

                while (sqlQuery.next())
                {
                    QString str_dt_value = sqlQuery.value(rec.indexOf("DT")).toDateTime().toString("dd.MM.yyyy hh:mm:ss");
                    QString str_type_value=sqlQuery.value(rec.indexOf("EVENTTYPE")).toString();
                    QString str_text_value=sqlQuery.value(rec.indexOf("EVENTTEXT")).toString();

                    vect_dt.push_front(str_dt_value);
                    vect_type.push_front(str_type_value);
                    vect_text.push_front(str_text_value);

                }
            }


        }
        else
        {
            logger->AddLog("ODBC: Ошибка "+db.lastError().text(),Qt::red);
            res=false;
        }

        if (res) logger->AddLog("ODBC: Выполнено :"+query,Qt::darkGreen);
        db.close();
    } // end of the block where the db object lives, it will be destroyed here

    QSqlDatabase::removeDatabase(connectionName);

    return res;
}
//============================================================================
/*
CREATE TABLE ALARMS
(
ID INTEGER,
DT TIMESTAMP,
ALARMTEXT VARCHAR(128),
PRIMARY KEY(ID)
);
+ autoincrement generator trigger on ID

CREATE TABLE EVENTS
(
ID INTEGER,
DT TIMESTAMP,
EVENTOBJECT VARCHAR(32),
EVENTTYPE VARCHAR(32),
EVENTTEXT VARCHAR(128),
PRIMARY KEY(ID)
);
+ autoincrement generator trigger on ID

*/
//============================================================================
