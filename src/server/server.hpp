/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: moritzknoll <moritzknoll@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:12:35 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/29 18:12:52 by moritzknoll      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "client.hpp"
#include "sockets.hpp"

class Server
{
  private:
	std::vector< ServerConfig >   _configs;
	std::map< int, ServerConfig > _serverConfigsByFd;
	std::vector< struct pollfd >  _pollSockets;
	std::map< int, Client >       _clients;

	void                          _acceptNewClient(int serverSocketFd);
	void                          _handleClientMessage(int clientSocketFd);
	void                          _sendResponseToClient(int clientSocketFd);
	void                          _cleanupClient(int clientSocketFd);
	bool                          _isCompleteRequest(std::string& request);
	void                          set_event(int fd, int state);

  public:
	Server(std::vector< ServerConfig > configs);
	~Server();

	void init(); // socket, bind, listen
	void run();  // main loop for accepting and handling clients
};
