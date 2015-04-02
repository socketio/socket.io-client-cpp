//
//  sio_client.h
//
//  Created by Melo Yao on 3/25/15.
//

#ifndef __SIO_CLIENT__H__
#define __SIO_CLIENT__H__
#include <string>
#include <functional>
#include "sio_message.h"

namespace sio {
    
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
    
    class client {
    public:
        enum close_reason
        {
            close_reason_normal,
            close_reason_drop
        };
        
        typedef std::function<void(void)> con_listener;
        
        typedef std::function<void(close_reason const& reason)> close_listener;

        typedef std::function<void(const std::string& name,message::ptr const& message,bool need_ack, message::ptr& ack_message)> event_listener_aux;
        
        typedef std::function<void(event& event)> event_listener;
        
        typedef std::function<void(message::ptr const& message)> error_listener;
        
        client();
        ~client();
        
        //set listeners and event bindings.
        void set_open_listener(con_listener const& l);
        
        void set_fail_listener(con_listener const& l);
        
        void set_connect_listener(con_listener const& l);
        
        void set_close_listener(close_listener const& l);
        
        void set_default_event_listener(event_listener_aux const& l);
        
        void set_default_event_listener(event_listener const& l);
        
        void set_error_listener(error_listener const& l); //socket io errors
        
        void bind_event(std::string const& event_name,event_listener const& func);
        
        void bind_event(std::string const& event_name,event_listener_aux const& func);
    
        void unbind_event(std::string const& event_name);
        
        void clear_socketio_listeners();
        
        void clear_con_listeners();
        
        void clear_event_bindings();
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
        
        bool connected() const;
        
        std::string const& get_sessionid() const;
        
        std::string const& get_namespace() const;
        
    private:
        class impl;
        impl* m_impl;
    };
    
}


#endif // __SIO_CLIENT__H__