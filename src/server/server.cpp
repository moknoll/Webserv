#include "server.hpp"
#include "socket.hpp"

Server::Server(){}

Server::Server(const ServerConfig &config) : socket(config){}

Server::~Server(){}


Socket Server::getSocket()const
{
	return this->socket;
}