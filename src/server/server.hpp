#pragma once
#include "socket.hpp"
#include "../ConfigParser/ServerConfig.hpp"

class Server{
	private: 
		Socket socket;
		//HTTPhandler h;

	public:  
		Server(const ServerConfig &config);
		Server();
		//Server operator=();
		
		//setSocket()?
		Socket getSocket()const;
};
