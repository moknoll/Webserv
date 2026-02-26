/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:48 by mknoll            #+#    #+#             */
/*   Updated: 2026/02/26 14:52:59 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Server::Server(int port) : _port(port), _serverfd(-1) {}

Server::~Server()
{
	if(_serverfd != -1)
		close(_serverfd);
}

void Server::init()
{
	int yes = 1;

	// 1. Create socket
	if((_serverfd = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
		throw std::runtime_error("Socket Creation failed");
	
	// 2. Set socket opt to reset
	if((setsockopt(_serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == SOCKET_ERROR)
		throw std::runtime_error("Setsockopt failed");
	
	// 3. Set serverFD to non block
	if((fcntl(_serverfd, F_SETFL, O_NONBLOCK)) == SOCKET_ERROR)
		throw std::runtime_error("Fcntl failed");
	
	// 4. Bind server adress to socket
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET; // Set IPv4
	serverAddr.sin_port = htons(_port); // set port
	serverAddr.sin_addr.s_addr = INADDR_ANY; // allow in from any IP
	bzero(&(serverAddr.sin_zero), 8);
	
	if ((bind(_serverfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) == SOCKET_ERROR)
		throw std::runtime_error("Bind failed");
	
	// 5. Listen 
	if((listen(_serverfd, BACKLOG)) == SOCKET_ERROR)	// backlog = number of connections allowed on queue
		throw std::runtime_error("listen failed");
	
	// 6. Set server FD to pull vector 
	struct pollfd pfd;
	pfd.fd = _serverfd;
	pfd.events = POLLIN;
	_pollfds.push_back(pfd);
	
	std::cout << "Server listening on port " << _port << std::endl;
}

void Server::run()
{
	while(1)
	{
		int ret = poll(_pollfds.data(), _pollfds.size(), -1);
		
		if(ret == -1)
			throw std::runtime_error("Poll failed");
		
		for (size_t i = 0; i < _pollfds.size(); i++)
		{
			// Case A: Read(POLLIN)
			if(_pollfds[i].revents & POLLIN)
			{
				if(_pollfds[i].fd == _serverfd)
					_acceptNewClient();
				else 
					_handleClientMessage(_pollfds[i].fd);
			}
			// Case B: Write(POLLOUT)
			// Also check if client still existst
			else if(_pollfds[i].revents & POLLOUT)
				_sendResponseToClient(_pollfds[i].fd);
		}
	}	
}

void Server::_acceptNewClient()
{
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int newClientFd = accept(_serverfd,(struct sockaddr *)&clientAddr, &addrLen);
	
	if(newClientFd == SOCKET_ERROR)
		return;
	
	// Set to non-blocking
	fcntl(newClientFd, F_SETFL, O_NONBLOCK);

	// 1. push into Vector 
	struct pollfd pfd;
	pfd.fd = newClientFd;
	pfd.events = POLLIN;
	_pollfds.push_back(pfd);
	
	// 2. Create new client object and safe into map 
	_clients.insert(std::make_pair(newClientFd, Client(newClientFd)));
	
	std::cout << "New Client connected: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
}

void Server::_handleClientMessage(int fd)
{
	char buffer[1024];
	int bytes = recv(fd, buffer, sizeof(buffer), 0);
	
	if (bytes <= 0)
		_cleanupClient(fd);
	else
	{
		// put data into clientbuffer
		_clients.at(fd).requestBuffer.append(buffer, bytes);
		std::cout << "Received: " << _clients.at(fd).requestBuffer << std::endl;

		// Plceholder fo Request complete (\r\b\r\b) 
		// this is up to parsing

		// Here we would parse the request and generate a response, for now we just send a static message back
		_clients.at(fd).responseBuffer = "Hello from Class-Server!\n";

		// Change Poll event to writing
		for(size_t i = 0; i < _pollfds.size(); i++)
		{
			if(_pollfds[i].fd == fd)
			{
				_pollfds[i].events = POLLIN | POLLOUT;
				break;
			}
		}
	}
}

void Server::_sendResponseToClient(int fd)
{
	// check that clients exists
	if (_clients.find(fd) == _clients.end())
		return;
	
	std::string &msg = _clients.at(fd).responseBuffer;
		
	if(msg.empty())
		return;

	int bytesSent = send(fd, msg.c_str(), msg.size(), 0);
	
	if(bytesSent >= 0)
	{
		msg.clear();
		_clients.at(fd).requestBuffer.clear(); // Empty for new request
		
		// reset Poll event : To reading
		for(size_t i = 0; i < _pollfds.size(); i++)
		{
			if(_pollfds[i].fd == fd)
			{
				_pollfds[i].events = POLLIN;
				break;
			}
		}
	}
}

void Server::_cleanupClient(int fd)
{
	// Close socket
	close(fd);
	
	// delete from poll vector
	for (size_t i = 0; i < _pollfds.size(); i++)
	{
		if(_pollfds[i].fd == fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}
	// delete from client map
	_clients.erase(fd);
	
	std::cout << "Client disconnected" << std::endl;
}

