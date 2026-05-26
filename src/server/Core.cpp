/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: moritzknoll <moritzknoll@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:48 by mknoll            #+#    #+#             */
/*   Updated: 2026/05/16 13:00:56 by moritzknoll      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Core.hpp"
// #include "../ConfigParser/ServerConfig.hpp"
#include "../logger/Logger.hpp"
#include "Client.hpp"
#include "Server.hpp"
// #include "Sockets.hpp"

#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netdb.h>
#include <sys/types.h>
#include <vector>

Core::Core(const std::vector< ServerConfig >& configs)
{
	for (size_t i = 0; i < configs.size(); i++)
	{
		Server* s = new Server(configs[i]);
		_servers.push_back(s);

		struct pollfd pfd;
		pfd.fd = s->getSocketFd();
		std::cout << "sock_fd:" << s->getSocketFd() << std::endl;
		pfd.events = POLLIN;
		_pollSockets.push_back(pfd);
	}
}

Core::~Core()
{
	// for(size_t i = 0; i < _serverfds.size(); i++)
	// {
	// 	if(_serverfds[i] != -1)
	// 		close(_serverfds[i]);
	// }
	// Check later on how to close?
	//
	std::vector< Server* >::iterator it_serv = _servers.begin();
	for (; it_serv != _servers.end(); ++it_serv)
		delete *it_serv;

	std::map< int, Client* >::iterator it_client = _clients.begin();
	for (; it_client != _clients.end(); ++it_client)
	{
		delete it_client->second;
	}
}

Server* Core::findServerByFd(int serverFd)
{
	for (size_t i = 0; i < _servers.size(); i++)
	{
		if (_servers[i]->getSocketFd() == serverFd)
			return _servers[i];
	}
	return NULL;
}

/**
 * @brief Runs the main event loop for the server.
 *
 * @throws std::runtime_error if poll() fails
 */
void Core::run()
{
	while (true)
	{
		int ret = poll(_pollSockets.data(), _pollSockets.size(), 3000);
		if (ret == -1)
			throw std::runtime_error("Poll failed");

		for (size_t i = 0; i < _pollSockets.size(); i++)
		{
			if (_pollSockets[i].revents & POLLIN)
			{
				Server* server = findServerByFd(_pollSockets[i].fd);
				if (server != NULL)
					_acceptNewClient(*server);
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
bool Core::_isCompleteRequest(std::string& request)
{
	(void) request;

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
void Core::_acceptNewClient(const Server& server)
{
	int                server_fd = server.getSocketFd();

	struct sockaddr_in clientAddr;
	socklen_t          addrLen = sizeof(clientAddr);

	int                newClientFd =
	    accept(server_fd, (struct sockaddr*) &clientAddr, &addrLen);

	if (newClientFd == SOCKET_ERROR)
		return;

	// // Debug: Show which config was assigned to the new client
	// std::ostringstream oss;
	// oss << "New Client FD: " << newClientFd
	// 	<< " | Accepted from Server Socket FD: " << serverSocketFd
	// 	<< " | Assigned Config Port: " << config->port
	// 	<< " | Config Host: " << config->host
	// 	<< " | Config Root: " << config->root;
	// LOG_DEBUG(oss.str());

	fcntl(newClientFd, F_SETFL, O_NONBLOCK);

	struct pollfd pfd;
	pfd.fd = newClientFd;
	pfd.events = POLLIN;
	_pollSockets.push_back(pfd);

	ServerConfig cfg = server.getConfig();
	_clients[newClientFd] = new Client(newClientFd, server.getConfig());

	std::cout << "New Client connected: " << newClientFd << std::endl;
	LOG_DEBUG("New client connected");
}

void Core::setEvent(int clientsocketFD, int state)
{
	// Change Poll event to writing
	for (size_t i = 0; i < _pollSockets.size(); i++)
	{
		if (_pollSockets[i].fd == clientsocketFD)
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
void Core::_handleClientMessage(int clientSocketFd)
{
	if (_clients.find(clientSocketFd) == _clients.end())
		return;
	Client* client = _clients.at(clientSocketFd);

	if (client == NULL)
		return;

	char    buffer[RECV_BUFFER];
	ssize_t recv_bytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);

	if (recv_bytes <= 0)
	{
		_cleanupClient(clientSocketFd);
	}
	else
	{
		client->appendRecvBuffer(buffer, recv_bytes);
		client->processRequest();

		if (client->isComplete())
		{
			setEvent(clientSocketFd, POLLOUT);
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
void Core::_sendResponseToClient(int clientSocketFd)
{
	if (_clients.find(clientSocketFd) == _clients.end())
		return;

	Client* client = _clients.at(clientSocketFd);
	if (client == NULL)
		return;

	std::string buffer = client->serialize();

	if (buffer.empty())
	{
		client->reset();
		std::cout << "Response sent" << std::endl;
		if (client->isKeepElive())
		{
			setEvent(clientSocketFd, POLLIN);
		}
		else
		{
			_cleanupClient(clientSocketFd);
		}
		return;
	}

	ssize_t sent_bytes = send(clientSocketFd, buffer.c_str(), buffer.size(), 0);
	if (sent_bytes == -1)
	{
		_cleanupClient(clientSocketFd);
		return;
	}

	size_t sent = static_cast< size_t >(sent_bytes);
	buffer.erase(0, sent);
	client->setSendBuffer(buffer);
}

/**
 * @brief Clean up and remove a disconnected client.
 *
 * @param clientSocketFd File descriptor of the client to clean up
 */
void Core::_cleanupClient(int clientSocketFd)
{
	if (_clients.find(clientSocketFd) != _clients.end())
	{
		Client* client = _clients.at(clientSocketFd);
		delete client;
	}

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
