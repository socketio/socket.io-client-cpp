//
//  sio_client_impl.cpp
//  SioChatDemo
//
//  Created by Melo Yao on 4/3/15.
//  Copyright (c) 2015 Melo Yao. All rights reserved.
//

#include "sio_client_impl.h"
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <mutex>
// Comment this out to disable handshake logging to stdout
#if DEBUG || _DEBUG
#define LOG(x) std::cout << x
#else
#define LOG(x)
#endif

using boost::posix_time::milliseconds;

namespace sio
{
    client_impl::client_impl() :
    m_con_state(con_closed),
    m_ping_interval(0),
    m_ping_timeout(0),
    m_network_thread()
    {
        using websocketpp::log::alevel;
#ifndef DEBUG
        m_client.clear_access_channels(alevel::all);
        m_client.set_access_channels(alevel::connect|alevel::disconnect|alevel::app);
#endif
        // Initialize the Asio transport policy
        m_client.init_asio();
        
        // Bind the clients we are using
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;
        m_client.set_open_handler(lib::bind(&client_impl::on_open,this,_1));
        m_client.set_close_handler(lib::bind(&client_impl::on_close,this,_1));
        m_client.set_fail_handler(lib::bind(&client_impl::on_fail,this,_1));
        m_client.set_message_handler(lib::bind(&client_impl::on_message,this,_1,_2));
        
        m_packet_mgr.set_decode_callback(lib::bind(&client_impl::on_decode,this,_1));
        
        m_packet_mgr.set_encode_callback(lib::bind(&client_impl::on_encode,this,_1,_2));
    }
    
    client_impl::~client_impl()
    {
        this->sockets_invoke_void(&sio::socket::on_close);
        sync_close();
    }
    
    // Websocket++ client client
    void client_impl::on_fail(connection_hdl con)
    {
        m_con.reset();
        m_con_state = con_closed;
        this->sockets_invoke_void(&sio::socket::on_disconnect);
        LOG("Connection failed." << std::endl);
        if(m_fail_listener)m_fail_listener();
    }
    
    void client_impl::on_pong()
    {
        if(m_ping_timeout_timer)
        {
            m_ping_timeout_timer->cancel();
            m_ping_timeout_timer.reset();
        }
    }
    
    void client_impl::on_handshake(message::ptr const& message)
    {
        if(message && message->get_flag() == message::flag_object)
        {
            const object_message* obj_ptr =static_cast<object_message*>(message.get());
            const map<string,message::ptr>* values = &(obj_ptr->get_map());
            auto it = values->find("sid");
            if (it!= values->end()) {
                m_sid = std::static_pointer_cast<string_message>(it->second)->get_string();
            }
            else
            {
                goto failed;
            }
            it = values->find("pingInterval");
            if (it!= values->end()&&it->second->get_flag() == message::flag_integer) {
                m_ping_interval = (unsigned)std::static_pointer_cast<int_message>(it->second)->get_int();
            }
            else
            {
                m_ping_interval = 25000;
            }
            it = values->find("pingTimeout");
            
            if (it!=values->end()&&it->second->get_flag() == message::flag_integer) {
                m_ping_timeout = (unsigned) std::static_pointer_cast<int_message>(it->second)->get_int();
            }
            else
            {
                m_ping_timeout = 60000;
            }
            
            m_ping_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
            boost::system::error_code ec;
            m_ping_timer->expires_from_now(milliseconds(m_ping_interval), ec);
            if(ec)LOG("ec:"<<ec.message()<<std::endl);
            m_ping_timer->async_wait(lib::bind(&client_impl::__ping,this,lib::placeholders::_1));
            LOG("On handshake,sid:"<<m_sid<<",ping interval:"<<m_ping_interval<<",ping timeout"<<"m_ping_timeout"<<m_ping_timeout<<std::endl);
            return;
        }
    failed:
        //just close it.
        m_client.get_io_service().dispatch(lib::bind(&client_impl::__close, this,close::status::policy_violation,"Handshake error"));
    }
    
    void client_impl::on_open(connection_hdl con)
    {
        LOG("Connected." << std::endl);
        m_con_state = con_opened;
        m_con = con;
        this->sockets_invoke_void(&sio::socket::on_open);
        if(m_open_listener)m_open_listener();
    }
    
    void client_impl::on_close(connection_hdl con)
    {
        LOG("Client Disconnected." << std::endl);
        m_con_state = con_closed;
        lib::error_code ec;
        close::status::value code = close::status::normal;
        client_type::connection_ptr conn_ptr  = m_client.get_con_from_hdl(con, ec);
        if (ec) {
            LOG("OnClose get conn failed"<<ec<<std::endl);
        }
        else
        {
            code = conn_ptr->get_local_close_code();
        }
        
        m_con.reset();
        this->clear_timers();
        client::close_reason reason;
        if(code == close::status::normal)
        {
            this->sockets_invoke_void(&sio::socket::on_disconnect);
            reason = client::close_reason_normal;
        }
        else
        {
            this->sockets_invoke_void(&sio::socket::on_disconnect);
            reason = client::close_reason_drop;
        }
        
        if(m_close_listener)
        {
            
            m_close_listener(reason);
        }
    }
    
