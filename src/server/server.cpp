/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:48 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/27 14:16:45 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "../ConfigParser/ServerConfig.hpp"
#include "../logger/Logger.hpp"
#include "client.hpp"
#include <sstream>
// #include "../http/request/HttpHeader.hpp"

Server::Server(std::vector<ServerConfig> configs): _configs(configs){}

Server::~Server()
{
	// for(size_t i = 0; i < _serverfds.size(); i++)
	// {
	// 	if(_serverfds[i] != -1)
	// 		close(_serverfds[i]);
	// }
	//Check later on how to close? 
}

void Server::init()
{
	int yes = 1;

	for (size_t i = 0; i < _configs.size(); i++)
	{
		int sock_fd = -1;		
		// 1. Create socket
		if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
			throw std::runtime_error("Socket Creation failed"); 

		// 2. Set socket opt to reset address/port
		if((setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == SOCKET_ERROR)
			throw std::runtime_error("Setsockopt failed");
	
		// 3. Set serverFD to non block
		if((fcntl(sock_fd, F_SETFL, O_NONBLOCK)) == SOCKET_ERROR)
			throw std::runtime_error("Fcntl failed");
	
		// 4. Bind server adress to socket
		// Mazbe chaneg this to getaddrinfo() -> beej chapter 4
		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET; // Set IPv4
		serverAddr.sin_port = htons(_configs[i].port); // set port
		serverAddr.sin_addr.s_addr = INADDR_ANY; // allow in from any IP
		bzero(&(serverAddr.sin_zero), 8);
	
		if ((bind(sock_fd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) == SOCKET_ERROR)
			throw std::runtime_error("Bind failed");
	
		// 5. Listen 
		if((listen(sock_fd, BACKLOG)) == SOCKET_ERROR)	// backlog = number of connections allowed on queue
			throw std::runtime_error("listen failed");
	
		// 6. Set server FD to pull vector 
		struct pollfd pfd;
		pfd.fd = sock_fd;
		pfd.events = POLLIN;
		_pollSockets.push_back(pfd);

		// 7. Save server fd and config in map
		_serverConfigsByFd[sock_fd] = _configs[i];
		
		// Debug: Log the server config mapping
		std::ostringstream oss;
		oss << "Server Socket FD: " << sock_fd 
			<< " | Port: " << _configs[i].port 
			<< " | Host: " << _configs[i].host 
			<< " | Root: " << _configs[i].root 
			<< " | Index: " << _configs[i].index
			<< " | Max Body Size: " << _configs[i].client_max_body_size;
		LOG_DEBUG(oss.str());
		
	}
}

void Server::run()
{
	while(1) 
	{
		int ret = poll(_pollSockets.data(), _pollSockets.size(), -1);
		
		if(ret == -1)
			throw std::runtime_error("Poll failed");
		
		for (size_t i = 0; i < _pollSockets.size(); i++)
		{
			// Case A: Read(POLLIN)
			if(_pollSockets[i].revents & POLLIN)
			{
				if(_serverConfigsByFd.count(_pollSockets[i].fd))
					_acceptNewClient(_pollSockets[i].fd);
				else 
					_handleClientMessage(_pollSockets[i].fd);
			}
			// Case B: Write(POLLOUT)
			// Also check if client still existst
			else if(_pollSockets[i].revents & POLLOUT)
				_sendResponseToClient(_pollSockets[i].fd);
		}
	}	
}

bool Server::_isCompleteRequest(std::string &request)
{
	(void)request;
	// size_t headerEnd = request.find("\r\n\r\n");
	// if(headerEnd == std::string::npos)
	// 	return false;
	
	// // For GET check content length
	// size_t contentLength = request.find("Content-Length:");
	// if(contentLength == std::string::npos)
	// 	return true;
	
	// // For POST
	
	// // For Delete 
	return true;
}

void Server::_acceptNewClient(int serverSocketFd)
{
	struct sockaddr_in clientAddr;
	// 1. Get the ServerConfig for this server socket from the map
	ServerConfig* config = &_serverConfigsByFd[serverSocketFd];
	socklen_t addrLen = sizeof(clientAddr);
	int newClientFd = accept(serverSocketFd,(struct sockaddr *)&clientAddr, &addrLen);
	
	if(newClientFd == SOCKET_ERROR)
		return;
	
	// Debug: Show which config was assigned to the new client
	std::ostringstream oss;
	oss << "New Client FD: " << newClientFd 
		<< " | Accepted from Server Socket FD: " << serverSocketFd 
		<< " | Assigned Config Port: " << config->port 
		<< " | Config Host: " << config->host 
		<< " | Config Root: " << config->root;
	LOG_DEBUG(oss.str());
	
	// Set to non-blocking
	fcntl(newClientFd, F_SETFL, O_NONBLOCK);

	// 2. push into Vector 
	struct pollfd pfd;
	pfd.fd = newClientFd;
	pfd.events = POLLIN;
	_pollSockets.push_back(pfd);
	
	// 3. Create new client object and safe into map 
	_clients.insert(std::make_pair(newClientFd, Client(newClientFd, config)));
	
	// std::cout << "New Client connected: " <<  << std::endl;
	//LOG_DEBUG( "New client connected");
}

void Server::_handleClientMessage(int clientSocketFd)
{
	char buffer[8192];
	int bytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);
	
	if (bytes <= 0)
		_cleanupClient(clientSocketFd);
	else
	{
		// put data into clientbuffer
		_clients.at(clientSocketFd).requestBuffer.append(buffer, bytes);
		// std::cout << "Received: " << _clients.at(clientSocketFd).requestBuffer << std::endl;
		Logger::getInstance().log(INFO, "Received", _clients.at(clientSocketFd).requestBuffer);
		// HttpHeader	buffer(_clients.at(fd).requestBuffer);
		if(_isCompleteRequest(_clients.at(clientSocketFd).requestBuffer))
		{	
			// Plceholder fo Request complete (\r\b\r\b) 
			std::string response = "Hello from Server"; 
        
       		_clients.at(clientSocketFd).responseBuffer = response;

			// Change Poll event to writing
			for(size_t i = 0; i < _pollSockets.size(); i++)
			{
				if(_pollSockets[i].fd == clientSocketFd)
					{
						_pollSockets[i].events = POLLIN | POLLOUT;
						break;
					}
			}
		}
	}
}

void Server::_sendResponseToClient(int clientSocketFd)
{
	// check that clients exists
	if (_clients.find(clientSocketFd) == _clients.end())
		return;
	
	std::string &msg = _clients.at(clientSocketFd).responseBuffer;
		
	if(msg.empty())
		return;

	int bytesSent = send(clientSocketFd, msg.c_str(), msg.size(), 0);
	
	if(bytesSent >= 0)
	{
		msg.clear();
		_clients.at(clientSocketFd).requestBuffer.clear(); // Empty for new request
		std::cout << "Response send" << std::endl; 
		// reset Poll event : To reading
		for(size_t i = 0; i < _pollSockets.size(); i++)
		{
			if(_pollSockets[i].fd == clientSocketFd)
			{
				_pollSockets[i].events = POLLIN;
				break;
			}
		}
	}
}

void Server::_cleanupClient(int clientSocketFd)
{
	// Close socket
	close(clientSocketFd);
	
	// delete from poll vector
	for (size_t i = 0; i < _pollSockets.size(); i++)
	{
		if(_pollSockets[i].fd == clientSocketFd)
		{
			_pollSockets.erase(_pollSockets.begin() + i);
			break;
		}
	}
	// delete from client map
	_clients.erase(clientSocketFd);
	
	// std::cout << "Client disconnected" << std::endl;
	LOG_DEBUG("Client disconnected");
	// Logger::log(DEBUG, "client disconnected");
}
