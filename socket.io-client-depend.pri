include(socket.io-client.pri)

msvc {
    PRE_TARGETDEPS += $$SOCKET_IO_CLIENT_BIN/socket.io-client.lib
} else {
    PRE_TARGETDEPS += $$SOCKET_IO_CLIENT_BIN/libsocket.io-client.a
}

LIBS += -L$$SOCKET_IO_CLIENT_BIN
LIBS += -lsocket.io-client

!isEmpty(USE_SYSTEM_OPENSSL) {
    LIBS += -lcrypto -lssl
} else:!isEmpty(OPENSSL_INCLUDE_DIR):!isEmpty(OPENSSL_LIB_DIR) {
    LIBS += -L$$OPENSSL_LIB_DIR    
    
    msvc {
        LIBS += -llibcrypto -llibssl
        PRE_TARGETDEPS += \
            $$OPENSSL_LIB_DIR/libcrypto.lib \
            $$OPENSSL_LIB_DIR/libssl.lib
    } else {
        LIBS += -lcrypto -lssl
        PRE_TARGETDEPS += \
            $$OPENSSL_LIB_DIR/libcrypto.a \
            $$OPENSSL_LIB_DIR/libssl.a
    }
}
