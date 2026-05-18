#pragma once
#include <vector>
#include <map>
#include <poll.h>
#include "socket.hpp"
#include "client.hpp"
#include "../ConfigParser/ServerConfig.hpp"


class Server{
	private: 
		Socket _socket;
		ServerConfig *_config; 

	public:  
		Server();
		Server(const ServerConfig &config);
		~Server();
		 
		Socket get_socket()const { return this->_socket; }
		ServerConfig get_config() { return *this->_config; }
};
