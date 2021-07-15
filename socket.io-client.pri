SOCKET_IO_CLIENT_ROOT = $$PWD
SOCKET_IO_CLIENT_SRC = $$SOCKET_IO_CLIENT_ROOT/src
SOCKET_IO_CLIENT_BIN = $$PWD/bin
SOCKET_IO_CLIENT_DEPEND = $$SOCKET_IO_CLIENT_ROOT/lib

msvc {
    lessThan(QMAKE_MSC_VER, 1900) {
        DEFINES += _WEBSOCKETPP_NOEXCEPT_TOKEN_=_NOEXCEPT
    } else {
        DEFINES += _WEBSOCKETPP_NOEXCEPT_TOKEN_=noexcept
    }
    QMAKE_CXXFLAGS += /wd4503
} else {
    DEFINES += _WEBSOCKETPP_NOEXCEPT_TOKEN_=noexcept
}

win32 {
    DEFINES += _WINSOCK_DEPRECATED_NO_WARNINGS
} else {
    DEFINES += _WEBSOCKETPP_CPP11_THREAD_
}

OPENSSL_INCLUDE_DIR = $$(OPENSSL_INCLUDE_DIR)
OPENSSL_LIB_DIR = $$(OPENSSL_LIB_DIR)

DEFINES += ASIO_STANDALONE
DEFINES += _WEBSOCKETPP_CPP11_TYPE_TRAITS_
DEFINES += _WEBSOCKETPP_CPP11_MEMORY_
DEFINES += _WEBSOCKETPP_CPP11_FUNCTIONAL_
DEFINES += _WEBSOCKETPP_CPP11_CHRONO_
DEFINES += _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
DEFINES += _WEBSOCKETPP_CPP11_REGEX_
DEFINES += _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
DEFINES += _WEBSOCKETPP_INITIALIZER_LISTS_
DEFINES += _WEBSOCKETPP_CONSTEXPR_TOKEN_=
DEFINES += RAPIDJSON_HAS_CXX11_RVALUE_REFS=1

macx:SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN-macx
linux:SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN-linux
win32 {
    SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN-win32
    msvc:SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN-msvc
}
clang:SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN-clang
else::gcc:SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN-gcc

SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN-$$QT_ARCH

CONFIG(debug, debug|release) {
    SOCKET_IO_CLIENT_BIN = $$SOCKET_IO_CLIENT_BIN/debug
}

INCLUDEPATH += \
    $$SOCKET_IO_CLIENT_SRC \
    $$SOCKET_IO_CLIENT_DEPEND/asio/asio/include \
    $$SOCKET_IO_CLIENT_DEPEND/catch/single_include \
    $$SOCKET_IO_CLIENT_DEPEND/rapidjson/include \
    $$SOCKET_IO_CLIENT_DEPEND/websocketpp
