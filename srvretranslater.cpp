#include "srvretranslater.h"


SrvReTranslater::SrvReTranslater()
{
    for (int i=0;i++;i<100)
    {
        buff[i]=0.0;
    }
    m_pServerSocket = new QTcpServer(this);
    connect(m_pServerSocket, SIGNAL(newConnection()), this, SLOT(NewConnection()));
}
//==================================================================================
SrvReTranslater::~SrvReTranslater()
{
    delete m_pServerSocket;
}
//==================================================================================
void SrvReTranslater::NewConnection()
{
    QTcpSocket* pClient = m_pServerSocket->nextPendingConnection();

    m_pClientSocketList.push_back(pClient);


    /*
    //!!! НЕОБХОДИМОСТЬ ОТПАЛА - ПРОБЛЕМА РЕШЕНА
    //Защита от множественных соединений с одного хоста
    //если кол-во соединений с одного хоста в течении 60 сек
    //превышает 15, соединения отклоняются


    m_pClientDtVector.push_back(QDateTime::currentDateTime());
    m_pClientIpVector.push_back(pClient->peerAddress().toString());

    for(int i=0;i<m_pClientDtVector.size();++i)
    {
        if (m_pClientDtVector[i] < QDateTime::currentDateTime().addSecs(-60))
        {
            m_pClientDtVector.remove(i);
            m_pClientIpVector.remove(i);
        }
        else
        {
            break;
        }
    }


    if (m_pClientIpVector.count(pClient->peerAddress().toString()) < 15)
    {

    */

        pClient->setSocketOption(QAbstractSocket:: KeepAliveOption, 1);
        pClient->write((char *)buff,num_float_tags*4);
        connect(pClient, SIGNAL(disconnected()), this, SLOT(ClientDisconnected()));
        connect(pClient, SIGNAL(readyRead()), this, SLOT(ClientSendData()));

    /*
    }
    else
    {
        pClient->disconnectFromHost();
    }
    */


}
//======================================================================================
void SrvReTranslater::ClientDisconnected()
{
    // client has disconnected, so remove from list
    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());
    m_pClientSocketList.removeOne(pClient);
}
//=======================================================================================
void SrvReTranslater::ClientSendData()
{
    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());
    pClient->readAll(); //очищаем буффер приема
    pClient->disconnectFromHost();   //отключаем клиента - по протоколу он не должен ничего слать -> это не наш клиент
}
//=======================================================================================
