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
		int     fd = s->getSocketFd();

		addFdtoPoll_(fd, POLLIN);
		fd_infos_[fd].server = s;
		fd_infos_[fd].type = FD_SERVER;

		// struct pollfd pfd;
		// pfd.fd = s->getSocketFd();
		// std::cout << "sock_fd:" << s->getSocketFd() << std::endl;
		// pfd.events = POLLIN;
		// poll_fds_.push_back(pfd);
		std::cout << "sock_fd:" << fd << std::endl;
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
	// std::vector< Server* >::iterator it_serv = _servers.begin();
	// for (; it_serv != _servers.end(); ++it_serv)
	// 	delete *it_serv;

	// std::map< int, Client* >::iterator it_client = _clients.begin();
	std::map< int, FdInfo >::iterator it_info = fd_infos_.begin();
	for (; it_info != fd_infos_.end(); ++it_info)
	{
		if (it_info->first == FD_SERVER)
			delete it_info->second.server;
		else if (it_info->first == FD_CLIENT)
			delete it_info->second.client;
	}
}

// Server* Core::findServerByFd(int serverFd)
// {
// 	for (size_t i = 0; i < _servers.size(); i++)
// 	{
// 		if (_servers[i]->getSocketFd() == serverFd)
// 			return _servers[i];
// 	}
// 	return NULL;
// }

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
			pollfd& pfd = poll_fds_[i];

			if (poll_fds_[i].revents & POLLIN)
			{
				handlePOLLIN(pfd);
			}
			else if (poll_fds_[i].revents & POLLOUT)
			{
				handlePOLLOUT(pfd);
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
void Core::acceptNewClient_(const Server& server)
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

	// ServerConfig cfg = server.getConfig();
	Client* client = new Client(newClientFd, server.getConfig());
	fd_infos_[newClientFd].client = client;
	fd_infos_[newClientFd].type = FD_CLIENT;
	std::cout << "size of poll fds: " << poll_fds_.size() << '\n';
	std::cout << "size of fd_infos_: " << fd_infos_.size() << '\n';

	std::cout << "New Client connected: " << newClientFd << std::endl;
	LOG_DEBUG("New client connected");
}

void Core::handleClientMessage_(Client* client, int client_fd)
{
	if (client == NULL)
		return;

	if (client->getHttpState() == Client::HTTP_RECV)
	{
		char    buffer[RECV_BUFFER];
		ssize_t recv_bytes = recv(client_fd, buffer, sizeof(buffer), 0);

		if (recv_bytes <= 0)
		{
			cleanupClient_(client_fd);
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
			fd_infos_[client->cgi_pipe_out].client = client;
			fd_infos_[client->cgi_pipe_out].type = FD_PIPE_OUT;
		}

		if (client->getFdCGI_in() != -1)
		{
			fd_infos_[client->cgi_pipe_in].client = client;
			fd_infos_[client->cgi_pipe_in].type = FD_PIPE_IN;
		}
	}
	setEvent_(client_fd, POLLOUT);
}

void Core::sendResponseToClient_(int client_fd)
{
	if (fd_infos_.find(client_fd) == fd_infos_.end())
		return;

	Client* client = fd_infos_.at(client_fd).client;
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
			setEvent_(client_fd, POLLIN);
		}
		else
		{
			cleanupClient_(client_fd);
		}
		return;
	}

	ssize_t sent_bytes = send(client_fd, buffer.c_str(), buffer.size(), 0);
	if (sent_bytes == -1)
	{
		cleanupClient_(client_fd);
		return;
	}

	size_t sent = static_cast< size_t >(sent_bytes);
	buffer.erase(0, sent);
	client->setSendBuffer(buffer);
}

void Core::cleanupClient_(int client_fd)
{
	if (fd_infos_.find(client_fd) != fd_infos_.end())
	{
		Client* client = fd_infos_.at(client_fd).client;
		delete client;
	}

	removePollFd(client_fd);

	fd_infos_.erase(client_fd);

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

void Core::setEvent_(int client_fd, int state)
{
	// Change Poll event to writing
	for (size_t i = 0; i < poll_fds_.size(); i++)
	{
		if (poll_fds_[i].fd == client_fd)
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
		fd_infos_.erase(client.cgi_pipe_in);
		close(client.cgi_pipe_in);
		client.cgi_pipe_in = -1;
	}
}

void Core::checkCGIProcesses()
{
	// std::map< int, Client* >::iterator it = _clients.begin();
	std::map< int, FdInfo >::iterator it = fd_infos_.begin();
	for (; it != fd_infos_.end(); ++it)
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
				fd_infos_.erase(client->cgi_pipe_in);
				fd_infos_.erase(client->cgi_pipe_out);
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

void Core::handlePOLLIN(pollfd& pfd)
{
	FdInfo* info = getFdInfo(pfd.fd);

	if (!info || !info->client)
		return;

	switch (info->type)
	{
		case FD_SERVER:   acceptNewClient_(*info->server); break;
		case FD_PIPE_OUT: readCGioutput(*info->client); break;
		case FD_CLIENT:   handleClientMessage_(info->client, pfd.fd); break;
		default:          break;
	}
}

void Core::handlePOLLOUT(pollfd& pfd)
{
	FdInfo* info = getFdInfo(pfd.fd);

	if (!info || !info->client)
		return;

	switch (info->type)
	{
		case FD_PIPE_IN: writeCGIinput(*info->client); break;
		case FD_CLIENT:
			sendResponseToClient_(pfd.fd);
			break;
			break;
		default: break;
	}
}

Core::FdInfo* Core::getFdInfo(int fd)
{
	std::map< int, FdInfo >::iterator it = fd_infos_.find(fd);

	if (it == fd_infos_.end())
		return NULL;

	return &(it->second);
}
