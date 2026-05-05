#include "server.hpp"
#include "socket.hpp"


Socket Server::getSocket()const
{
	return this->socket;
}