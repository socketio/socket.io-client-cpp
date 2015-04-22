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
#ifndef _WIN32
#include "json.hpp" //nlohmann::json cannot build in MSVC
#endif

using namespace sio;
BOOST_AUTO_TEST_SUITE(test_packet)

BOOST_AUTO_TEST_CASE( test_packet_construct_1 )
{
    packet p("/nsp",nullptr,1001,true);
    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_message() == nullptr);
    BOOST_CHECK(p.get_nsp() == std::string("/nsp"));
    BOOST_CHECK(p.get_pack_id() == 1001);
}

BOOST_AUTO_TEST_CASE( test_packet_construct_2 )
{
    packet p(packet::frame_ping);
    BOOST_CHECK(p.get_frame() == packet::frame_ping);
    BOOST_CHECK(p.get_message() == nullptr);
    BOOST_CHECK(p.get_nsp() == std::string(""));
    BOOST_CHECK(p.get_pack_id() == 0xFFFFFFFF);
}

BOOST_AUTO_TEST_CASE( test_packet_construct_3 )
{
    packet p(packet::type_connect,"/nsp",nullptr);
    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_type() == packet::type_connect);
    BOOST_CHECK(p.get_message() == nullptr);
    BOOST_CHECK(p.get_nsp() == std::string("/nsp"));
    BOOST_CHECK(p.get_pack_id() == 0xFFFFFFFF);
}

BOOST_AUTO_TEST_CASE( test_packet_accept_1 )
{
    packet p(packet::type_connect,"/nsp",nullptr);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    BOOST_CHECK(buffers.size() == 0);
    BOOST_CHECK_MESSAGE(payload == "40/nsp",std::string("outputing payload:")+payload);
}

BOOST_AUTO_TEST_CASE( test_packet_accept_2 )
{
    packet p(packet::frame_ping);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    BOOST_CHECK(buffers.size() == 0);
    BOOST_CHECK_MESSAGE(payload == "2",std::string("outputing payload:")+payload);
}

BOOST_AUTO_TEST_CASE( test_packet_accept_3 )
{
    message::ptr array = array_message::create();
    array->get_vector().push_back(string_message::create("event"));
    array->get_vector().push_back(string_message::create("text"));
    packet p("/nsp",array,1001,true);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    BOOST_CHECK(p.get_type() == packet::type_ack);
    BOOST_CHECK(buffers.size() == 0);
    BOOST_CHECK_MESSAGE(payload == "43/nsp,1001[\"event\",\"text\"]",std::string("outputing payload:")+payload);
}

#ifndef _WIN32
BOOST_AUTO_TEST_CASE( test_packet_accept_4 )
{
    message::ptr binObj = object_message::create();
    binObj->get_map()["desc"] = string_message::create("Bin of 100 bytes");
    char bin[100];
    memset(bin,0,100*sizeof(char));
    binObj->get_map()["bin1"] = binary_message::create(std::shared_ptr<const std::string>(new std::string(bin,100)));
    char bin2[50];
    memset(bin2,1,50*sizeof(char));
    binObj->get_map()["bin2"] = binary_message::create(std::make_shared<const std::string>(bin2,50));

    packet p("/nsp",binObj,1001,false);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    BOOST_CHECK(p.get_type() == packet::type_binary_event);
    BOOST_REQUIRE(buffers.size() == 2);
    size_t json_start = payload.find("{");
    BOOST_REQUIRE(json_start!=std::string::npos);
    std::string header = payload.substr(0,json_start);
    BOOST_CHECK_MESSAGE(header=="452-/nsp,1001",std::string("outputing payload header:")+header);
    std::string json = payload.substr(json_start);
    nlohmann::json j = nlohmann::json::parse(json);
    BOOST_CHECK_MESSAGE(j["desc"].get<std::string>() == "Bin of 100 bytes", std::string("outputing payload desc:") + j["desc"].get<std::string>());
    BOOST_CHECK_MESSAGE((bool)j["bin1"]["_placeholder"] , std::string("outputing payload bin1:") + j["bin1"].dump());
    BOOST_CHECK_MESSAGE((bool)j["bin2"]["_placeholder"] , std::string("outputing payload bin2:") + j["bin2"].dump());
    int bin1Num = j["bin1"]["num"].get<int>();
    char numchar[] = {0,0};
    numchar[0] = bin1Num+'0';
    BOOST_CHECK_MESSAGE(buffers[bin1Num]->length()==101 , std::string("outputing payload bin1 num:")+numchar);
    BOOST_CHECK(buffers[bin1Num]->at(50)==0 && buffers[bin1Num]->at(0) == packet::frame_message);
    int bin2Num = j["bin2"]["num"].get<int>();
    numchar[0] = bin2Num+'0';
    BOOST_CHECK_MESSAGE(buffers[bin2Num]->length()==51 , std::string("outputing payload bin2 num:") + numchar);
    BOOST_CHECK(buffers[bin2Num]->at(25)==1 && buffers[bin2Num]->at(0) == packet::frame_message);
}
#endif

