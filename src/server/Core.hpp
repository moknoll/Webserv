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

#define RECV_BUFFER 8192

enum FdType
{
	FD_CLIENT = 0,
	FD_PIPE_IN,
	FD_PIPE_OUT
};

struct FdInfo
{
	Client* client;
	FdType  type;
};

class Core
{
  private:
	std::vector< Server* >       _servers;
	std::vector< struct pollfd > poll_fds_;
	std::map< int, FdInfo >      _clients;

	void                         _acceptNewClient(const Server& server);
	void                         _handleClientMessage(int clientSocketFd);
	void                         _sendResponseToClient(int clientSocketFd);
	void                         _cleanupClient(int clientSocketFd);

	void                         setEvent_(int clientsocketFD, int state);
	void                         addFdtoPoll_(int fd, int event);

	Client*                      FindClient(int fd) const;

	void                         readCGioutput(Client& client);
	void                         writeCGIinput(Client& client);

	void                         checkCGIProcesses();

	// void                         cleanPollFds();
	void                         removePollFd(int fd);

	Core();

  public:
	Core(const std::vector< ServerConfig >& configs);
	~Core();

	void    run(); // main loop for accepting and handling clients
	Server* findServerByFd(int serverFd);
};
