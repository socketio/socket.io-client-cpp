#-------------------------------------------------
#
# Project created by QtCreator 2015-03-30T19:25:23
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SioChatDemo
TEMPLATE = app

CONFIG+=no_keywords
CONFIG+=c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    nicknamedialog.cpp

HEADERS  += mainwindow.h \
    nicknamedialog.h

FORMS    += mainwindow.ui \
    nicknamedialog.ui

CONFIG(debug, debug|release):DEFINES +=DEBUG=1

LIBS += -L$$PWD/../../../build/lib/ -lsioclient
LIBS += -L$$PWD/../../../build/lib/ -lboost_system
LIBS += -L$$PWD/../../../build/lib/ -lboost_date_time
LIBS += -L$$PWD/../../../build/lib/ -lboost_random

INCLUDEPATH += $$PWD/../../../build/include
DEPENDPATH += $$PWD/../../../build/lib
