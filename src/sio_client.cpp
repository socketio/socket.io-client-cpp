//
//  sio_client.h
//
//  Created by Melo Yao on 3/25/15.
//

#include "sio_client.h"

#ifndef _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_STL_ 1
#endif

#include <websocketpp/client.hpp>
#if _DEBUG || DEBUG
#include <websocketpp/config/debug_asio_no_tls.hpp>
typedef websocketpp::config::debug_asio client_config;
#else
#include <websocketpp/config/asio_no_tls_client.hpp>
typedef websocketpp::config::asio_client client_config;
#endif

#include <memory>
#include <map>
#include <queue>
#include <thread>
#include <sstream>
#include <rapidjson/document.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "sio_packet.h"
// Comment this out to disable handshake logging to stdout
#if DEBUG || _DEBUG
#define LOG(x) std::cout << x
#else
#define LOG(x)
#endif

using namespace rapidjson;
using namespace websocketpp;
using boost::posix_time::milliseconds;
using std::stringstream;

namespace sio
{
    typedef client<client_config> client_type;
    
    class client::impl {
    protected:
        impl();
        
        ~impl();
        
        //set listeners and event bindings.
#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
void set_##__FIELD__(__TYPE__ const& l) \
{ m_##__FIELD__ = l;}
        
        SYNTHESIS_SETTER(con_listener,open_listener)
        
        SYNTHESIS_SETTER(con_listener,fail_listener)
        
        SYNTHESIS_SETTER(con_listener,connect_listener)
        
        SYNTHESIS_SETTER(close_listener,close_listener)
        
        SYNTHESIS_SETTER(event_listener, default_event_listener)
        
        SYNTHESIS_SETTER(error_listener, error_listener) //socket io errors
        
        void bind_event(std::string const& event_name,event_listener const& func)
        {
            m_event_binding[event_name] = func;
        }
        
        void unbind_event(std::string const& event_name)
        {
            auto it = m_event_binding.find(event_name);
            if(it!=m_event_binding.end())
            {
                m_event_binding.erase(it);
            }
        }
        
        void clear_socketio_listeners()
        {
            m_default_event_listener = nullptr;
            m_error_listener = nullptr;
        }
        
        void clear_con_listeners()
        {
            m_open_listener = nullptr;
            m_close_listener = nullptr;
            m_fail_listener = nullptr;
            m_connect_listener = nullptr;
        }
        
        void clear_event_bindings()
        {
            m_event_binding.clear();
        }
        // Client Functions - such as send, etc.
        
        //event emit functions, for plain message,json and binary
        void emit(std::string const& name, std::string const& message);
        
        void emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack);
        
        void emit(std::string const& name, message::ptr const& args);
        
        void emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack);
        
