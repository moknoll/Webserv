#include "Socket.hpp"
#include "../constants.hpp"
#include "../lib/ws.hpp"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <string>
#include <unistd.h>

Socket::Socket(const ServerConfig& config) :
        fd_(-1)
{
	int              yes = 1;
	int              status;
	struct addrinfo  hints;
	struct addrinfo *servinfo, *p;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	this->host_ = config.host;
	this->port_ = ws::to_string(config.port);

	if ((status = getaddrinfo(this->host_.c_str(), this->port_.c_str(), &hints, &servinfo)) != 0)
		throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(status));
	
	this->fd_ = -1;

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((this->fd_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol)))
			if (this->fd_ == SOCKET_ERROR)
				continue;
		if ((setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == SOCKET_ERROR)
		{
			close(this->fd_);
			continue;
		}
		if ((fcntl(this->fd_, F_SETFL, O_NONBLOCK)) == SOCKET_ERROR)
		{
			close(this->fd_);
			continue;
		}
		if ((bind(this->fd_, p->ai_addr, p->ai_addrlen)) == SOCKET_ERROR)
		{
			close(this->fd_);
			continue;
		}
		break;
	}

	if (p == NULL)
		throw std::runtime_error("Could not bind to" + host_ + ":" + port_);

	freeaddrinfo(servinfo);
	if (listen(this->fd_, BACKLOG) == SOCKET_ERROR)
		throw std::runtime_error("listen failed");
}

Socket::~Socket()
{
	close(fd_);
}

std::string Socket::getHost() const
{
	return this->host_;
}

int Socket::getPort() const
{
	return std::atoi((this->port_).c_str());
}

int Socket::getFd() const
{
	return this->fd_;
}

