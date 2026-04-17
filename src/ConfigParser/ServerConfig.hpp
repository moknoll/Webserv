#pragma once

#include <string>

class ServerConfig {
	public:
		int port;
		std::string root;
		std::string index;
		std::string host;
		size_t 		client_max_body_size;

		ServerConfig();
		int _getPort();
};
