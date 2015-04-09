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


INCLUDEPATH += $$PWD/../../../build/include
DEPENDPATH += $$PWD/../../../build/lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../build/lib/release/ -lsioclient
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../build/lib/debug/ -lsioclient
else:unix: LIBS += -L$$PWD/../../../build/lib/ -lsioclient


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../build/lib/release/ -lboost_random
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../build/lib/debug/ -lboost_random
else:unix: LIBS += -L$$PWD/../../../build/lib/ -lboost_random

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../build/lib/release/ -lboost_system
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../build/lib/debug/ -lboost_system
else:unix: LIBS += -L$$PWD/../../../build/lib/ -lboost_system

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../build/lib/release/ -lboost_date_time
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../build/lib/debug/ -lboost_date_time
else:unix: LIBS += -L$$PWD/../../../build/lib/ -lboost_date_time
