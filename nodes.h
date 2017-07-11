#ifndef NODES_H
#define NODES_H


#include <QTimer>
#include <QTime>
#include <QFile>
#include <QtScript/QScriptEngine>

#include "autostopthread.h"
#include "srvretranslater.h"
#include "logger.h"
#include "scadaserver.h"

#include "./src_libmodbus/modbus.h"

class ScadaServer;

//==============================================================
struct virt_expr_member_struct
{
    QString objectName;
    uint numInBuff;
};

//сравнение для сортировки по убыванию длины имени объекта
bool operator<(const virt_expr_member_struct &a, const virt_expr_member_struct &b);

struct virt_tag_struct
{
    uint numInBuff;
    QString virtTagExpression;
    QVector<virt_expr_member_struct> vectVirtTagExprMembers;
};

//==============================================================
class CommonTrend
{
private:
    constexpr static const float min_float=-3.4028234663852886e+38;   //минимальное число float, используется как "пустое" значение,
                                                                      //для отличия от NaN которое обозначает результат некоректного скриптового выражения
    float lastValues[10];

public:
    CommonTrend(QString objectName,QString trendName,uint numInBuff)
    {
        m_objectName=objectName;
        m_trendName=trendName;
        m_numInBuff=numInBuff;
        for (int i=0;i<10;++i)
        {
            lastValues[i]=min_float;
        }
    }
    QString m_objectName;
    QString m_trendName;
    uint m_numInBuff;

    void AddLastValue(float val)
    {
        for (int i=1;i<10;++i)
        {
            lastValues[i]=lastValues[i-1];
        }
        lastValues[0]=val;
    }
    float GetLastValue(uint depth=0)
    {
        if (depth>=10) return min_float;
        return lastValues[depth];
    }
};

enum NodeType
{
    MODBUS=0,
    MNU_SCADA
};
//==============================================================
class CommonNode: public AutoStopThread
{
    Q_OBJECT
public:

    CommonNode();
    virtual ~CommonNode();

    static CommonNode* CreateNode(QString objectName, QString objectType,
                                  QString IP_addr, uint port,
                                  uint port_repl, uint port_local,
                                  uint modbus_start_address,
                                  uint num_float_tags);

    QString m_IP_addr;
    unsigned int m_port;
    unsigned int m_port_repl;
    QString m_nameObject;
    QString m_typeObject;   //modbus or mnu_scada or virtual

    QString m_text_client;
    QString m_text_repl;
    unsigned int m_modbus_start_address;
    bool m_isConnected;
    bool m_isReaded;
    SrvReTranslater m_srv;
    unsigned int m_port_local;
    QString FormattedNodeString();
    uint m_num_reads;
    uint m_sec_counter;
    unsigned int m_this_number;
    ScadaServer *ss;

    Logger *logger;
private:
    static uint nodes_counter;

signals:
    void textchange(int iUzel,QString objectName, QString newText);
    void textchange_repl(int iUzel,QString objectName, QString newText);
    void textSave2LogFile(int iUzel,QString objectName, QString newText);

};
//==============================================================

class ModbusNode: public CommonNode
{
    Q_OBJECT
public:
    explicit ModbusNode(int this_number,QString objectName,QString objectType,
                        QString IP_addr, uint port,
                        uint port_repl,uint port_local,
                        uint modbus_start_address,
                        uint num_float_tags);
    virtual ~ModbusNode()
    {
        ;
    }
    virtual void run();   // poll loop thread
};


//============================================================
class MnuScadaNode : public CommonNode
{
    Q_OBJECT
public:
    explicit MnuScadaNode(int this_number,QString objectName,QString objectType,
                          QString IP_addr, uint port,
                          uint port_repl,uint port_local,
                          uint modbus_start_address,
                          uint num_float_tags);
    virtual ~MnuScadaNode();

    QTcpSocket *socket;
    QTimer timerConnectLoop;

    virtual void run();  //trend recovery thread

    void doConnect();
    bool getStateConnected();
    bool getStateConnecting();

