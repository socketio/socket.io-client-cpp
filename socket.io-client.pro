CONFIG -= qt

TARGET = socket.io-client

TEMPLATE = lib
CONFIG += staticlib

include(socket.io-client.pri)

!isEmpty(USE_SYSTEM_OPENSSL) {
    DEFINES += HAVE_OPENSSL
} else:!isEmpty(OPENSSL_INCLUDE_DIR):!isEmpty(OPENSSL_LIB_DIR) {
    DEFINES += HAVE_OPENSSL
    INCLUDEPATH += $$OPENSSL_INCLUDE_DIR
}

CONFIG += c++11

msvc {
    QMAKE_CXXFLAGS_WARN_ON += /wd4267
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += _SCL_SECURE_NO_WARNINGS
} else:clang|gcc {
    QMAKE_CXXFLAGS_WARN_ON += \
        -Wno-null-pointer-subtraction \
        -Wno-unknown-warning-option \
        -Wno-unknown-warning \
        -Wno-unused-command-line-argument
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
    $$SOCKET_IO_CLIENT_SRC/internal/sio_client_config.h \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_client_impl.h \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_packet.h

SOURCES += \
    $$SOCKET_IO_CLIENT_SRC/sio_client.cpp \
    $$SOCKET_IO_CLIENT_SRC/sio_socket.cpp \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_client_impl.cpp \
    $$SOCKET_IO_CLIENT_SRC/internal/sio_packet.cpp

DESTDIR = $$SOCKET_IO_CLIENT_BIN
