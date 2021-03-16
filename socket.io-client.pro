CONFIG -= qt

TARGET = socket.io-client

TEMPLATE = lib
CONFIG += staticlib

include(socket.io-client.pri)

CONFIG += c++11 warn_off

unix|win32-g++ {
    QMAKE_CXXFLAGS_WARN_OFF -= -w
    QMAKE_CXXFLAGS += -Wall
} else {
    win32 {
        QMAKE_CXXFLAGS_WARN_OFF -= -W0
        QMAKE_CXXFLAGS += -W3 /wd4267
        DEFINES += _CRT_SECURE_NO_WARNINGS
        DEFINES += _SCL_SECURE_NO_WARNINGS
    }
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG=1
} else {
    DEFINES += NDEBUG
}

HEADERS += \
    $$SOCKET_IO_CLIENT_SRC/sio_client.h \
    $$SOCKET_IO_CLIENT_SRC/sio_message.h \
    $$SOCKET_IO_CLIENT_SRC/sio_socket.h \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_client_impl.h \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_packet.h

SOURCES += \
    $$SOCKET_IO_CLIENT_SRC/sio_client.cpp \
    $$SOCKET_IO_CLIENT_SRC/sio_socket.cpp \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_client_impl.cpp \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_packet.cpp

DESTDIR = $$SOCKET_IO_CLIENT_BIN