        void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr);
        
        void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack);
        
        
        void connect(const std::string& uri);
        
        void reconnect(const std::string& uri);
        
        // Closes the connection
        void close();
        
        void sync_close();
        
        std::string const& get_sessionid() const { return m_sid; }
        
        bool connected() const { return m_connected; }
        
        friend class client;
    private:
        void send(std::shared_ptr<const std::string> const& payload_ptr,frame::opcode::value opcode);
        
        void __close(close::status::value const& code,std::string const& reason);
        
        void __connect(const std::string& uri);
        
        void __send(std::shared_ptr<const std::string> const&  payload_ptr,frame::opcode::value opcode);
        
        void __ack(int msgId,string const& name,message::ptr const& ack_message);
        
        void __ping(const boost::system::error_code& ec);
        
        void __timeout_pong(const boost::system::error_code& ec);
        
        void __timeout_connection(const boost::system::error_code& ec);
        
        void run_loop();
        
        void on_decode(packet const& pack);
        void on_encode(bool isBinary,shared_ptr<const string> const& payload);
        
        // Callbacks
        void on_fail(connection_hdl con);
        void on_open(connection_hdl con);
        void on_close(connection_hdl con);
        void on_message(connection_hdl con, client_type::message_ptr msg);
        void on_handshake(message::ptr const& message);
        void on_connected();
        void on_pong();
        
        void on_pong_timeout();
        
        // Message Parsing callbacks.
        void on_socketio_event(int msgId,const std::string& name, message::ptr const& message);
        void on_socketio_ack(int msgId, message::ptr const& message);
        
        void on_socketio_error(message::ptr const& err_message);
        
        void reset_states();
        
        void clear_timers();
        
        // Connection pointer for client functions.
        connection_hdl m_con;
        client_type m_client;
        // Socket.IO server settings
        std::string m_sid;
        unsigned int m_ping_interval;
        unsigned int m_ping_timeout;
        
        bool m_connected;
        std::string m_nsp;
        
        // Currently we assume websocket as the transport, though you can find others in this string
        std::map<unsigned int, std::function<void (message::ptr const&)> > m_acks;
        
        std::map<std::string, event_listener> m_event_binding;
        
        static unsigned int s_global_event_id;
        
        std::unique_ptr<std::thread> m_network_thread;
        
        struct message_queue_element
        {
            frame::opcode::value opcode;
            std::shared_ptr<const std::string> payload_ptr;
        };
        
        packet_manager m_packet_mgr;
        
        std::queue<message_queue_element> m_message_queue;
        
        std::unique_ptr<boost::asio::deadline_timer> m_ping_timer;
        
        std::unique_ptr<boost::asio::deadline_timer> m_ping_timeout_timer;
        
        std::unique_ptr<boost::asio::deadline_timer> m_connection_timer;
        
        con_listener m_open_listener;
        con_listener m_connect_listener;
        con_listener m_fail_listener;
        close_listener m_close_listener;
        
        event_listener m_default_event_listener;
        error_listener m_error_listener;
        
    };
    
    client::client():
    m_impl(new impl())
    {
    }
    
    client::~client()
    {
        delete m_impl;
    }
    
    void client::set_open_listener(con_listener const& l)
    {
        m_impl->set_open_listener(l);
    }
    
    void client::set_fail_listener(con_listener const& l)
    {
        m_impl->set_fail_listener(l);
    }
    
    void client::set_connect_listener(con_listener const& l)
    {
        m_impl->set_connect_listener(l);
    }
    
    void client::set_close_listener(close_listener const& l)
    {
        m_impl->set_close_listener(l);
    }
    
    void client::set_default_event_listener(event_listener const& l)
    {
        m_impl->set_default_event_listener(l);
    }
    
    void client::set_error_listener(error_listener const& l)
    {
        m_impl->set_error_listener(l);
    }
    
    void client::bind_event(std::string const& event_name,event_listener const& func)
    {
        m_impl->bind_event(event_name,func);
    }
    
    void client::unbind_event(std::string const& event_name)
    {
        m_impl->unbind_event(event_name);
    }
    
    void client::clear_socketio_listeners()
    {
        m_impl->clear_socketio_listeners();
    }
    
    void client::clear_con_listeners()
    {
        m_impl->clear_con_listeners();
    }
    
    void client::clear_event_bindings()
    {
        m_impl->clear_event_bindings();
    }
    // Client Functions - such as send, etc.
    
    //event emit functions, for plain message,json and binary
    void client::emit(std::string const& name, std::string const& message)
    {
        m_impl->emit(name,message);
    }
    
    void client::emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack)
    {
        m_impl->emit(name,message,ack);
    }
    
    void client::emit(std::string const& name, message::ptr const& args)
    {
        m_impl->emit(name,args);
    }
    
    void client::emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack)
    {
        m_impl->emit(name,args,ack);
    }
    
    void client::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr)
    {
        m_impl->emit(name,binary_ptr);
    }
    
    void client::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack)
    {
        m_impl->emit(name,binary_ptr,ack);
    }
    
    void client::connect(const std::string& uri)
    {
        m_impl->connect(uri);
    }
    
    void client::reconnect(const std::string& uri)
    {
        m_impl->reconnect(uri);
    }
    
    // Closes the connection
    void client::close()
    {
        m_impl->close();
    }
    
    void client::sync_close()
    {
        m_impl->sync_close();
    }
    
    std::string const& client::get_sessionid() const
    {
        return m_impl->get_sessionid();
    }
    
    bool client::connected() const
    {
        return m_impl->connected();
    }
    
    
    client::impl::impl() :
    m_connected(false),
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
        using websocketpp::lib::bind;
        m_client.set_open_client(bind(&client::impl::on_open,this,::_1));
        m_client.set_close_client(bind(&client::impl::on_close,this,::_1));
        m_client.set_fail_client(bind(&client::impl::on_fail,this,::_1));
        m_client.set_message_client(bind(&client::impl::on_message,this,::_1,::_2));
        
        m_packet_mgr.set_decode_callback(bind(&client::impl::on_decode,this,::_1));
        
        m_packet_mgr.set_encode_callback(bind(&client::impl::on_encode,this,::_1,::_2));
    }
    
    client::impl::~impl()
    {
        sync_close();
    }
    
    // Websocket++ client client
    void client::impl::on_fail(connection_hdl con)
    {
        m_con.reset();
        m_connected = false;
        
        LOG("Connection failed." << std::endl);
        if(m_fail_listener)m_fail_listener();
    }
    
    void client::impl::on_connected()
    {
        LOG("On Connected." << std::endl);
        m_connected = true;
        if(m_connection_timer)
        {
            boost::system::error_code ec;
            m_connection_timer->cancel(ec);
            m_connection_timer.reset();
        }
        if(m_connect_listener)
        {
            m_connect_listener();
        }
    }
    
    void client::impl::on_pong()
    {
        if(m_ping_timeout_timer)
        {
            m_ping_timeout_timer->cancel();
            m_ping_timeout_timer.reset();
        }
    }
    
    void client::impl::on_handshake(message::ptr const& message)
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
            m_ping_timer->async_wait(lib::bind(&client::impl::__ping,this,lib::placeholders::_1));
            LOG("On handshake,sid:"<<m_sid<<std::endl);
            return;
        }
    failed:
        //just close it.
        m_client.get_io_service().dispatch(lib::bind(&client::impl::__close, this,close::status::policy_violation,"Handshake error"));
    }
    
    void client::impl::on_open(connection_hdl con)
    {
        LOG("Connected." << std::endl);
        m_con = con;
        m_connection_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
        boost::system::error_code ec;
        m_connection_timer->expires_from_now(milliseconds(60000), ec);
        m_connection_timer->async_wait(lib::bind(&client::impl::__timeout_connection,this,lib::placeholders::_1));
        
        if(m_open_listener)m_open_listener();
    }
    
    void client::impl::on_close(connection_hdl con)
    {
        LOG("Client Disconnected." << std::endl);
        m_connected = false;
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
        if(m_close_listener)
        {
            close_reason reason;
            if(code == close::status::normal)
            {
                reason = close_reason_normal;
            }
            else
            {
                reason = close_reason_drop;
            }
            m_close_listener(reason);
        }
    }
    
    void client::impl::on_message(connection_hdl con, client_type::message_ptr msg)
    {
        if (m_ping_timeout_timer) {
            boost::system::error_code ec;
            m_ping_timeout_timer->expires_from_now(milliseconds(m_ping_timeout),ec);
        }
        // Parse the incoming message according to socket.IO rules
        m_packet_mgr.put_payload(msg->get_payload());
    }
    
    void client::impl::on_pong_timeout()
    {
        LOG("Pong timeout"<<std::endl);
        m_client.get_io_service().dispatch(lib::bind(&client::impl::__close, this,close::status::policy_violation,"Pong timeout"));
    }
    
    void client::impl::on_decode(packet const& p)
    {
        switch(p.get_frame())
        {
            case packet::frame_message:
            {
                switch (p.get_type())
                {
                        // Connect open
                    case packet::type_connect:
                    {
                        LOG("Received Message type (Connect)"<<std::endl);
                        this->on_connected();
                        break;
                    }
                    case packet::type_disconnect:
                    {
                        LOG("Received Message type (Disconnect)"<<std::endl);
                        close();
                        break;
                    }
                    case packet::type_event:
                    case packet::type_binary_event:
                    {
                        LOG("Received Message type (Event)"<<std::endl);
                        const message::ptr ptr = p.get_message();
                        if(ptr->get_flag() == message::flag_array)
                        {
                            const array_message* array_ptr = static_cast<const array_message*>(ptr.get());
                            if(array_ptr->get_vector().size() >= 1&&array_ptr->get_vector()[0]->get_flag() == message::flag_string)
                            {
                                const string_message* name_ptr = static_cast<const string_message*>(array_ptr->get_vector()[0].get());
                                message::ptr value_ptr;
                                if(array_ptr->get_vector().size()>1)
                                {
                                    value_ptr = array_ptr->get_vector()[1];
                                }
                                this->on_socketio_event(p.get_pack_id(),name_ptr->get_string(), value_ptr);
                            }
                        }
                        
                        break;
                    }
                        // Ack
                    case packet::type_ack:
                    case packet::type_binary_ack:
                    {
                        LOG("Received Message type (ACK)"<<std::endl);
                        const message::ptr ptr = p.get_message();
                        if(ptr->get_flag() == message::flag_array)
                        {
                            const array_message* array_ptr = static_cast<const array_message*>(ptr.get());
                            if(array_ptr->get_vector().size() >= 1&&array_ptr->get_vector()[0]->get_flag() == message::flag_string)
                            {
                                message::ptr value_ptr;
                                if(array_ptr->get_vector().size()>1)
                                {
                                    value_ptr = array_ptr->get_vector()[1];
                                }
                                this->on_socketio_ack(p.get_pack_id(), value_ptr);
                                break;
                            }
                        }
                        this->on_socketio_ack(p.get_pack_id(),ptr);
                        break;
                    }
                        // Error
                    case packet::type_error:
                    {
                        LOG("Received Message type (ERROR)"<<std::endl);
                        this->on_socketio_error(p.get_message());
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case packet::frame_open:
                this->on_handshake(p.get_message());
                break;
            case packet::frame_close:
                //FIXME how to deal?
                break;
            case packet::frame_pong:
                this->on_pong();
                break;
                
            default:
                break;
        }
    }
    
    void client::impl::on_encode(bool isBinary,shared_ptr<const string> const& payload)
    {
        LOG("encoded payload length:"<<payload->length()<<std::endl);
        m_client.get_io_service().dispatch(lib::bind(&client::impl::__send,this,payload,isBinary?frame::opcode::binary:frame::opcode::text));
    }
    
    
    unsigned int client::impl::s_global_event_id = 1;
    
    void client::impl::emit(std::string const& name, std::string const& message)
    {
        message::ptr msg_ptr = make_message(name, message);
        packet p(m_nsp, msg_ptr);
        m_packet_mgr.encode(p);
    }
    
    void client::impl::emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack)
    {
        message::ptr msg_ptr = make_message(name, message);
        packet p(m_nsp, msg_ptr,s_global_event_id);
        m_acks[s_global_event_id++] = ack;
        m_packet_mgr.encode(p);
    }
    
    void client::impl::emit(std::string const& name, message::ptr const& args)
    {
        message::ptr msg_ptr = make_message(name, args);
        packet p(m_nsp, msg_ptr);
        m_packet_mgr.encode(p);
    }
    
    void client::impl::emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack)
    {
        message::ptr msg_ptr = make_message(name, args);
        packet p(m_nsp, msg_ptr,s_global_event_id);
        m_acks[s_global_event_id++] = ack;
        m_packet_mgr.encode(p);
    }
    
    void client::impl::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr)
    {
        message::ptr msg_ptr = make_message(name, binary_ptr);
        packet p(m_nsp, msg_ptr);
        m_packet_mgr.encode(p);
    }
    
    void client::impl::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack)
    {
        message::ptr msg_ptr = make_message(name, binary_ptr);
        packet p(m_nsp, msg_ptr,s_global_event_id);
        m_acks[s_global_event_id++] = ack;
        m_packet_mgr.encode(p);
    }
    
    void client::impl::__ping(const boost::system::error_code& ec)
    {
        if(ec || m_con.expired())
        {
            return;
        }
        std::string ping_msg;
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
            m_ping_timer->async_wait(lib::bind(&client::impl::__ping,this,lib::placeholders::_1));
        }
        if(!m_ping_timeout_timer)
        {
            m_ping_timeout_timer.reset(new boost::asio::deadline_timer(m_client.get_io_service()));
            boost::system::error_code timeout_ec;
            m_ping_timeout_timer->expires_from_now(milliseconds(m_ping_timeout), timeout_ec);
            m_ping_timeout_timer->async_wait(lib::bind(&client::impl::__timeout_pong, this,lib::placeholders::_1));
        }
    }
    
    void client::impl::__timeout_pong(const boost::system::error_code &ec)
    {
        if(ec)
        {
            return;
        }
        this->on_pong_timeout();
    }
    
    void client::impl::__timeout_connection(const boost::system::error_code &ec)
    {
        if(ec)
        {
            return;
        }
        m_connection_timer.reset();
        LOG("Connection timeout"<<std::endl);
        this->__close(close::status::policy_violation,"Connection timeout");
    }
    
    void client::impl::__close(close::status::value const& code,std::string const& reason)
    {
        LOG("Close by reason:"<<reason << std::endl);
        if (m_con.expired())
        {
            std::cerr << "Error: No active session" << std::endl;
        }
        else
        {
            std::string payload;
            packet pack(packet::type_disconnect);
            m_packet_mgr.encode(pack,
                                [&](bool isBin,shared_ptr<const string> payload)
                                {
                                    lib::error_code ec;
                                    this->m_client.send(this->m_con, *payload, frame::opcode::text, ec);
                                });
            lib::error_code ec;
            m_client.close(m_con, code, reason, ec);
        }
    }
    
    void client::impl::close()
    {
        m_client.get_io_service().dispatch(lib::bind(&client::impl::__close, this,close::status::normal,"End by user"));
    }
    
    void client::impl::sync_close()
    {
        m_client.get_io_service().dispatch(lib::bind(&client::impl::__close, this,close::status::normal,"End by user"));
        if(m_network_thread)
        {
            m_network_thread->join();
            m_network_thread.reset();
        }
    }
    
    
    void client::impl::__send(std::shared_ptr<const std::string> const& payload_ptr,frame::opcode::value opcode)
    {
        if(connected())
        {
            //delay the ping, since we already have message to send.
            boost::system::error_code timeout_ec;
            m_ping_timer->expires_from_now(milliseconds(m_ping_interval),timeout_ec);
            while(m_message_queue.size()>0)
            {
                message_queue_element element = m_message_queue.front();
                m_message_queue.pop();
                m_client.send(m_con,*(element.payload_ptr),element.opcode);
            }
            m_client.send(m_con,*payload_ptr,opcode);
        }
        else
        {
            message_queue_element element = (message_queue_element){.opcode = opcode,.payload_ptr = payload_ptr};
            m_message_queue.push(element);
        }
    }
    
    void client::impl::__ack(int msgId,string const& name,message::ptr const& ack_message)
    {
        packet pack(m_nsp,make_message(name, ack_message),msgId,true);
        m_packet_mgr.encode(pack);
    }
    
    void client::impl::__connect(const std::string& uri)
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
    
    void client::impl::clear_timers()
    {
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
        if(m_connection_timer)
        {
            m_connection_timer->cancel(ec);
            m_connection_timer.reset();
        }
    }
    
    void client::impl::reset_states()
    {
        m_connected = false;
        m_acks.clear();
        m_client.reset();
        m_sid.clear();
        m_packet_mgr.reset();
        //clear all queued messages.
        while(!m_message_queue.empty())
        {
            m_message_queue.pop();
        }
    }
    
    void client::impl::connect(const std::string& uri)
    {
        if(m_network_thread)
        {
            m_network_thread->join();
        }
        this->reset_states();
        m_client.get_io_service().dispatch(lib::bind(&client::impl::__connect,this,uri));
        m_network_thread.reset(new std::thread(lib::bind(&client::impl::run_loop,this)));//uri lifecycle?
    }
    
    void client::impl::reconnect(const std::string& uri)
    {
        if(m_network_thread)
        {
            m_network_thread->join();
        }
        
        m_client.get_io_service().dispatch(lib::bind(&client::impl::__connect,this,uri));
        m_network_thread.reset(new std::thread(lib::bind(&client::impl::run_loop,this)));//uri
    }
    
    void client::impl::run_loop()
    {
        
        m_client.run();
        m_client.reset();
        m_client.get_alog().write(websocketpp::log::alevel::devel,
                                  "run loop end");
        
    }
    
    // This is where you'd add in behavior to handle events.
    // By default, nothing is done with the endpoint or ID params.
    void client::impl::on_socketio_event(int msgId,const std::string& name, message::ptr const& message)
    {
        event_listener *functor_ptr = &(m_default_event_listener);
        auto it = m_event_binding.find(name);
        if(it!=m_event_binding.end())
        {
            functor_ptr = &(it->second);
        }
        bool needAck = msgId >= 0;
        
        message::ptr ack_message;
        if (*functor_ptr) {
            (*functor_ptr)(name, message,needAck, ack_message);
        }
        if(needAck)
        {
            this->__ack(msgId, name, ack_message);
        }
    }
    
    // This is where you'd add in behavior to handle ack
    void client::impl::on_socketio_ack(int msgId, message::ptr const& message)
    {
        auto it = m_acks.find(msgId);
        if(it!=m_acks.end())
        {
            (it->second)(message);
            m_acks.erase(it);
        }
    }
    
    // This is where you'd add in behavior to handle errors
    void client::impl::on_socketio_error(message::ptr const& err_message)
    {
        if(m_error_listener)m_error_listener(err_message);
    }
}