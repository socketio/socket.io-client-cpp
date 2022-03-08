cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
PROJECT(sioclient
        VERSION 3.1.0
        )

option(BUILD_SHARED_LIBS "Build the shared library" OFF)
option(BUILD_UNIT_TESTS "Builds unit tests target" OFF)

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
    # These will force WebsocketPP to compile with C++11
    -D_WEBSOCKETPP_CPP11_STL_
    -D_WEBSOCKETPP_CPP11_FUNCTIONAL_
)

add_library(sioclient ${ALL_SRC})
target_include_directories(sioclient PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src 
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/lib/websocketpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/rapidjson/include
    ${CMAKE_CURRENT_LIST_DIR}/lib/asio/asio/include
)

if (CMAKE_VERSION VERSION_GREATER "3.1")
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
    ${CMAKE_CURRENT_LIST_DIR}/src 
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/lib/websocketpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/rapidjson/include
    ${CMAKE_CURRENT_LIST_DIR}/lib/asio/asio/include
    ${OPENSSL_INCLUDE_DIR}
)

if (CMAKE_VERSION VERSION_GREATER "3.1")
set_property(TARGET sioclient_tls PROPERTY CXX_STANDARD 11)
set_property(TARGET sioclient_tls PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(sioclient_tls PRIVATE ${OPENSSL_LIBRARIES} )
else()
set_property(TARGET sioclient_tls APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++11")
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

include(GNUInstallDirs)

install(FILES ${ALL_HEADERS} 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS ${TARGET_LIBRARIES}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(BUILD_UNIT_TESTS)
message(STATUS "Building with unit test support.")
enable_testing()
add_subdirectory(test)
endif()
