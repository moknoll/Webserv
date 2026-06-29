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
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
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
		poll_fds_.push_back(pfd);
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
		int ret = poll(poll_fds_.data(), poll_fds_.size(), 3000);
		if (ret == -1)
			throw std::runtime_error("Poll failed");

		for (size_t i = 0; i < poll_fds_.size(); i++)
		{
			// if ((_pollSockets[i].revents & POLLERR)
			//     || _pollSockets[i].revents & POLLHUP)
			// clean soccketss

			if (poll_fds_[i].revents & POLLIN)
			{
				Server* server = findServerByFd(poll_fds_[i].fd);
				if (server != NULL)
					_acceptNewClient(*server);
				else
				{
					Client* client = FindClient(poll_fds_[i].fd);
					if (client == NULL)
						continue;

					if (client->getHttpState() == Client::CGI_STATE)
						readCGioutput(*client);
					else
						_handleClientMessage(poll_fds_[i].fd);
				}
			}
			else if (poll_fds_[i].revents & POLLOUT)
				_sendResponseToClient(poll_fds_[i].fd);
		}
		checkCGIProcesses();
	}
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

	fcntl(newClientFd, F_SETFL, O_NONBLOCK);

	struct pollfd pfd;
	pfd.fd = newClientFd;
	pfd.events = POLLIN;
	poll_fds_.push_back(pfd);

	ServerConfig cfg = server.getConfig();
	Client*      client = new Client(newClientFd, server.getConfig());
	_clients[newClientFd] = client;
	Vclients_.push_back(client);

	std::cout << "New Client connected: " << newClientFd << std::endl;
	LOG_DEBUG("New client connected");
}

void Core::_handleClientMessage(int clientSocketFd)
{
	if (_clients.find(clientSocketFd) == _clients.end())
		return;
	Client* client = FindClient(clientSocketFd);

	if (client == NULL)
		return;

	int c_state = client->getHttpState();

	if (c_state == Client::HTTP_INIT || Client::HTTP_RECV == c_state)
	{
		char    buffer[RECV_BUFFER];
		ssize_t recv_bytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);

		if (recv_bytes <= 0)
		{
			_cleanupClient(clientSocketFd);
		}

		client->parseRequest(buffer, recv_bytes);
		if (!client->isRequestComplete())
			return;
		client->processRequest();
	}

	if (client->getHttpState() == Client::CGI_STATE)
	{
		int fd_out = client->getFdCGI_out();
		int fd_in = client->getFdCGI_in();
		if (fd_out != -1)
			addFdtoPoll_(fd_out, POLLIN);
		if (fd_in != -1)
			addFdtoPoll_(fd_in, POLLOUT);
	}

	if (client->getHttpState() == Client::HTTP_SEND)
	{
		setEvent_(clientSocketFd, POLLOUT);
	}
}

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
		if (client->isKeepAlive())
		{
			setEvent_(clientSocketFd, POLLIN);
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

	removePollFd(clientSocketFd);

	_clients.erase(clientSocketFd);

	LOG_DEBUG("Client disconnected");
}

Client* Core::FindClient(int fd) const
{
	for (size_t i = 0; i < Vclients_.size(); ++i)
	{
		if (Vclients_[i]->getClientFd() == fd || Vclients_[i]->getFdCGI_out()
		    || Vclients_[i]->getFdCGI_in())
			return Vclients_[i];
	}
	return NULL;
}

void Core::addFdtoPoll_(int fd, int event)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = event;
	poll_fds_.push_back(pfd);
}

void Core::setEvent_(int clientsocketFD, int state)
{
	// Change Poll event to writing
	for (size_t i = 0; i < poll_fds_.size(); i++)
	{
		if (poll_fds_[i].fd == clientsocketFD)
		{
			poll_fds_[i].events = state;
			break;
		}
	}
}

void Core::readCGioutput(Client& client)
{
	if (client.getHttpState() != Client::CGI_STATE)
		return;
	int fd = client.getFdCGI_out();

	LOG_DEBUG("Try read from CGI PIPE output");

	char    buf[RECV_BUFFER];
	ssize_t r = read(fd, buf, sizeof(buf));

	if (r > 0)
	{
		client.cgi.appendCgiOutput(buf, r);
	}
}

void Core::checkCGIProcesses()
{
	for (size_t i = 0; i < Vclients_.size(); ++i)
	{
		Client* client = Vclients_[i];
		if (client == NULL)
			continue;
		if (client->getHttpState() == Client::CGI_STATE)
		{
			if (client->CGIProcessFinished())
			{
				LOG_DEBUG("CGI FInished");
				removePollFd(client->getFdCGI_in());
				removePollFd(client->getFdCGI_out());
				setEvent_(client->getClientFd(), POLLOUT);
			}
		}
	}
}

// void CGiTimeOut()
// {
// 	time_t now = std::time(NULL);
//
// 	maxTimeout = 60;
// 	for client form clients:
// 	{
// 		client is CGI.
// 		{
// 			now - startCGi > maxTimeout;
// 			kill cgi;
// 		}
// 		now - lastrequestfrom client > maxTimeout;
// 		cleanClient;
// 	}
// }
//

void Core::removePollFd(int fd)
{
	if (fd == -1)
		return;

	for (size_t i = 0; i < poll_fds_.size(); i++)
	{
		if (poll_fds_[i].fd == fd)
		{
			poll_fds_.erase(poll_fds_.begin() + i);
			break;
		}
	}
}
