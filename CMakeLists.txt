cmake_minimum_required(VERSION 3.12...3.27 FATAL_ERROR)

PROJECT(sioclient
    VERSION 3.1.0
)

option(BUILD_SHARED_LIBS "Build the shared library" OFF)
option(BUILD_UNIT_TESTS "Builds unit tests target" OFF)
option(USE_SUBMODULES "Use source in local submodules instead of system libraries" ON)
option(DISABLE_LOGGING "Do not print logging messages" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(DEFAULT_BUILD_TYPE "Release")
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Testing only available if this is the main app
    # Note this needs to be done in the main CMakeLists
    # since it calls enable_testing, which must be in the
    # main CMakeLists.
    include(CTest)
endif()

add_definitions(
    # These will force ASIO to compile without Boost
    -DBOOST_DATE_TIME_NO_LIB
    -DBOOST_REGEX_NO_LIB
    -DASIO_STANDALONE

    # These will force sioclient to compile with C++11
    -D_WEBSOCKETPP_CPP11_STL_
    -D_WEBSOCKETPP_CPP11_FUNCTIONAL_
    -D_WEBSOCKETPP_CPP11_TYPE_TRAITS_
    -D_WEBSOCKETPP_CPP11_CHRONO_
)

if (DISABLE_LOGGING)
    add_definitions(-DSIO_DISABLE_LOGGING)
endif()

set(ALL_SRC 
    "src/sio_client.cpp"
    "src/sio_socket.cpp"
    "src/internal/sio_client_impl.cpp"
    "src/internal/sio_packet.cpp"
)
add_library(sioclient ${ALL_SRC})

if(USE_SUBMODULES)
    set(MODULE_INCLUDE_DIRS
        ${CMAKE_CURRENT_LIST_DIR}/lib/websocketpp
        ${CMAKE_CURRENT_LIST_DIR}/lib/rapidjson/include
        ${CMAKE_CURRENT_LIST_DIR}/lib/asio/asio/include
    )
else()
    find_package(websocketpp CONFIG REQUIRED)
    find_package(asio CONFIG REQUIRED)
    find_package(RapidJSON CONFIG REQUIRED)
    target_link_libraries(sioclient PRIVATE websocketpp::websocketpp asio asio::asio rapidjson)
endif()

target_include_directories(sioclient 
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
    ${MODULE_INCLUDE_DIRS}
)

set_property(TARGET sioclient PROPERTY CXX_STANDARD 11)
set_property(TARGET sioclient PROPERTY CXX_STANDARD_REQUIRED ON)

if(BUILD_SHARED_LIBS)
    set_target_properties(sioclient
        PROPERTIES
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
    )
endif()

list(APPEND TARGET_LIBRARIES sioclient)

find_package(OpenSSL)

if(OPENSSL_FOUND)
    add_library(sioclient_tls ${ALL_SRC})
    target_include_directories(sioclient_tls PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${MODULE_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
    )

    set_property(TARGET sioclient_tls PROPERTY CXX_STANDARD 11)
    set_property(TARGET sioclient_tls PROPERTY CXX_STANDARD_REQUIRED ON)

    target_link_libraries(sioclient_tls PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    if (NOT USE_SUBMODULES) 
        target_link_libraries(sioclient_tls PRIVATE websocketpp::websocketpp asio asio::asio rapidjson)
    endif()

    target_compile_definitions(sioclient_tls PRIVATE -DSIO_TLS)

    if(BUILD_SHARED_LIBS)
        set_target_properties(sioclient_tls
            PROPERTIES
            SOVERSION ${PROJECT_VERSION_MAJOR}
            VERSION ${PROJECT_VERSION}
        )
    endif()

    list(APPEND TARGET_LIBRARIES sioclient_tls)
endif()

export(PACKAGE sioclient)

include(GNUInstallDirs)

file(GLOB ALL_HEADERS ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
install(FILES ${ALL_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS ${TARGET_LIBRARIES} EXPORT sioclientTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# === generate a CMake Config File ===
include(CMakePackageConfigHelpers)
set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/sioclient)
string(REGEX REPLACE "([^;]+)" "find_dependency(\\1)" _find_dependency_calls "${_package_dependencies}")
string(REPLACE ";" "\n" _find_dependency_calls "${_find_dependency_calls}")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/sioclient/sioclientConfigVersion.cmake"
    VERSION ${sioclient_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export(EXPORT sioclientTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/sioclient/sioclientTargets.cmake"
    NAMESPACE sioclient::
)

configure_package_config_file(sioclientConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/sioclient/sioclientConfig.cmake"
    INSTALL_DESTINATION "${ConfigPackageLocation}"
)

install(EXPORT sioclientTargets
    NAMESPACE
    sioclient::
    DESTINATION
    ${ConfigPackageLocation}
)
install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/sioclient/sioclientConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/sioclient/sioclientConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/sioclient/sioclientTargets.cmake"
    DESTINATION
    ${ConfigPackageLocation}
)

if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING) OR BUILD_UNIT_TESTS)
    add_subdirectory(test)
endif()
