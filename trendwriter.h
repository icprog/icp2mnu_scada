#ifndef TRENDWRITER_H
#define TRENDWRITER_H
#include "autostopthread.h"
#include "scadaserver.h"
#include "nodes.h"
#include "logger.h"
#include <QHash>


class ScadaServer;
class CommonTrend;
class Logger;



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


public slots:



private slots:


};

#endif // TRENDWRITER_H
