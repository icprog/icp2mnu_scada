#include "nodes.h"

//allocate memory for static member
uint CommonNode::nodes_counter=0;


//сравнение для сортировки по убыванию длины имени объекта, лямбду не принимает, не включен с++11 - включил но переписывать не буду
bool operator<(const virt_expr_member_struct &a, const virt_expr_member_struct &b)
{
    return a.objectName.length() > b.objectName.length();
}
//================================================================================
CommonNode::CommonNode()
{
    memset(m_srv.buff,0.0,100);
    m_text_client="connecting...";
    m_text_repl="";
    m_isConnected=false;
    m_isReaded=false;
    ss = ScadaServer::Instance();
}
//================================================================================
CommonNode::~CommonNode()
{

}
//================================================================================
CommonNode* CommonNode::CreateNode(QString objectName,QString objectType,
                                   QString IP_addr, uint port,
                                   uint port_repl,uint port_local,
                                   uint modbus_start_address,
                                   uint num_float_tags)
{

    CommonNode *node=NULL;

    if (objectType=="modbus")
    {
        node = new ModbusNode(nodes_counter,objectName,objectType,
                              IP_addr, port,port_repl,port_local,
                              modbus_start_address,num_float_tags);
    }

    if (objectType=="mnu_scada")
    {
        node = new MnuScadaNode(nodes_counter,objectName,objectType,
                                IP_addr, port,port_repl,port_local,
                                modbus_start_address,num_float_tags);

    }

    if (objectType=="virtual")
    {
        node = new VirtualNode(nodes_counter,objectName,objectType,
                               IP_addr, port,port_repl,port_local,
                               modbus_start_address,num_float_tags);

    }

    if (objectType=="region")
    {
        node = new RegionNode(nodes_counter,objectName,objectType,
                               IP_addr, port,port_repl,port_local,
                               modbus_start_address,num_float_tags);

    }

    nodes_counter++;
    return node;
}
//================================================================================
QString CommonNode::FormattedNodeString()
{

    char node_text[256];
    for (uint i=0; i<255; ++i) node_text[i]=' ';
    node_text[255]=0;

    //Небезопасно - по хорошему переделать на strncpy, а лучше на strlcpy, но его нет в mingw

    strcpy(&node_text[0],  m_nameObject.toStdString().c_str());
    node_text[strlen(node_text)]=' '; //уберем добавленный конец строки
    strcpy(&node_text[20], (m_IP_addr+":" + QString::number(m_port)).toStdString().c_str());
    node_text[strlen(node_text)]=' ';
    strcpy(&node_text[39], (QString("(") + m_typeObject).toStdString().c_str());
    node_text[strlen(node_text)]=' ';
    strcpy(&node_text[49], (QString(",tags=") + QString::number(m_srv.num_float_tags)+")").toStdString().c_str());
    node_text[strlen(node_text)]=' ';
    strcpy(&node_text[59], (m_text_client+ " " + m_text_repl).toStdString().c_str());
    node_text[strlen(node_text)]=' ';
    strcpy(&node_text[76], (QString(" ---> 127.0.0.1:")+ QString::number(m_port_local) + "(MADBUS)  === ").toStdString().c_str());
    node_text[strlen(node_text)]=' ';
    strcpy(&node_text[111], QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz").toStdString().c_str());

    return QString(node_text);

    /*  c++ + Qt
        QString node_text;
        node_text.fill(' ',134);
        node_text.replace(0,m_nameObject.length(),m_nameObject);
        node_text.replace(20,(m_IP_addr+":" + QString::number(m_port)).length(),(m_IP_addr+":" + QString::number(m_port)));
        node_text.replace(39,(QString("(") + m_typeObject).length(),(QString("(") + m_typeObject));
        node_text.replace(49,(QString(",tags=") + QString::number(m_srv.num_float_tags)+")").length(),(QString(",tags=") + QString::number(m_srv.num_float_tags)+")"));
        node_text.replace(59,(m_text_client+ " " + m_text_repl).length(),(m_text_client+ " " + m_text_repl));
        node_text.replace(76,(QString(" ---> 127.0.0.1:")+ QString::number(m_port_local) + "(MADBUS)  === ").length(),(QString(" ---> 127.0.0.1:")+ QString::number(m_port_local) + "(MADBUS)  === "));
        node_text.replace(111,QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz").length(),QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz"));
        return node_text;
    */
}
//================================================================================
ModbusNode::ModbusNode(int this_number,QString objectName,QString objectType,
                       QString IP_addr, uint port,
                       uint port_repl,uint port_local,
                       uint modbus_start_address,
                       uint num_float_tags)
{
    logger=Logger::Instance();

    m_this_number=this_number;

    m_nameObject=objectName;
    m_typeObject=objectType;
    m_IP_addr=IP_addr;
    m_port=port;
    m_port_repl=port_repl;
    m_port_local=port_local;
    m_modbus_start_address=modbus_start_address;
    m_srv.num_float_tags=num_float_tags;

    start();
}
//================================================================================
void ModbusNode::run()
{
    int res;

    modbus_t *mb;
    uint16_t tab_reg[200];

    if (m_IP_addr.isEmpty())
    {
        //QMessageBox::information(this,"Configuration message","Your configuration is incorrect - no IP address!!!",QMessageBox::Ok);
        emit textchange(m_this_number, m_nameObject, "ERROR-No IP adress!!!");
        return;
    }

    //inialized context
    mb = modbus_new_tcp(m_IP_addr.toStdString().c_str(), m_port);

    for(;;)
    {
        //connect if not connected
        if(!m_isConnected)
        {

            res=modbus_connect(mb);

            if (res!=-1)
            {
                m_isConnected=true;
                modbus_set_slave(mb, 1);

                emit textchange(m_this_number,m_nameObject,  "connected");
                emit textSave2LogFile(m_this_number,m_nameObject,  "connected");
                //srv->m_pServerSocket->setSocketOption(QAbstractSocket:: KeepAliveOption, 1);
            }
            else
            {
                m_isConnected=false;
                //emit textchange(m_this_number,m_nameObject,  "connecting failure");
            }

        }

        if (m_isConnected)
        {

            res=modbus_read_registers(mb, m_modbus_start_address, m_srv.num_float_tags*2, tab_reg);
            if (res==m_srv.num_float_tags*2)
            {
                for (uint nn=0; nn < m_srv.num_float_tags; ++nn)
                {
                    m_srv.buff[nn]=modbus_get_float(&tab_reg[nn*2]);

                }
                m_isReaded=true;
            }
            else  //error read? closing
            {
                m_isConnected=false;
                m_isReaded=false;
                modbus_close(mb);
                emit textchange(m_this_number,m_nameObject,  "disconnected");
                emit textSave2LogFile(m_this_number,m_nameObject,  "disconnected");
            }


        }


        for(int i=0; i<50; ++i) //было 22 - 4.4 sec delay, стало 10 с
        {
            if (CheckThreadStop())
            {
                modbus_close(mb);
                modbus_free(mb);
                return;
            }
            Sleep(200);
        }

    }//for(;;)


}
//=============================================================================
MnuScadaNode::MnuScadaNode(int this_number, QString objectName, QString objectType, QString IP_addr, uint port, uint port_repl, uint port_local, uint modbus_start_address, uint num_float_tags)
{
    logger=Logger::Instance();

    socket = new QTcpSocket(this);

    m_this_number=this_number;

    m_nameObject=objectName;
    m_typeObject=objectType;
    m_IP_addr=IP_addr;
    m_port=port;
    m_port_repl=port_repl;
    m_port_local=port_local;
    m_modbus_start_address=modbus_start_address;
    m_srv.num_float_tags=num_float_tags;
    m_num_reads=0;
    m_sec_counter=0;

    connect(socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));
    connect(&timerConnectLoop,SIGNAL(timeout()),this,SLOT(TimerConnectLoopEvent()));

    need_restart_repl=false;

    timerConnectLoop.start(1000);

}
//================================================================================
MnuScadaNode::~MnuScadaNode()
{
    /*
    if (isConnected==true)
    {
        socket->disconnectFromHost();
        socket->waitForDisconnected(5000);
    }
    */

    timerConnectLoop.stop(); // - error QObject::killTimers: timers cannot be stopped from another thread
    delete socket;

}
//===================================================================================
void MnuScadaNode::doConnect()
{
    // this is not blocking call
    socket->connectToHost(m_IP_addr, m_port);
    //isConnecting=true;
    // we need to wait...
    //if(!socket->waitForConnected(5000))
    //{
    //    qDebug() << "Error: " << socket->errorString();
    //}
}
//==================================================================================
void MnuScadaNode::TimerConnectLoopEvent()
{
//    qDebug() << "timer connect loop event: ";

    //попытки подключения каждые 10 сек.
    if (m_sec_counter%10==0 && getStateConnected()==false && getStateConnecting()==false)
    {
        doConnect();

    }

    //if no data 30 seconds - reconnect

    if (m_sec_counter==25) //раз в 30 секунд, на 25 секунде
    {
        if (m_num_reads==0 && getStateConnected()==true)
        {
            socket->disconnectFromHost();
            //qDebug() << "error - no data in 20 sec";
        }
        m_num_reads=0;
    }

    m_sec_counter=(m_sec_counter+1)%30;  //30sec

}
//====================================================================================
void MnuScadaNode::connected()
{
    qDebug() << m_nameObject << " connected...";

    m_isReaded=false;
    m_isConnected=true;
    m_num_reads=1;
    emit textchange(m_this_number,m_nameObject,  "connected");
    emit textSave2LogFile(m_this_number,m_nameObject,  "connected");

    if (m_port_repl>0)  //если порт репликации > 0, если ==0-репликация отключена
    {
        if (isRunning()==false)
        {
            start();
            emit textchange_repl(m_this_number,m_nameObject,  "tr.repl");
        }
        else
        {
            need_restart_repl=true;

        }
    }

}
//=====================================================================================
void MnuScadaNode::error(QAbstractSocket::SocketError err)
{

    emit textSave2LogFile(m_this_number,m_nameObject,  "socket error: " + QString::number(err));
    socket->disconnectFromHost();
    qDebug() << m_nameObject << " error " + QString::number(err);
    m_isReaded=false;
}
//=======================================================================================
void MnuScadaNode::disconnected()
{
    m_isReaded=false;
    m_isConnected=false;

    emit textchange(m_this_number,m_nameObject,  "disconnected");
    emit textSave2LogFile(m_this_number,m_nameObject,  "disconnected");

    qDebug() << m_nameObject << " disconnected...";
}
//=======================================================================================
void MnuScadaNode::bytesWritten(qint64 bytes)
{
    qDebug() << bytes << " bytes written...";
}
//=======================================================================================
void MnuScadaNode::readyRead()
{
    //qDebug() << "reading..."+QString::number(socket->bytesAvailable());


    // read the data from the socket
    if (socket->bytesAvailable()==m_srv.num_float_tags*4)
    {
        socket->read((char *)(m_srv.buff), m_srv.num_float_tags*4);
        m_isReaded=true;
        m_num_reads++;
    }
    else
    {
        emit textSave2LogFile(m_this_number,m_nameObject,  "socket read error(byte count)!!! - recv "+QString::number(socket->bytesAvailable())+
                                                           ", need "+QString::number(m_srv.num_float_tags*4));
        /*  - посчитал излишним, достаточно очистить буфер
        if (socket->bytesAvailable()>0 &&  (socket->bytesAvailable() % (m_srv.num_float_tags*4) == 0))  //пакет данных кратен требуемому, по логам такое бывает
        {                                                                                               //читаем первую партию
            socket->read((char *)(m_srv.buff), m_srv.num_float_tags*4);
            m_isReaded=true;
            m_num_reads++;
        }
        */
        socket->readAll(); //очищаем буффер приема
       //socket->disconnectFromHost();
       // m_isReaded=false;
    }
}
//===========================================================================================
/*
QAbstractSocket::UnconnectedState	0	The socket is not connected.
QAbstractSocket::HostLookupState	1	The socket is performing a host name lookup.
QAbstractSocket::ConnectingState	2	The socket has started establishing a connection.
QAbstractSocket::ConnectedState     3	A connection is established.
QAbstractSocket::BoundState         4	The socket is bound to an address and port.
QAbstractSocket::ClosingState       6	The socket is about to close (data may still be waiting to be written).
QAbstractSocket::ListeningState     5	For internal use only.
*/
//=============================================================
bool MnuScadaNode::getStateConnected()
{
    return socket->state()==QAbstractSocket::ConnectedState;
}
//=============================================================
bool MnuScadaNode::getStateConnecting()
{
    return socket->state()==QAbstractSocket::HostLookupState || socket->state()==QAbstractSocket::ConnectingState;
}
//=============================================================
//поток восстановления трендовых файлов после отключения/пропадания связи с узлов "mnu_scada"
//+функции из проекта на VC++ 2003 - переделано под куте
// tags replication  ===========================================================================================

