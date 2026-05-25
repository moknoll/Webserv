#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
{
	port = 0;
	root = "";
	index = "";
	host = "";
	client_max_body_size = 0;
}

ServerConfig::ServerConfig(const ServerConfig& other)
    : port(other.port), host(other.host), server_name(other.server_name),
      root(other.root), index(other.index), autoindex(other.autoindex),
      client_max_body_size(other.client_max_body_size),
      error_pages(other.error_pages), redirect(other.redirect),
      locations(other.locations)
{
}

int ServerConfig::_getPort()
{
	return this->port;
}
