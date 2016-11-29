#include "event.h"


//=================================================================
bool operator<(const event_expr_member_struct &a, const event_expr_member_struct &b)
{
    return a.objectName.length() > b.objectName.length();
}
//===================================================================================================================
Event::Event(QString eventType, QString eventExpression, QVector<event_expr_member_struct> eventVectExprMembers,
               QString eventComparison, float eventValue, QString &eventText, uint eventdelaySec)
{

type=eventType;
expression=eventExpression;
vectExprMembers=eventVectExprMembers;
comparison=eventComparison;
eventUpriseValue=eventValue;
text=eventText;
delaySec=eventdelaySec;

expressionValue=min_float;
expressionValue_prev=min_float;

preactive=false;
//active;

if (eventType=="connect") eventColor.setRgb(250,250,160);
if (eventType=="disconnect") eventColor.setRgb(250,250,160);
if (eventType=="start") eventColor.setRgb(160,245,150);
if (eventType=="stop")  eventColor.setRgb(250,115,115);
if (eventType=="other")  eventColor.setRgb(160,170,250);
}
//==========================================================================================
Event::~Event()
{

}
//==========================================================================================
bool Event::CheckEventCondition(float val)
{
    if (comparison == "=")
    {
        return val==eventUpriseValue;
    }

    if (comparison == ">")
    {
        return val > eventUpriseValue;
    }

    if (comparison == "<")
    {
        return val < eventUpriseValue;
    }

    return false;
}
//==========================================================================================
void Event::SetValueAndCheckEventUprise(float newValue)
{

    //if (event->expressionValue_prev==min_float)

    expressionValue=  newValue;

    if (expressionValue_prev!=min_float)
    {
        //найдем фронт
        if ( !CheckEventCondition(expressionValue_prev) && CheckEventCondition(expressionValue))
        {
            preactive=true;
            dt_preactivated=QDateTime::currentDateTime();
        }

        if (preactive==true && CheckEventCondition(expressionValue))
        {
            if (dt_preactivated.secsTo(QDateTime::currentDateTime()) >= delaySec)
            {
                //event uprised
                dt_activated=QDateTime::currentDateTime();
                emit EventUprise(this);
                preactive=false;
            }
        }

        if (preactive==true && !CheckEventCondition(expressionValue))
        {
                //event not uprised till delay time
                preactive=false;
        }

    }


    expressionValue_prev=expressionValue;



   // if (false) emit EventUprise(QDateTime::currentDateTime(),"sometext");
}
//==========================================================================================
QString Event::GetDateTimeOfEventUprise()
{
    QString tmp;
    tmp.sprintf("%.2u.%.2u.%.4u %.2u:%.2u:%.2u",
                dt_activated.date().day(),dt_activated.date().month(),dt_activated.date().year(),
                dt_activated.time().hour(),dt_activated.time().minute(),dt_activated.time().second());
    return tmp;
}
//==========================================================================================
event_message_struct* Event::GetEventMessageStruct()
{
    snprintf(event_message.DT_activate,20,"%s",this->GetDateTimeOfEventUprise().toStdString().c_str());
    snprintf(event_message.text,EVENT_MESSAGE_TEXT,"%s ",text.toStdString().c_str());

    this->eventColor.getRgb(&(event_message.color_r),&(event_message.color_g),&(event_message.color_b));

    return &event_message;
}
//==========================================================================================
Events::Events()
{
    logger=Logger::Instance();
    allEventsList.clear();

    eventDB=new OdbcDb("fire_triol","SYSDBA","784523");
    //eventDB->AddEvent2DB(QDateTime::currentDateTime(), "system", "Event Server Started...");

    logger->AddLog("EVENTS: Старт подсистемы...",Qt::darkCyan);

    QVector<QString> vect_dt;
    QVector<QString> vect_type;
    QVector<QString> vect_text;

    eventDB->ReadLastEventsFromDB(vect_dt, vect_type, vect_text,maxMessagesListCount);

    for(int i=0;i<vect_dt.length();++i)
    {
        event_message_struct event_message;
        snprintf(event_message.DT_activate,20,"%s",vect_dt[i].toStdString().c_str());
        snprintf(event_message.text,EVENT_MESSAGE_TEXT,"%s ",vect_text[i].toStdString().c_str());

        QColor event_color;
        if (vect_type[i]=="connect") event_color.setRgb(250,250,160);
        if (vect_type[i]=="disconnect") event_color.setRgb(250,250,160);
        if (vect_type[i]=="start") event_color.setRgb(160,245,150);
        if (vect_type[i]=="stop")  event_color.setRgb(250,115,115);
        if (vect_type[i]=="other")  event_color.setRgb(160,170,250);

        event_color.getRgb(&(event_message.color_r),&(event_message.color_g),&(event_message.color_b));

        lastUprisedEvents.push_back(event_message);

    }

//tcp server
    m_pEventServerSocket = new QTcpServer(this);
    m_pEventServerSocket->listen(QHostAddress::Any, 7001);

    connect(m_pEventServerSocket, SIGNAL(newConnection()), this, SLOT(NewConnection()));
}
//==========================================================================================
Events::~Events()
{
    foreach(Event *event, allEventsList)
    {
        delete event;
    }
    allEventsList.clear();

    //eventDB->AddEvent2DB(QDateTime::currentDateTime(),"system","Events Server Closed...");
    delete eventDB;
    delete m_pEventServerSocket;
}
//==========================================================================================
void Events::AddEvent(Event *event)
{
    allEventsList.push_back(event);
    connect(event,SIGNAL(EventUprise(Event *)),this,SLOT(EventUprised(Event *)));
    logger->AddLog("EVENTS: Добавлен: "+event->text+", expr="+event->expression+" ,delay="+QString::number(event->delaySec),Qt::darkCyan);
}
//==========================================================================================
QString Events::GetEventFormattedText(QDateTime dt, QString message)
{
    return dt.toString("dd.MM.yyyy hh:mm:ss")+"  "+message;
}
//==========================================================================================
void Events::EventUprised(Event *event)
{

    logger->AddLog("EVENTS: "+event->dt_activated.toString("dd.MM.yyyy hh:mm:ss")+ " "+event->text,Qt::darkCyan);

    QString eventObject;
    if (event->type=="connect" || event->type=="disconnect")
    {
        eventObject=event->expression;
    }
    else
    {
        if (event->vectExprMembers.size()>0)
        {
            eventObject=event->vectExprMembers[0].objectName;
        }
        else
        {
            eventObject="none";
        }
    }
    eventDB->AddEvent2DB(event->dt_activated,eventObject, event->type,event->text);

    //event_message_struct tmp=*(event->GetEventMessageStruct());

    lastUprisedEvents.push_back( *(event->GetEventMessageStruct()) );

    if (lastUprisedEvents.size() > maxMessagesListCount) lastUprisedEvents.pop_front();

    SendNewEventToAllClients( *(event->GetEventMessageStruct()) );

    //QMessageBox::information(NULL,"event",message);
}
//==========================================================================================
void Events::NewConnection()
{
    QTcpSocket* pClient = m_pEventServerSocket->nextPendingConnection();
    pClient->setSocketOption(QAbstractSocket:: KeepAliveOption, 1);
    m_pEventClientSocketList.push_back(pClient);

    //TO DO: send events
    SendAllEventsToOneClient(pClient);

    connect(pClient, SIGNAL(disconnected()), this, SLOT(ClientDisconnected()));
    connect(pClient,SIGNAL(readyRead()),this,SLOT(ClientWrite()));
    //lastClient=pClient;

    logger->AddLog("EVENTS: К серверу сообщений подключился "+pClient->peerAddress().toString(),Qt::darkCyan);
    //alarmDB->AddAlarm2DB("До сервера алармів під`єднався "+pClient->peerAddress().toString());

}
//======================================================================================
void Events::ClientDisconnected()
{
    // client has disconnected, so remove from list
    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());
    m_pEventClientSocketList.removeOne(pClient);
    logger->AddLog("EVENTS: От сервера сообщений отключился "+pClient->peerAddress().toString(),Qt::darkCyan);
    //alarmDB->AddAlarm2DB("Від сервера алармів від`єднався "+pClient->peerAddress().toString());

}
//===========================================================================================================
void Events::ClientWrite()
{
    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());

    // ошибка, не тот сервер, тут ничего не должно писаться
    pClient->disconnectFromHost();


}
//====================================================================================
void Events::SendAllEventsToOneClient(QTcpSocket* pClient)
{
    foreach(event_message_struct event_message, lastUprisedEvents)
    {
        pClient->write((char *)&event_message,sizeof(event_message_struct));
        pClient->waitForBytesWritten(5000);
    }
}
//==========================================================================================
void Events::SendNewEventToAllClients(event_message_struct event_message)
{

    foreach(QTcpSocket* pClient, m_pEventClientSocketList)
    {
        pClient->write((char *)&event_message,sizeof(event_message_struct));
        pClient->waitForBytesWritten(5000);
    }
}
//==========================================================================================
