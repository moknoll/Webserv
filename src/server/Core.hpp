/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Core.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:12:35 by mknoll            #+#    #+#             */
/*   Updated: 2026/06/27 11:25:00 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// #include "../ConfigParser/ServerConfig.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <map>
#include <vector>

#define RECV_BUFFER 8192

class Core
{
  private:
	std::vector< Server* >       _servers;
	std::vector< struct pollfd > _pollSockets;
	std::map< int, Client* >     _clients;

	void                         _acceptNewClient(const Server& server);
	void                         _handleClientMessage(int clientSocketFd);
	void                         _sendResponseToClient(int clientSocketFd);
	void                         _cleanupClient(int clientSocketFd);
	bool                         _isCompleteRequest(std::string& request); //??
	void                         setEvent(int clientsocketFD, int state);

	Core();

  public:
	Core(const std::vector< ServerConfig >& configs);
	~Core();

	void    run(); // main loop for accepting and handling clients
	Server* findServerByFd(int serverFd);
};

enum FdRoleType
{
    FD_SERVER = 0,
    FD_CLIENT,
    FD_CGI_STDIN,
    FD_CGI_STDOUT
};

struct FdRole
{
    FdRoleType type;
    int client_fd; // zu welchem Client gehört die Pipe
};

std::map<int, FdRole> _fdRoles;

void _registerFd(int fd, short events, FdRoleType type, int clientFd);
void _unregisterFd(int fd);
void _updateFdEvents(int fd, short events);

void _registerCgiFds(int clientFd);
void _handleCgiFdEvent(int fd, short revents);
void _cleanupCgiForClient(int clientFd);