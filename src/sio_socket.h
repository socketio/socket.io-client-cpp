#ifndef SIO_SOCKET_H
#define SIO_SOCKET_H
#include "sio_message.h"
#include <functional>
namespace sio
{
    class event_adapter;
    
    class event
    {
    public:
        const std::string& get_nsp() const;
        
        const std::string& get_name() const;
        
        const message::ptr& get_message() const;
        
        bool need_ack() const;
        
        void put_ack_message(message::ptr const& ack_message);
        
        message::ptr const& get_ack_message() const;
        
    protected:
        event(std::string const& nsp,std::string const& name,message::ptr const& message,bool need_ack);
        
        message::ptr& __get_ack_message();
        
    private:
        const std::string m_nsp;
        const std::string m_name;
        const message::ptr m_message;
        const bool m_need_ack;
        message::ptr m_ack_message;
        
        friend class event_adapter;
    };
    
    class client_impl;
    class packet;
    
    class socket
    {
    public:
        typedef std::function<void(const std::string& name,message::ptr const& message,bool need_ack, message::ptr& ack_message)> event_listener_aux;
        
        typedef std::function<void(event& event)> event_listener;
        
        typedef std::function<void(message::ptr const& message)> error_listener;
        
        typedef std::function<void(void)> con_listener;
        
        typedef std::shared_ptr<socket> ptr;
        
        ~socket();
        
        void on(std::string const& event_name,event_listener const& func);
        
        void on(std::string const& event_name,event_listener_aux const& func);
        
        void off(std::string const& event_name);
        
        void off_all();
        
        void close();
        
        void set_connect_listener(con_listener const& l);
        
        void set_close_listener(con_listener const& l);
        
        void set_error_listener(error_listener const& l);
        
        void clear_listeners();
        
        void emit(std::string const& name, std::string const& message);
        
        void emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack);
        
        void emit(std::string const& name, message::ptr const& args);
        
        void emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack);
        
        void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr);
        
        void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack);
        
        std::string const& get_namespace() const;
        
    protected:
        socket(client_impl*,std::string const&);

        void on_connected();
        
        void on_close();
        
        void on_open();
        
        void on_disconnect();
        
        void on_message_packet(packet const& p);
        
        friend class client_impl;
        
    private:
        //disable copy constructor and assign operator.
        socket(socket const& sock){}
        void operator=(socket const& sock){}

        class impl;
        impl *m_impl;
    };
}
#endif // SIO_SOCKET_H
