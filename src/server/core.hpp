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
#include "client.hpp"
#include "server.hpp"
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
