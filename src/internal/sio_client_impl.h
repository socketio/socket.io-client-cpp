#ifndef SIO_CLIENT_IMPL_H
#define SIO_CLIENT_IMPL_H

#include <cstdint>
#ifdef _WIN32
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
#if SIO_TLS
#include <websocketpp/config/debug_asio.hpp>
typedef websocketpp::config::debug_asio_tls client_config_tls;
#endif //SIO_TLS
#include <websocketpp/config/debug_asio_no_tls.hpp>
typedef websocketpp::config::debug_asio client_config;
#else
#if SIO_TLS
#include <websocketpp/config/asio_client.hpp>
typedef websocketpp::config::asio_tls_client client_config_tls;
#endif //SIO_TLS
#include <websocketpp/config/asio_no_tls_client.hpp>
typedef websocketpp::config::asio_client client_config;
#endif //DEBUG
#include <boost/asio/deadline_timer.hpp>

#include <memory>
#include <map>
#include <thread>
#include "../sio_client.h"
#include "sio_packet.h"

namespace sio
{
    using namespace websocketpp;
    
    typedef websocketpp::client<client_config> client_type_no_tls;
#if SIO_TLS
    typedef websocketpp::client<client_config_tls> client_type_tls;
#endif

    struct client_impl_base {
        enum con_state
        {
            con_opening,
            con_opened,
            con_closing,
            con_closed
        };

        client_impl_base() {}
        virtual ~client_impl_base() {}

        // listeners and event bindings. (see SYNTHESIS_SETTER below)
        virtual void set_open_listener(client::con_listener const&)=0;
        virtual void set_fail_listener(client::con_listener const&)=0;
        virtual void set_reconnect_listener(client::reconnect_listener const&)=0;
        virtual void set_reconnecting_listener(client::con_listener const&)=0;
        virtual void set_close_listener(client::close_listener const&)=0;
        virtual void set_socket_open_listener(client::socket_listener const&)=0;
        virtual void set_socket_close_listener(client::socket_listener const&)=0;

        // used by sio::client
        virtual void clear_con_listeners()=0;
        virtual void clear_socket_listeners()=0;
        virtual void connect(const std::string& uri, const std::map<std::string, std::string>& queryString,
                             const std::map<std::string, std::string>& httpExtraHeaders)=0;
        virtual sio::socket::ptr const& socket(const std::string& nsp)=0;
        virtual void close()=0;
        virtual void sync_close()=0;
        virtual bool opened() const=0;
        virtual std::string const& get_sessionid() const=0;
        virtual void set_reconnect_attempts(unsigned attempts)=0;
        virtual void set_reconnect_delay(unsigned millis)=0;
        virtual void set_reconnect_delay_max(unsigned millis)=0;

        // used by sio::socket
        virtual void send(packet& p)=0;
        virtual void remove_socket(std::string const& nsp)=0;
        virtual boost::asio::io_service& get_io_service()=0;
        virtual void on_socket_closed(std::string const& nsp)=0;
        virtual void on_socket_opened(std::string const& nsp)=0;

        // used for selecting whether or not to use TLS
        static bool is_tls(const std::string& uri);

    protected:
        // Wrap protected member functions of sio::socket because only client_impl_base is friended.
        sio::socket* new_socket(std::string const&);
        void socket_on_message_packet(sio::socket::ptr&, packet const&);
        typedef void (sio::socket::*socket_void_fn)(void);
        inline socket_void_fn socket_on_close() { return &sio::socket::on_close; }
        inline socket_void_fn socket_on_disconnect() { return &sio::socket::on_disconnect; }
        inline socket_void_fn socket_on_open() { return &sio::socket::on_open; }
    };

    template<typename client_type>
    class client_impl: public client_impl_base {
    public:
        typedef typename client_type::message_ptr message_ptr;

        client_impl(const std::string& uri = std::string());
        void template_init(); // template-specific initialization

        ~client_impl();
        