int mod(int number1,int divider)
{
    div_t fff=div(number1, divider);
    return fff.rem;
}
//=======================================================================================
void PlusOneDay(int &iYear,int &iMonth,int &iDay)
{
if ((iMonth==12) && (iDay==31)) {iYear++;iMonth=1;iDay=1;return;}
if ((iMonth==1) && (iDay==31)) {iMonth=2;iDay=1;return;}

if ((mod(iYear,4)==0) && (iMonth==2) && (iDay==29)) {iMonth=3;iDay=1;return;}
if ((mod(iYear,4)!=0) && (iMonth==2) && (iDay==28)) {iMonth=3;iDay=1;return;}

if ((iMonth==3) && (iDay==31)) {iMonth=4;iDay=1;return;}
if ((iMonth==4) && (iDay==30)) {iMonth=5;iDay=1;return;}
if ((iMonth==5) && (iDay==31)) {iMonth=6;iDay=1;return;}
if ((iMonth==6) && (iDay==30)) {iMonth=7;iDay=1;return;}
if ((iMonth==7) && (iDay==31)) {iMonth=8;iDay=1;return;}
if ((iMonth==8) && (iDay==31)) {iMonth=9;iDay=1;return;}
if ((iMonth==9) && (iDay==30)) {iMonth=10;iDay=1;return;}
if ((iMonth==10) && (iDay==31)) {iMonth=11;iDay=1;return;}
if ((iMonth==11) && (iDay==30)) {iMonth=12;iDay=1;return;}

iDay++;return;
}
//========================================================================================
void MinusOneDay(int &iYear,int &iMonth,int &iDay)
{
if ((iMonth==1) && (iDay==1)) {iYear--;iMonth=12;iDay=31;return;}
if ((iMonth==2) && (iDay==1)) {iMonth=1;iDay=31;return;}

if ((mod(iYear,4)==0) && (iMonth==3) && (iDay==1)) {iMonth=2;iDay=29;return;}
if ((mod(iYear,4)!=0) && (iMonth==3) && (iDay==1)) {iMonth=2;iDay=28;return;}

if ((iMonth==4) && (iDay==1)) {iMonth=3;iDay=31;return;}
if ((iMonth==5) && (iDay==1)) {iMonth=4;iDay=30;return;}
if ((iMonth==6) && (iDay==1)) {iMonth=5;iDay=31;return;}
if ((iMonth==7) && (iDay==1)) {iMonth=6;iDay=30;return;}
if ((iMonth==8) && (iDay==1)) {iMonth=7;iDay=31;return;}
if ((iMonth==9) && (iDay==1)) {iMonth=8;iDay=31;return;}
if ((iMonth==10) && (iDay==1)) {iMonth=9;iDay=30;return;}
if ((iMonth==11) && (iDay==1)) {iMonth=10;iDay=31;return;}
if ((iMonth==12) && (iDay==1)) {iMonth=11;iDay=30;return;}

iDay--;return;
}
//=================================================================================
void MinusInterval(int &iYear,int &iMonth,int &iDay, int &iHour,int &iMinute,int &iSecond, int interval_sec)
{
    if (interval_sec>60*60*24)
    {
        MinusOneDay(iYear,iMonth,iDay);
        MinusInterval(iYear,iMonth,iDay, iHour,iMinute,iSecond,interval_sec-60*60*24);
    }
    else
    {
        if (interval_sec<=(iHour*60*60+iMinute*60+iSecond))
        {
            int temp=(iHour*60*60+iMinute*60+iSecond)-interval_sec;
            iHour=temp/3600;
            iMinute=(temp-iHour*3600)/60;
            iSecond=temp-iHour*3600-iMinute*60;
        }
        else
        {
            MinusOneDay(iYear,iMonth,iDay);
            int temp=24*60*60+(iHour*60*60+iMinute*60+iSecond)-interval_sec;

            iHour=temp/3600;
            iMinute=(temp-iHour*3600)/60;
            iSecond=temp-iHour*3600-iMinute*60;
        }
    }
}
//=============================================================================================================
void PlusInterval(int &iYear,int &iMonth,int &iDay, int &iHour,int &iMinute,int &iSecond, int interval_sec)
{
    if (interval_sec>60*60*24)
    {
        PlusOneDay(iYear,iMonth,iDay);
        PlusInterval(iYear,iMonth,iDay, iHour,iMinute,iSecond,interval_sec-60*60*24);
    }
    else
    {
        if ((interval_sec+iHour*60*60+iMinute*60+iSecond)<24*60*60)
        {
            int temp=iHour*60*60+iMinute*60+iSecond+interval_sec;
            iHour=temp/3600;
            iMinute=(temp-iHour*3600)/60;
            iSecond=temp-iHour*3600-iMinute*60;
        }
        else
        {
            PlusOneDay(iYear,iMonth,iDay);
            int temp=-24*60*60+(iHour*60*60+iMinute*60+iSecond)+interval_sec;

            iHour=temp/3600;
            iMinute=(temp-iHour*3600)/60;
            iSecond=temp-iHour*3600-iMinute*60;
        }
    }
}
//================================================================================================
int Interval(int sYear,int sMonth,int sDay, int sHour,int sMinute,int sSecond,int eYear,int eMonth,int eDay, int eHour,int eMinute,int eSecond)
{
//if ((sYear>eYear) || ((sYear==eYear) && (sMonth>eMonth)) ||
//	((sYear==eYear) && (sMonth==eMonth) && (sDay>eDay)) return 0;
    int summa_sec=0;
//если день старт и енд не равны, то срабатывает один из циклов
//startday < end
    for (; ((sYear<eYear) || ((sYear==eYear) && (sMonth<eMonth)) || ((sYear==eYear) && (sMonth==eMonth) && (sDay<eDay))); PlusOneDay(sYear,sMonth,sDay))
        summa_sec+=86400;
//startday > end  - пока закоментировал - не проверено
//for (;((sYear>eYear) || ((sYear==eYear) && (sMonth>eMonth)) || ((sYear==eYear) && (sMonth==eMonth) && (sDay>eDay)));MinusOneDay(sYear,sMonth,sDay))
//summa_sec-=86400;
//startday == end
    summa_sec+=(eHour-sHour)*3600+(eMinute-sMinute)*60+eSecond-sSecond;
    return summa_sec;
}
//=================================================================================================
void MnuScadaNode::run()
{
    QTcpSocket repl_socket;
    int aligner=0;

    if (m_port_repl==0) return;  //если порт=0 -> репликация выключена

    msleep(1000);

    for (int cycles_cnt=0; cycles_cnt<2; ++cycles_cnt)
    {
        foreach(CommonTrend *trend, ss->vectCommonTrends)
        {

            if (m_nameObject==trend->m_objectName)
            {
                repl_socket.connectToHost(m_IP_addr, m_port_repl);
                if (repl_socket.waitForConnected(5000))  //true if connected
                {

                    //qDebug() << "repl connected";
                    float buff_file[17280];

                    QDateTime now_time=QDateTime::currentDateTime();

                    int iYear_st=now_time.date().year();
                    int iMonth_st=now_time.date().month();
                    int iDay_st=now_time.date().day();
                    int iHour_st=0;
                    int iMinute_st=0;
                    int iSecond_st=0;

                    int iYear_end=now_time.date().year();
                    int iMonth_end=now_time.date().month();
                    int iDay_end=now_time.date().day();
                    PlusOneDay(iYear_end,iMonth_end,iDay_end);

                    int iDataPosition_end=(now_time.time().hour()*60*60+now_time.time().minute()*60+now_time.time().second())/5;

                    //interval of trend replication - 2 weeks
                    MinusInterval(iYear_st,iMonth_st,iDay_st,iHour_st,iMinute_st,iSecond_st,86400*7*2);

                    for(; !(iYear_st==iYear_end && iMonth_st==iMonth_end && iDay_st==iDay_end);
                            PlusOneDay(iYear_st,iMonth_st,iDay_st))
                    {
                        QString filename;

                        filename.sprintf("%s%s\\%s_%.2u_%.2u_%.4u.trn",ss->trend_path,trend->m_objectName.toStdString().c_str(),
                                         trend->m_trendName.toStdString().c_str(),iDay_st,iMonth_st,iYear_st);

                        QFile trend_file(filename);


                        if (!trend_file.exists())
                        {
                            if (trend_file.open(QIODevice::ReadWrite))
                            {
                                trend_file.write((char *)ss->empty_file,69120);
                                trend_file.close();
                                //trend_file.waitForBytesWritten(5000);
                            }

                        }
                        trend_file.open(QIODevice::ReadWrite);


                        trend_file.read((char *)buff_file,69120);


                        int iDataCountInFile=17280;
                        if (iYear_st==now_time.date().year() && iMonth_st==now_time.date().month() && iDay_st==now_time.date().day())
                        {
                            iDataCountInFile=iDataPosition_end;
                        }

                        bool pos_start_found=false;
                        int  pos_start=0,pos_end=0;
                        for (int j=0; j<iDataCountInFile; j++)
                        {
                            if (!pos_start_found && buff_file[j]==ss->min_float)
                            {
                                pos_start_found=true;
                                pos_start=j;
                            }

                            if (  (pos_start_found && buff_file[j]!=ss->min_float) || (pos_start_found && j==iDataCountInFile-1)
                                    || (pos_start_found && j-pos_start>200) )
                            {
                                struct request_struct
                                {
                                    char FileName[32];
                                    int Year,Month,Day;
                                    int Position;
                                    int Count;
                                } request;

                                if (pos_start_found && buff_file[j]!=ss->min_float)
                                {
                                    aligner=0;
                                }
                                if ((pos_start_found && j==iDataCountInFile-1) || (pos_start_found && j-pos_start>200))
                                {
                                    aligner=1;
                                }


                                pos_end=j;
                                pos_start_found=false;
                                strcpy(request.FileName,trend->m_trendName.toStdString().c_str());
                                request.Year=iYear_st;
                                request.Month=iMonth_st;
                                request.Day=iDay_st;
                                request.Position=pos_start;
                                request.Count=pos_end-pos_start+aligner;

                                //qDebug() << "request: " << request.FileName;
                                //qDebug() << "request: " << request.Year;
                                //qDebug() << "request: " << request.Month;
                                //qDebug() << "request: " << request.Day;
                                //qDebug() << "request: " << request.Position;
                                //qDebug() << "request: " << request.Count;

                                int result;
                                result=repl_socket.write((char *)&request,sizeof(request));

                                if (result!=SOCKET_ERROR)
                                {
                                    float buff_new[300];
                                    repl_socket.waitForReadyRead(5000);
                                    result=repl_socket.read((char *)buff_new,(request.Count)*4);

                                    if (result==request.Count*4)
                                    {
                                        trend_file.seek(pos_start*4);//,CFile::begin);
                                        trend_file.write((char *)buff_new,(request.Count)*4);
                                        //trend_file.Flush();
                                    }
                                }

                                if (CheckThreadStop())
                                {
                                    qDebug() << m_nameObject << " thread repl stop1";
                                    return;
                                }

                                if (m_isConnected==false)  // socket which read current data disconnected, so close replication thread too
                                {
                                    qDebug() << m_nameObject << " thread repl stop network disconnected";
                                    emit textchange_repl(m_this_number,m_nameObject, "");
                                    return;
                                }

                                msleep(250); // delay for decrease CPU usage
                            }
                        }
                        trend_file.close();
                        if (CheckThreadStop())
                        {
                            qDebug() << m_nameObject << " thread repl stop2";
                            return;
                        }
                        msleep(100); //delay before next file open -> for decrease CPU usage
                    }
                }
                repl_socket.close();
                if (CheckThreadStop())
                {
                    qDebug() << m_nameObject << " thread repl stop3";
                    return;
                }
            }
        }

        if (need_restart_repl==true)
        {
            cycles_cnt=0;
            need_restart_repl=false;
        }
    }

    emit textchange_repl(m_this_number,m_nameObject, "");

    return;

}

