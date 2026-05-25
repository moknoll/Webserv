#pragma once

#include "../ConfigParser/ServerConfig.hpp"

#include "socket.hpp"
#include <poll.h>

class Server
{
  private:
	Socket       socket_;
	ServerConfig config_;

	Server();
	Server(const Server& other);
	Server& operator=(const Server& other);

  public:
	Server(const ServerConfig& config);
	~Server();

	const Socket&       getSocket() const;
	int                 getSocketFd() const;
	const ServerConfig& getConfig() const;
};
