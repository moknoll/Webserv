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
#include "../constants.hpp"
#include "../lib/ws.hpp"
#include "../logger/Logger.hpp"
#include "Client.hpp"
#include "Server.hpp"

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
		int     fd = s->getSocketFd();

		addFdtoPoll_(fd, POLLIN);
		fd_infos_[fd].server = s;
		fd_infos_[fd].type = FD_SERVER;

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

void Core::run()
{
	while (true)
	{
		int ret = poll(poll_fds_.data(), poll_fds_.size(), 3000);
		if (ret == -1)
			throw std::runtime_error("Poll failed");
		// if (ret == 0) // What to do

		// ckeckTimeOutCGI(); // ?????????????????????
		// ckeckTimeOutClient();
		for (size_t i = 0; i < poll_fds_.size(); i++)
		{
			pollfd& pfd = poll_fds_[i];

			// if (poll_fds_[i].revents == POLLERR || poll_fds_[i].revents ==
			// POLLHUP) // What to do
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

	Client* client = new Client(newClientFd, server.getConfig());
	fd_infos_[newClientFd].client = client;
	fd_infos_[newClientFd].type = FD_CLIENT;

	LOG_DEBUG("size of poll fds: " + ws::to_string(poll_fds_.size())
	          + "; size of fd_infos_: " + ws::to_string(fd_infos_.size()));
	LOG_DEBUG("New client connected: with fd = " + ws::to_string(newClientFd));
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
		registerCgiFds(client);
	}
	setEvent_(client_fd, POLLOUT);
}

void Core::sendResponseToClient_(int client_fd)
{
	if (fd_infos_.find(client_fd) == fd_infos_.end())
		return;

	Client* client = fd_infos_.at(client_fd).client;
	if (client == NULL)
		return;

	if (client->getHttpState() != Client::HTTP_SEND)
		return;
	std::string buffer = client->serialize();

	if (buffer.empty())
	{
		LOG_DEBUG("Response sent");
		client->reset();
		if (client->isKeepAlive())
		{
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

	LOG_DEBUG("Client " + ws::to_string(client_fd) + " disconnected");
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

void Core::setEvent_(int fd, int state)
{
	for (size_t i = 0; i < poll_fds_.size(); i++)
	{
		if (poll_fds_[i].fd == fd)
		{
			poll_fds_[i].events = state;
			break;
		}
	}
}

void Core::readCGi_output_(Client* client, int fd)
{
	if (!client || client->getHttpState() != Client::CGI_STATE)
		return;

	LOG_DEBUG("Try read from CGI PIPE output");

	char    buf[RECV_BUFFER];
	ssize_t r = read(fd, buf, sizeof(buf));

	if (r > 0)
	{
		client->appendCgiOutput(buf, r);
	}
}

void Core::writeCGI_input_(Client* client, int fd)
{
	if (!client || fd == -1)
		return;

	LOG_DEBUG("Write to CGI input");
	if (client->writeRequestBody(fd))
	{
		removePollFd(fd);
		fd_infos_.erase(fd);
	}
}

void Core::checkCGIProcesses()
{
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
				CgiContext cgi_ctx = client->getCGIContext();
				LOG_DEBUG("CGI FInished");
				removePollFd(cgi_ctx.stdout_pipe);
				removePollFd(cgi_ctx.stdin_pipe);
				fd_infos_.erase(cgi_ctx.stdout_pipe);
				fd_infos_.erase(cgi_ctx.stdin_pipe);
				setEvent_(client->getClientFd(), POLLOUT);
			}
		}
	}
}

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
		case FD_PIPE_OUT: readCGi_output_(info->client, pfd.fd); break;
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
		case FD_PIPE_IN: writeCGI_input_(info->client, pfd.fd); break;
		case FD_CLIENT:  sendResponseToClient_(pfd.fd); break;
		default:         break;
	}
}

Core::FdInfo* Core::getFdInfo(int fd)
{
	std::map< int, FdInfo >::iterator it = fd_infos_.find(fd);

	if (it == fd_infos_.end())
		return NULL;

	return &(it->second);
}

void Core::registerCgiFds(Client* client)
{
	const CgiContext cgi_ctx = client->getCGIContext();

	addFdtoPoll_(cgi_ctx.stdout_pipe, POLLIN);
	addFdtoPoll_(cgi_ctx.stdin_pipe, POLLOUT);
	if (cgi_ctx.stdout_pipe != -1)
	{
		fd_infos_[cgi_ctx.stdout_pipe].client = client;
		fd_infos_[cgi_ctx.stdout_pipe].type = FD_PIPE_OUT;
	}

	if (cgi_ctx.stdin_pipe != -1)
	{
		fd_infos_[cgi_ctx.stdin_pipe].client = client;
		fd_infos_[cgi_ctx.stdin_pipe].type = FD_PIPE_IN;
	}
}