//=========================================================================================

VirtualNode::VirtualNode(int this_number,QString objectName,QString objectType,
                         QString IP_addr, uint port,
                         uint port_repl,uint port_local,
                         uint modbus_start_address,
                         uint num_float_tags)
{
    logger=Logger::Instance();

    m_this_number=this_number;

    m_nameObject=objectName;
    m_typeObject=objectType;
    m_IP_addr=IP_addr;
    m_port=port;
    m_port_repl=port_repl;
    m_port_local=port_local;
    m_modbus_start_address=modbus_start_address;
    m_srv.num_float_tags=num_float_tags;

    // emit textchange(m_this_number,m_nameObject,  "running");
    // start();
    connect(&timerCalculateVariables,SIGNAL(timeout()),this,SLOT(TimerCalculateVariablesEvent()));

    timerCalculateVariables.start(1000);

}
//=========================================================================================
VirtualNode::~VirtualNode()
{
    timerCalculateVariables.stop();
}
//=========================================================================================
void VirtualNode::TimerCalculateVariablesEvent()
{

    //убедимся что все узлы необходимые для этого вирт.контроллера присутствуют
    //QVector<virt_tag_struct> vectVirtTags
    bool allControllersConnected=true;
    foreach(virt_tag_struct virtTag,vectVirtTags)
    {
        foreach(virt_expr_member_struct exprMember, virtTag.vectVirtTagExprMembers)
        {
            if (exprMember.objectName!=m_nameObject) // чтоб можно было использовать свои же тэги, в противном случае он будет ждать себя же
            {
                if (!ss->hashCommonNodes[exprMember.objectName]->m_isReaded) allControllersConnected=false;
            }
        }
    }

    //считаем значения
    if (allControllersConnected)
    {

        foreach(virt_tag_struct virtTag,vectVirtTags)
        {
            float tmp_TagValue=0.0;
            QString tmp_virtTagExpression;
            tmp_virtTagExpression=virtTag.virtTagExpression;

            foreach(virt_expr_member_struct exprMember, virtTag.vectVirtTagExprMembers)
            {
                //Logger::Instance()->AddLog(exprMember.objectName+","+QString::number(exprMember.numInBuff),Qt::black);


                tmp_virtTagExpression.replace(exprMember.objectName+"["+QString::number(exprMember.numInBuff)+"]",
                                              QString::number(ss->hashCommonNodes[exprMember.objectName]->m_srv.buff[exprMember.numInBuff]));
            }

            tmp_TagValue=virtControllerScriptEngine.evaluate(tmp_virtTagExpression).toNumber();

            //Logger::Instance()->AddLog(virtTag.virtTagExpression+ "="+tmp_virtTagExpression+"="+QString::number(tmp_TagValue),Qt::black);

            if ( tmp_TagValue!=tmp_TagValue)  //тривиальная проверка на NaN
            {
                tmp_TagValue=0.0;
            }

            m_srv.buff[virtTag.numInBuff]=tmp_TagValue;

        }


        if (m_isConnected==false)
        {
            emit textchange(m_this_number,m_nameObject,  "connected");
            emit textSave2LogFile(m_this_number,m_nameObject,  "connected");
        }
        m_isConnected=true;
        m_isReaded=true;
    }
    else
    {
        if (m_isConnected==true)
        {
            emit textchange(m_this_number,m_nameObject,  "disconnected");
            emit textSave2LogFile(m_this_number,m_nameObject,  "disconnected");
        }
        m_isConnected=false;
        m_isReaded=false;
    }


}
//=========================================================================================
RegionNode::RegionNode(int this_number,QString objectName,QString objectType,
                       QString IP_addr, uint port,
                       uint port_repl,uint port_local,
                       uint modbus_start_address,
                       uint num_float_tags)
{
    logger=Logger::Instance();

    m_this_number=this_number;

    m_nameObject=objectName;
    m_typeObject=objectType;
    m_IP_addr=IP_addr;
    m_port=port;
    m_port_repl=port_repl;
    m_port_local=port_local;
    m_modbus_start_address=modbus_start_address;
    m_srv.num_float_tags=num_float_tags;

    m_pCmdListenerServerSocket = new QTcpServer(this);
    m_pCmdListenerServerSocket->listen(QHostAddress::Any, m_port_local+1000);

    m_cmdListenerRequest.cmd=0x00;

    connect(m_pCmdListenerServerSocket, SIGNAL(newConnection()), this, SLOT(CmdListenerNewConnection()));

    connect(this,SIGNAL(cmdListenerResultReady(unsigned char,unsigned char,uint16_t*)),this,SLOT(CmdListenerSendResult(unsigned char,unsigned char,uint16_t*)));

    qDebug() << "region constr";
    start();
}
//================================================================================
RegionNode::~RegionNode()
{
    m_pCmdListenerServerSocket->close();
    m_pCmdListenerClientSocketList.clear();
    delete m_pCmdListenerServerSocket;
}
//================================================================================
void RegionNode::CmdListenerNewConnection()
{

    QTcpSocket* pClient = m_pCmdListenerServerSocket->nextPendingConnection();
    pClient->setSocketOption(QAbstractSocket:: KeepAliveOption, 1);

    m_pCmdListenerClientSocketList.push_back(pClient);

    connect(pClient, SIGNAL(disconnected()), this, SLOT(CmdListenerClientDisconnected()));


    //if ( m_pCmdListenerClientSocketList.count()==1)  //монопольный режим
    //{
        connect(pClient,SIGNAL(readyRead()),this,SLOT(CmdListenerReadyRead()));
    //}
    //else
    //{
    //    pClient->disconnectFromHost();
    //}

    //lastClient=pClient;
    logger->AddLog(m_nameObject +  ": RegionCtrl connected client "+pClient->peerAddress().toString(), Qt::magenta);
}
//======================================================================================
void RegionNode::CmdListenerClientDisconnected()
{
            qDebug() << "region 333333";
    // client has disconnected, so remove from list
    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());
    m_pCmdListenerClientSocketList.removeOne(pClient);

    logger->AddLog(m_nameObject +  ": RegionCtrl disconnected client "+ pClient->peerAddress().toString(), Qt::magenta);

}
//=======================================================================================

