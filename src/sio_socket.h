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

        const message::list& get_messages() const;
        
        bool need_ack() const;
        
        void put_ack_message(message::list const& ack_message);
        
        message::list const& get_ack_message() const;
        
    protected:
        event(std::string const& nsp,std::string const& name,message::list const& messages,bool need_ack);
        event(std::string const& nsp,std::string const& name,message::list&& messages,bool need_ack);

        message::list& get_ack_message_impl();
        
    private:
        const std::string m_nsp;
        const std::string m_name;
        const message::list m_messages;
        const bool m_need_ack;
        message::list m_ack_message;
        
        friend class event_adapter;
    };
    
    class client_impl;
    class packet;
    
    //The name 'socket' is taken from concept of official socket.io.
    class socket
    {
    public:
        typedef std::function<void(const std::string& name,message::ptr const& message,bool need_ack, message::list& ack_message)> event_listener_aux;
        
        typedef std::function<void(event& event)> event_listener;
        
        typedef std::function<void(message::ptr const& message)> error_listener;
        
        typedef std::shared_ptr<socket> ptr;
        
        ~socket();
        
        void on(std::string const& event_name,event_listener const& func);
        
        void on(std::string const& event_name,event_listener_aux const& func);
        
        void off(std::string const& event_name);
        
        void off_all();
        
        void close();
        
        void on_error(error_listener const& l);
        
        void off_error();

        void emit(std::string const& name, message::list const& msglist = nullptr, std::function<void (message::list const&)> const& ack = nullptr);
        
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
        socket(socket const&){}
        void operator=(socket const&){}

        class impl;
        impl *m_impl;
    };
}
#endif // SIO_SOCKET_H
