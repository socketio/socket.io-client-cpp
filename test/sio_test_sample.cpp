//
//  sio_test_sample.cpp
//
//  Created by Melo Yao on 3/24/15.
//

#include "sio_client.h"

#include <functional>
#include <iostream>
#include <thread>

using namespace sio;

class connection_listener
{
    sio::client &handler;
    
public:
    
    connection_listener(sio::client& h):
    handler(h)
    {
    }
    

    void on_connected()
    {
        handler.emit("test_text", "test payload");
        std::shared_ptr<std::string> binary = std::make_shared<std::string>();
        char test[100];
        memset(test, 0, sizeof(char)*100);
        binary->append(test, 100);
        handler.emit("test_binary", binary);
        
        message::ptr obj = object_message::create();
        message::ptr b_p = binary_message::create(binary);

        obj->get_map()["bin"] = b_p;
        obj->get_map()["path"] = string_message::create("./test.bin");
        handler.emit("test ack", obj,[](message::ptr const& ack_data){
            std::cout<<"Listener1:test ack:"<<std::endl;
            if(ack_data)
            {
                if (ack_data->get_flag() == message::flag_string) {
                    std::cout<<static_cast<string_message*>(ack_data.get())->get_string()<<std::endl;
                }
            }
        });
        
    }
    void on_close(handler::close_reason const& reason)
    {
        std::cout<<"Listener1:sio closed "<<std::endl;
    }
};


class connection_listener2
{
    sio::client &handler;
    
public:
    
    connection_listener2(sio::client& h):
    handler(h)
    {
    }
    

    void on_connected()
    {
        
        std::shared_ptr<std::string> binary(new std::string());
        char test[100];
        memset(test, 0, sizeof(char)*100);
        binary->append(test, 100);
        message::ptr obj = object_message::create();
        message::ptr b_p = binary_message::create(binary);
        object_message* o = static_cast<object_message*>(obj.get());
        o->get_map()["bin"] = b_p;
        o->get_map()["path"] = string_message::create("./test.bin");
        handler.emit("test ack", obj,[](message::ptr const& ack_data){
            std::cout<<"Listener2:test ack"<<std::endl;
            if(ack_data)
            {
                if (ack_data->get_flag() == message::flag_string) {
                    std::cout<<static_cast<string_message*>(ack_data.get())->get_string()<<std::endl;
                }
            }
        });
    }
    
    void on_fail()
    {
        std::cout<<"Listener2:sio failed "<<std::endl;
    }
};

int main(int argc ,const char* args[])
{
    sio::client h;
    connection_listener l(h);
    h.set_connect_listener(std::bind(&connection_listener::on_connected, &l));
    h.set_close_listener(std::bind(&connection_listener::on_close, &l,std::placeholders::_1));
    h.connect("http://127.0.0.1:3000");
    
    std::this_thread::sleep_for(std::chrono::seconds(10));

    h.sync_close();//will block when truely closed.
    h.clear_con_listeners();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connection_listener2 l2(h);
    h.set_connect_listener(std::bind(&connection_listener2::on_connected, &l2));
    h.set_fail_listener(std::bind(&connection_listener2::on_fail,&l2));
    //reconnect only can be used in case that sio is closed by network drop,so here, reconnect won't be success, you are expected to receive fail event.
    h.reconnect("http://127.0.0.1:3000");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    h.clear_con_listeners();
}

