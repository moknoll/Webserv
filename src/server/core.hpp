/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:12:35 by mknoll            #+#    #+#             */
/*   Updated: 2026/05/05 14:00:02 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "server.hpp"
#include "sockets.hpp"
#include "client.hpp"

class Core{
  private:
	std::vector<Server>				_servers;
	std::map< int, Server>			_serverByFd;
	std::vector< struct pollfd > 	_pollSockets;
	std::map< int, Client >			_clients;

	void                          _acceptNewClient(int serverSocketFd);
	void                          _handleClientMessage(int clientSocketFd);
	void                          _sendResponseToClient(int clientSocketFd);
	void                          _cleanupClient(int clientSocketFd);
	bool						  _isCompleteRequest(std::string &request);

  public:
	Core(std::vector< ServerConfig > &configs);
	~Core();

	void init(); // socket, bind, listen
	void run();  // main loop for accepting and handling clients
};
