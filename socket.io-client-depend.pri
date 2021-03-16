include(socket.io-client.pri)

msvc {
    PRE_TARGETDEPS += $$SOCKET_IO_CLIENT_BIN/socket.io-client.lib
} else {
    PRE_TARGETDEPS += $$SOCKET_IO_CLIENT_BIN/libsocket.io-client.a
}

LIBS += -L$$SOCKET_IO_CLIENT_BIN
LIBS += -lsocket.io-client
