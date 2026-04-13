/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:12:35 by mknoll            #+#    #+#             */
/*   Updated: 2026/02/26 14:50:42 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "sockets.hpp"
#include "client.hpp"


class Server {
	private: 
		int 						_port;
		int 						_serverfd;
		std::vector<struct pollfd> 	_pollfds;
		std::map<int, Client> 		_clients;

		void 	_acceptNewClient();
		void 	_handleClientMessage(int fd);
		void 	_sendResponseToClient(int fd);
		void 	_cleanupClient(int fd);
	public:
		Server(int port);
		~Server();

		void init(); // socket, bind, listen
		void run(); // main loop for accepting and handling clients
};

#endif