#include "sio_socket.h"
#include "sio_packet.h"
namespace sio {

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


class socket::impl
{
public:

    impl(client::impl *,std::string const&);
    ~impl();

    void on(std::string const& event_name,event_listener_aux const& func)
    {
        m_event_binding[event_name] = event_adapter::do_adapt(func);
    }

    void on(std::string const& event_name,event_listener const& func)
    {
        m_event_binding[event_name] = func;
    }

    void off(std::string const& event_name)
    {
        auto it = m_event_binding.find(event_name);
        if(it!=m_event_binding.end())
        {
            m_event_binding.erase(it);
        }
    }

    void all_off()
    {
        m_event_binding.clear();
    }

#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
    void set_##__FIELD__(__TYPE__ const& l) \
    { m_##__FIELD__ = l;}
    SYNTHESIS_SETTER(con_listener,connect_listener)

    SYNTHESIS_SETTER(error_listener, error_listener) //socket io errors

    SYNTHESIS_SETTER(con_listener, disconnect_listener) //socket io errors
#undef SYNTHESIS_SETTER

    void connect();

    void emit(std::string const& name, std::string const& message);

    void emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack);

    void emit(std::string const& name, message::ptr const& args);

    void emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack);

    void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr);

    void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack);

    std::string const& get_namespace() const {return m_nsp;}

protected:
    void on_connected();

    void on_disconnected();

    void on_message_packet(packet const& packet);

private:

    // Message Parsing callbacks.
    void on_socketio_event(const std::string& nsp, int msgId,const std::string& name, message::ptr const& message);
    void on_socketio_ack(int msgId, message::ptr const& message);
    void on_socketio_error(message::ptr const& err_message);

    void __ack(int msgId,string const& name,message::ptr const& ack_message);

    static unsigned int s_global_event_id;

    client::impl *m_client;

    bool m_connected;
    std::string m_nsp;

    // Currently we assume websocket as the transport, though you can find others in this string
    std::map<unsigned int, std::function<void (message::ptr const&)> > m_acks;

    std::map<std::string, event_listener> m_event_binding;

    con_listener m_connect_listener;

    con_listener m_disconnect_listener;

    error_listener m_error_listener;

    std::unique_ptr<boost::asio::deadline_timer> m_connection_timer;
};

socket::impl(client::impl *client,std::string const& nsp):
m_client(client),m_nsp(nsp)
{

}

socket::~impl()
{

}

unsigned int client::impl::s_global_event_id = 1;

void socket::impl::emit(std::string const& name, std::string const& message)
{
    message::ptr msg_ptr = make_message(name, message);
    packet p(m_nsp, msg_ptr);
    m_client->send(p);
}

void socket::impl::emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack)
{
    message::ptr msg_ptr = make_message(name, message);
    packet p(m_nsp, msg_ptr,s_global_event_id);
    m_acks[s_global_event_id++] = ack;
    m_client->send(p);
}

void socket::impl::emit(std::string const& name, message::ptr const& args)
{
    message::ptr msg_ptr = make_message(name, args);
    packet p(m_nsp, msg_ptr);
    m_client->send(p);
}

void socket::impl::emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack)
{
    message::ptr msg_ptr = make_message(name, args);
    packet p(m_nsp, msg_ptr,s_global_event_id);
    m_acks[s_global_event_id++] = ack;
    m_client->send(p);
}

void socket::impl::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr)
{
    message::ptr msg_ptr = make_message(name, binary_ptr);
    packet p(m_nsp, msg_ptr);
    m_client->send(p);
}

void socket::impl::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack)
{
    message::ptr msg_ptr = make_message(name, binary_ptr);
    packet p(m_nsp, msg_ptr,s_global_event_id);
    m_acks[s_global_event_id++] = ack;
    m_client->send(p);
}

 void connect()
 {
     if(!m_connect)
     {
         packet p(packet::type_connect,m_nsp);
         m_client->send(p);
     }
 }

void socket::impl::on_connected()
{

        m_connected = true;
        if(m_connect_listener)
        {
            m_connect_listener();
        }
}

void socket::impl::on_disconnected()
{
    m_connected = false;
    m_client->remove_socket(m_nsp);
    m_client = NULL;
    if(m_disconnect_listener)
    {
        m_disconnect_listener();
    }
}

void socket::impl::on_message_packet(packet const& p)
{
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
                this->on_disconnected();
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

void socket::impl::on_socketio_ack(int msgId, message::ptr const& message)
{
    auto it = m_acks.find(msgId);
    if(it!=m_acks.end())
    {
        (it->second)(message);
        m_acks.erase(it);
    }
}

void socket::impl::on_socketio_error(message::ptr const& err_message)
{
    if(m_error_listener)m_error_listener(err_message);
}

}




