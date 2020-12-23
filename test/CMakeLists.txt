add_executable(sio_test sio_test.cpp)
set_property(TARGET sio_test PROPERTY CXX_STANDARD 11)
set_property(TARGET sio_test PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(sio_test sioclient)
target_include_directories(sio_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/catch/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)
add_test(sioclient_test sio_test)
