//
//  sio_client.h
//
//  Created by Melo Yao on 3/25/15.
//

#include "sio_client.h"
#include "internal/sio_client_impl.h"

#include <sstream>
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
    class event_adapter
    {
    public:
        static void adapt_func(client::event_listener_aux  const& func, event& event)
        {
            func(event.get_name(),event.get_message(),event.need_ack(),event.__get_ack_message());
        }
        
        static inline client::event_listener do_adapt(client::event_listener_aux const& func)
        {
            return std::bind(&event_adapter::adapt_func, func,std::placeholders::_1);
        }
        
        static inline event create_event(std::string const& nsp,std::string const& name,message::ptr const& message,bool need_ack)
        {
            return event(nsp,name,message,need_ack);
        }
    };
    
    inline
    const std::string& event::get_nsp() const
    {
        return m_nsp;
    }
    
    inline
    const std::string& event::get_name() const
    {
        return m_name;
    }
    
    inline
    const message::ptr& event::get_message() const
    {
        return m_message;
    }
    
    inline
    bool event::need_ack() const
    {
        return m_need_ack;
    }
    
    inline
    void event::put_ack_message(message::ptr const& ack_message)
    {
        if(m_need_ack)
            m_ack_message = ack_message;
    }
    
    inline
    event::event(std::string const& nsp,std::string const& name,message::ptr const& message,bool need_ack):
    m_nsp(nsp),
    m_name(name),
    m_message(message),
    m_need_ack(need_ack)
    {
    }
    
    message::ptr const& event::get_ack_message() const
    {
        return m_ack_message;
    }
    
    message::ptr& event::__get_ack_message()
    {
        return m_ack_message;
    }
    
    
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
    
    void client::set_default_event_listener(event_listener_aux const& l)
    {
        m_impl->set_default_event_listener(event_adapter::do_adapt(l));
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
    
    void client::bind_event(std::string const& event_name,event_listener_aux const& func)
    {
        m_impl->bind_event(event_name,event_adapter::do_adapt(func));
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
    
    bool client::connected() const
    {
        return m_impl->connected();
    }
    
    std::string const& client::get_sessionid() const
    {
        return m_impl->get_sessionid();
    }
    
    std::string const& client::get_namespace() const
    {
        return m_impl->get_namespace();
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
		using websocketpp::lib::placeholders::_2;
        m_client.set_open_handler(lib::bind(&client::impl::on_open,this,_1));
        m_client.set_close_handler(lib::bind(&client::impl::on_close,this,_1));
        m_client.set_fail_handler(lib::bind(&client::impl::on_fail,this,_1));
        m_client.set_message_handler(lib::bind(&client::impl::on_message,this,_1,_2));
        
        m_packet_mgr.set_decode_callback(lib::bind(&client::impl::on_decode,this,_1));
        
        m_packet_mgr.set_encode_callback(lib::bind(&client::impl::on_encode,this,_1,_2));
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
            if(ec)LOG("ec:"<<ec.message()<<std::endl);
            m_ping_timer->async_wait(lib::bind(&client::impl::__ping,this,lib::placeholders::_1));
            LOG("On handshake,sid:"<<m_sid<<",ping interval:"<<m_ping_interval<<",ping timeout"<<"m_ping_timeout"<<m_ping_timeout<<std::endl);
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
        if(m_nsp.length()>0)//send connect only if we got nsp, otherwise wait for default one.
        {
            packet p(packet::type_connect, m_nsp);
            m_packet_mgr.encode(p,
                                [&](bool isBin,shared_ptr<const string> payload)
            {
                lib::error_code ec;
                this->m_client.send(this->m_con, *payload, frame::opcode::text, ec);
            });
        }
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
            m_ping_timeout_timer->async_wait(lib::bind(&client::impl::__timeout_pong, this,lib::placeholders::_1));
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
                        if(p.get_nsp() == m_nsp)
                        {
                            this->on_connected();
                        }
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
                                this->on_socketio_event(p.get_nsp(), p.get_pack_id(),name_ptr->get_string(), value_ptr);
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
            packet pack(packet::type_disconnect,m_nsp);
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
            if(m_ping_timer)
            {
                m_ping_timer->expires_from_now(milliseconds(m_ping_interval),timeout_ec);
                m_ping_timer->async_wait(lib::bind(&client::impl::__ping,this,lib::placeholders::_1));
            }
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
            message_queue_element element = INTIALIZER(message_queue_element){opcode,payload_ptr};
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
            if(uo.get_resource()!="/")
            {
                m_nsp = uo.get_resource();
            }
            else
            {
                m_nsp.clear();
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
    void client::impl::on_socketio_event(const std::string& nsp,int msgId,const std::string& name, message::ptr const& message)
    {
        event_listener *functor_ptr = &(m_default_event_listener);
        auto it = m_event_binding.find(name);
        if(it!=m_event_binding.end())
        {
            functor_ptr = &(it->second);
        }
        bool needAck = msgId >= 0;
        event ev = event_adapter::create_event(nsp,name, message,needAck);
        if (*functor_ptr) {
            (*functor_ptr)(ev);
        }
        if(needAck)
        {
            this->__ack(msgId, name, ev.get_ack_message());
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
