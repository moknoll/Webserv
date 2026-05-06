#pragma once
#include <vector>
#include <map>
#include <poll.h>
#include "socket.hpp"
#include "client.hpp"
#include "../ConfigParser/ServerConfig.hpp"


class Server{
	private: 
		Socket socket;
		//HTTPhandler h;

	public:  
		Server(const ServerConfig &config);
		Server();
		~Server();
		//Server operator=();
		
		void setSocket(); 
		Socket getSocket()const;
};
