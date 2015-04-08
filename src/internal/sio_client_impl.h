#ifndef SIO_CLIENT_IMPL_H
#define SIO_CLIENT_IMPL_H

#ifdef WIN32
#define _WEBSOCKETPP_CPP11_THREAD_
#define BOOST_ALL_NO_LIB
//#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_NO_CPP11_FUNCTIONAL_
#define INTIALIZER(__TYPE__)
#else
#define _WEBSOCKETPP_CPP11_STL_ 1
#define INTIALIZER(__TYPE__) (__TYPE__)
#endif
#include <websocketpp/client.hpp>
#if _DEBUG || DEBUG
#include <websocketpp/config/debug_asio_no_tls.hpp>
typedef websocketpp::config::debug_asio client_config;
#else
#include <websocketpp/config/asio_no_tls_client.hpp>
typedef websocketpp::config::asio_client client_config;
#endif
#include <boost/asio/deadline_timer.hpp>

#include <memory>
#include <map>
#include <thread>
#include "../sio_client.h"
#include "sio_packet.h"

namespace sio
{
    using namespace websocketpp;
    
    typedef websocketpp::client<client_config> client_type;
    
    class client_impl {
        
    protected:
        enum con_state
        {
            con_opening,
            con_opened,
            con_closing,
            con_closed
        };
        
        client_impl();
        
        ~client_impl();
        
        //set listeners and event bindings.
#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
void set_##__FIELD__(__TYPE__ const& l) \
{ m_##__FIELD__ = l;}
        
        SYNTHESIS_SETTER(client::con_listener,open_listener)
        
        SYNTHESIS_SETTER(client::con_listener,fail_listener)
        
        SYNTHESIS_SETTER(client::close_listener,close_listener)
        
        SYNTHESIS_SETTER(client::socket_listener,socket_open_listener)
        
        SYNTHESIS_SETTER(client::socket_listener,socket_close_listener)
        
#undef SYNTHESIS_SETTER
        
        
        void clear_con_listeners()
        {
            m_open_listener = nullptr;
            m_close_listener = nullptr;
            m_fail_listener = nullptr;
        }
        
        void clear_socket_listeners()
        {
            m_socket_open_listener = nullptr;
            m_socket_close_listener = nullptr;
        }
        
        // Client Functions - such as send, etc.
        void connect(const std::string& uri);
        
        void reconnect(const std::string& uri);
        
        socket::ptr const& socket(const std::string& nsp);
        
        // Closes the connection
        void close();
        
        void sync_close();
        
        bool opened() const { return m_con_state == con_opened; }
        
        std::string const& get_sessionid() const { return m_sid; }
        
        friend class client;
    protected:
        void send(packet& p);
        
        void remove_socket(std::string const& nsp);
        
        boost::asio::io_service& get_io_service();
        
        void on_socket_closed(std::string const& nsp);
        
        void on_socket_opened(std::string const& nsp);
        
    private:
        void __close(close::status::value const& code,std::string const& reason);
        
        void __connect(const std::string& uri);
        
        void __send(std::shared_ptr<const std::string> const&  payload_ptr,frame::opcode::value opcode);
        
        void __ping(const boost::system::error_code& ec);
        
        void __timeout_pong(const boost::system::error_code& ec);
        
        void __timeout_connection(const boost::system::error_code& ec);
        
        socket::ptr get_socket_locked(std::string const& nsp);
        
        void sockets_invoke_void(void (sio::socket::*fn)(void));
        
        void run_loop();
        
        void on_decode(packet const& pack);
        void on_encode(bool isBinary,shared_ptr<const string> const& payload);
        
        // Callbacks
        void on_fail(connection_hdl con);
        void on_open(connection_hdl con);
        void on_close(connection_hdl con);
        void on_message(connection_hdl con, client_type::message_ptr msg);
        void on_handshake(message::ptr const& message);
        
        void on_pong();
        
        void on_pong_timeout();
        
        void reset_states();
        
        void clear_timers();
        
        // Connection pointer for client functions.
        connection_hdl m_con;
        client_type m_client;
        // Socket.IO server settings
        std::string m_sid;
        unsigned int m_ping_interval;
        unsigned int m_ping_timeout;
        
        std::unique_ptr<std::thread> m_network_thread;
        
        packet_manager m_packet_mgr;
        
        std::unique_ptr<boost::asio::deadline_timer> m_ping_timer;
        
        std::unique_ptr<boost::asio::deadline_timer> m_ping_timeout_timer;
        
        con_state m_con_state;
        
        client::con_listener m_open_listener;
        client::con_listener m_fail_listener;
        client::close_listener m_close_listener;
        
        client::socket_listener m_socket_open_listener;
        client::socket_listener m_socket_close_listener;
        
        std::map<const std::string,socket::ptr> m_sockets;
        
        std::mutex m_socket_mutex;
        
        friend class sio::client;
        friend class sio::socket;
    };
}
#endif // SIO_CLIENT_IMPL_H

