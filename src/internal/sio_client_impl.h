#pragma once

#include <cstdint>
#ifdef _WIN32
#define _WEBSOCKETPP_CPP11_THREAD_
//#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_NO_CPP11_FUNCTIONAL_
#else
#define _WEBSOCKETPP_CPP11_STL_ 1
#endif
#include <websocketpp/client.hpp>

#if defined(_DEBUG) || defined(DEBUG)
#include <websocketpp/config/debug.hpp>
#else
#include <websocketpp/config/core_client.hpp>
#endif

#include <websocketpp/transport/asio/endpoint.hpp>
#ifdef HAVE_OPENSSL
#include <websocketpp/transport/asio/security/tls.hpp>
#include <asio/ssl/context.hpp>
#endif

#include <asio/steady_timer.hpp>
#include <asio/error_code.hpp>
#include <asio/io_service.hpp>

#include <memory>
#include <map>
#include <thread>
#include <chrono>

#include "../sio_client.h"
#include "sio_packet.h"
#include "sio_client_config.h"

// Comment this out to disable handshake logging to stdout
#if defined(_DEBUG) || defined(DEBUG)
#define LOG(x) std::cout << x
#else
#define LOG(x) do {} while (0)
#endif

namespace sio
{
#if defined(_DEBUG) || defined(DEBUG)
#ifdef HAVE_OPENSSL
    using tls_client_config = client_config<
            websocketpp::config::debug_core,
            websocketpp::transport::asio::tls_socket::endpoint>;
#endif
    using non_tls_client_config = client_config<
            websocketpp::config::debug_core,
            websocketpp::transport::asio::basic_socket::endpoint>;
#else
#ifdef HAVE_OPENSSL
    using tls_client_config = client_config<
            websocketpp::config::core_client,
            websocketpp::transport::asio::tls_socket::endpoint>;
#endif
    using non_tls_client_config = client_config<
            websocketpp::config::core_client,
            websocketpp::transport::asio::basic_socket::endpoint>;
#endif

    using std::chrono::milliseconds;
    using namespace websocketpp;
    class client_impl_base {
    public:
        virtual ~client_impl_base() = default;

    protected:
        client_impl_base();
        client_impl_base(const client_impl_base& other);

        enum con_state
        {
            con_opening,
            con_opened,
            con_closing,
            con_closed
        };

    public:
        // set listeners
#define SYNTHESIS_SETTER(__TYPE__,__FIELD__) \
        inline void set_##__FIELD__(__TYPE__ const& l) \
        { m_##__FIELD__ = l;}

        SYNTHESIS_SETTER(client::con_listener,open_listener)

        SYNTHESIS_SETTER(client::con_listener,fail_listener)

        SYNTHESIS_SETTER(client::reconnect_listener,reconnect_listener)

        SYNTHESIS_SETTER(client::con_listener,reconnecting_listener)

        SYNTHESIS_SETTER(client::close_listener,close_listener)

        SYNTHESIS_SETTER(client::socket_listener,socket_open_listener)

        SYNTHESIS_SETTER(client::socket_listener,socket_close_listener)

#undef SYNTHESIS_SETTER

        // set reconnection config
        inline void set_reconnect_attempts(unsigned attempts)
        {
            m_reconn_attempts = attempts;
        }

        void set_reconnect_delay(unsigned millis);

        void set_reconnect_delay_max(unsigned millis);

        // cleanups
        void clear_con_listeners();
        void clear_socket_listeners();

        // opens connection
        void connect(const std::string& uri, const std::map<std::string, std::string>& queryString, const std::map<std::string, std::string>& httpExtraHeaders);

        sio::socket::ptr const& socket(const std::string& nsp);

        inline bool opened() const { return m_con_state == con_opened; }

        inline std::string const& get_sessionid() const { return m_sid; }

        // Closes the connection
        void close();
        void sync_close();

    protected:
        // pure virtual
        virtual asio::io_service& get_io_service() = 0;
        virtual void run_loop() = 0;
        virtual void connect_impl(const std::string& uri, const std::string& query) = 0;
        virtual void close_impl(websocketpp::close::status::value const& code,std::string const& reason) = 0;
        virtual void send_impl(std::shared_ptr<const std::string> const&  payload_ptr,websocketpp::frame::opcode::value opcode) = 0;

        virtual void ping(const asio::error_code& ec) = 0;

        virtual void reset_states() = 0;

        // client methods
        void send(packet& p);

        void remove_socket(std::string const& nsp);

        void sockets_invoke_void(void (sio::socket::*fn)(void));

        void socket_invoke_disconnected();
        void socket_invoke_closed();

        void sync_close_internal(asio::io_service&);

        socket::ptr get_socket_locked(std::string const& nsp);

        //websocket callbacks
        void on_fail(connection_hdl con);

        void on_open(connection_hdl con);

        void on_socket_closed(std::string const& nsp);

        void on_socket_opened(std::string const& nsp);

        //socketio callbacks
        void on_handshake(message::ptr const& message);

        void on_pong();

        void on_decode(packet const& pack);
        void on_encode(bool isBinary,shared_ptr<const string> const& payload);

        // timers
        void timeout_pong(const asio::error_code& ec);
        void timeout_reconnect(asio::error_code const& ec);

        void clear_timers();
        unsigned next_delay() const;

        // Percent encode query string
        std::string encode_query_string(const std::string &query);

    protected:
        connection_hdl m_con;
        // Socket.IO server settings
        std::string m_sid;
        std::string m_base_url;
        std::string m_query_string;
        std::map<std::string, std::string> m_http_headers;

        unsigned int m_ping_interval;
        unsigned int m_ping_timeout;

        std::unique_ptr<std::thread> m_network_thread;

        packet_manager m_packet_mgr;

        std::unique_ptr<asio::steady_timer> m_ping_timer;

        std::unique_ptr<asio::steady_timer> m_ping_timeout_timer;

        std::unique_ptr<asio::steady_timer> m_reconn_timer;

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

        friend class socket;
    };