    bool need_restart_repl;

public slots:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError er);
    void bytesWritten(qint64 bytes);
    void readyRead();

    void TimerConnectLoopEvent();



};
//===============================================================
//Виртуальный контроллер на сервере
//производит рассчет величин на основе полученных с удаленных узлов
//к нему можно подключиться по протоколу MADBUS
class VirtualNode: public CommonNode
{
    Q_OBJECT
public:
    explicit VirtualNode(int this_number,QString objectName,QString objectType,
                         QString IP_addr, uint port,
                         uint port_repl,uint port_local,
                         uint modbus_start_address,
                         uint num_float_tags);
    virtual ~VirtualNode();

    QVector<virt_tag_struct> vectVirtTags;

private:
    QTimer timerCalculateVariables;
    QScriptEngine virtControllerScriptEngine;

public slots:
    void TimerCalculateVariablesEvent();
//      virtual void run();   //do not need, QTimer is better
};
//================================================================

/*
  Узел для реализации протокола РЕГИОН - протокол телеметрии  погружных
  насосов скважин (комплекс ТРИОЛ, там контроллер УМКА)
  Передача по модбасРТУ, потом модемом преобразовывается в модбасТСП.
  Специфика в том, что данные передаются как двухбайтный инт., со
  сдвигом десятичной запятой при необходимости. Передается 32 параметра
  адреса начинаются с 0x0100 в области INPUT REGISTERS (30xxxx)(т.е.
  диапазон десятичный адресов - 30257-30288)

  + в диапазоне HOLDING REGISTERS есть регистры для записи в них управляющих значений,
  для управления открывается отдельный сокет-сервер,

  + команда старт/стоп - модбас команда 0x05 по адресу 0x0201 (write single coil)

*/

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
struct CmdListenerRequest
{
    uint16_t cmd;
    uint16_t addr;
    uint16_t data; // value or number of registers
};

/*
struct CmdListenerResponse
{
    unsigned char byte1; // cmd- 0x06, 0x05, 0x03, 0x04 or ERROR byte = E0, E1, E2
    unsigned char byte2; // byte2=0x00 if write command OK, or number of registers if read command OK;
                         // if number of registers>0 then data written in cmdListenerResponseData array
                        //  if byte1==error, byte2==command which is not executed (0x06, 0x05, 0x03, 0x04)
};
*/
struct Multiplier
{
    Multiplier(){}
    Multiplier(float mult,int index):multiplier(mult),index_in_buff(index){}
    float multiplier;
    int index_in_buff;
};

class RegionNode: public CommonNode
{
    Q_OBJECT
public:
    explicit RegionNode(int this_number,QString objectName,QString objectType,
                        QString IP_addr, uint port,
                        uint port_repl,uint port_local,
                        uint modbus_start_address,
                        uint num_float_tags);
    virtual ~RegionNode();

    virtual void run();   // poll loop thread

    QTcpServer* m_pCmdListenerServerSocket;
    QList<QTcpSocket*> m_pCmdListenerClientSocketList;

    QTcpSocket* lastRequestClient;
    CmdListenerRequest m_cmdListenerRequest;
    //CmdListenerResponse m_cmdListenerResponse;
    uint16_t m_pCmdListenerResponseData[256];

    //
    int m_register_count;
    QVector<Multiplier> m_multipliers;

    //В настройках протокола УМКИ есть параметр "Дискретность давления на входе",
    //что определяет этот множитель как 1 или 0.01, что определяет разницу между
    //протоколами region и region2 соответственно
    float m_pressureInMultiplier;
    void SetPressureInMultiplier(float newPressureInMultiplier)
    {
        m_pressureInMultiplier=newPressureInMultiplier;
    }



public slots:
    // New client connection
    void CmdListenerNewConnection();
    // Slot to handle disconnected client
    void CmdListenerClientDisconnected();
    void CmdListenerReadyRead();
    void CmdListenerSendResult(unsigned char byte1, unsigned char byte2, uint16_t* responseData);

signals:
    void cmdListenerResultReady(unsigned char byte1, unsigned char byte2, uint16_t* responseData);

};

//==============================================================

#endif // NODES_H
