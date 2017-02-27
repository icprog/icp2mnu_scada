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
    pClient->setSocketOption(QAbstractSocket:: KeepAliveOption, 1);
    pClient->write((char *)buff,num_float_tags*4);
    m_pClientSocketList.push_back(pClient);

    connect(pClient, SIGNAL(disconnected()), this, SLOT(ClientDisconnected()));
    connect(pClient, SIGNAL(readyRead()), this, SLOT(ClientSendData()));
    //lastClient=pClient;
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
