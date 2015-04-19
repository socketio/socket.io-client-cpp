cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
PROJECT(sioclient)

function(get_lib_name out_lib_name component)
if(MSVC)
set(${out_lib_name} "${component}.lib" PARENT_SCOPE)
else()
set(${out_lib_name} "lib${component}.a" PARENT_SCOPE)
endif()
endfunction()

function(install_debug_libs lib_dir)
get_lib_name(renamed_system "boost_system")
install(FILES ${Boost_SYSTEM_LIBRARY_DEBUG}
    CONFIGURATIONS "Debug" DESTINATION ${lib_dir} RENAME ${renamed_system}
)
get_lib_name(renamed_date_time "boost_date_time")
install(FILES ${Boost_DATE_TIME_LIBRARY_DEBUG}
    CONFIGURATIONS "Debug" DESTINATION ${lib_dir} RENAME ${renamed_date_time}
)
get_lib_name(renamed_random "boost_random")
install(FILES ${Boost_RANDOM_LIBRARY_DEBUG}
    CONFIGURATIONS "Debug" DESTINATION ${lib_dir} RENAME ${renamed_random}
)

install(TARGETS sioclient
    CONFIGURATIONS "Debug" DESTINATION ${lib_dir}
)
endfunction()

function(install_release_libs lib_dir)
get_lib_name(renamed_system "boost_system")
install(FILES ${Boost_SYSTEM_LIBRARY_RELEASE}
    CONFIGURATIONS "Release" DESTINATION ${lib_dir} RENAME ${renamed_system}
)
get_lib_name(renamed_date_time "boost_date_time")
install(FILES ${Boost_DATE_TIME_LIBRARY_RELEASE}
    CONFIGURATIONS "Release" DESTINATION ${lib_dir} RENAME ${renamed_date_time}
)
get_lib_name(renamed_random "boost_random")
install(FILES ${Boost_RANDOM_LIBRARY_RELEASE}
    CONFIGURATIONS "Release" DESTINATION ${lib_dir} RENAME ${renamed_random}
)

install(TARGETS sioclient
    CONFIGURATIONS "Release" DESTINATION ${lib_dir}
)
endfunction()


MESSAGE(STATUS ${CMAKE_CURRENT_LIST_DIR})
if(NOT CMAKE_BUILD_TYPE )
MESSAGE(STATUS "not define build type, set to release" )
set(CMAKE_BUILD_TYPE Release )
elseif(NOT (${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "Debug" ))
MESSAGE(SEND_ERROR "CMAKE_BUILD_TYPE must be either Release or Debug")
return()
endif()

set(BOOST_VER "1.55.0" CACHE STRING "boost version" )

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON) 
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost ${BOOST_VER} REQUIRED COMPONENTS system date_time random) 

aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src ALL_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src/internal ALL_SRC)
file(GLOB ALL_HEADERS ${CMAKE_CURRENT_LIST_DIR}/src/*.h )
MESSAGE(STATUS ${ALL_HEADERS} )

add_library(sioclient STATIC ${ALL_SRC}) 
target_include_directories(sioclient PRIVATE ${Boost_INCLUDE_DIRS} 
    ${CMAKE_CURRENT_LIST_DIR}/src 
    ${CMAKE_CURRENT_LIST_DIR}/lib/websocketpp 
    ${CMAKE_CURRENT_LIST_DIR}/lib/rapidjson/include
)

set_property(TARGET sioclient PROPERTY CXX_STANDARD 11)
set_property(TARGET sioclient PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(sioclient PRIVATE ${Boost_LIBRARIES})
install(FILES ${ALL_HEADERS} 
    DESTINATION "${CMAKE_CURRENT_LIST_DIR}/build/include"
    )

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
install_debug_libs("${CMAKE_CURRENT_LIST_DIR}/build/lib/${CMAKE_BUILD_TYPE}" )
else()
install_release_libs("${CMAKE_CURRENT_LIST_DIR}/build/lib/${CMAKE_BUILD_TYPE}" )
endif()