/*
 структура запроса команды на выполнение -
 команды: запись 1 регистра в HOLDING REGISTERS                    (6 байт = 2 команда + 2 адрес + 2 значение)   0x06 - write single register
         ЗАПИСЬ 1 значения в COIL STATUS(пуск/останов)             (6 байт = 2 команда + 2 адрес + 2 значение)   0x05(write single coil) (0xFF00 или 0x0000 - другие значения недопустимы по стандарту)
         запрос чтения одного или более регистров HOLDING REGISTERS(6 байт = 2 команда + 2 адрес + 2 количество) 0x03 - read holding registers
         запрос чтения одного или более регистров INPUT REGISTERS  (6 байт = 2 команда + 2 адрес + 2 количество) 0x04 - read input registers
         авторизация ???

ответы:  ОК в ответ на запись в HOLDING REGISTERS           2 байта = 1й байт 0x06 2й 0x00
         ОК в ответ на запись в COIL STATUS(пуск/останов)   2 байта = 1й байт 0x05 2й 0x00
         ОК + один или более HOLDING REGISTERS              2 байта = 1й байт 0x03 2й - кол-во регистров + (кол-во регистров)*2 байта
         ОК + один или более INPUT REGISTERS                2 байта = 1й байт 0x04 2й - кол-во регистров + (кол-во регистров)*2 байта

         ошибка - команда не выполнена, нет связи    2 байта = 1й байт 0xE0 2й- не выполненная cmd (т.е 0x06,0x05,0x03,0x04)   (error 0)
         ошибка - команда не выполнена               2 байта = 1й байт 0xE1 2й- не выполненная cmd (т.е 0x06,0x05,0x03,0x04)   (error 1)
         ошибка - выполняю команду, busy             2 байта = 1й байт 0xE2 2й- не выполненная cmd (т.е 0x06,0x05,0x03,0x04)   (error 2)
                    (одновременно выполняется только 1 команда, очереди команд нет -
                     защита от одновременного управления и забивания очереди
                     !!! - нужно подумать о монопольном режиме управления

*/
//=======================================================================================
struct
{
    unsigned char byte1;
    unsigned char byte2;
    uint16_t registers[256];
} response;
//=======================================================================================
void RegionNode::CmdListenerReadyRead()
{

    QTcpSocket* pClient = static_cast<QTcpSocket*>(QObject::sender());


    if (m_cmdListenerRequest.cmd==0x00)  // текущей команды нет
    {
        // read the data from the socket
        if (pClient->bytesAvailable()==sizeof(CmdListenerRequest))
        {

            pClient->read((char *)(&m_cmdListenerRequest), sizeof(CmdListenerRequest));
            lastRequestClient=pClient;
            logger->AddLog(m_nameObject +  ": Region Request: cmd:"+QString::number(m_cmdListenerRequest.cmd)+ " " +
                                                                                "addr:" + QString::number(m_cmdListenerRequest.addr)+ " " +
                                                                                "data:" + QString::number(m_cmdListenerRequest.data),Qt::magenta);
        }
        else
        {
            logger->AddLog(m_nameObject +  ": Region Request: wrong request "+QString::number(pClient->bytesAvailable())+
                                                               ", need " + QString::number(sizeof(CmdListenerRequest)),Qt::magenta);

            pClient->readAll(); //очищаем буффер приема
            pClient->disconnectFromHost();//если команда не 6 байт - протокол не наш, отключаем клиента
        }
    }
    else
    {

        CmdListenerRequest tmp_cmdListenerRequest;
        tmp_cmdListenerRequest.cmd=0x00;

        if (pClient->bytesAvailable()==sizeof(CmdListenerRequest))
        {
            pClient->read((char *)(&tmp_cmdListenerRequest), sizeof(CmdListenerRequest));
        }
        else
        {
            pClient->readAll(); //очищаем буффер приема
            pClient->disconnectFromHost();//если команда не 6 байт - протокол не наш, отключаем клиента
        }

        logger->AddLog(m_nameObject +  ": Region Request: Device Busy: cmd:"+QString::number(tmp_cmdListenerRequest.cmd)+ " " +
                                                                            "addr:" + QString::number(tmp_cmdListenerRequest.addr)+ " " +
                                                                            "data:" + QString::number(tmp_cmdListenerRequest.data), Qt::magenta);

        unsigned char ans[2];
        ans[0]=0xE2;
        ans[1]=tmp_cmdListenerRequest.cmd;
        pClient->write((char*) ans,2);       //см. протокол
    }
}
//=======================================================================================


