#ifndef SRVRETRANSLATER_H
#define SRVRETRANSLATER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QVector>
#include <QDateTime>

class SrvReTranslater : public QObject
{
    Q_OBJECT

public:
    SrvReTranslater();
    virtual ~SrvReTranslater();
    float buff[100];
    unsigned int num_float_tags; //1 float tag = 2 modbus registers
    QTcpServer* m_pServerSocket;
    QList<QTcpSocket*> m_pClientSocketList;


    //Защита от множественных соединений с одного хоста
    //если кол-во соединений с одного хоста в течении 60 сек
    //превышает 15, соединения отклоняются

    QVector<QString> m_pClientIpVector;
    QVector<QDateTime> m_pClientDtVector;


public slots:
    // Slot to handle disconnected client
    void ClientDisconnected();
    void ClientSendData();

private slots:
    // New client connection
    void NewConnection();
};


#endif // SRVRETRANSLATER_H
