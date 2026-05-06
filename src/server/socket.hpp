#pragma once
#include "../ConfigParser/ServerConfig.hpp"
#include <string>

class Socket{
	private: 
		int _sockFd;
		int _port;
		std::string _host;

	public: 
		Socket();
		Socket(const ServerConfig &config);
		Socket operator=(const Socket &obj);
		~Socket();

		int getFd()const;
		int getPort()const;
		std::string getHost()const;
		void setFD(int sockfd);
		void setPort(int Port);
		void setHost(std::string host); 
};	