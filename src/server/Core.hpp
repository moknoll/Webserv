/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: moritzknoll <moritzknoll@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:12:35 by mknoll            #+#    #+#             */
/*   Updated: 2026/05/06 13:26:52 by moritzknoll      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// #include "../ConfigParser/ServerConfig.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <map>
#include <vector>

extern bool g_running;

class Core
{
  private:
	enum FdType
	{
		FD_SERVER = 0,
		FD_CLIENT,
		FD_PIPE_IN,
		FD_PIPE_OUT
	};

	struct FdInfo
	{
		union
		{
			Server* server;
			Client* client;
		};
		FdType type;
	};

	// std::vector< Server* >       _servers;
	std::vector< struct pollfd > poll_fds_;
	std::map< int, FdInfo >      fd_infos_;

	void                         acceptNewClient_(const Server& server);
	void    handleClientMessage_(Client* client, int client_fd);
	void    sendResponseToClient_(Client* client, int client_fd);
	void    cleanupClient_(int client_fd);
	void    handlePOLLIN(pollfd& pfd);
	void    handlePOLLOUT(pollfd& pfd);
	void    handlePollerr(pollfd& pfd);
	void    setEvent_(int client_fd, int state);
	void    addFdtoPoll_(int fd, int event);
	void    readCGi_output_(Client* client, int fd);
	void    writeCGI_input_(Client* client, int fd);
	void    checkCGIProcesses();
	void    checkClientTimeouts();
	void    registerCgiFds(Client* client);
	void    removePollFd(int fd);
	FdInfo* getFdInfo(int fd);

	Core();

  public:
	Core(const std::vector< ServerConfig >& configs);
	~Core();

	void run(); // main loop for accepting and handling clients
	            // Server* findServerByFd(int serverFd);
};
