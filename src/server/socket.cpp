#include "socket.hpp"

Socket::Socket() : _sockFd(-1) {}

Socket::Socket(const ServerConfig &config)
{
	// Initialize socket based on config
	// This is a placeholder, actual implementation will involve creating
	// a socket, binding it to the specified port, and setting it to listen
	this->_port = config.port;
	this->_sockFd = -1; // Placeholder for actual socket file descriptor
	this->_host = config.host;
}

Socket Socket::operator=(const Socket &obj)
{
	if (this != &obj)
	{
		this->_sockFd = obj._sockFd;
		this->_port = obj._port;
		this->_host = obj._host;
	}
	return *this;
}

std::string Socket::getHost() const
{
	return this->_host;
}

int Socket::getPort()const
{
	return this->_port;
}

int Socket::getFd()const
{
	return this->_sockFd;
}


void Socket::setFD(int sockFD)
{
	this->_sockFd = sockFD;
}