BOOST_AUTO_TEST_CASE( test_packet_parse_1 )
{
    packet p;
    bool hasbin = p.parse("42/nsp,1001[\"event\",\"text\"]");
    BOOST_CHECK(!hasbin);
    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_type() == packet::type_event);
    BOOST_CHECK(p.get_nsp() == "/nsp");
    BOOST_CHECK(p.get_pack_id() == 1001);
    BOOST_CHECK(p.get_message()->get_flag() == message::flag_array);
    BOOST_REQUIRE(p.get_message()->get_vector()[0]->get_flag() == message::flag_string);
    BOOST_CHECK(p.get_message()->get_vector()[0]->get_string() == "event");
    BOOST_REQUIRE(p.get_message()->get_vector()[1]->get_flag() == message::flag_string);
    BOOST_CHECK(p.get_message()->get_vector()[1]->get_string() == "text");

    hasbin = p.parse("431111[\"ack\",{\"count\":5}]");
    BOOST_CHECK(!hasbin);
    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_type() == packet::type_ack);
    BOOST_CHECK(p.get_pack_id() == 1111);
    BOOST_CHECK(p.get_nsp() == "/");
    BOOST_CHECK(p.get_message()->get_flag() == message::flag_array);
    BOOST_REQUIRE(p.get_message()->get_vector()[0]->get_flag() == message::flag_string);
    BOOST_CHECK(p.get_message()->get_vector()[0]->get_string() == "ack");
    BOOST_REQUIRE(p.get_message()->get_vector()[1]->get_flag() == message::flag_object);
    BOOST_CHECK(p.get_message()->get_vector()[1]->get_map()["count"]->get_int() == 5);
}

BOOST_AUTO_TEST_CASE( test_packet_parse_2 )
{
    packet p;
    bool hasbin = p.parse("3");
    BOOST_CHECK(!hasbin);
    BOOST_CHECK(p.get_frame() == packet::frame_pong);
    BOOST_CHECK(!p.get_message());
    BOOST_CHECK(p.get_nsp() == "/");
    BOOST_CHECK(p.get_pack_id() == -1);
    hasbin = p.parse("2");

    BOOST_CHECK(!hasbin);
    BOOST_CHECK(p.get_frame() == packet::frame_ping);
    BOOST_CHECK(!p.get_message());
    BOOST_CHECK(p.get_nsp() == "/");
    BOOST_CHECK(p.get_pack_id() == -1);
}

BOOST_AUTO_TEST_CASE( test_packet_parse_3 )
{
    packet p;
    bool hasbin = p.parse("40/nsp");
    BOOST_CHECK(!hasbin);
    BOOST_CHECK(p.get_type() == packet::type_connect);
    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_nsp() == "/nsp");
    BOOST_CHECK(p.get_pack_id() == -1);
    BOOST_CHECK(!p.get_message());
    p.parse("40");
    BOOST_CHECK(p.get_type() == packet::type_connect);
    BOOST_CHECK(p.get_nsp() == "/");
    BOOST_CHECK(p.get_pack_id() == -1);
    BOOST_CHECK(!p.get_message());
    p.parse("44\"error\"");
    BOOST_CHECK(p.get_type() == packet::type_error);
    BOOST_CHECK(p.get_nsp() == "/");
    BOOST_CHECK(p.get_pack_id() == -1);
    BOOST_CHECK(p.get_message()->get_flag() == message::flag_string);
    p.parse("44/nsp,\"error\"");
    BOOST_CHECK(p.get_type() == packet::type_error);
    BOOST_CHECK(p.get_nsp() == "/nsp");
    BOOST_CHECK(p.get_pack_id() == -1);
    BOOST_CHECK(p.get_message()->get_flag() == message::flag_string);
}

BOOST_AUTO_TEST_CASE( test_packet_parse_4 )
{
    packet p;
    bool hasbin = p.parse("452-/nsp,101[\"bin_event\",[{\"_placeholder\":true,\"num\":1},{\"_placeholder\":true,\"num\":0},\"text\"]]");
    BOOST_CHECK(hasbin);
    char buf[101];
    buf[0] = packet::frame_message;
    memset(buf+1,0,100);

    std::string bufstr(buf,101);
    std::string bufstr2(buf,51);
    BOOST_CHECK(p.parse_buffer(bufstr));
    BOOST_CHECK(!p.parse_buffer(bufstr2));

    BOOST_CHECK(p.get_frame() == packet::frame_message);
    BOOST_CHECK(p.get_nsp() == "/nsp");
    BOOST_CHECK(p.get_pack_id() == 101);
    message::ptr msg = p.get_message();
    BOOST_REQUIRE(msg&&msg->get_flag() == message::flag_array);
    BOOST_CHECK(msg->get_vector()[0]->get_string() == "bin_event");
    message::ptr array = msg->get_vector()[1];
    BOOST_REQUIRE(array->get_flag() == message::flag_array);
    BOOST_REQUIRE(array->get_vector()[0]->get_flag() == message::flag_binary);
    BOOST_REQUIRE(array->get_vector()[1]->get_flag() == message::flag_binary);
    BOOST_REQUIRE(array->get_vector()[2]->get_flag() == message::flag_string);
    BOOST_CHECK(array->get_vector()[0]->get_binary()->size() == 50);
    BOOST_CHECK(array->get_vector()[1]->get_binary()->size() == 100);
    BOOST_CHECK(array->get_vector()[2]->get_string() == "text");

}

BOOST_AUTO_TEST_SUITE_END()

