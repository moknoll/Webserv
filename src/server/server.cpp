/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: moritzknoll <moritzknoll@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:48 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/30 10:10:07 by moritzknoll      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "../ConfigParser/ServerConfig.hpp"
#include "../http/HttpHandler.hpp"
// #include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../lib/ws.hpp"
#include "../logger/Logger.hpp"
#include "client.hpp"

#include <iostream>
#include <sstream>
#include <sys/poll.h>
// #include "../http/request/HttpHeader.hpp"

Server::Server(std::vector< ServerConfig > configs) : _configs(configs) {}

Server::~Server()
{
	// for(size_t i = 0; i < _serverfds.size(); i++)
	// {
	// 	if(_serverfds[i] != -1)
	// 		close(_serverfds[i]);
	// }
	// Check later on how to close?
}

/**
 * @brief Initializes the server by setting up listening sockets.
 *
 * @throws std::runtime_error if socket creation, configuration, binding, or
 *         listening fails
 *
 * @see Server::_configs for the list of configurations to initialize
 */
void Server::init()
{
	int yes = 1;
	int status;
	for (size_t i = 0; i < _configs.size(); i++)
	{
		struct addrinfo  hints;
		struct addrinfo *servinfo, *p;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // check this
		std::string host = _configs[i].host;
		std::string port = ws::to_string(_configs[i].port);

		std::cout << "Config " << i << ": host='" << host << "' port='" << port
		          << "'" << std::endl;
		if ((status =
		         getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo))
		    != 0)
			throw std::runtime_error(std::string("getaddrinfo failed: ")
			                         + gai_strerror(status));

		int sock_fd = -1;

		for (p = servinfo; p != NULL; p = p->ai_next)
		{
			if ((sock_fd =
			         socket(p->ai_family, p->ai_socktype, p->ai_protocol)))
				if (sock_fd == SOCKET_ERROR)
					continue;
			if ((setsockopt(
			        sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
			    == SOCKET_ERROR)
			{
				close(sock_fd);
				continue;
			}
			if ((fcntl(sock_fd, F_SETFL, O_NONBLOCK)) == SOCKET_ERROR)
			{
				close(sock_fd);
				continue;
			}
			if ((bind(sock_fd, p->ai_addr, p->ai_addrlen)) == SOCKET_ERROR)
			{
				close(sock_fd);
				continue;
			}
			break;
			std::cout << "Setup complete" << std::endl;
		}

		if (p == NULL)
			throw std::runtime_error("Could not bind to" + host + ":" + port);

		freeaddrinfo(servinfo);

		if (listen(sock_fd, BACKLOG) == SOCKET_ERROR)
			throw std::runtime_error("listen failed");

		struct pollfd pfd;
		pfd.fd = sock_fd;
		pfd.events = POLLIN;
		_pollSockets.push_back(pfd);

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

/**
 * @brief Runs the main event loop for the server.
 *
 * @throws std::runtime_error if poll() fails
 */
void Server::run()
{
	while (1)
	{
		int ret = poll(_pollSockets.data(), _pollSockets.size(), -1);

		if (ret == -1)
			throw std::runtime_error("Poll failed");

		for (size_t i = 0; i < _pollSockets.size(); i++)
		{
			if (_pollSockets[i].revents & POLLIN)
			{
				if (_serverConfigsByFd.count(_pollSockets[i].fd))
					_acceptNewClient(_pollSockets[i].fd);
				else
					_handleClientMessage(_pollSockets[i].fd);
			}
			else if (_pollSockets[i].revents & POLLOUT)
				_sendResponseToClient(_pollSockets[i].fd);
		}
	}
}

/**
 * @brief Checks if the HTTP request in the buffer is complete.
 *
 * Determines whether a received HTTP request has been fully received and is
 * ready for processing. Currently a placeholder that always returns true.
 *
 * @param request Reference to the request string buffer to validate
 * @return true if the request is complete, false otherwise
 */
bool Server::_isCompleteRequest(std::string& request)
{
	(void) request;
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

/**
 * @brief Accepts a new incoming connection on a listening server socket.
 *
 * @param serverSocketFd File descriptor of the listening server socket to
 *                       accept a new client from.
 * @return void
 */
void Server::_acceptNewClient(int serverSocketFd)
{
	struct sockaddr_in clientAddr;
	ServerConfig*      config = &_serverConfigsByFd[serverSocketFd];
	socklen_t          addrLen = sizeof(clientAddr);
	int                newClientFd =
	    accept(serverSocketFd, (struct sockaddr*) &clientAddr, &addrLen);

	if (newClientFd == SOCKET_ERROR)
		return;

	// Debug: Show which config was assigned to the new client
	std::ostringstream oss;
	oss << "New Client FD: " << newClientFd
	    << " | Accepted from Server Socket FD: " << serverSocketFd
	    << " | Assigned Config Port: " << config->port
	    << " | Config Host: " << config->host
	    << " | Config Root: " << config->root;
	// LOG_DEBUG(oss.str());

	fcntl(newClientFd, F_SETFL, O_NONBLOCK);

	struct pollfd pfd;
	pfd.fd = newClientFd;
	pfd.events = POLLIN;
	_pollSockets.push_back(pfd);

	_clients.insert(std::make_pair(newClientFd, Client(newClientFd, config)));

	// std::cout << "New Client connected: " <<  << std::endl;
	// LOG_DEBUG( "New client connected");
}

void Server::set_event(int fd, int state)
{
	for (size_t i = 0; i < _pollSockets.size(); i++)
	{
		if (_pollSockets[i].fd == fd)
		{
			_pollSockets[i].events = state;
			break;
		}
	}
}

/**
 * @brief Reads incoming data from a client socket and prepares a response.
 *
 * @param clientSocketFd File descriptor of the connected client socket
 * @return void
 */
void Server::_handleClientMessage(int clientSocketFd)
{
	char buffer[8192];
	int  bytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);

	if (bytes <= 0)
		_cleanupClient(clientSocketFd);
	else
	{
		_clients.at(clientSocketFd).requestBuffer.append(buffer, bytes);

		// Logger::getInstance().log(
		// INFO, "Received", _clients.at(clientSocketFd).requestBuffer);

		if (_isCompleteRequest(_clients.at(clientSocketFd).requestBuffer))
		{
			// std::string response = "Hello from Server";
			std::string request_buf = _clients.at(clientSocketFd).requestBuffer;
			HttpRequest req;
			req.parse(request_buf);
			std::cout << '\n'<< req.getRequestStatus() << '\n';
			std::cout << '\n'<< req.getURI() << '\n';
			HttpHandler handle(_configs[0]);
			HttpResponse resp = handle.handle(req);

			_clients.at(clientSocketFd).responseBuffer = resp.buildResponse();

			// Change Poll event to writing
			// set_event(clientSocketFd, POLLIN | POLLOUT);
			set_event(clientSocketFd, POLLOUT);
		}
	}
}

/**
 * @brief Sends the prepared response to a connected client and resets client
 * state.
 *
 * Notes:
 * - Currently treats any non-negative send() result as success and does not
 *   handle partial writes or EAGAIN/EWOULDBLOCK retries.
 * - No error logging is performed on send() failure; callers should ensure
 *   the client is cleaned up elsewhere if needed.
 *
 * @param clientSocketFd File descriptor of the connected client socket.
 */
void Server::_sendResponseToClient(int clientSocketFd)
{
	// check that clients exists
	if (_clients.find(clientSocketFd) == _clients.end())
		return;

	std::string& msg = _clients.at(clientSocketFd).responseBuffer;

	if (msg.empty())
		return;

	int bytesSent = send(clientSocketFd, msg.c_str(), msg.size(), 0);

	if (bytesSent >= 0)
	{
		msg.clear();
		_clients.at(clientSocketFd).requestBuffer.clear();
		std::cout << "Response send" << std::endl;

		set_event(clientSocketFd, POLLIN);
	}
}

/**
 * @brief Clean up and remove a disconnected client.
 *
 * @param clientSocketFd File descriptor of the client to clean up
 */
void Server::_cleanupClient(int clientSocketFd)
{
	close(clientSocketFd);

	for (size_t i = 0; i < _pollSockets.size(); i++)
	{
		if (_pollSockets[i].fd == clientSocketFd)
		{
			_pollSockets.erase(_pollSockets.begin() + i);
			break;
		}
	}
	_clients.erase(clientSocketFd);

	LOG_DEBUG("Client disconnected");
}
