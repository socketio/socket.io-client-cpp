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
    
    class client {
    public:
        enum close_reason
        {
            close_reason_normal,
            close_reason_drop
        };
        
        typedef std::function<void(void)> con_listener;
        
        typedef std::function<void(close_reason const& reason)> close_listener;
        
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

        void clear_event_bindings();

        void clear_socketio_listeners();
        
        void clear_con_listeners();
        

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
