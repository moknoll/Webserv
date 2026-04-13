#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>

class ServerConfig {
	public:
		int port;
		std::string root;
		std::string index;
		std::string host;
		size_t 		client_max_body_size;

		ServerConfig();
};

#endif