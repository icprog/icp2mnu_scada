#include "trendwriter.h"

//=========================================================================================
TrendWriter::TrendWriter()
{
    ss=ScadaServer::Instance();
    logger=Logger::Instance();
}
//========================================================================================
void TrendWriter::StartTrendServer(uint trendServerPort)
{

    m_pTrendServerPort=trendServerPort;
    logger->AddLog(QString("ТРЕНДЫ: Старт подсистемы на ") + QString::number(m_pTrendServerPort)+ " порту", Qt::darkBlue);
    //tcp server
        m_pTrendServerSocket = new QTcpServer(this);
        m_pTrendServerSocket->listen(QHostAddress::Any, m_pTrendServerPort);

        connect(m_pTrendServerSocket, SIGNAL(newConnection()), this, SLOT(NewConnection()));
}
//========================================================================================
TrendWriter::~TrendWriter()
{
    delete m_pTrendServerSocket;
}
//========================================================================================
void TrendWriter::FuncFileWriter(CommonTrend *this_trend_info, char *str_date, uint time_pos)
{
    QString filename;

    filename.sprintf("%s%s\\%s_%s.trn",ss->trend_path,this_trend_info->m_objectName.toStdString().c_str(),this_trend_info->m_trendName.toStdString().c_str(),str_date);

    QFile trend(filename);

    if (!trend.exists())  //Open(filename,CFile::modeWrite|CFile::modeNoTruncate|CFile::shareDenyNone,NULL))
    {

        if (trend.open(QIODevice::ReadWrite))
        {
            trend.write((char *)ss->empty_file,69120);
            trend.seek(time_pos*4);//, CFile::begin);
            trend.write((char *)&(ss->hashCommonNodes[this_trend_info->m_objectName]->m_srv.buff[this_trend_info->m_numInBuff]),4);
            trend.close();
        }
    }
    else
    {
        trend.open(QIODevice::ReadWrite);
        trend.seek(time_pos*4);//, CFile::begin);
        trend.write((char *)&(ss->hashCommonNodes[this_trend_info->m_objectName]->m_srv.buff[this_trend_info->m_numInBuff]),4);
        trend.close();
    }

    this_trend_info->AddLastValue(ss->hashCommonNodes[this_trend_info->m_objectName]->m_srv.buff[this_trend_info->m_numInBuff]);

    return;
}

//=========================================================================================
void TrendWriter::run()
{

    int itime,iprevtime=-1;

    uint tr_cnt=0;
    //int tr_number=ss->vectCommonTrends.size();

    for(;;)
    {
        Sleep(200);

        QDateTime currDateTime=QDateTime::currentDateTime();

        itime=currDateTime.time().second() / 5;
        if (itime!=iprevtime)
        {
            char buf_date[20];
            int time22=(currDateTime.time().hour()*60*60+currDateTime.time().minute()*60+currDateTime.time().second())/5;
            snprintf(buf_date,20,"%.2u_%.2u_%.4u",currDateTime.date().day(),currDateTime.date().month(),currDateTime.date().year());

            foreach (CommonTrend *tr,ss->vectCommonTrends)
            {

                if (/*ss->hashCommonNodes.contains(tr->m_objectName) && */ss->hashCommonNodes[tr->m_objectName]->m_isReaded)
                {
                    FuncFileWriter(tr,buf_date,time22);
                }


                tr_cnt++;  //уменьшение загрузки CPU,                
                if (tr_cnt >= 5)
                {
                    tr_cnt=0;
                    msleep(10);
                }

                //usleep(2500000 / tr_number); //растянем запись трендов на 2,5 сек.
            }
        }
        iprevtime=itime;
        if (CheckThreadStop()) return;
    }

}
//===========================TCP SERVER=====================================================