    void client_impl::on_message(connection_hdl con, client_type::message_ptr msg)
    {
        if (m_ping_timeout_timer) {
            boost::system::error_code ec;
            m_ping_timeout_timer->expires_from_now(milliseconds(m_ping_timeout),ec);
            m_ping_timeout_timer->async_wait(lib::bind(&client_impl::__timeout_pong, this,lib::placeholders::_1));
        }
        // Parse the incoming message according to socket.IO rules
        m_packet_mgr.put_payload(msg->get_payload());
    }
    
    void client_impl::on_pong_timeout()
    {
        LOG("Pong timeout"<<std::endl);
        m_client.get_io_service().dispatch(lib::bind(&client_impl::__close, this,close::status::policy_violation,"Pong timeout"));
    }
    
    void client_impl::on_decode(packet const& p)
    {
        switch(p.get_frame())
        {
            case packet::frame_message:
            {
                socket::ptr so_ptr = get_socket_locked(p.get_nsp());
                if(so_ptr)so_ptr->on_message_packet(p);
                break;
            }
            case packet::frame_open:
                this->on_handshake(p.get_message());
                break;
            case packet::frame_close:
                //FIXME how to deal?
                this->__close(close::status::abnormal_close, "End by server");
                break;
            case packet::frame_pong:
                this->on_pong();
                break;
                
            default:
                break;
        }
    }
    
    void client_impl::on_encode(bool isBinary,shared_ptr<const string> const& payload)
    {
        LOG("encoded payload length:"<<payload->length()<<std::endl);
        m_client.get_io_service().dispatch(lib::bind(&client_impl::__send,this,payload,isBinary?frame::opcode::binary:frame::opcode::text));
    }
    
    void client_impl::__ping(const boost::system::error_code& ec)
    {
        if(ec || m_con.expired())
        {
            LOG("ping exit,con is expired?"<<m_con.expired()<<",ec:"<<ec.message()<<std::endl);
            return;
        }
        packet p(packet::frame_ping);
        m_packet_mgr.encode(p,
                            [&](bool isBin,shared_ptr<const string> payload)
                            {
                                lib::error_code ec;
                                this->m_client.send(this->m_con, *payload, frame::opcode::text, ec);
                            });
        if(m_ping_timer)
        {
            boost::system::error_code e_code;
            m_ping_timer->expires_from_now(milliseconds(m_ping_interval), e_code);
            m_ping_timer->async_wait(lib::bind(&client_impl::__ping,this,lib::placeholders::_1));
        }
        if(!m_ping_timeout_timer)
        {
            m_ping_timeout_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
            boost::system::error_code timeout_ec;
            m_ping_timeout_timer->expires_from_now(milliseconds(m_ping_timeout), timeout_ec);
            m_ping_timeout_timer->async_wait(lib::bind(&client_impl::__timeout_pong, this,lib::placeholders::_1));
        }
    }
    
    void client_impl::__timeout_pong(const boost::system::error_code &ec)
    {
        if(ec)
        {
            return;
        }
        this->on_pong_timeout();
    }
    
    
    void client_impl::__close(close::status::value const& code,std::string const& reason)
    {
        LOG("Close by reason:"<<reason << std::endl);
        if (m_con.expired())
        {
            std::cerr << "Error: No active session" << std::endl;
        }
        else
        {
            lib::error_code ec;
            m_client.close(m_con, code, reason, ec);
        }
    }
    
    void client_impl::close()
    {
        m_con_state = con_closing;
        this->sockets_invoke_void(&sio::socket::close);
        m_client.get_io_service().dispatch(lib::bind(&client_impl::__close, this,close::status::normal,"End by user"));
    }
    
    void client_impl::sync_close()
    {
        m_con_state = con_closing;
        this->sockets_invoke_void(&sio::socket::close);
        m_client.get_io_service().dispatch(lib::bind(&client_impl::__close, this,close::status::normal,"End by user"));
        if(m_network_thread)
        {
            m_network_thread->join();
            m_network_thread.reset();
        }
    }
    
    void client_impl::send(packet& p)
    {
        m_packet_mgr.encode(p);
    }
    
    void client_impl::remove_socket(std::string const& nsp)
    {
        std::lock_guard<std::mutex> guard(m_socket_mutex);
        auto it = m_sockets.find(nsp);
        if(it!= m_sockets.end())
        {
            m_sockets.erase(it);
        }
    }
    
    boost::asio::io_service& client_impl::get_io_service()
    {
        return m_client.get_io_service();
    }
    
    void client_impl::on_socket_closed(std::string const& nsp)
    {
        if(m_socket_close_listener)m_socket_close_listener(nsp);
    }
    
    void client_impl::on_socket_opened(std::string const& nsp)
    {
        if(m_socket_open_listener)m_socket_open_listener(nsp);
    }
    
