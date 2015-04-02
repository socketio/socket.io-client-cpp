#include "sio_socket.h"
namespace sio {
class socket::impl
{
public:
    typedef std::function<void(const std::string& name,message::ptr const& message,bool need_ack, message::ptr& ack_message)> event_listener;

    typedef std::function<void(message::ptr const& message)> error_listener;

    typedef std::function<void(void)> con_listener;

    impl();
    ~impl();

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

    void clear_event_bindings()
    {
        m_event_binding.clear();
    }

#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
    void set_##__FIELD__(__TYPE__ const& l) \
    { m_##__FIELD__ = l;}
    SYNTHESIS_SETTER(con_listener,connect_listener)

    SYNTHESIS_SETTER(error_listener, error_listener) //socket io errors
#undef SYNTHESIS_SETTER

    void emit(std::string const& name, std::string const& message);

    void emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack);

    void emit(std::string const& name, message::ptr const& args);

    void emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack);

    void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr);

    void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack);

protected:
    void on_connected();
private:

    void __ack(int msgId,string const& name,message::ptr const& ack_message);

    static unsigned int s_global_event_id;

    client::impl *m_client;

    bool m_connected;
    std::string m_nsp;

    // Currently we assume websocket as the transport, though you can find others in this string
    std::map<unsigned int, std::function<void (message::ptr const&)> > m_acks;

    std::map<std::string, event_listener> m_event_binding;

    con_listener m_open_listener;

            std::unique_ptr<boost::asio::deadline_timer> m_connection_timer;
};

socket::impl()
{

}

socket::~impl()
{

}


void socket::emit(std::string const& name, std::string const& message);

void socket::emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack);

void socket::emit(std::string const& name, message::ptr const& args);

void socket::emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack);

void socket::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr);

void socket::emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack);

}




