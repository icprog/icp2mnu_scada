#ifndef ODBCDB_H
#define ODBCDB_H


#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QVariant>
#include <QMessageBox>
#include <QMutex>
#include <QDateTime>
#include "logger.h"


class OdbcDb
{
private:
//    QSqlDatabase *db;
    QString odbcName;
    QString user;
    QString pass;
    unsigned int counter;
    QMutex counterMutex;
    QString GetNextName();
public:
    Logger *logger;
    OdbcDb(QString odbcName,QString user, QString pass);
    ~OdbcDb();
    bool ExecQuery(QString query);
    void AddAlarm2DB(QString what);
    void AddEvent2DB(QDateTime dt, QString object, QString type, QString what);
    bool ReadLastEventsFromDB(QVector<QString> &vect_dt, QVector<QString> &vect_type, QVector<QString> &vect_text, uint maxcount);

};

#endif // ODBCDB_H