//=======================================================================================
void RegionNode::CmdListenerSendResult(unsigned char byte1, unsigned char byte2, uint16_t *responseData)
{



    //send answer to lastRequestClient
    //if (m_pCmdListenerClientSocketList.first(lastRequestClient)) //check it not disconnected yet, it will be in the list
    //{
        response.byte1=byte1;
        response.byte2=byte2;

        if (responseData!=NULL)
        {
            memcpy_s(response.registers,256*sizeof(uint16_t),m_pCmdListenerResponseData,byte2*2);
        }

        if (byte1==0x03 || byte1==0x04)
        {
            lastRequestClient->write((char*) &response,2+byte2*2); //см. протокол - 2 байта + запрошенные регистры
        }
        else
        {
            lastRequestClient->write((char*) &response,2);       //см. протокол
        }

        logger->AddLog(m_nameObject +  ": Region Answer: byte1:"+QString::number(response.byte1) +
                                                         " byte2:" + QString::number(response.byte2), Qt::magenta);
    //}
    //m_cmdListenerRequest.cmd=0x00;

}
//=======================================================================================
void RegionNode::run()
{
    int res;

    modbus_t *mb;
    uint16_t tab_reg[200];


    float multipliers[32]={
            1.0, //0  Код причины останова
            1.0, //1  Состояние СУ+причины, мешающие запуску
            0.1, //2  Ток фазы А, Ампер
            0.1, //3  Ток фазы В
            0.1, //4  Ток фазы С
            1.0, //5  Дисбаланс токов, %
            1.0, //6  Напряжение АВ, Вольт
            1.0, //7  Напряжение ВС, Вольт
            1.0, //8  Напряжение СА, Вольт
            1.0, //9  Дисбаланс напряжений, %
            1.0, //10 Сопротивление изоляции, кОм
            0.01,//11 Коэффициент мощности (cos F)
            1.0, //12 Коэффициент загрузки, %
            1.0, //13 Активная мощность, кВт
            0.1, //14 Ток двигателя, А
            1.0, //15 Температура двигателя, С
            0.01,//16 Давление на приеме насоса, атм
            0.01,//17 Рабочая частота, Гц
            0.01,//18 Выходная частота, Гц
            1.0, //19 Выходной ток (полный ток СУ), А
            1.0, //20 Выходное напряжение, В
            1.0, //21 Ток в звене постоянного напряжения, В
            1.0, //22 Динамический уровень, м
            1.0, //23 Общее количество пусков, ед.
            1.0, //24 Устьевое давление (буферное давление), атм
            1.0, //25 Затрубное давление, атм
            1.0, //26 Линейное давление, атм
            1.0, //27 Температура на приеме насоса (температура пласт. жидкости, температура окружения), С
            1.0, //28 Вибрация насоса по оси X, м/с2
            1.0, //29 Вибрация насоса по оси Y, м/с2
            1.0, //30 Давление на выкиде насоса, атм
            1.0, //31 Температура на выкиде насоса, С

            };



    if (m_IP_addr.isEmpty())
    {
        //QMessageBox::information(this,"Configuration message","Your configuration is incorrect - no IP address!!!",QMessageBox::Ok);
        emit textchange(m_this_number, m_nameObject, "ERROR-No IP adress!!!");
        return;
    }

    //inialized context
    mb = modbus_new_tcp(m_IP_addr.toStdString().c_str(), m_port);


    /*
   Специфика протокола в том, что нужно устанавливать таймаут не меньше 5 сек. и
   опрашивать не реже раз в 3 сек, лучше чаще, тем меньше обрывов связи
   Этот эксперимент подтверждает и модбас полл
   */

    for(;;)
    {
        //connect if not connected
        if(!m_isConnected)
        {

            res=modbus_connect(mb);
            modbus_set_slave(mb, 1);

            if (modbus_set_response_timeout(mb, 30, 0)==-1) qDebug() << "timeout set error";
            modbus_set_byte_timeout(mb, 0, 0);
            //If both to_sec and to_usec are zero, this timeout will not be used at all.
            //In this case, modbus_set_response_timeout() governs the entire handling of the response,
            //the full confirmation response must be received before expiration of the response timeout.
            //When a byte timeout is set, the response timeout is only used to wait for until the first byte of the response.

            //response_timeout.tv_sec=6;
            //response_timeout.tv_usec=0;
            //modbus_set_response_timeout(mb, &response_timeout);

            if (res!=-1)
            {
                m_isConnected=true;




                qDebug() << "region connected";

                emit textchange(m_this_number,m_nameObject,  "connected");
                emit textSave2LogFile(m_this_number,m_nameObject,  "connected");
                //srv->m_pServerSocket->setSocketOption(QAbstractSocket:: KeepAliveOption, 1);

            }
            else
            {
                m_isConnected=false;
                //emit textchange(m_this_number,m_nameObject,  "connecting failure");
            }

        }

        if (m_isConnected)
        {

            res=modbus_read_input_registers(mb, m_modbus_start_address, m_srv.num_float_tags, tab_reg);

            qDebug() << "region read"+ QString::number(res) << "from addr "+QString::number(m_modbus_start_address)+ " waiting"+ QString::number(m_srv.num_float_tags);

            if (res==m_srv.num_float_tags)
            {
                for (uint nn=0; nn < m_srv.num_float_tags; ++nn)
                {
                    m_srv.buff[nn]=tab_reg[nn]*multipliers[nn];

                }
                m_isReaded=true;
            }
            else  //error read? closing
            {
                m_isConnected=false;
                m_isReaded=false;
                modbus_close(mb);
                emit textchange(m_this_number,m_nameObject,  "disconnected");
                emit textSave2LogFile(m_this_number,m_nameObject,  "disconnected");
            }


        }


        for(int i=0; i<10; ++i) //было 22 - 4.4 sec delay, стало 10 с, стало 2с.,
        {
            if (m_cmdListenerRequest.cmd!=0x00)  //проверяем наличие команды в очереди, если есть выполняем
            {
                if (m_isConnected)
                {

                    //запись 1 регистра в HOLDING REGISTERS     (6 байт = 2 команда + 2 адрес + 2 значение)   0x06 - write single register
                    if (m_cmdListenerRequest.cmd==0x06)
                    {
                        int res = modbus_write_register(mb,m_cmdListenerRequest.addr, m_cmdListenerRequest.data);
                        //The function shall return 1 if successful. Otherwise it shall return -1 and set errno.

                        if (res==1)
                        {
                            emit cmdListenerResultReady(m_cmdListenerRequest.cmd,0x00,NULL);
                        }
                        else
                        {
                            emit cmdListenerResultReady(0xE1,m_cmdListenerRequest.cmd,NULL);
                        }
                    }

                    //ЗАПИСЬ 1 значения в COIL STATUS(пуск/останов) (6 байт = 2 команда + 2 адрес + 2 значение)   0x05(write single coil) (0xFF00 или 0x0000 - другие значения недопустимы по стандарту)
                    if (m_cmdListenerRequest.cmd==0x05)
                    {
                        int res = modbus_write_bit(mb,m_cmdListenerRequest.addr, m_cmdListenerRequest.data);
                        //The function shall return 1 if successful. Otherwise it shall return -1 and set errno.

                        if (res==1)
                        {
                            emit cmdListenerResultReady(m_cmdListenerRequest.cmd,0x00,NULL);
                        }
                        else
                        {
                            emit cmdListenerResultReady(0xE1,m_cmdListenerRequest.cmd,NULL);
                        }
                    }

                    //запрос чтения одного или более регистров HOLDING REGISTERS(6 байт = 2 команда + 2 адрес + 2 количество) 0x03 - read holding registers
                    if (m_cmdListenerRequest.cmd==0x03)
                    {
                        int res = modbus_read_registers(mb,m_cmdListenerRequest.addr, m_cmdListenerRequest.data, m_pCmdListenerResponseData);
                        //The function shall return the number of read registers if successful. Otherwise it shall return -1 and set errno.

                        if (res==m_cmdListenerRequest.data)
                        {
                            emit cmdListenerResultReady(m_cmdListenerRequest.cmd,m_cmdListenerRequest.data,m_pCmdListenerResponseData);
                        }
                        else
                        {
                            emit cmdListenerResultReady(0xE1,m_cmdListenerRequest.cmd,NULL);
                        }
                    }

                    //запрос чтения одного или более регистров INPUT REGISTERS  (6 байт = 2 команда + 2 адрес + 2 количество) 0x04 - read input registers
                    if (m_cmdListenerRequest.cmd==0x04)
                    {
                        int res = modbus_read_input_registers(mb,m_cmdListenerRequest.addr, m_cmdListenerRequest.data, m_pCmdListenerResponseData);
                        //The function shall return the number of read registers if successful. Otherwise it shall return -1 and set errno.

                        if (res==m_cmdListenerRequest.data)
                        {
                            emit cmdListenerResultReady(m_cmdListenerRequest.cmd,m_cmdListenerRequest.data,m_pCmdListenerResponseData);
                        }
                        else
                        {
                            emit cmdListenerResultReady(0xE1,m_cmdListenerRequest.cmd,NULL);
                        }
                    }


                } //if (m_isConnected)
                else
                {
                    cmdListenerResultReady(0xE0,m_cmdListenerRequest.cmd,NULL); //no connection error
                }

                m_cmdListenerRequest.cmd=0x00;
                //reset command after send response in SLOT RegionNode::CmdListenerSendResult(unsigned char byte1, unsigned char byte2, uint16_t *responseData)
                // this slot will be connected to SIGNAL cmdListenerResultReady


            } //if (m_cmdListenerRequest.cmd!=0x00)  //проверяем наличие команды в очереди, если есть выполняем

            if (CheckThreadStop())
            {
                modbus_close(mb);
                modbus_free(mb);
                return;
            }
            Sleep(200);
        }

    }//for(;;)


}
//======================================================================================================