        //set listeners and event bindings.
#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
    void set_##__FIELD__(__TYPE__ const& l) \
        { m_##__FIELD__ = l;}
        
        SYNTHESIS_SETTER(client::con_listener,open_listener)
        
        SYNTHESIS_SETTER(client::con_listener,fail_listener)

        SYNTHESIS_SETTER(client::reconnect_listener,reconnect_listener)

        SYNTHESIS_SETTER(client::con_listener,reconnecting_listener)
        
        SYNTHESIS_SETTER(client::close_listener,close_listener)
        
        SYNTHESIS_SETTER(client::socket_listener,socket_open_listener)
        
        SYNTHESIS_SETTER(client::socket_listener,socket_close_listener)
        
#undef SYNTHESIS_SETTER
        
        
        void clear_con_listeners()
        {
            m_open_listener = nullptr;
            m_close_listener = nullptr;
            m_fail_listener = nullptr;
            m_reconnect_listener = nullptr;
            m_reconnecting_listener = nullptr;
        }
        
        void clear_socket_listeners()
        {
            m_socket_open_listener = nullptr;
            m_socket_close_listener = nullptr;
        }
        
        // Client Functions - such as send, etc.
        void connect(const std::string& uri, const std::map<std::string, std::string>& queryString,
                     const std::map<std::string, std::string>& httpExtraHeaders);
        
        sio::socket::ptr const& socket(const std::string& nsp);
        
        // Closes the connection
        void close();
        
        void sync_close();
        
        bool opened() const { return m_con_state == con_opened; }
        
        std::string const& get_sessionid() const { return m_sid; }

        void set_reconnect_attempts(unsigned attempts) {m_reconn_attempts = attempts;}

        void set_reconnect_delay(unsigned millis) {m_reconn_delay = millis;if(m_reconn_delay_max<millis) m_reconn_delay_max = millis;}

        void set_reconnect_delay_max(unsigned millis) {m_reconn_delay_max = millis;if(m_reconn_delay>millis) m_reconn_delay = millis;}
        
    public:
        void send(packet& p);
        
        void remove_socket(std::string const& nsp);
        
        boost::asio::io_service& get_io_service();
        
        void on_socket_closed(std::string const& nsp);
        
        void on_socket_opened(std::string const& nsp);
        
    private:
        void run_loop();

        void connect_impl(const std::string& uri, const std::string& query);

        void close_impl(close::status::value const& code,std::string const& reason);
        
        void send_impl(std::shared_ptr<const std::string> const&  payload_ptr,frame::opcode::value opcode);
        
        void ping(const boost::system::error_code& ec);
        
        void timeout_pong(const boost::system::error_code& ec);

        void timeout_reconnect(boost::system::error_code const& ec);

        unsigned next_delay() const;

        socket::ptr get_socket_locked(std::string const& nsp);
        
        void sockets_invoke_void(void (sio::socket::*fn)(void));
        
        void on_decode(packet const& pack);
        void on_encode(bool isBinary,shared_ptr<const string> const& payload);
        
        //websocket callbacks
        void on_fail(connection_hdl con);

        void on_open(connection_hdl con);

        void on_close(connection_hdl con);

        void on_message(connection_hdl con, message_ptr msg);

        //socketio callbacks
        void on_handshake(message::ptr const& message);

        void on_pong();

        void reset_states();

        void clear_timers();
        
        // Connection pointer for client functions.
        connection_hdl m_con;
        client_type m_client;
        // Socket.IO server settings
        std::string m_sid;
        std::string m_base_url;
        std::string m_query_string;
        std::map<std::string, std::string> m_http_headers;

        unsigned int m_ping_interval;
        unsigned int m_ping_timeout;
        
        std::unique_ptr<std::thread> m_network_thread;
        
        packet_manager m_packet_mgr;
        
        std::unique_ptr<boost::asio::deadline_timer> m_ping_timer;
        
        std::unique_ptr<boost::asio::deadline_timer> m_ping_timeout_timer;

        std::unique_ptr<boost::asio::deadline_timer> m_reconn_timer;
        
        con_state m_con_state;
        
        client::con_listener m_open_listener;
        client::con_listener m_fail_listener;
        client::con_listener m_reconnecting_listener;
        client::reconnect_listener m_reconnect_listener;
        client::close_listener m_close_listener;
        
        client::socket_listener m_socket_open_listener;
        client::socket_listener m_socket_close_listener;
        
        std::map<const std::string,socket::ptr> m_sockets;
        
        std::mutex m_socket_mutex;

        unsigned m_reconn_delay;

        unsigned m_reconn_delay_max;

        unsigned m_reconn_attempts;

        unsigned m_reconn_made;
        
        friend class sio::client;
        friend class sio::socket;
    };
}
#endif // SIO_CLIENT_IMPL_H

