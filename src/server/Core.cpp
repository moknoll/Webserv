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

void track_now(struct pollfd fd)
{
	if (fd.events & POLLIN)
		std::cout << "Tracking POLLIN\n";
	else if (fd.events & POLLOUT)
		std::cout << "Tracking POLLOUT\n";
	else if ((fd.events & (POLLIN | POLLOUT)) == (POLLIN | POLLOUT))
		std::cout << "Tracking POLLIN and POLLOUT\n";
}

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

	// std::map< int, Client* >::iterator it_client = _clients.begin();
	std::map< int, FdInfo >::iterator it_client = _clients.begin();
	for (; it_client != _clients.end(); ++it_client)
	{
		delete it_client->second.client;
		// delete it_client->second;
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

			// track_now(poll_fds_[i]);
			if (poll_fds_[i].revents & POLLIN)
			{
				Server* server = findServerByFd(poll_fds_[i].fd);
				if (server != NULL)
					_acceptNewClient(*server);
				else
				{
					if (_clients.find(poll_fds_[i].fd) == _clients.end())
						continue;
					FdInfo fd_info = _clients.at(poll_fds_[i].fd);

					if (fd_info.client == NULL)
						continue;

					if (fd_info.type == FD_PIPE_OUT)
					{
						std::cout << "POLLIN: " << poll_fds_[i].fd
						          << "type: FD_PIPE_OUT\n";
						readCGioutput(*fd_info.client);
					}
					else if (fd_info.type == FD_CLIENT)
					{
						std::cout << "POLLIN: " << poll_fds_[i].fd
						          << "type: Client\n";

						_handleClientMessage(poll_fds_[i].fd);
					}
				}
			}
			else if (poll_fds_[i].revents & POLLOUT)
			{
				if (_clients.find(poll_fds_[i].fd) == _clients.end())
					continue;
				FdInfo fd_info = _clients.at(poll_fds_[i].fd);

				if (fd_info.client == NULL)
					continue;

				if (fd_info.type == FD_PIPE_IN)
				{
					std::cout << "POLLOUT: " << poll_fds_[i].fd
					          << "type: FD_PIPE_IN\n";
					writeCGIinput(*fd_info.client);
				}
				else if (fd_info.type == FD_CLIENT)
					_sendResponseToClient(poll_fds_[i].fd);
			}
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

	addFdtoPoll_(newClientFd, POLLIN);
	// struct pollfd pfd;
	// pfd.fd = newClientFd;
	// pfd.events = POLLIN;
	// poll_fds_.push_back(pfd);

	ServerConfig cfg = server.getConfig();
	Client*      client = new Client(newClientFd, server.getConfig());
	_clients[newClientFd].client = client;
	_clients[newClientFd].type = FD_CLIENT;
	std::cout << "size of poll fds: " << poll_fds_.size() << '\n';

	std::cout << "New Client connected: " << newClientFd << std::endl;
	LOG_DEBUG("New client connected");
}

void Core::_handleClientMessage(int clientSocketFd)
{
	if (_clients.find(clientSocketFd) == _clients.end())
		return;
	// Client* client = FindClient(clientSocketFd);
	Client* client = _clients.at(clientSocketFd).client;

	if (client == NULL)
		return;

	if (client->getHttpState() == Client::HTTP_RECV)
	{
		char    buffer[RECV_BUFFER];
		ssize_t recv_bytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);

		if (recv_bytes <= 0)
		{
			_cleanupClient(clientSocketFd);
			return;
		}

		client->parseRequest(buffer, recv_bytes);
		if (!client->isRequestComplete())
			return;

		client->processRequest();
	}

	if (client->getHttpState() == Client::CGI_STATE)
	{
		addFdtoPoll_(client->cgi_pipe_out, POLLIN);
		addFdtoPoll_(client->cgi_pipe_in, POLLOUT);
		if (client->getFdCGI_out() != -1)
		{
			_clients[client->cgi_pipe_out].client = client;
			_clients[client->cgi_pipe_out].type = FD_PIPE_OUT;
		}

		if (client->getFdCGI_in() != -1)
		{
			_clients[client->cgi_pipe_in].client = client;
			_clients[client->cgi_pipe_in].type = FD_PIPE_IN;
		}
	}
	// else
	// if (client->getHttpState() == Client::HTTP_SEND)
	// { setEvent_(clientSocketFd, POLLOUT);
	// }
	setEvent_(clientSocketFd, POLLOUT);
}

void Core::_sendResponseToClient(int clientSocketFd)
{
	if (_clients.find(clientSocketFd) == _clients.end())
		return;

	Client* client = _clients.at(clientSocketFd).client;
	// Client* client = FindClient(clientSocketFd);
	if (client == NULL)
		return;

	if (client->getHttpState() != Client::HTTP_SEND)
		return;
	std::string buffer = client->serialize();

	if (buffer.empty())
	{
		client->reset();
		LOG_DEBUG("Response sent");
		if (client->isKeepAlive())
		{
			// LOG_DEBUG("isKeepAlive");
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
		Client* client = _clients.at(clientSocketFd).client;
		delete client;
	}

	removePollFd(clientSocketFd);

	_clients.erase(clientSocketFd);

	LOG_DEBUG("Client disconnected");
}

void Core::addFdtoPoll_(int fd, int event)
{
	if (fd == -1)
		return;

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
	int fd = client.cgi_pipe_out;

	LOG_DEBUG("Try read from CGI PIPE output");

	char    buf[RECV_BUFFER];
	ssize_t r = read(fd, buf, sizeof(buf));

	if (r > 0)
	{
		// client.cgi.appendCgiOutput(buf, r);
		client.cgi_output_buf.append(buf, r);
	}
}

void Core::writeCGIinput(Client& client)
{
	if (client.cgi_pipe_in == -1)
		return;
	LOG_DEBUG("Write to CGI input");
	if (client.writeRequestBody())
	{
		removePollFd(client.cgi_pipe_in);
		_clients.erase(client.cgi_pipe_in);
		close(client.cgi_pipe_in);
		client.cgi_pipe_in = -1;
	}
}

void Core::checkCGIProcesses()
{
	// std::map< int, Client* >::iterator it = _clients.begin();
	std::map< int, FdInfo >::iterator it = _clients.begin();
	for (; it != _clients.end(); ++it)
	{
		Client* client = it->second.client;
		if (client == NULL)
			continue;
		if (client->getHttpState() == Client::CGI_STATE)
		{
			if (client->CGIProcessFinished())
			{
				LOG_DEBUG("CGI FInished");
				removePollFd(client->cgi_pipe_in);
				removePollFd(client->cgi_pipe_out);
				_clients.erase(client->cgi_pipe_in);
				_clients.erase(client->cgi_pipe_out);
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
