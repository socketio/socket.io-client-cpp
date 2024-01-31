//
//  sio_client_impl.cpp
//  SioChatDemo
//
//  Created by Melo Yao on 4/3/15.
//  Copyright (c) 2015 Melo Yao. All rights reserved.
//

#include "sio_client_impl.h"

#include <functional>
#include <sstream>
#include <mutex>
#include <cmath>

namespace sio
{
    /*************************public:*************************/
    client_impl_base::client_impl_base() :
        m_ping_interval(0),
        m_ping_timeout(0),
        m_network_thread(),
        m_con_state(con_closed),
        m_reconn_delay(5000),
        m_reconn_delay_max(25000),
        m_reconn_attempts(0xFFFFFFFF),
        m_reconn_made(0)
    {
        // Bind the clients we are using
        using std::placeholders::_1;
        using std::placeholders::_2;

        m_packet_mgr.set_decode_callback(std::bind(&client_impl_base::on_decode,this,_1));

        m_packet_mgr.set_encode_callback(std::bind(&client_impl_base::on_encode,this,_1,_2));
    }

    client_impl_base::client_impl_base(const client_impl_base &other)
        : client_impl_base()
    {
        m_open_listener = other.m_open_listener;
        m_fail_listener = other.m_fail_listener;
        m_reconnecting_listener = other.m_reconnecting_listener;
        m_reconnect_listener = other.m_reconnect_listener;
        m_close_listener = other.m_close_listener;
        m_socket_open_listener = other.m_socket_open_listener;
        m_socket_close_listener = other.m_socket_close_listener;
        m_reconn_delay = other.m_reconn_delay;
        m_reconn_delay_max = other.m_reconn_delay_max;
        m_reconn_attempts = other.m_reconn_attempts;
    }

    void client_impl_base::clear_con_listeners()
    {
        m_open_listener = nullptr;
        m_close_listener = nullptr;
        m_fail_listener = nullptr;
        m_reconnect_listener = nullptr;
        m_reconnecting_listener = nullptr;
    }

    void client_impl_base::clear_socket_listeners()
    {
        m_socket_open_listener = nullptr;
        m_socket_close_listener = nullptr;
    }

    void client_impl_base::connect(const string& uri, const map<string,string>& query, const map<string, string>& headers)
    {
        if(m_reconn_timer)
        {
            m_reconn_timer->cancel();
            m_reconn_timer.reset();
        }
        if(m_network_thread)
        {
            if(m_con_state == con_closing||m_con_state == con_closed)
            {
                //if client is closing, join to wait.
                //if client is closed, still need to join,
                //but in closed case,join will return immediately.
                m_network_thread->join();
                m_network_thread.reset();//defensive
            }
            else
            {
                //if we are connected, do nothing.
                return;
            }
        }
        m_con_state = con_opening;
        m_base_url = uri;
        m_reconn_made = 0;

        string query_str;
        for(map<string,string>::const_iterator it=query.begin();it!=query.end();++it){
            query_str.append("&");
            query_str.append(it->first);
            query_str.append("=");
            string query_str_value=encode_query_string(it->second);
            query_str.append(query_str_value);
        }
        m_query_string = std::move(query_str);

        m_http_headers = headers;

        reset_states();
        get_io_service().dispatch(std::bind(&client_impl_base::connect_impl,this,uri,m_query_string));
        m_network_thread.reset(new thread(std::bind(&client_impl_base::run_loop,this)));//uri lifecycle?

    }

