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

#include <memory>
#include <map>
#include <queue>
#include <thread>
#include <sio_socket.h>


namespace sio
{
    typedef websocketpp::client<client_config> client_type;

    class client::impl {
    protected:
        impl();

        ~impl();

        //set listeners and event bindings.
#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
void set_##__FIELD__(__TYPE__ const& l) \
{ m_##__FIELD__ = l;}

        SYNTHESIS_SETTER(con_listener,open_listener)

        SYNTHESIS_SETTER(con_listener,fail_listener)

        SYNTHESIS_SETTER(close_listener,close_listener)

        SYNTHESIS_SETTER(event_listener, default_event_listener)
#undef SYNTHESIS_SETTER

        void clear_socketio_listeners()
        {
            m_default_event_listener = nullptr;
        }

        void clear_con_listeners()
        {
            m_open_listener = nullptr;
            m_close_listener = nullptr;
            m_fail_listener = nullptr;
        }

        // Client Functions - such as send, etc.

        std::shared_ptr<sio::socket> socket(const std::string& namespace);

        void connect(const std::string& uri);

        void reconnect(const std::string& uri);

        // Closes the connection
        void close();

        void sync_close();

        bool connected() const { return m_connected; }

        std::string const& get_sessionid() const { return m_sid; }

        std::string const& get_namespace() const { return m_nsp; }

        friend class client;
    protected:
        void send(packet const& packet);

    private:
        void send(std::shared_ptr<const std::string> const& payload_ptr,frame::opcode::value opcode);

        void __close(close::status::value const& code,std::string const& reason);

        void __connect(const std::string& uri);

        void __send(std::shared_ptr<const std::string> const&  payload_ptr,frame::opcode::value opcode);


        void __ping(const boost::system::error_code& ec);

        void __timeout_pong(const boost::system::error_code& ec);

        void __timeout_connection(const boost::system::error_code& ec);

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

        // Message Parsing callbacks.
        void on_socketio_event(const std::string& nsp, int msgId,const std::string& name, message::ptr const& message);
        void on_socketio_ack(int msgId, message::ptr const& message);

        void on_socketio_error(message::ptr const& err_message);

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

        struct message_queue_element
        {
            frame::opcode::value opcode;
            std::shared_ptr<const std::string> payload_ptr;
        };

        packet_manager m_packet_mgr;

        std::queue<message_queue_element> m_message_queue;

        std::unique_ptr<boost::asio::deadline_timer> m_ping_timer;

        std::unique_ptr<boost::asio::deadline_timer> m_ping_timeout_timer;


        con_listener m_open_listener;
        con_listener m_fail_listener;
        close_listener m_close_listener;

        event_listener m_default_event_listener;
        friend class sio::socket;
    };
}
#endif // SIO_CLIENT_IMPL_H

