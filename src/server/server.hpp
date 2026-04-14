/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:12:35 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/14 13:11:39 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "sockets.hpp"
#include "../src/ConfigParser/ServerConfig.hpp"
#include "client.hpp"


class Server {
	private:
		std::vector<ServerConfig>	_configs;
		std::map<int, ServerConfig> _serverfds;
		std::vector<struct pollfd> 	_pollfds;
		std::map<int, Client> 		_clients;

		void 	_acceptNewClient(int fd);
		void 	_handleClientMessage(int fd);
		void 	_sendResponseToClient(int fd);
		void 	_cleanupClient(int fd);
	public:
		Server(std::vector<ServerConfig> configs);
		~Server();

		void init(); // socket, bind, listen
		void run(); // main loop for accepting and handling clients
};