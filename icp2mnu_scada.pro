#-------------------------------------------------
#
# Project created by QtCreator 2015-07-14T08:18:39
#
#-------------------------------------------------

QT       += core gui network sql script

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = icp2mnu_scada
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    src_libmodbus/modbus-tcp.c \
    src_libmodbus/modbus.c \
    src_libmodbus/modbus-data.c \
    src_libmodbus/modbus-rtu.c \
    srvretranslater.cpp \
    nodes.cpp \
    alarm.cpp \
    tag.cpp \
    logger.cpp \
    odbcdb.cpp \
    configreader.cpp \
    nodedataviewer.cpp \
    scadaserver.cpp \
    trendwriter.cpp \
    SingleApplication/singleapplication.cpp \
    event.cpp \
    trendserver.cpp

HEADERS  += mainwindow.h \
    autostopthread.h \
    srvretranslater.h \
    nodes.h \
    alarm.h \
    tag.h \
    logger.h \
    odbcdb.h \
    configreader.h \
    nodedataviewer.h \
    scadaserver.h \
    trendwriter.h \
    SingleApplication/singleapplication.h \
    event.h \
    trendserver.h

FORMS    += mainwindow.ui \
    logger.ui \
    nodedataviewer.ui

LIBS += -lws2_32

#add ico to windows application
RC_FILE = myapp.rc

RESOURCES += \
    res.qrc

DEFINES += QAPPLICATION_CLASS=QApplication # for using Single Application