    socket::ptr const& client_impl_base::socket(string const& nsp)
    {
        lock_guard<mutex> guard(m_socket_mutex);
        string aux;
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
            pair<const string, socket::ptr> p(aux,shared_ptr<sio::socket>(new sio::socket(this,aux)));
            return (m_sockets.insert(p).first)->second;
        }
    }

    void client_impl_base::close()
    {
        m_con_state = con_closing;
        sockets_invoke_void(&sio::socket::close);
        get_io_service().dispatch(std::bind(&client_impl_base::close_impl, this,close::status::normal,"End by user"));
    }

    void client_impl_base::sync_close()
    {
        sync_close_internal(get_io_service());
    }

    void client_impl_base::set_reconnect_delay(unsigned millis)
    {
        m_reconn_delay = millis;
        if (m_reconn_delay_max < millis)
            m_reconn_delay_max = millis;
    }

    void client_impl_base::set_reconnect_delay_max(unsigned millis)
    {
        m_reconn_delay_max = millis;
        if (m_reconn_delay > millis)
            m_reconn_delay = millis;
    }

    void client_impl_base::sync_close_internal(lib::asio::io_service& service)
    {
        m_con_state = con_closing;
        sockets_invoke_void(&sio::socket::close);
        service.dispatch(std::bind(&client_impl_base::close_impl, this,close::status::normal,"End by user"));
        if(m_network_thread)
        {
            m_network_thread->join();
            m_network_thread.reset();
        }
    }

    /*************************protected:*************************/
    void client_impl_base::send(packet& p)
    {
        m_packet_mgr.encode(p);
    }

    void client_impl_base::remove_socket(string const& nsp)
    {
        lock_guard<mutex> guard(m_socket_mutex);
        auto it = m_sockets.find(nsp);
        if(it!= m_sockets.end())
        {
            m_sockets.erase(it);
        }
    }

    void client_impl_base::on_socket_closed(string const& nsp)
    {
        if(m_socket_close_listener)m_socket_close_listener(nsp);
    }

    void client_impl_base::on_socket_opened(string const& nsp)
    {
        if(m_socket_open_listener)m_socket_open_listener(nsp);
    }

    void client_impl_base::timeout_pong(const asio::error_code &ec)
    {
        if(ec)
        {
            return;
        }
        LOG("Pong timeout"<<endl);
        get_io_service().dispatch(std::bind(&client_impl_base::close_impl, this,close::status::policy_violation,"Pong timeout"));
    }

    void client_impl_base::timeout_reconnect(asio::error_code const& ec)
    {
        if(ec)
        {
            return;
        }
        if(m_con_state == con_closed)
        {
            m_con_state = con_opening;
            m_reconn_made++;
            reset_states();
            LOG("Reconnecting..."<<endl);
            if(m_reconnecting_listener) m_reconnecting_listener();
            get_io_service().dispatch(std::bind(&client_impl_base::connect_impl,this,m_base_url,m_query_string));
        }
    }

    unsigned client_impl_base::next_delay() const
    {
        //no jitter, fixed power root.
        unsigned reconn_made = min<unsigned>(m_reconn_made,32);//protect the pow result to be too big.
        return static_cast<unsigned>(min<double>(m_reconn_delay * pow(1.5,reconn_made),m_reconn_delay_max));
    }

    socket::ptr client_impl_base::get_socket_locked(string const& nsp)
    {
        lock_guard<mutex> guard(m_socket_mutex);
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

    void client_impl_base::sockets_invoke_void(void (socket::*fn)(void))
    {
        map<const string,socket::ptr> socks;
        {
            lock_guard<mutex> guard(m_socket_mutex);
            socks.insert(m_sockets.begin(),m_sockets.end());
        }
        for (auto it = socks.begin(); it!=socks.end(); ++it) {
            ((*(it->second)).*fn)();
        }
    }

    void client_impl_base::on_fail(connection_hdl)
    {
        m_con.reset();
        m_con_state = con_closed;
        socket_invoke_disconnected();
        LOG("Connection failed." << endl);
        if(m_reconn_made<m_reconn_attempts)
        {
            LOG("Reconnect for attempt:"<<m_reconn_made<<endl);
            unsigned delay = next_delay();
            if(m_reconnect_listener) m_reconnect_listener(m_reconn_made,delay);
            m_reconn_timer.reset(new asio::steady_timer(get_io_service()));
            asio::error_code ec;
            m_reconn_timer->expires_from_now(milliseconds(delay), ec);
            m_reconn_timer->async_wait(std::bind(&client_impl_base::timeout_reconnect,this, std::placeholders::_1));
        }
        else
        {
            if(m_fail_listener)m_fail_listener();
        }
    }

    void client_impl_base::on_open(connection_hdl con)
    {
        LOG("Connected." << endl);
        m_con_state = con_opened;
        m_con = con;
        m_reconn_made = 0;
        sockets_invoke_void(&sio::socket::on_open);
        socket("");
        if(m_open_listener)m_open_listener();
    }

    void client_impl_base::on_handshake(message::ptr const& message)
    {
        if(message && message->get_flag() == message::flag_object)
        {
            const object_message* obj_ptr =static_cast<object_message*>(message.get());
            const map<string,message::ptr>* values = &(obj_ptr->get_map());
            auto it = values->find("sid");
            if (it!= values->end()) {
                m_sid = static_pointer_cast<string_message>(it->second)->get_string();
            }
            else
            {
                goto failed;
            }
            it = values->find("pingInterval");
            if (it!= values->end()&&it->second->get_flag() == message::flag_integer) {
                m_ping_interval = (unsigned)static_pointer_cast<int_message>(it->second)->get_int();
            }
            else
            {
                m_ping_interval = 25000;
            }
            it = values->find("pingTimeout");

            if (it!=values->end()&&it->second->get_flag() == message::flag_integer) {
                m_ping_timeout = (unsigned) static_pointer_cast<int_message>(it->second)->get_int();
            }
            else
            {
                m_ping_timeout = 60000;
            }

            m_ping_timer.reset(new asio::steady_timer(get_io_service()));
            asio::error_code ec;
            m_ping_timer->expires_from_now(milliseconds(m_ping_interval), ec);
            if(ec)LOG("ec:"<<ec.message()<<endl);
            m_ping_timer->async_wait(std::bind(&client_impl_base::ping,this, std::placeholders::_1));
            LOG("On handshake,sid:"<<m_sid<<",ping interval:"<<m_ping_interval<<",ping timeout"<<m_ping_timeout<<endl);
            return;
        }
failed:
        //just close it.
        get_io_service().dispatch(std::bind(&client_impl_base::close_impl, this,close::status::policy_violation,"Handshake error"));
    }

    void client_impl_base::on_pong()
    {
        if(m_ping_timeout_timer)
        {
            m_ping_timeout_timer->cancel();
            m_ping_timeout_timer.reset();
        }
    }

    void client_impl_base::on_decode(packet const& p)
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
            on_handshake(p.get_message());
            break;
        case packet::frame_close:
            //FIXME how to deal?
            close_impl(close::status::abnormal_close, "End by server");
            break;
        case packet::frame_pong:
            on_pong();
            break;

        default:
            break;
        }
    }

    void client_impl_base::on_encode(
            bool isBinary,shared_ptr<const string> const& payload)
    {
        LOG("encoded payload length:"<<payload->length()<<endl);
        get_io_service().dispatch(
                    std::bind(&client_impl_base::send_impl,this,payload,
                              isBinary ? frame::opcode::binary
                                       : frame::opcode::text));
    }

    void client_impl_base::clear_timers()
    {
        LOG("clear timers"<<endl);
        asio::error_code ec;
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

    std::string client_impl_base::encode_query_string(const std::string &query)
    {
        ostringstream ss;
        ss << std::hex;
        // Percent-encode (RFC3986) non-alphanumeric characters.
        for(const char c : query){
            if((c >= 'a' && c <= 'z') || (c>= 'A' && c<= 'Z')
               || (c >= '0' && c<= '9')){
                ss << c;
            } else {
                ss << '%' << std::uppercase << std::setw(2)
                   << int((unsigned char) c) << std::nouppercase;
            }
        }
        ss << std::dec;
        return ss.str();
    }

    void client_impl_base::socket_invoke_disconnected()
    {
        sockets_invoke_void(&sio::socket::on_disconnect);
    }

    void client_impl_base::socket_invoke_closed()
    {
        sockets_invoke_void(&sio::socket::on_close);
    }
}
