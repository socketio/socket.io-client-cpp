cmake_minimum_required(VERSION 3.4 FATAL_ERROR)

PROJECT(sioclient
    VERSION 3.1.0
)

option(BUILD_SHARED_LIBS "Build the shared library" OFF)
option(BUILD_UNIT_TESTS "Builds unit tests target" OFF)
option(USE_SUBMODULES "Use source in local submodules instead of system libraries" ON)

set(MAJOR 1)
set(MINOR 6)
set(PATCH 0)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(DEFAULT_BUILD_TYPE "Release")
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src ALL_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src/internal ALL_SRC)

file(GLOB ALL_HEADERS ${CMAKE_CURRENT_LIST_DIR}/src/*.h)

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

target_include_directories(sioclient PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
    ${MODULE_INCLUDE_DIRS}
)

if(CMAKE_VERSION VERSION_GREATER "3.1")
    set_property(TARGET sioclient PROPERTY CXX_STANDARD 11)
    set_property(TARGET sioclient PROPERTY CXX_STANDARD_REQUIRED ON)
else()
    set_property(TARGET sioclient APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++11")
endif()

if(BUILD_SHARED_LIBS)
    set_target_properties(sioclient
        PROPERTIES
        SOVERSION ${MAJOR}
        VERSION ${MAJOR}.${MINOR}.${PATCH}
    )
endif()

list(APPEND TARGET_LIBRARIES sioclient)

find_package(OpenSSL)

if(OPENSSL_FOUND)
    add_library(sioclient_tls ${ALL_SRC})
    target_include_directories(sioclient_tls PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
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
            SOVERSION ${MAJOR}
            VERSION ${MAJOR}.${MINOR}.${PATCH}
        )
    endif()

    list(APPEND TARGET_LIBRARIES sioclient_tls)
endif()

export(PACKAGE sioclient)

include(GNUInstallDirs)

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

if(BUILD_UNIT_TESTS)
    message(STATUS "Building with unit test support.")
    enable_testing()
    add_subdirectory(test)
endif()
