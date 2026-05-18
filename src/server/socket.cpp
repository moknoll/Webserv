#include "socket.hpp"
#include "sockets.hpp"
#include <string>
#include <sstream>

Socket::Socket() : _sockFd(-1) {}


static std::string to_string(int num)
{
	std::string str; 
	std::stringstream ss; 

	ss << num; 

	str = ss.str(); 
	return str; 
}

Socket::Socket(const ServerConfig &config)
{
	int yes = 1;
	int status; 
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	this->_host = config.host;
	this->_port = to_string(config.port);
	std::cout << "Config: host='" << this->_host << "' port='" << this->_port << "'" << std::endl;

	if ((status = getaddrinfo(this->_host.c_str(), this->_port.c_str(), &hints, &servinfo)) != 0)
		throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(status));
	this->_sockFd = -1; 

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if((this->_sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)))
			if (this->_sockFd == SOCKET_ERROR)
				continue;
		if ((setsockopt(this->_sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == SOCKET_ERROR)
		{
			close(this->_sockFd); 
			continue;
		}
		if ((fcntl(this->_sockFd, F_SETFL, O_NONBLOCK)) == SOCKET_ERROR)
		{
			close(this->_sockFd); 
			continue;
		}
		if ((bind(this->_sockFd, p->ai_addr, p->ai_addrlen)) == SOCKET_ERROR)
		{
			close(this->_sockFd); 
			continue;
		}
		break;  
		std::cout << "sock_fd" << this->_sockFd << std::endl;
		std::cout << "Setup complete" << std::endl; 
	}
	if (p == NULL)
		throw std::runtime_error("Could not bind to" + _host + ":" + _port); 
		
	freeaddrinfo(servinfo);
	if (listen(this->_sockFd, BACKLOG) == SOCKET_ERROR)
		throw std::runtime_error("listen failed"); 
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
	return atoi((this->_port).c_str());
}

int Socket::getFd()const
{
	return this->_sockFd;
}

Socket::~Socket() {}
