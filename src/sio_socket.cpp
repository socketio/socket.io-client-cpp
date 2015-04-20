#include "sio_socket.h"
#include "internal/sio_packet.h"
#include "internal/sio_client_impl.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/system/error_code.hpp>
#include <queue>
#include <cstdarg>

#if DEBUG || _DEBUG
#define LOG(x) std::cout << x
#else
#define LOG(x)
#endif

#define NULL_GUARD(_x_)  \
    if(_x_ == NULL) return

namespace sio
{
    class event_adapter
    {
    public:
        static void adapt_func(socket::event_listener_aux  const& func, event& event)
        {
            func(event.get_name(),event.get_message(),event.need_ack(),event.get_ack_message_impl());
        }
        
        static inline socket::event_listener do_adapt(socket::event_listener_aux const& func)
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
    
    message::ptr& event::get_ack_message_impl()
    {
        return m_ack_message;
    }
    
    class socket::impl
    {
    public:
        
        impl(client_impl *,std::string const&);
        ~impl();
        
        void on(std::string const& event_name,event_listener_aux const& func);
        
        void on(std::string const& event_name,event_listener const& func);
        
        void off(std::string const& event_name);
        
        void off_all();
        
#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
    void set_##__FIELD__(__TYPE__ const& l) \
        { m_##__FIELD__ = l;}
        
        SYNTHESIS_SETTER(error_listener, error_listener) //socket io errors
        
#undef SYNTHESIS_SETTER
        
        void on_error(error_listener const& l);
        
        void off_error();
        
        void close();
        
        void emit(std::string const& name, message::list const& msglist, std::function<void (message::ptr const&)> const& ack);
        
        std::string const& get_namespace() const {return m_nsp;}
        
    protected:
        void on_connected();
        
        void on_close();
        
        void on_open();
        
        void on_message_packet(packet const& packet);
        
        void on_disconnect();
        
    private:
        
        // Message Parsing callbacks.
        void on_socketio_event(const std::string& nsp, int msgId,const std::string& name, message::ptr const& message);
        void on_socketio_ack(int msgId, message::ptr const& message);
        void on_socketio_error(message::ptr const& err_message);
        
        event_listener get_bind_listener_locked(string const& event);
        
        void ack(int msgId,string const& name,message::ptr const& ack_message);
        
        void timeout_connection(const boost::system::error_code &ec);
        
        void send_connect();
        
        void send_packet(packet& p);
        
        static event_listener s_null_event_listener;
        
        static unsigned int s_global_event_id;
        
        sio::client_impl *m_client;
        
        bool m_connected;
        std::string m_nsp;
        
        std::map<unsigned int, std::function<void (message::ptr const&)> > m_acks;
        
        std::map<std::string, event_listener> m_event_binding;
        
        error_listener m_error_listener;
        
        std::unique_ptr<boost::asio::deadline_timer> m_connection_timer;
        
        std::queue<packet> m_packet_queue;
        
        std::mutex m_event_mutex;
        
        friend class socket;
    };
    
    void socket::impl::on(std::string const& event_name,event_listener_aux const& func)
    {
        this->on(event_name,event_adapter::do_adapt(func));
    }
    
    void socket::impl::on(std::string const& event_name,event_listener const& func)
    {
        std::lock_guard<std::mutex> guard(m_event_mutex);
        m_event_binding[event_name] = func;
    }
    
    void socket::impl::off(std::string const& event_name)
    {
        std::lock_guard<std::mutex> guard(m_event_mutex);
        auto it = m_event_binding.find(event_name);
        if(it!=m_event_binding.end())
        {
            m_event_binding.erase(it);
        }
    }
    
    void socket::impl::off_all()
    {
        std::lock_guard<std::mutex> guard(m_event_mutex);
        m_event_binding.clear();
    }
    
    void socket::impl::on_error(error_listener const& l)
    {
        m_error_listener = l;
    }
    
    void socket::impl::off_error()
    {
        m_error_listener = nullptr;
    }
    
    socket::impl::impl(client_impl *client,std::string const& nsp):
        m_client(client),
        m_nsp(nsp),
        m_connected(false)
    {
        NULL_GUARD(client);
        if(m_client->opened())
        {
            send_connect();
        }
    }
    
    socket::impl::~impl()
    {
        
    }
    
    unsigned int socket::impl::s_global_event_id = 1;
    
    void socket::impl::emit(std::string const& name, message::list const& msglist, std::function<void (message::ptr const&)> const& ack)
    {
        NULL_GUARD(m_client);
        message::ptr msg_ptr = msglist.to_array_message(name);
        int pack_id;
        if(ack)
        {
            pack_id = s_global_event_id++;
            std::lock_guard<std::mutex> guard(m_event_mutex);
            m_acks[pack_id] = ack;
        }
        else
        {
            pack_id = -1;
        }
        packet p(m_nsp, msg_ptr,pack_id);
        send_packet(p);
    }
    
    void socket::impl::send_connect()
    {
        NULL_GUARD(m_client);
        packet p(packet::type_connect,m_nsp);
        m_client->send(p);
        m_connection_timer.reset(new boost::asio::deadline_timer(m_client->get_io_service()));
        boost::system::error_code ec;
        m_connection_timer->expires_from_now(boost::posix_time::milliseconds(20000), ec);
        m_connection_timer->async_wait(std::bind(&socket::impl::timeout_connection,this, std::placeholders::_1));
    }
    
