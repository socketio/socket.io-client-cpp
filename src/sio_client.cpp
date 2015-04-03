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
    
    void client::clear_con_listeners()
    {
        m_impl->clear_con_listeners();
    }
    
    void client::connect(const std::string& uri)
    {
        m_impl->connect(uri);
    }
    
    void client::reconnect(const std::string& uri)
    {
        m_impl->reconnect(uri);
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
    
}
