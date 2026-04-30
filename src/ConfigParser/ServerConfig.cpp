#include "ServerConfig.hpp"

// ServerConfig::ServerConfig()
// {
// 	port = 0;
// 	root = "";
// 	index = "";
// 	host = "";
// 	client_max_body_size = 0;
// }

// int ServerConfig::_getPort()
// {
// 	return this->port;
// }

// -------- Location --------

Location::Location() : autoindex(false), client_max_body_size(0)
{
	// Everything else empty by default
}

// -------- ServerConfig --------

ServerConfig::ServerConfig() : port(0), client_max_body_size(0)
{
	// Everything else empty by default
}
