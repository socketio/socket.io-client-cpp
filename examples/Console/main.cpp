//
//  sio_test_sample.cpp
//
//  Created by Melo Yao on 3/24/15.
//

#include "../../src/sio_client.h"

#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#ifdef WIN32
#define HIGHLIGHT(__O__) std::cout<<__O__<<std::endl
#define EM(__O__) std::cout<<__O__<<std::endl

#include <stdio.h>
#include <tchar.h>
#define MAIN_FUNC int _tmain(int argc, _TCHAR* argv[])
#else
#define HIGHLIGHT(__O__) std::cout<<"\e[1;31m"<<__O__<<"\e[0m"<<std::endl
#define EM(__O__) std::cout<<"\e[1;30;1m"<<__O__<<"\e[0m"<<std::endl

#define MAIN_FUNC int main(int argc ,const char* args[])
#endif

using namespace sio;
using namespace std;
std::mutex _lock;

std::condition_variable_any _cond;
bool connect_finish = false;

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
        _lock.lock();
        _cond.notify_all();
        connect_finish = true;
        _lock.unlock();
    }
    void on_close(client::close_reason const& reason)
    {
        std::cout<<"sio closed "<<std::endl;
        exit(0);
    }
    
    void on_fail()
    {
        std::cout<<"sio failed "<<std::endl;
        exit(0);
    }
};

int participants = -1;

socket::ptr current_socket;

void bind_events()
{
	current_socket->on("new message", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck,message::list &ack_resp)
                       {
                           _lock.lock();
                           string user = data->get_map()["username"]->get_string();
                           string message = data->get_map()["message"]->get_string();
                           EM(user<<":"<<message);
                           _lock.unlock();
                       }));
    
    current_socket->on("user joined",sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck,message::list &ack_resp)
                       {
                           _lock.lock();
                           string user = data->get_map()["username"]->get_string();
                           participants  = data->get_map()["numUsers"]->get_int();
                           bool plural = participants !=1;
                           
                           //     abc "
                           HIGHLIGHT(user<<" joined"<<"\nthere"<<(plural?" are ":"'s ")<< participants<<(plural?" participants":" participant"));
                           _lock.unlock();
                       }));
    current_socket->on("user left", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck,message::list &ack_resp)
                       {
                           _lock.lock();
                           string user = data->get_map()["username"]->get_string();
                           participants  = data->get_map()["numUsers"]->get_int();
                           bool plural = participants !=1;
                           HIGHLIGHT(user<<" left"<<"\nthere"<<(plural?" are ":"'s ")<< participants<<(plural?" participants":" participant"));
                           _lock.unlock();
                       }));
}

MAIN_FUNC
{

    sio::client h;
    connection_listener l(h);
    
    h.set_open_listener(std::bind(&connection_listener::on_connected, &l));
    h.set_close_listener(std::bind(&connection_listener::on_close, &l,std::placeholders::_1));
    h.set_fail_listener(std::bind(&connection_listener::on_fail, &l));
    h.connect("http://127.0.0.1:3000");
    _lock.lock();
    if(!connect_finish)
    {
        _cond.wait(_lock);
    }
    _lock.unlock();
	current_socket = h.socket();
Login:
    string nickname;
    while (nickname.length() == 0) {
        HIGHLIGHT("Type your nickname:");
        
        getline(cin, nickname);
    }
	current_socket->on("login", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck,message::list &ack_resp){
        _lock.lock();
        participants = data->get_map()["numUsers"]->get_int();
        bool plural = participants !=1;
        HIGHLIGHT("Welcome to Socket.IO Chat-\nthere"<<(plural?" are ":"'s ")<< participants<<(plural?" participants":" participant"));
        _cond.notify_all();
        _lock.unlock();
        current_socket->off("login");
    }));
    current_socket->emit("add user", nickname);
    _lock.lock();
    if (participants<0) {
        _cond.wait(_lock);
    }
    _lock.unlock();
    bind_events();
    
    HIGHLIGHT("Start to chat,commands:\n'$exit' : exit chat\n'$nsp <namespace>' : change namespace");
    for (std::string line; std::getline(std::cin, line);) {
        if(line.length()>0)
        {
            if(line == "$exit")
            {
                break;
            }
            else if(line.length() > 5&&line.substr(0,5) == "$nsp ")
            {
                string new_nsp = line.substr(5);
                if(new_nsp == current_socket->get_namespace())
                {
                    continue;
                }
                current_socket->off_all();
                current_socket->off_error();
                //per socket.io, default nsp should never been closed.
                if(current_socket->get_namespace() != "/")
                {
                    current_socket->close();
                }
                current_socket = h.socket(new_nsp);
                bind_events();
                //if change to default nsp, we do not need to login again (since it is not closed).
                if(current_socket->get_namespace() == "/")
                {
                    continue;
                }
                goto Login;
            }
            current_socket->emit("new message", line);
            _lock.lock();
            EM("\t\t\t"<<line<<":"<<"You");
            _lock.unlock();
        }
    }
    HIGHLIGHT("Closing...");
    h.sync_close();
    h.clear_con_listeners();
	return 0;
}

