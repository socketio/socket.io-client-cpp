cmake_minimum_required (VERSION 2.6)
PROJECT(sioclient)

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON) 
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.57.0 COMPONENTS system date_time random asio) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/lib/websocketpp ${PROJECT_SOURCE_DIR}/lib/rapidjson/include) 
    aux_source_directory(${PROJECT_SOURCE_DIR}/src ALL_SRC)
    add_library(sioclient STATIC ${ALL_SRC}) 
    target_link_libraries(sioclient ${Boost_LIBRARIES})
endif()