    template<typename CONFIG>
    class client_impl : public client_impl_base {
    private:
        void init_client()
        {
#ifndef DEBUG
            using websocketpp::log::alevel;
            m_client.clear_access_channels(alevel::all);
            m_client.set_access_channels(alevel::connect|alevel::disconnect|alevel::app);
#endif
            // Initialize the Asio transport policy
            m_client.init_asio();

            // Bind the clients we are using
            using std::placeholders::_1;
            using std::placeholders::_2;
            m_client.set_open_handler(std::bind(&client_impl::on_open,this,_1));
            m_client.set_close_handler(std::bind(&client_impl::on_close,this,_1));
            m_client.set_fail_handler(std::bind(&client_impl::on_fail,this,_1));
            m_client.set_message_handler(std::bind(&client_impl::on_message,this,_1,_2));
#ifdef HAVE_OPENSSL
            init_tls_handler(m_client);
#endif
        }

    public:
        using client_type = websocketpp::client<CONFIG>;

        client_impl(const client_impl_base& other)
            : client_impl_base(other)
        {
            init_client();
        }

        client_impl()
            : client_impl_base()
        {
            init_client();
        }

        virtual ~client_impl() override
        {
            socket_invoke_closed();
            sync_close_internal(m_client.get_io_service());
        }

    protected:
        virtual asio::io_service& get_io_service() override
        {
            return m_client.get_io_service();
        }

        virtual void run_loop() override
        {
            m_client.run();
            m_client.reset();
            m_client.get_alog().write(websocketpp::log::alevel::devel,
                                      "run loop end");
        }

        virtual void connect_impl(const std::string& uri, const std::string& queryString) override
        {
            do{
                websocketpp::uri uo(uri);
                ostringstream ss;

                if (m_client.is_secure()) {
                    ss<<"wss://";
                } else {
                    ss<<"ws://";
                }
                const std::string host(uo.get_host());
                // As per RFC2732, literal IPv6 address should be enclosed in "[" and "]".
                if(host.find(':')!=std::string::npos){
                    ss<<"["<<uo.get_host()<<"]";
                } else {
                    ss<<uo.get_host();
                }
                ss<<":"<<uo.get_port()<<"/socket.io/?EIO=3&transport=websocket";
                if(m_sid.size()>0){
                    ss<<"&sid="<<m_sid;
                }
                ss<<"&t="<<time(NULL)<<queryString;
                lib::error_code ec;
                typename client_type::connection_ptr con = m_client.get_connection(ss.str(), ec);
                if (ec) {
                    m_client.get_alog().write(websocketpp::log::alevel::app,
                                              "Get Connection Error: "+ec.message());
                    break;
                }

                for( auto&& header: m_http_headers ) {
                    con->replace_header(header.first, header.second);
                }

                m_client.connect(con);
                return;
            }
            while(0);
            if(m_fail_listener)
            {
                m_fail_listener();
            }
        }

        virtual void close_impl(websocketpp::close::status::value const& code,std::string const& reason) override
        {
            LOG("Close by reason:"<<reason << endl);
            if(m_reconn_timer)
            {
                m_reconn_timer->cancel();
                m_reconn_timer.reset();
            }
            if (m_con.expired())
            {
                cerr << "Error: No active session" << endl;
            }
            else
            {
                websocketpp::lib::error_code ec;
                m_client.close(m_con, code, reason, ec);
            }
        }

        virtual void send_impl(std::shared_ptr<const std::string> const&  payload_ptr,websocketpp::frame::opcode::value opcode) override
        {
            if(m_con_state == con_opened)
            {
                websocketpp::lib::error_code ec;
                m_client.send(m_con,*payload_ptr,opcode,ec);
                if(ec)
                {
                    cerr<<"Send failed,reason:"<< ec.message()<<endl;
                }
            }
        }

