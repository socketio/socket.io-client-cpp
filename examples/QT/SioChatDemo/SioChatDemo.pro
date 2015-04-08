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
    ../../../src/sio_client.cpp \
    ../../../src/sio_socket.cpp \
    ../../../src/internal/sio_packet.cpp \
    ../../../src/internal/sio_client_impl.cpp \
    nicknamedialog.cpp

HEADERS  += mainwindow.h \
        ../../../src/sio_client.h \
        ../../../src/sio_message.h \
        ../../../src/sio_socket.h \
        ../../../src/internal/sio_packet.h \
        ../../../src/internal/sio_client_impl.h \
    nicknamedialog.h

FORMS    += mainwindow.ui \
    nicknamedialog.ui

CONFIG(debug, debug|release):DEFINES +=DEBUG=1

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/release/ -lboost
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/debug/ -lboost
else:unix: LIBS += -L$$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/ -lboost

INCLUDEPATH += $$PWD/../../iOS/SioChatDemo/boost/src/boost_1_55_0
INCLUDEPATH += $$PWD/../../../lib/rapidjson/include
INCLUDEPATH += $$PWD/../../../lib/websocketpp
INCLUDEPATH += $$PWD/../../../src
DEPENDPATH += $$PWD/../../iOS/SioChatDemo/boost/src/boost_1_55_0

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/release/libboost.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/debug/libboost.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/release/boost.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/debug/boost.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../iOS/SioChatDemo/boost/osx/build/x86_64/libboost.a
