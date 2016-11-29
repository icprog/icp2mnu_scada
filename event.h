#ifndef EVENT_H
#define EVENT_H

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QColor>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSettings>

#include "logger.h"
#include "odbcdb.h"


const uint EVENT_MESSAGE_TEXT=128;


/*
enum EventType
{
    EventDisconnectType,
    EventConnectType,
    EventEventType
};
*/

/*
enum EventCondition
{
    EventConditionBigger,
    EventConditionLess,
    EventConditionEqual
};
*/


struct event_expr_member_struct
{
    //QString sign;
    QString objectName;
    uint numInBuff;
};

bool operator<(const event_expr_member_struct &a, const event_expr_member_struct &b);




struct event_message_struct
{
    char DT_activate[20];
    char text[EVENT_MESSAGE_TEXT];
    int color_r;
    int color_g;
    int color_b;
};





class Event : public QObject
{
    Q_OBJECT

public:
    bool preactive;
    bool active;
    QString text;
    //EventCondition eventCondition;
    QString type;
    QString expression;
    QVector<event_expr_member_struct> vectExprMembers;
    QString comparison;

    uint delaySec;

    QColor eventColor;

    event_message_struct  event_message;

    event_message_struct* GetEventMessageStruct();
    //int color_r;
    //int color_g;
    //int color_b;

public:
    constexpr static const float min_float=-3.4028234663852886e+38;   //минимальное число float, используется как "пустое" значение,
                                                                      //для отличия от NaN которое обозначает результат некоректного скриптового выражения
    float eventUpriseValue;
    float expressionValue;
    float expressionValue_prev;
    QDateTime dt_preactivated;  //активный, ждем время задержки
    QDateTime dt_activated;
    explicit Event(QString eventType, QString eventExpression, QVector<event_expr_member_struct> eventVectExprMembers,
                   QString eventComparison, float eventValue, QString &eventText, uint eventdelaySec=0);
    virtual ~Event();
    bool CheckEventCondition(float val);
    void SetValueAndCheckEventUprise(float newValue);
    QString GetDateTimeOfEventUprise();


signals:
    void EventUprise(Event *event);
};
//================================================================================
class Events : public QObject
{
    Q_OBJECT
public:
    Events();
    virtual ~Events();

    QVector<Event *> allEventsList;
    QVector<event_message_struct> lastUprisedEvents;

    void AddEvent(Event *event);
    void SendAllEventsToOneClient(QTcpSocket* pClient);
    void SendNewEventToAllClients(event_message_struct event_message);
    QString GetEventFormattedText(QDateTime dt, QString message);

private:
    static const unsigned int maxMessagesListCount=50;
    Logger *logger;
    OdbcDb *eventDB;
    QTcpServer* m_pEventServerSocket;
    QList<QTcpSocket*> m_pEventClientSocketList;
    //QTcpSocket* lastClient;


public slots:
    void EventUprised(Event *event);
    //tcp server
    // Slot to handle disconnected client
    void ClientDisconnected();
    void ClientWrite();

private slots:
    // New client connection
    void NewConnection();

};


//=================================================================================
#endif // EVENT_H