    void socket::impl::close()
    {
        NULL_GUARD(m_client);
        if(m_connected)
        {
            packet p(packet::type_disconnect,m_nsp);
            send_packet(p);
            
            if(!m_connection_timer)
            {
                m_connection_timer.reset(new boost::asio::deadline_timer(m_client->get_io_service()));
            }
            boost::system::error_code ec;
            m_connection_timer->expires_from_now(boost::posix_time::milliseconds(3000), ec);
            m_connection_timer->async_wait(lib::bind(&socket::impl::on_close, this));
        }
    }
    
    void socket::impl::on_connected()
    {
        if(m_connection_timer)
        {
            m_connection_timer->cancel();
            m_connection_timer.reset();
        }
        if(!m_connected)
        {
            m_connected = true;
            m_client->on_socket_opened(m_nsp);
            while (!m_packet_queue.empty()) {
                m_client->send(m_packet_queue.front());
                m_packet_queue.pop();
            }
        }
    }
    
    void socket::impl::on_close()
    {
        NULL_GUARD(m_client);
        if(m_connection_timer)
        {
            m_connection_timer->cancel();
            m_connection_timer.reset();
        }
        m_connected = false;
        while (!m_packet_queue.empty()) {
            m_packet_queue.pop();
        }
        m_client->on_socket_closed(m_nsp);
        m_client->remove_socket(m_nsp);
        m_client = NULL;
    }
    
    void socket::impl::on_open()
    {
        send_connect();
    }
    
    void socket::impl::on_disconnect()
    {
        NULL_GUARD(m_client);
        if(m_connected)
        {
            m_connected = false;
            while (!m_packet_queue.empty()) {
                m_packet_queue.pop();
            }
        }
    }
    
    void socket::impl::on_message_packet(packet const& p)
    {
        NULL_GUARD(m_client);
        if(p.get_nsp() == m_nsp)
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
                this->on_close();
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
        }
    }
    
    void socket::impl::on_socketio_event(const std::string& nsp,int msgId,const std::string& name, message::ptr const& message)
    {
        bool needAck = msgId >= 0;
        event ev = event_adapter::create_event(nsp,name, message,needAck);
        event_listener func = this->get_bind_listener_locked(name);
        if(func)func(ev);
        if(needAck)
        {
            this->ack(msgId, name, ev.get_ack_message());
        }
    }
    
    void socket::impl::ack(int msgId, const string &name, const message::ptr &ack_message)
    {
        message::list li(ack_message);
        packet p(m_nsp, li.to_array_message(name),msgId,true);
        send_packet(p);
    }
    
    void socket::impl::on_socketio_ack(int msgId, message::ptr const& message)
    {
        std::function<void (message::ptr const&)> l;
        {
            std::lock_guard<std::mutex> guard(m_event_mutex);
            auto it = m_acks.find(msgId);
            if(it!=m_acks.end())
            {
                l = it->second;
                m_acks.erase(it);
            }
        }
        if(l)l(message);
    }
    
    void socket::impl::on_socketio_error(message::ptr const& err_message)
    {
        if(m_error_listener)m_error_listener(err_message);
    }
    
    void socket::impl::timeout_connection(const boost::system::error_code &ec)
    {
        NULL_GUARD(m_client);
        if(ec)
        {
            return;
        }
        m_connection_timer.reset();
        LOG("Connection timeout"<<std::endl);
        this->on_disconnect();
    }
    
    void socket::impl::send_packet(sio::packet &p)
    {
        NULL_GUARD(m_client);
        if(m_connected)
        {
            while (!m_packet_queue.empty()) {
                m_client->send(m_packet_queue.front());
                m_packet_queue.pop();
            }
            m_client->send(p);
        }
        else
        {
            m_packet_queue.push(p);
        }
    }
    
    socket::event_listener socket::impl::get_bind_listener_locked(const string &event)
    {
        std::lock_guard<std::mutex> guard(m_event_mutex);
        auto it = m_event_binding.find(event);
        if(it!=m_event_binding.end())
        {
            return it->second;
        }
        return socket::event_listener();
    }
    
    socket::socket(client_impl* client,std::string const& nsp):
        m_impl(new impl(client,nsp))
    {
    }
    
    socket::~socket()
    {
        delete m_impl;
    }
    
    void socket::on(std::string const& event_name,event_listener const& func)
    {
        m_impl->on(event_name, func);
    }
    
    void socket::on(std::string const& event_name,event_listener_aux const& func)
    {
        m_impl->on(event_name, func);
    }
    
    void socket::off(std::string const& event_name)
    {
        m_impl->off(event_name);
    }
    
    void socket::off_all()
    {
        m_impl->off_all();
    }
    
    void socket::close()
    {
        m_impl->close();
    }
    
    void socket::on_error(error_listener const& l)
    {
        m_impl->on_error(l);
    }
    
    void socket::off_error()
    {
        m_impl->off_error();
    }

    void socket::emit(std::string const& name, message::list const& msglist, std::function<void (message::ptr const&)> const& ack)
    {
        m_impl->emit(name, msglist,ack);
    }
    
    std::string const& socket::get_namespace() const
    {
        return m_impl->get_namespace();
    }
    
    void socket::on_connected()
    {
        m_impl->on_connected();
    }
    
    void socket::on_close()
    {
        m_impl->on_close();
    }
    
    void socket::on_open()
    {
        m_impl->on_open();
    }
    
    void socket::on_message_packet(packet const& p)
    {
        m_impl->on_message_packet(p);
    }
    
    void socket::on_disconnect()
    {
        m_impl->on_disconnect();
    }
}




