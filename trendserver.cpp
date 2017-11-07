#include "trendserver.h"

TrendServer::TrendServer(QObject *parent)
{
    ss=ScadaServer::Instance();
    logger=Logger::Instance();
}
//==============================================================
TrendServer::~TrendServer()
{

}
//=================================================================
void TrendServer::StartTrendServer(uint tcpPort)
{

    m_TrendServerPort=tcpPort;

    if(!this->listen(QHostAddress::Any,m_TrendServerPort))
    {
        logger->AddLog(QString("ТРЕНДЫ: ОШИБКА СТАРТА на ") + QString::number(m_TrendServerPort)+ " порту", Qt::red);
        qDebug() << "Could not start server";
        emit newMessage("Could not start TREND server");
    }
    else
    {
        logger->AddLog(QString("ТРЕНДЫ: Старт подсистемы на ") + QString::number(m_TrendServerPort)+ " порту", Qt::darkBlue);
        emit newMessage("Trend Server listening to port "+QString::number(m_TrendServerPort) + "...");
    }

}
//=================================================================
void TrendServer::incomingConnection(qintptr socketDescriptor)
{
    // We have a new connection
    qDebug() << socketDescriptor << " Connecting...";

    Worker *thread = new Worker(socketDescriptor, ss, this);

    connect(thread,SIGNAL(newMessage(QString)),this,SLOT(NewMessageFromWorker(QString)));

    // connect signal/slot
    // once a thread is not needed, it will be beleted later
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();
}
//=================================================================

void TrendServer::NewMessageFromWorker(QString message)
{
    //logger->AddLog("ТРЕНДЫ: Worker Say : "+message,Qt::darkBlue);
    emit newMessage(message);
}

//=================================================================

Worker::Worker(qintptr ID, ScadaServer *scada_server, QObject *parent)
{
    this->socketDescriptor = ID;
    ss=scada_server;
}
//====================================================================================
void Worker::run()
{


    pClient=new QTcpSocket();

    // set the ID
    if(!pClient->setSocketDescriptor(this->socketDescriptor))
    {
        // something's wrong, we just emit a signal
        emit error(pClient->error());
        return;
    }



    connect(pClient, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
    connect(pClient, SIGNAL(disconnected()), this, SLOT(disconnected()));

    // We'll have multiple clients, we want to know which is which
    qDebug() << socketDescriptor << " Client connected";

    emit newMessage("new connection ID="+QString::number(socketDescriptor) + ", IP="+pClient->peerAddress().toString());
    // make this thread a loop,
    // thread will stay alive so that signal/slot to function properly
    // not dropped out in the middle when thread dies



    exec();



}
//====================================================================================

void Worker::readyRead()
{


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
void Worker::disconnected()
{
    qDebug() << socketDescriptor << " Disconnected";


    pClient->deleteLater();
    exit(0);
}
//====================================================================================
