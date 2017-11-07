#ifndef TRENDSERVER_H
#define TRENDSERVER_H

#include <QTcpServer>
#include <QThread>
#include <QTcpSocket>
#include <QFile>
#include <QDebug>

#include "scadaserver.h"
#include "logger.h"

class ScadaServer;
class Logger;

//==================================================================

struct trend_query_struct
{
    char fileName[64];
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int count;
};

//===================================================================
class TrendServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TrendServer(QObject *parent = 0);
    virtual ~TrendServer();
    void StartTrendServer(uint tcpPort);

private:
    ScadaServer *ss;
    Logger *logger;

    uint m_TrendServerPort;

protected:
    void incomingConnection(qintptr socketDescriptor);

signals:
    void newMessage(QString message);

public slots:
    void NewMessageFromWorker(QString message);



};

//=====================================================================
class Worker : public QThread
{
    Q_OBJECT
public:
    explicit Worker(qintptr ID, ScadaServer *scada_server, QObject *parent = 0);
    void run();

signals:
    void error(QTcpSocket::SocketError socketerror);
    void newMessage(QString message);

public slots:
    void readyRead();
    void disconnected();



private:
    QTcpSocket *pClient;
    qintptr socketDescriptor;
    ScadaServer *ss;

};
//======================================================================


#endif // TRENDSERVER_H
