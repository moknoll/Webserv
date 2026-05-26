#include "Server.hpp"
#include "Socket.hpp"
#include <unistd.h>

Server::Server(const ServerConfig& config) : socket_(config), config_(config) {}

Server::~Server() {}

const Socket& Server::getSocket() const
{
	return this->socket_;
}

int Server::getSocketFd() const
{
	return socket_.getFd();
}

const ServerConfig& Server::getConfig() const
{
	return this->config_;
}