//=================================================================
void TrendWriter::NewConnection()
{
    QTcpSocket* pClient = m_pTrendServerSocket->nextPendingConnection();
    pClient->setSocketOption(QAbstractSocket:: KeepAliveOption, 1);
    m_pTrendClientSocketList.push_back(pClient);

    connect(pClient, SIGNAL(disconnected()), this, SLOT(ClientDisconnected()));
    connect(pClient,SIGNAL(readyRead()),this,SLOT(ClientWrite()));
    //lastClient=pClient;

    logger->AddLog("ТРЕНДЫ: К серверу трендов подключился "+pClient->peerAddress().toString(),Qt::darkBlue);
    //alarmDB->AddAlarm2DB("До сервера алармів під`єднався "+pClient->peerAddress().toString());

}
//======================================================================================
void TrendWriter::ClientDisconnected()
{
    // client has disconnected, so remove from list
    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());
    m_pTrendClientSocketList.removeOne(pClient);
    logger->AddLog("ТРЕНДЫ: От сервера трендов отключился "+pClient->peerAddress().toString(),Qt::darkBlue);
    //alarmDB->AddAlarm2DB("Від сервера алармів від`єднався "+pClient->peerAddress().toString());

}
//===========================================================================================================
void TrendWriter::ClientWrite()
{
    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());

    qDebug() << "trends reading form client..."+QString::number(pClient->bytesAvailable());
    trend_query_struct trend_query;

    // read the data from the socket
    if (pClient->bytesAvailable() == sizeof(trend_query_struct))
    {
        pClient->read((char *)&trend_query,sizeof(trend_query_struct));

        //TO DO: send trend data to client


        if (strchr(trend_query.fileName,'\\')==0 && strchr(trend_query.fileName,'/')==0)
        {
            qDebug() << trend_query.fileName << "-  error in filename format? must be obj/trend or object\\trend";
            pClient->disconnectFromHost();
            return;
        }

        char objName[32];
        char trName[32];

        if (strchr(trend_query.fileName,'\\'))
        {
            sscanf(trend_query.fileName,"%s\\%s",objName,trName);
        }
        if (strchr(trend_query.fileName,'/'))
        {
            sscanf(trend_query.fileName,"%s/%s",objName,trName);
        }




        //Если значения полностью есть в онлайн-буфере(проверять на мин_флоат) -> брать оттуда

        QString filename;
        static float file_buff[17280];
        uint data_offset=(trend_query.hour*60*60 + trend_query.minute*60 + trend_query.second) / 5;
        uint data_length=trend_query.count;

        filename.sprintf("%s%s_%.2u_%.2u_%.4u.trn",ss->trend_path,trend_query.fileName,trend_query.day,trend_query.month,trend_query.year);

        QFile trend(filename);

       // buffer_position=(paint_trend_info->endHour*60*60+paint_trend_info->endMinute*60+paint_trend_info->endSecond-
       //     paint_trend_info->startHour*60*60-paint_trend_info->startMinute*60-paint_trend_info->startSecond)/5;

        if (!trend.open(QIODevice::ReadOnly))
        {
            for (int j=0;j<data_length;j++) file_buff[j]=ss->min_float;
            pClient->write((char *)file_buff,data_length*4);
        }
        else
        {
            if (trend.seek(data_offset*4))
            {
                if (trend.read((char *)file_buff,data_length*4))
                {
                    pClient->write((char *)file_buff,data_length*4);
                }
                else
                {
                    for (int j=0;j<data_length;j++) file_buff[j]=ss->min_float;
                    pClient->write((char *)file_buff,data_length*4);
                }
            }
            else
            {
                for (int j=0;j<data_length;j++) file_buff[j]=ss->min_float;
                pClient->write((char *)file_buff,data_length*4);
            }
            trend.close();
        }





        qDebug() << trend_query.fileName << "  " << trend_query.day << "." << trend_query.month << "."  << trend_query.year <<
                    "  " << trend_query.hour << ":" << trend_query.minute << ":" << trend_query.second << "  count:" << trend_query.count;

    }
    else
    {
        pClient->disconnectFromHost();
    }

}
//====================================================================================
