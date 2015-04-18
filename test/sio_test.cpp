//
//  sio_test_sample.cpp
//
//  Created by Melo Yao on 3/24/15.
//

#include <sio_client.h>
#include <internal/sio_packet.h>
#include <functional>
#include <iostream>
#include <thread>

#define BOOST_TEST_MODULE sio_test
#include <boost/test/unit_test.hpp>

using namespace sio;
BOOST_AUTO_TEST_SUITE(packet_test)

BOOST_AUTO_TEST_CASE( packet_test_construct_1 )
{
    packet p("nsp",nullptr,1001,true);
    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_message() == nullptr);
    BOOST_CHECK(p.get_nsp() == std::string("nsp"));
    BOOST_CHECK(p.get_pack_id() == 1001);
}

BOOST_AUTO_TEST_CASE( packet_test_construct_2 )
{
    packet p(packet::frame_ping);
    BOOST_CHECK(p.get_frame() == packet::frame_ping);
    BOOST_CHECK(p.get_message() == nullptr);
    BOOST_CHECK(p.get_nsp() == std::string(""));
    BOOST_CHECK(p.get_pack_id() == 0xFFFFFFFF);
}

BOOST_AUTO_TEST_CASE( packet_test_construct_3 )
{
    packet p(packet::type_connect,"nsp",nullptr);
    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_type() == packet::type_connect);
    BOOST_CHECK(p.get_message() == nullptr);
    BOOST_CHECK(p.get_nsp() == std::string("nsp"));
    BOOST_CHECK(p.get_pack_id() == 0xFFFFFFFF);
}

BOOST_AUTO_TEST_SUITE_END()

