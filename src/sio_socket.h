#ifndef SIO_SOCKET_H
#define SIO_SOCKET_H
#include "internal/sio_client_impl.h"
namespace sio
{
class socket
{
public:
    typedef std::function<void(const std::string& name,message::ptr const& message,bool need_ack, message::ptr& ack_message)> event_listener;

    typedef std::function<void(message::ptr const& message)> error_listener;

    typedef std::function<void(void)> con_listener;

    socket();
    ~socket();

    void bind_event(std::string const& event_name,event_listener const& func);

    void unbind_event(std::string const& event_name);

    void clear_event_bindings();


    void set_connect_listener(con_listener const& l) ;
    void set_error_listener(error_listener const& l);

    void emit(std::string const& name, std::string const& message);

    void emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack);

    void emit(std::string const& name, message::ptr const& args);

    void emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack);

    void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr);

    void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack);

private:
    class impl;
    impl *m_impl;
};
}
#endif // SIO_SOCKET_H
