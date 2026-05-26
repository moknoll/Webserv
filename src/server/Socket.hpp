#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include <string>

#define SOCKET_ERROR -1
#define BACKLOG      10 // How may pending connections queue will hold

class Socket
{
  private:
	int         fd_;
	std::string port_;
	std::string host_;

	Socket();
	Socket& operator=(const Socket& other);
	Socket(const Socket& other);

  public:
	Socket(const ServerConfig& config);
	~Socket();

	int         getFd() const;
	int         getPort() const;
	std::string getHost() const;
};
