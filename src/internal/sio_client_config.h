#pragma once

#include <websocketpp/transport/asio/endpoint.hpp>

namespace sio
{
template <typename INHERIT, typename SOCKET_TYPE>
struct client_config : public INHERIT
{
	using type = client_config;
	using base = INHERIT;

	using concurrency_type = typename base::concurrency_type;

	using request_type = typename base::request_type;
	using response_type = typename base::response_type;

	using message_type = typename base::message_type;
	using con_msg_manager_type = typename base::con_msg_manager_type;
	using endpoint_msg_manager_type = typename base::endpoint_msg_manager_type;

	using alog_type = typename base::alog_type;
	using elog_type = typename base::elog_type;

	using rng_type = typename base::rng_type;

	struct transport_config : public base::transport_config
	{
		using concurrency_type = type::concurrency_type;
		using alog_type = type::alog_type;
		using elog_type = type::elog_type;
		using request_type = type::request_type;
		using response_type = type::response_type;
		using socket_type = SOCKET_TYPE;

		static const long timeout_dns_resolve = 15000;
	};

	using transport_type =
		websocketpp::transport::asio::endpoint<transport_config>;
};
}
