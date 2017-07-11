#ifndef TRENDWRITER_H
#define TRENDWRITER_H
#include "autostopthread.h"
#include "scadaserver.h"
#include "nodes.h"
#include "logger.h"
#include <QHash>
#include <QTcpServer>

class ScadaServer;
class CommonTrend;
class Logger;

struct trend_query_struct
{
    char fileName[32];
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int count;
};

class TrendWriter: public AutoStopThread
{
    Q_OBJECT
private:
    ScadaServer *ss;
    Logger *logger;
    void run();
    void FuncFileWriter(CommonTrend *this_trend_info, char *str_date, uint time_pos);
public:
    TrendWriter();
    ~TrendWriter();
//    QHash< QString, QHash<QString, CommonTrend *> > hashCommonTrends;
    QTcpServer* m_pTrendServerSocket;
    QList<QTcpSocket*> m_pTrendClientSocketList;
    void StartTrendServer(uint trendServerPort);
    uint m_pTrendServerPort;


public slots:
    //tcp server
    // Slot to handle disconnected client
    void ClientDisconnected();
    void ClientWrite();

private slots:
    // New client connection
    void NewConnection();

};

#endif // TRENDWRITER_H
