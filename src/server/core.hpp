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

#include "../ConfigParser/ServerConfig.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "sockets.hpp"
#include "client.hpp"
#include <vector>
#include <map>

class Server;
// class Client;
class ServerConfig;

class Core{
  private:
	std::vector<Server>				_servers;
	std::vector< struct pollfd > 	_pollSockets;
	std::map <int, Client>				_clients;

	void                          _acceptNewClient(int serverSocketFd);
	void                          _handleClientMessage(int clientSocketFd);
	void                          _sendResponseToClient(int clientSocketFd);
	void                          _cleanupClient(int clientSocketFd);
	bool						  _isCompleteRequest(std::string &request);
	void						  set_event(int clientsocketFD, int state);

  public:
	Core(); 
	Core(std::vector< ServerConfig > &configs);
	~Core();

	void init(); // socket, bind, listen
	void run();  // main loop for accepting and handling clients
	Server *find_server_by_Fd(int serverFd); 
};