        //websocket callbacks
        void on_close(connection_hdl con)
        {
            LOG("Client Disconnected." << endl);
            con_state m_con_state_was = m_con_state;
            m_con_state = con_closed;
            websocketpp::lib::error_code ec;
            auto code = websocketpp::close::status::normal;
            typename client_type::connection_ptr conn_ptr  = m_client.get_con_from_hdl(con, ec);
            if (ec) {
                LOG("OnClose get conn failed"<<ec<<endl);
            }
            else
            {
                code = conn_ptr->get_local_close_code();
            }

            m_con.reset();
            clear_timers();
            client::close_reason reason;

            socket_invoke_disconnected();
            // If we initiated the close, no matter what the close status was,
            // we'll consider it a normal close. (When using TLS, we can
            // sometimes get a TLS Short Read error when closing.)
            if(code == close::status::normal || m_con_state_was == con_closing)
            {
                reason = client::close_reason_normal;
            }
            else
            {
                if(m_reconn_made<m_reconn_attempts)
                {
                    LOG("Reconnect for attempt:"<<m_reconn_made<<endl);
                    unsigned delay = next_delay();
                    if(m_reconnect_listener) m_reconnect_listener(m_reconn_made,delay);
                    m_reconn_timer.reset(new asio::steady_timer(m_client.get_io_service()));
                    asio::error_code ec;
                    m_reconn_timer->expires_from_now(std::chrono::milliseconds(delay), ec);
                    m_reconn_timer->async_wait(std::bind(&client_impl::timeout_reconnect,this, std::placeholders::_1));
                    return;
                }
                reason = client::close_reason_drop;
            }

            if(m_close_listener)
            {
                m_close_listener(reason);
            }
        }

        void on_message(connection_hdl, typename client_type::message_ptr msg)
        {
            if (m_ping_timeout_timer) {
                asio::error_code ec;
                m_ping_timeout_timer->expires_from_now(std::chrono::milliseconds(m_ping_timeout),ec);
                m_ping_timeout_timer->async_wait(std::bind(&client_impl::timeout_pong, this, std::placeholders::_1));
            }
            // Parse the incoming message according to socket.IO rules
            m_packet_mgr.put_payload(msg->get_payload());
        }

        virtual void ping(const asio::error_code& ec) override
        {
            if(ec || m_con.expired())
            {
                if (ec != asio::error::operation_aborted)
                    LOG("ping exit,con is expired?"<<m_con.expired()<<",ec:"<<ec.message()<<endl);
                return;
            }
            packet p(packet::frame_ping);
            m_packet_mgr.encode(p, [&](bool /*isBin*/,shared_ptr<const string> payload)
            {
                lib::error_code ec;
                m_client.send(m_con, *payload, frame::opcode::text, ec);
            });
            if(m_ping_timer)
            {
                asio::error_code e_code;
                m_ping_timer->expires_from_now(milliseconds(m_ping_interval), e_code);
                m_ping_timer->async_wait(std::bind(&client_impl::ping,this, std::placeholders::_1));
            }
            if(!m_ping_timeout_timer)
            {
                m_ping_timeout_timer.reset(new asio::steady_timer(m_client.get_io_service()));
                std::error_code timeout_ec;
                m_ping_timeout_timer->expires_from_now(milliseconds(m_ping_timeout), timeout_ec);
                m_ping_timeout_timer->async_wait(std::bind(&client_impl::timeout_pong, this, std::placeholders::_1));
            }
        }

        virtual void reset_states() override
        {
            m_client.reset();
            m_sid.clear();
            m_packet_mgr.reset();
        }

#ifdef HAVE_OPENSSL
        typedef websocketpp::lib::shared_ptr<asio::ssl::context> context_ptr;

        context_ptr on_tls_init(connection_hdl)
        {
            context_ptr ctx = context_ptr(new  asio::ssl::context(asio::ssl::context::tlsv12));
            asio::error_code ec;
            ctx->set_options(asio::ssl::context::default_workarounds |
                                 asio::ssl::context::no_sslv2 |
                                 asio::ssl::context::single_dh_use,ec);
            if(ec)
            {
                cerr<<"Init tls failed,reason:"<< ec.message()<<endl;
            }

            return ctx;
        }

        void init_tls_handler(websocketpp::client<tls_client_config>& client)
        {
            using std::placeholders::_1;
            client.set_tls_init_handler(
                        std::bind(&client_impl::on_tls_init,this,_1));
        }

        void init_tls_handler(websocketpp::client<non_tls_client_config>&)
        {
            // do nothing
        }
#endif

    private:
        client_type m_client;
    };
}
