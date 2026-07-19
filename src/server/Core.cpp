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
#include <map>
#include <netdb.h>
#include <set>
#include <string>
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

		LOG_INFO("Server is running on port " + ws::to_string(configs[i].port));
		LOG_DEBUG("Server socket created successfully (fd: " + ws::to_string(fd)
		          + ")");
	}
}

Core::~Core()
{
	LOG_DEBUG("Call Core destructor");
	std::map< int, FdInfo >::iterator it_info = fd_infos_.begin();
	for (; it_info != fd_infos_.end(); ++it_info)
	{
		FdInfo inof = it_info->second;

		if (inof.type == FD_SERVER)
		{
			delete it_info->second.server;
			LOG_DEBUG("Delete Server");
		}
		else if (inof.type == FD_CLIENT)
		{
			delete it_info->second.client;
			LOG_DEBUG("Delete Client");
		}
	}
}

void Core::run()
{
	while (g_running)
	{
		int ret = poll(poll_fds_.data(), poll_fds_.size(), 3000);
		if (ret == -1)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Poll failed");
		}

		for (size_t i = 0; i < poll_fds_.size(); i++)
		{
			pollfd& pfd = poll_fds_[i];

			if (poll_fds_[i].revents & (POLLERR | POLLHUP))
			{
				handlePollerr(pfd);
				continue;
			}

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
		checkClientTimeouts();
	}
	LOG_INFO("Server shotdown");
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

	LOG_INFO("New client connected: (fd: " + ws::to_string(newClientFd) + ")");
	LOG_DEBUG("Size of poll fds: " + ws::to_string(poll_fds_.size()));
	LOG_DEBUG("Size of fd_infos_: " + ws::to_string(fd_infos_.size()));
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

		client->updateLastActivity();
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

void Core::sendResponseToClient_(Client* client, int client_fd)
{
	if (!client || client->getHttpState() != Client::HTTP_SEND)
		return;

	std::string buffer = client->serialize();

	if (buffer.empty())
	{
		LOG_DEBUG("Sending Response finished");
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

	client->updateLastActivity();
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

	LOG_DEBUG("Try read from CGI output");

	if (client->readCgiOutput_(fd))
	{
		removePollFd(fd);
		fd_infos_.erase(fd);
	}
}

void Core::writeCGI_input_(Client* client, int fd)
{
	if (!client || fd == -1)
		return;

	LOG_DEBUG("Write to CGI input");
	if (client->writeCgiInput(fd))
	{
		removePollFd(fd);
		fd_infos_.erase(fd);
	}
}

void Core::checkCGIProcesses()
{
	std::set< int >                   fds_to_remove;

	std::map< int, FdInfo >::iterator it = fd_infos_.begin();
	for (; it != fd_infos_.end(); ++it)
	{
		FdInfo info = it->second;

		if (info.type != FD_CLIENT || info.client == NULL)
			continue;

		Client* client = info.client;

		if (client->getHttpState() == Client::CGI_STATE)
		{
			CgiContext cgi_ctx = client->getCGIContext();

			if (client->CGIProcessFinished())
			{
				LOG_DEBUG("CGI FInished");
				fds_to_remove.insert(cgi_ctx.stdin_pipe);
				fds_to_remove.insert(cgi_ctx.stdout_pipe);
				setEvent_(client->getClientFd(), POLLOUT);
			}
		}
	}

	std::set< int >::iterator it_r = fds_to_remove.begin();
	for (; it_r != fds_to_remove.end(); ++it_r)
	{
		removePollFd(*it_r);
		fd_infos_.erase(*it_r);
	}
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

	if (!info)
		return;

	switch (info->type)
	{
		case FD_PIPE_IN: writeCGI_input_(info->client, pfd.fd); break;
		case FD_CLIENT:  sendResponseToClient_(info->client, pfd.fd); break;
		default:         break;
	}
}

void Core::handlePollerr(pollfd& pfd)
{
	FdInfo* info = getFdInfo(pfd.fd);

	if (!info)
		return;

	switch (info->type)
	{
		case FD_CLIENT:
		{
			cleanupClient_(pfd.fd);
			LOG_DEBUG("FD Client iS POLLERR & POLLHUP");
			break;
		}
		case FD_PIPE_IN: removePollFd(pfd.fd); break;
		case FD_PIPE_OUT:
		{
			LOG_DEBUG("FD PIPE OUT iS POLLERR & POLLHUP");
			Client* client = info->client;
			if (!client)
				return;
			if (client->readCgiOutput_(pfd.fd))
			{
				removePollFd(pfd.fd);
				fd_infos_.erase(pfd.fd);
			}
		}
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

void Core::checkClientTimeouts()
{
	time_t now = std::time(NULL);

	for (std::map< int, FdInfo >::iterator it = fd_infos_.begin();
	     it != fd_infos_.end();)
	{
		FdInfo info = it->second;

		if (info.type == FD_CLIENT)
		{
			Client* client = info.client;
			if (client && client->getHttpState() != Client::CGI_STATE
			    && now - client->getLastActivity() > KEEPALIVE_TIMEOUT)
			{
				int fd = client->getClientFd();
				++it;
				cleanupClient_(fd);
				LOG_DEBUG("Client " + ws::to_string(fd) + " TIMED OUT");
				continue;
			}
		}
		++it;
	}
}
