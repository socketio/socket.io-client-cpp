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

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#ifndef _WIN32
#include "json.hpp" //nlohmann::json cannot build in MSVC
#endif

using namespace sio;

TEST_CASE( "test_packet_construct_1" )
{
    packet p("/nsp",nullptr,1001,true);
    CHECK(p.get_frame() == packet::frame_message);
    CHECK(p.get_message() == nullptr);
    CHECK(p.get_nsp() == std::string("/nsp"));
    CHECK(p.get_pack_id() == 1001);
}

TEST_CASE( "test_packet_construct_2" )
{
    packet p(packet::frame_ping);
    CHECK(p.get_frame() == packet::frame_ping);
    CHECK(p.get_message() == nullptr);
    CHECK(p.get_nsp() == std::string(""));
    CHECK(p.get_pack_id() == 0xFFFFFFFF);
}

TEST_CASE( "test_packet_construct_3" )
{
    packet p(packet::type_connect,"/nsp",nullptr);
    CHECK(p.get_frame() == packet::frame_message);
    CHECK(p.get_type() == packet::type_connect);
    CHECK(p.get_message() == nullptr);
    CHECK(p.get_nsp() == std::string("/nsp"));
    CHECK(p.get_pack_id() == 0xFFFFFFFF);
}

TEST_CASE( "test_packet_accept_1" )
{
    packet p(packet::type_connect,"/nsp",nullptr);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    CHECK(buffers.size() == 0);
    CHECK(payload == "40/nsp");
    INFO("outputing payload:" << payload)
}

TEST_CASE( "test_packet_accept_2" )
{
    packet p(packet::frame_ping);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    CHECK(buffers.size() == 0);
    CHECK(payload == "2");
    INFO("outputing payload:" << payload)
}

TEST_CASE( "test_packet_accept_3" )
{
    message::ptr array = array_message::create();
    array->get_vector().push_back(string_message::create("event"));
    array->get_vector().push_back(string_message::create("text"));
    packet p("/nsp",array,1001,true);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    CHECK(p.get_type() == packet::type_ack);
    CHECK(buffers.size() == 0);
    CHECK(payload == "43/nsp,1001[\"event\",\"text\"]");
    INFO("outputing payload:" << payload)
}

#ifndef _WIN32
TEST_CASE( "test_packet_accept_4" )
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
    CHECK(p.get_type() == packet::type_binary_event);
    REQUIRE(buffers.size() == 2);
    size_t json_start = payload.find("{");
    REQUIRE(json_start!=std::string::npos);
    std::string header = payload.substr(0,json_start);
    CHECK(header=="452-/nsp,1001");
    INFO("outputing payload:" << payload)
    std::string json = payload.substr(json_start);
    nlohmann::json j = nlohmann::json::parse(json);
    CHECK(j["desc"].get<std::string>() == "Bin of 100 bytes");
    INFO("outputing payload desc::" << j["desc"].get<std::string>())
    CHECK((bool)j["bin1"]["_placeholder"]);
    INFO("outputing payload bin1:" << j["bin1"].dump())
    CHECK((bool)j["bin2"]["_placeholder"]);
    INFO("outputing payload bin2:" << j["bin2"].dump())
    int bin1Num = j["bin1"]["num"].get<int>();
    char numchar[] = {0,0};
    numchar[0] = bin1Num+'0';
    CHECK(buffers[bin1Num]->length()==101);
    INFO("outputing payload bin1 num:" << numchar)
    CHECK(buffers[bin1Num]->at(50)==0);
    CHECK(buffers[bin1Num]->at(0) == packet::frame_message);
    int bin2Num = j["bin2"]["num"].get<int>();
    numchar[0] = bin2Num+'0';
    CHECK(buffers[bin2Num]->length()==51);
    INFO("outputing payload bin2 num:" << numchar)
    CHECK(buffers[bin2Num]->at(25)==1);
    CHECK(buffers[bin2Num]->at(0) == packet::frame_message);
}
#endif

