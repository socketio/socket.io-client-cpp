//
//  sio_client.h
//
//  Created by Melo Yao on 3/25/15.
//

#include "sio_client.h"
#include "internal/sio_client_impl.h"

using namespace websocketpp;
using boost::posix_time::milliseconds;
using std::stringstream;

namespace sio
{
    client::client():
        m_impl(new client_impl())
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
    
    void client::set_close_listener(close_listener const& l)
    {
        m_impl->set_close_listener(l);
    }
    
    void client::set_socket_open_listener(socket_listener const& l)
    {
        m_impl->set_socket_open_listener(l);
    }
    
    void client::set_reconnect_listener(reconnect_listener const& l)
    {
        m_impl->set_reconnect_listener(l);
    }

    void client::set_reconnecting_listener(con_listener const& l)
    {
        m_impl->set_reconnecting_listener(l);
    }

    void client::set_socket_close_listener(socket_listener const& l)
    {
        m_impl->set_socket_close_listener(l);
    }
    
    void client::clear_con_listeners()
    {
        m_impl->clear_con_listeners();
    }
    
    void client::clear_socket_listeners()
    {
        m_impl->clear_socket_listeners();
    }

    void client::connect(const std::string& uri)
    {
        m_impl->connect(uri, {}, {});
    }

    void client::connect(const std::string& uri, const std::map<string,string>& query)
    {
        m_impl->connect(uri, query, {});
    }

    void client::connect(const std::string& uri, const std::map<std::string,std::string>& query,
                         const std::map<std::string,std::string>& http_extra_headers)
    {
        m_impl->connect(uri, query, http_extra_headers);
    }
    
    socket::ptr const& client::socket(const std::string& nsp)
    {
        return m_impl->socket(nsp);
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
    
    bool client::opened() const
    {
        return m_impl->opened();
    }
    
    std::string const& client::get_sessionid() const
    {
        return m_impl->get_sessionid();
    }

    void client::set_reconnect_attempts(int attempts)
    {
        m_impl->set_reconnect_attempts(attempts);
    }

    void client::set_reconnect_delay(unsigned millis)
    {
        m_impl->set_reconnect_delay(millis);
    }

    void client::set_reconnect_delay_max(unsigned millis)
    {
        m_impl->set_reconnect_delay_max(millis);
    }
    
}
