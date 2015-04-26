//
//  sio_client.h
//
//  Created by Melo Yao on 3/25/15.
//

#ifndef SIO_CLIENT_H
#define SIO_CLIENT_H
#include <string>
#include <functional>
#include "sio_message.h"
#include "sio_socket.h"

namespace sio
{
    class client_impl;
    
    class client {
    public:
        enum close_reason
        {
            close_reason_normal,
            close_reason_drop
        };
        
        typedef std::function<void(void)> con_listener;
        
        typedef std::function<void(close_reason const& reason)> close_listener;

        typedef std::function<void(unsigned, unsigned)> reconnect_listener;
        
        typedef std::function<void(std::string const& nsp)> socket_listener;
        
        client();
        ~client();
        
        //set listeners and event bindings.
        void set_open_listener(con_listener const& l);
        
        void set_fail_listener(con_listener const& l);
        
        void set_reconnecting_listener(con_listener const& l);

        void set_reconnect_listener(reconnect_listener const& l);

        void set_close_listener(close_listener const& l);
        
        void set_socket_open_listener(socket_listener const& l);
        
        void set_socket_close_listener(socket_listener const& l);
        
        void clear_con_listeners();
        
        void clear_socket_listeners();
        
        // Client Functions - such as send, etc.
        void connect(const std::string& uri);

        void connect(const std::string& uri, const std::map<std::string,std::string>& query);

        void set_reconnect_attempts(int attempts);

        void set_reconnect_delay(unsigned millis);

        void set_reconnect_delay_max(unsigned millis);
        
        sio::socket::ptr const& socket(const std::string& nsp = "");
        
        // Closes the connection
        void close();
        
        void sync_close();
        
        bool opened() const;
        
        std::string const& get_sessionid() const;
        
    private:
        //disable copy constructor and assign operator.
        client(client const& cl){}
        void operator=(client const& cl){}
        
        client_impl* m_impl;
    };
    
}


#endif // __SIO_CLIENT__H__