    void client_impl::__send(std::shared_ptr<const std::string> const& payload_ptr,frame::opcode::value opcode)
    {
        if(m_con_state == con_opened)
        {
            //delay the ping, since we already have message to send.
            boost::system::error_code timeout_ec;
            if(m_ping_timer)
            {
                m_ping_timer->expires_from_now(milliseconds(m_ping_interval),timeout_ec);
                m_ping_timer->async_wait(lib::bind(&client_impl::__ping,this,lib::placeholders::_1));
            }
            lib::error_code ec;
            m_client.send(m_con,*payload_ptr,opcode,ec);
            if(ec)
            {
                std::cerr<<"Send failed,reason:"<< ec.message()<<std::endl;
            }
        }
    }
    
    void client_impl::__connect(const std::string& uri)
    {
        do{
            websocketpp::uri uo(uri);
            std::ostringstream ss;
            if (m_sid.size()==0) {
                ss<<"ws://"<<uo.get_host()<<":"<<uo.get_port()<<"/socket.io/?EIO=4&transport=websocket&t="<<time(NULL);
            }
            else
            {
                ss<<"ws://"<<uo.get_host()<<":"<<uo.get_port()<<"/socket.io/?EIO=4&transport=websocket&sid="<<m_sid<<"&t="<<time(NULL);
            }
            lib::error_code ec;
            client_type::connection_ptr con = m_client.get_connection(ss.str(), ec);
            if (ec) {
                m_client.get_alog().write(websocketpp::log::alevel::app,
                                          "Get Connection Error: "+ec.message());
                break;
            }
            
            m_client.connect(con);
            return;
        }
        while(0);
        if(m_fail_listener)
        {
            m_fail_listener();
        }
    }
    
    void client_impl::clear_timers()
    {
        LOG("clear timers"<<std::endl);
        boost::system::error_code ec;
        if(m_ping_timeout_timer)
        {
            m_ping_timeout_timer->cancel(ec);
            m_ping_timeout_timer.reset();
        }
        if(m_ping_timer)
        {
            m_ping_timer->cancel(ec);
            m_ping_timer.reset();
        }
    }
    
    void client_impl::reset_states()
    {
        m_client.reset();
        m_sid.clear();
        m_packet_mgr.reset();
    }
    
    void client_impl::connect(const std::string& uri)
    {
        if(m_network_thread)
        {
            if(m_con_state == con_closing||m_con_state == con_closed)
            {
                //if client is closing, join to wait.
                //if client is closed, still need to join,
                //but in closed case,join will return immediately.
                m_network_thread->join();
            }
            else
            {
                //if we are connected, do nothing.
                return;
            }
        }
        m_con_state = con_opening;
        this->reset_states();
        m_client.get_io_service().dispatch(lib::bind(&client_impl::__connect,this,uri));
        m_network_thread.reset(new std::thread(lib::bind(&client_impl::run_loop,this)));//uri lifecycle?
        
    }
    
    void client_impl::reconnect(const std::string& uri)
    {
        if(m_network_thread)
        {
            if(m_con_state == con_closing)
            {
                m_network_thread->join();
            }
            else
            {
                return;
            }
        }
        if(m_con_state == con_closed)
        {
            m_con_state = con_opening;
            
            m_client.get_io_service().dispatch(lib::bind(&client_impl::__connect,this,uri));
            m_network_thread.reset(new std::thread(lib::bind(&client_impl::run_loop,this)));//uri
        }
    }
    
    socket::ptr const& client_impl::socket(std::string const& nsp)
    {
        std::lock_guard<std::mutex> guard(m_socket_mutex);
        std::string aux;
        if(nsp == "")
        {
            aux = "/";
        }
        else if( nsp[0] != '/')
        {
            aux.append("/",1);
            aux.append(nsp);
        }
        else
        {
            aux = nsp;
        }
        
        auto it = m_sockets.find(aux);
        if(it!= m_sockets.end())
        {
            return it->second;
        }
        else
        {
            std::pair<const std::string, socket::ptr> p(aux,std::shared_ptr<sio::socket>(new sio::socket(this,aux)));
            return (m_sockets.insert(p).first)->second;
        }
    }
    
    void client_impl::run_loop()
    {
        
        m_client.run();
        m_client.reset();
        m_client.get_alog().write(websocketpp::log::alevel::devel,
                                  "run loop end");
    }
    
    socket::ptr client_impl::get_socket_locked(std::string const& nsp)
    {
        std::lock_guard<std::mutex> guard(m_socket_mutex);
        auto it = m_sockets.find(nsp);
        if(it != m_sockets.end())
        {
            return it->second;
        }
        else
        {
            return socket::ptr();
        }
    }
    
    void client_impl::sockets_invoke_void(void (sio::socket::*fn)(void))
    {
        std::map<const std::string,socket::ptr> socks;
        {
            std::lock_guard<std::mutex> guard(m_socket_mutex);
            socks.insert(m_sockets.begin(),m_sockets.end());
        }
        for (auto it = socks.begin(); it!=socks.end(); ++it) {
            ((*(it->second)).*fn)();
        }
    }
}
