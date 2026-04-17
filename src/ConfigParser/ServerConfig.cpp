#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
{
	port = 0;
	root = "";
	index = "";
	host = "";
	client_max_body_size = 0;
}

int ServerConfig::_getPort()
{
	return this->port;
}
