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
#include <boost/algorithm/string/predicate.hpp>
#include <mutex>
#include <cmath>
// Comment this out to disable handshake logging to stdout
#if DEBUG || _DEBUG
#define LOG(x) std::cout << x
#else
#define LOG(x)
#endif

using boost::posix_time::milliseconds;
using namespace std;

namespace sio
{
    /*************************public:*************************/
    template<typename client_type>
    client_impl<client_type>::client_impl(const string& uri) :
        m_base_url(uri),
        m_con_state(con_closed),
        m_ping_interval(0),
        m_ping_timeout(0),
        m_network_thread(),
        m_reconn_attempts(0xFFFFFFFF),
        m_reconn_made(0),
        m_reconn_delay(5000),
        m_reconn_delay_max(25000)
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
        m_client.set_open_handler(lib::bind(&client_impl<client_type>::on_open,this,_1));
        m_client.set_close_handler(lib::bind(&client_impl<client_type>::on_close,this,_1));
        m_client.set_fail_handler(lib::bind(&client_impl<client_type>::on_fail,this,_1));
        m_client.set_message_handler(lib::bind(&client_impl<client_type>::on_message,this,_1,_2));
        m_packet_mgr.set_decode_callback(lib::bind(&client_impl<client_type>::on_decode,this,_1));
        m_packet_mgr.set_encode_callback(lib::bind(&client_impl<client_type>::on_encode,this,_1,_2));
        template_init();
    }

    template<typename client_type>
    client_impl<client_type>::~client_impl()
    {
        this->sockets_invoke_void(socket_on_close());
        sync_close();
    }

    template<typename client_type>
    void client_impl<client_type>::connect(const string& uri, const map<string,string>& query, const map<string, string>& headers)
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
        m_reconn_made = 0;
        if(!uri.empty())
        {
            m_base_url = uri;
        }

        string query_str;
        for(map<string,string>::const_iterator it=query.begin();it!=query.end();++it){
            query_str.append("&");
            query_str.append(it->first);
            query_str.append("=");
            query_str.append(it->second);
        }
        m_query_string=move(query_str);

        m_http_headers = headers;

        this->reset_states();
        m_client.get_io_service().dispatch(lib::bind(&client_impl<client_type>::connect_impl,this,m_base_url,m_query_string));
        m_network_thread.reset(new thread(lib::bind(&client_impl<client_type>::run_loop,this)));//uri lifecycle?

    }

    template<typename client_type>
    socket::ptr const& client_impl<client_type>::socket(string const& nsp)
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
            pair<const string, socket::ptr> p(aux,shared_ptr<sio::socket>(new_socket(aux)));
            return (m_sockets.insert(p).first)->second;
        }
    }

    template<typename client_type>
    void client_impl<client_type>::close()
    {
        m_con_state = con_closing;
        this->sockets_invoke_void(&sio::socket::close);
        m_client.get_io_service().dispatch(lib::bind(&client_impl<client_type>::close_impl, this,close::status::normal,"End by user"));
    }

    template<typename client_type>
    void client_impl<client_type>::sync_close()
    {
        m_con_state = con_closing;
        this->sockets_invoke_void(&sio::socket::close);
        m_client.get_io_service().dispatch(lib::bind(&client_impl<client_type>::close_impl, this,close::status::normal,"End by user"));
        if(m_network_thread)
        {
            m_network_thread->join();
            m_network_thread.reset();
        }
    }

    /*************************protected:*************************/
    template<typename client_type>
    void client_impl<client_type>::send(packet& p)
    {
        m_packet_mgr.encode(p);
    }

    template<typename client_type>
    void client_impl<client_type>::remove_socket(string const& nsp)
    {
        lock_guard<mutex> guard(m_socket_mutex);
        auto it = m_sockets.find(nsp);
        if(it!= m_sockets.end())
        {
            m_sockets.erase(it);
        }
    }

    template<typename client_type>
    boost::asio::io_service& client_impl<client_type>::get_io_service()
    {
        return m_client.get_io_service();
    }

    template<typename client_type>
    void client_impl<client_type>::on_socket_closed(string const& nsp)
    {
        if(m_socket_close_listener)m_socket_close_listener(nsp);
    }

    template<typename client_type>
    void client_impl<client_type>::on_socket_opened(string const& nsp)
    {
        if(m_socket_open_listener)m_socket_open_listener(nsp);
    }

    /*************************private:*************************/
    template<typename client_type>
    void client_impl<client_type>::run_loop()
    {

        m_client.run();
        m_client.reset();
        m_client.get_alog().write(websocketpp::log::alevel::devel,
                                  "run loop end");
    }

    template<typename client_type>
    void client_impl<client_type>::connect_impl(const string& uri, const string& queryString)
    {
        do{
            websocketpp::uri uo(uri);
            ostringstream ss;

            if(is_tls(uri))
            {
                // This requires SIO_TLS to have been compiled in.
                ss<<"wss://";
            }
            else
            {
                ss<<"ws://";
            }

            const std::string host(uo.get_host());
            // As per RFC2732, literal IPv6 address should be enclosed in "[" and "]".
            if(host.find(':')!=std::string::npos){
                ss<<"["<<uo.get_host()<<"]";
            } else {
                ss<<uo.get_host();
            }
            ss<<":"<<uo.get_port()<<"/socket.io/?EIO=4&transport=websocket";
            if(m_sid.size()>0){
                ss<<"&sid="<<m_sid;
            }
            ss<<"&t="<<time(NULL)<<queryString;
            lib::error_code ec;
            typename client_type::connection_ptr con = m_client.get_connection(ss.str(), ec);
            if (ec) {
                m_client.get_alog().write(websocketpp::log::alevel::app,
                                          "Get Connection Error: "+ec.message());
                break;
            }

            for( auto&& header: m_http_headers ) {
                con->replace_header(header.first, header.second);
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

    template<typename client_type>
    void client_impl<client_type>::close_impl(close::status::value const& code,string const& reason)
    {
        LOG("Close by reason:"<<reason << endl);
        if(m_reconn_timer)
        {
            m_reconn_timer->cancel();
            m_reconn_timer.reset();
        }
        if (m_con.expired())
        {
            cerr << "Error: No active session" << endl;
        }
        else
        {
            lib::error_code ec;
            m_client.close(m_con, code, reason, ec);
        }
    }

    template<typename client_type>
    void client_impl<client_type>::send_impl(shared_ptr<const string> const& payload_ptr,frame::opcode::value opcode)
    {
        if(m_con_state == con_opened)
        {
            //delay the ping, since we already have message to send.
            boost::system::error_code timeout_ec;
            if(m_ping_timer)
            {
                m_ping_timer->expires_from_now(milliseconds(m_ping_interval),timeout_ec);
                m_ping_timer->async_wait(lib::bind(&client_impl<client_type>::ping,this,lib::placeholders::_1));
            }
            lib::error_code ec;
            m_client.send(m_con,*payload_ptr,opcode,ec);
            if(ec)
            {
                cerr<<"Send failed,reason:"<< ec.message()<<endl;
            }
        }
    }

    template<typename client_type>
    void client_impl<client_type>::ping(const boost::system::error_code& ec)
    {
        if(ec || m_con.expired())
        {
            if (ec != boost::asio::error::operation_aborted)
                LOG("ping exit,con is expired?"<<m_con.expired()<<",ec:"<<ec.message()<<endl);
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
            m_ping_timer->async_wait(lib::bind(&client_impl<client_type>::ping,this,lib::placeholders::_1));
        }
        if(!m_ping_timeout_timer)
        {
            m_ping_timeout_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
            boost::system::error_code timeout_ec;
            m_ping_timeout_timer->expires_from_now(milliseconds(m_ping_timeout), timeout_ec);
            m_ping_timeout_timer->async_wait(lib::bind(&client_impl<client_type>::timeout_pong, this,lib::placeholders::_1));
        }
    }

    template<typename client_type>
    void client_impl<client_type>::timeout_pong(const boost::system::error_code &ec)
    {
        if(ec)
        {
            return;
        }
        LOG("Pong timeout"<<endl);
        m_client.get_io_service().dispatch(lib::bind(&client_impl<client_type>::close_impl, this,close::status::policy_violation,"Pong timeout"));
    }

    template<typename client_type>
    void client_impl<client_type>::timeout_reconnect(boost::system::error_code const& ec)
    {
        if(ec)
        {
            return;
        }
        if(m_con_state == con_closed)
        {
            m_con_state = con_opening;
            m_reconn_made++;
            this->reset_states();
            LOG("Reconnecting..."<<endl);
            if(m_reconnecting_listener) m_reconnecting_listener();
            m_client.get_io_service().dispatch(lib::bind(&client_impl<client_type>::connect_impl,this,m_base_url,m_query_string));
        }
    }

    template<typename client_type>
    unsigned client_impl<client_type>::next_delay() const
    {
        //no jitter, fixed power root.
        unsigned reconn_made = min<unsigned>(m_reconn_made,32);//protect the pow result to be too big.
        return static_cast<unsigned>(min<double>(m_reconn_delay * pow(1.5,reconn_made),m_reconn_delay_max));
    }

    template<typename client_type>
    socket::ptr client_impl<client_type>::get_socket_locked(string const& nsp)
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

    template<typename client_type>
    void client_impl<client_type>::sockets_invoke_void(void (sio::socket::*fn)(void))
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

    template<typename client_type>
    void client_impl<client_type>::on_fail(connection_hdl con)
    {
        m_con.reset();
        m_con_state = con_closed;
        this->sockets_invoke_void(socket_on_disconnect());
        LOG("Connection failed." << endl);
        if(m_reconn_made<m_reconn_attempts)
        {
            LOG("Reconnect for attempt:"<<m_reconn_made<<endl);
            unsigned delay = this->next_delay();
            if(m_reconnect_listener) m_reconnect_listener(m_reconn_made,delay);
            m_reconn_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
            boost::system::error_code ec;
            m_reconn_timer->expires_from_now(milliseconds(delay), ec);
            m_reconn_timer->async_wait(lib::bind(&client_impl<client_type>::timeout_reconnect,this,lib::placeholders::_1));
        }
        else
        {
            if(m_fail_listener)m_fail_listener();
        }
    }

    template<typename client_type>
    void client_impl<client_type>::on_open(connection_hdl con)
    {
        LOG("Connected." << endl);
        m_con_state = con_opened;
        m_con = con;
        m_reconn_made = 0;
        this->sockets_invoke_void(socket_on_open());
        this->socket("");
        if(m_open_listener)m_open_listener();
    }

    template<typename client_type>
    void client_impl<client_type>::on_close(connection_hdl con)
    {
        LOG("Client Disconnected." << endl);
        m_con_state = con_closed;
        lib::error_code ec;
        close::status::value code = close::status::normal;
        typename client_type::connection_ptr conn_ptr  = m_client.get_con_from_hdl(con, ec);
        if (ec) {
            LOG("OnClose get conn failed"<<ec<<endl);
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
            this->sockets_invoke_void(socket_on_disconnect());
            reason = client::close_reason_normal;
        }
        else
        {
            this->sockets_invoke_void(socket_on_disconnect());
            if(m_reconn_made<m_reconn_attempts)
            {
                LOG("Reconnect for attempt:"<<m_reconn_made<<endl);
                unsigned delay = this->next_delay();
                if(m_reconnect_listener) m_reconnect_listener(m_reconn_made,delay);
                m_reconn_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
                boost::system::error_code ec;
                m_reconn_timer->expires_from_now(milliseconds(delay), ec);
                m_reconn_timer->async_wait(lib::bind(&client_impl<client_type>::timeout_reconnect,this,lib::placeholders::_1));
                return;
            }
            reason = client::close_reason_drop;
        }
        
        if(m_close_listener)
        {
            m_close_listener(reason);
        }
    }

    template<typename client_type>
    void client_impl<client_type>::on_message(connection_hdl con, message_ptr msg)
    {
        if (m_ping_timeout_timer) {
            boost::system::error_code ec;
            m_ping_timeout_timer->expires_from_now(milliseconds(m_ping_timeout),ec);
            m_ping_timeout_timer->async_wait(lib::bind(&client_impl<client_type>::timeout_pong, this,lib::placeholders::_1));
        }
        // Parse the incoming message according to socket.IO rules
        m_packet_mgr.put_payload(msg->get_payload());
    }

    template<typename client_type>
    void client_impl<client_type>::on_handshake(message::ptr const& message)
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

            m_ping_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
            boost::system::error_code ec;
            m_ping_timer->expires_from_now(milliseconds(m_ping_interval), ec);
            if(ec)LOG("ec:"<<ec.message()<<endl);
            m_ping_timer->async_wait(lib::bind(&client_impl<client_type>::ping,this,lib::placeholders::_1));
            LOG("On handshake,sid:"<<m_sid<<",ping interval:"<<m_ping_interval<<",ping timeout"<<m_ping_timeout<<endl);
            return;
        }
failed:
        //just close it.
        m_client.get_io_service().dispatch(lib::bind(&client_impl<client_type>::close_impl, this,close::status::policy_violation,"Handshake error"));
    }

    template<typename client_type>
    void client_impl<client_type>::on_pong()
    {
        if(m_ping_timeout_timer)
        {
            m_ping_timeout_timer->cancel();
            m_ping_timeout_timer.reset();
        }
    }

    template<typename client_type>
    void client_impl<client_type>::on_decode(packet const& p)
    {
        switch(p.get_frame())
        {
        case packet::frame_message:
        {
            socket::ptr so_ptr = get_socket_locked(p.get_nsp());
            if(so_ptr)socket_on_message_packet(so_ptr, p);
            break;
        }
        case packet::frame_open:
            this->on_handshake(p.get_message());
            break;
        case packet::frame_close:
            //FIXME how to deal?
            this->close_impl(close::status::abnormal_close, "End by server");
            break;
        case packet::frame_pong:
            this->on_pong();
            break;

        default:
            break;
        }
    }

    template<typename client_type>
    void client_impl<client_type>::on_encode(bool isBinary,shared_ptr<const string> const& payload)
    {
        LOG("encoded payload length:"<<payload->length()<<endl);
        m_client.get_io_service().dispatch(lib::bind(&client_impl<client_type>::send_impl,this,payload,isBinary?frame::opcode::binary:frame::opcode::text));
    }

    template<typename client_type>
    void client_impl<client_type>::clear_timers()
    {
        LOG("clear timers"<<endl);
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

    template<typename client_type>
    void client_impl<client_type>::reset_states()
    {
        m_client.reset();
        m_sid.clear();
        m_packet_mgr.reset();
    }
    
    template<>
    void client_impl<client_type_no_tls>::template_init()
    {
    }

#if SIO_TLS
    typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
    static context_ptr on_tls_init(connection_hdl conn)
    {
        context_ptr ctx = context_ptr(new  boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));
        boost::system::error_code ec;
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::single_dh_use,ec);
        if(ec)
        {
            cerr<<"Init tls failed,reason:"<< ec.message()<<endl;
        }
        
        return ctx;
    }

    template<>
    void client_impl<client_type_tls>::template_init()
    {
        m_client.set_tls_init_handler(&on_tls_init);
    }
#endif

    bool client_impl_base::is_tls(const string& uri)
    {
        websocketpp::uri uo(uri);
        if(boost::iequals(uo.get_scheme(), "http") || boost::iequals(uo.get_scheme(), "ws"))
        {
            return false;
        }
#if SIO_TLS
        else if(boost::iequals(uo.get_scheme(), "https") || boost::iequals(uo.get_scheme(), "wss"))
        {
            return true;
        }
#endif
        else
        {
            throw std::runtime_error("unsupported URI scheme");
        }
    }

    socket*
    client_impl_base::new_socket(const string& nsp)
    { return new sio::socket(this, nsp); }

    void
    client_impl_base::socket_on_message_packet(socket::ptr& s, const packet& p)
    { s->on_message_packet(p); }

    template class client_impl<client_type_no_tls>;
#if SIO_TLS
    template class client_impl<client_type_tls>;
#endif
}