TEST_CASE( "test_packet_parse_1" )
{
    packet p;
    bool hasbin = p.parse("42/nsp,1001[\"event\",\"text\"]");
    CHECK(!hasbin);
    CHECK(p.get_frame() == packet::frame_message);
    CHECK(p.get_type() == packet::type_event);
    CHECK(p.get_nsp() == "/nsp");
    CHECK(p.get_pack_id() == 1001);
    CHECK(p.get_message()->get_flag() == message::flag_array);
    REQUIRE(p.get_message()->get_vector()[0]->get_flag() == message::flag_string);
    CHECK(p.get_message()->get_vector()[0]->get_string() == "event");
    REQUIRE(p.get_message()->get_vector()[1]->get_flag() == message::flag_string);
    CHECK(p.get_message()->get_vector()[1]->get_string() == "text");

    hasbin = p.parse("431111[\"ack\",{\"count\":5}]");
    CHECK(!hasbin);
    CHECK(p.get_frame() == packet::frame_message);
    CHECK(p.get_type() == packet::type_ack);
    CHECK(p.get_pack_id() == 1111);
    CHECK(p.get_nsp() == "/");
    CHECK(p.get_message()->get_flag() == message::flag_array);
    REQUIRE(p.get_message()->get_vector()[0]->get_flag() == message::flag_string);
    CHECK(p.get_message()->get_vector()[0]->get_string() == "ack");
    REQUIRE(p.get_message()->get_vector()[1]->get_flag() == message::flag_object);
    CHECK(p.get_message()->get_vector()[1]->get_map()["count"]->get_int() == 5);
}

TEST_CASE( "test_packet_parse_2" )
{
    packet p;
    bool hasbin = p.parse("3");
    CHECK(!hasbin);
    CHECK(p.get_frame() == packet::frame_pong);
    CHECK(!p.get_message());
    CHECK(p.get_nsp() == "/");
    CHECK(p.get_pack_id() == -1);
    hasbin = p.parse("2");

    CHECK(!hasbin);
    CHECK(p.get_frame() == packet::frame_ping);
    CHECK(!p.get_message());
    CHECK(p.get_nsp() == "/");
    CHECK(p.get_pack_id() == -1);
}

TEST_CASE( "test_packet_parse_3" )
{
    packet p;
    bool hasbin = p.parse("40/nsp");
    CHECK(!hasbin);
    CHECK(p.get_type() == packet::type_connect);
    CHECK(p.get_frame() == packet::frame_message);
    CHECK(p.get_nsp() == "/nsp");
    CHECK(p.get_pack_id() == -1);
    CHECK(!p.get_message());
    p.parse("40");
    CHECK(p.get_type() == packet::type_connect);
    CHECK(p.get_nsp() == "/");
    CHECK(p.get_pack_id() == -1);
    CHECK(!p.get_message());
    p.parse("44\"error\"");
    CHECK(p.get_type() == packet::type_error);
    CHECK(p.get_nsp() == "/");
    CHECK(p.get_pack_id() == -1);
    CHECK(p.get_message()->get_flag() == message::flag_string);
    p.parse("44/nsp,\"error\"");
    CHECK(p.get_type() == packet::type_error);
    CHECK(p.get_nsp() == "/nsp");
    CHECK(p.get_pack_id() == -1);
    CHECK(p.get_message()->get_flag() == message::flag_string);
}

TEST_CASE( "test_packet_parse_4" )
{
    packet p;
    bool hasbin = p.parse("452-/nsp,101[\"bin_event\",[{\"_placeholder\":true,\"num\":1},{\"_placeholder\":true,\"num\":0},\"text\"]]");
    CHECK(hasbin);
    char buf[101];
    buf[0] = packet::frame_message;
    memset(buf+1,0,100);

    std::string bufstr(buf,101);
    std::string bufstr2(buf,51);
    CHECK(p.parse_buffer(bufstr));
    CHECK(!p.parse_buffer(bufstr2));

    CHECK(p.get_frame() == packet::frame_message);
    CHECK(p.get_nsp() == "/nsp");
    CHECK(p.get_pack_id() == 101);
    message::ptr msg = p.get_message();
    REQUIRE(msg);
    REQUIRE(msg->get_flag() == message::flag_array);
    CHECK(msg->get_vector()[0]->get_string() == "bin_event");
    message::ptr array = msg->get_vector()[1];
    REQUIRE(array->get_flag() == message::flag_array);
    REQUIRE(array->get_vector()[0]->get_flag() == message::flag_binary);
    REQUIRE(array->get_vector()[1]->get_flag() == message::flag_binary);
    REQUIRE(array->get_vector()[2]->get_flag() == message::flag_string);
    CHECK(array->get_vector()[0]->get_binary()->size() == 50);
    CHECK(array->get_vector()[1]->get_binary()->size() == 100);
    CHECK(array->get_vector()[2]->get_string() == "text");

}
