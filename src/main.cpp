/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:52 by mknoll            #+#    #+#             */
/*   Updated: 2026/03/26 11:08:06 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/server.hpp"
#include "ConfigParser/ConfigParser.hpp"
#include "ConfigParser/ServerConfig.hpp"

#include <iostream>

int checkArguments(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: ./websrerv <config_file>" << std::endl;
		return 0;
	}
	if (std::string(argv[1]) != "config.conf")
	{
		std::cout << "Error: Config file must be named 'config.conf'" << std::endl;
		return 0;
	}
	return 1;
}

int main(int argc, char *argv[])
{
	if (checkArguments(argc, argv) == 0)
		return 1;
	std::string filename = argv[1];
	try {
		ConfigParser parser(filename);
		ServerConfig config = parser.parse();
		// std::cout << "Config parsed successfully:" << std::endl;
		// std::cout << "Port: " << config.port << std::endl;
		// std::cout << "Host: " << config.host << std::endl;
		// std::cout << "Root: " << config.root << std::endl;
		// std::cout << "Index: " << config.index << std::endl;
		// std::cout << "Client Max Body Size: " << config.client_max_body_size << std::endl;
		
		// TODO: Use the parsed config to initialize and run the server

		Server webserver(MYPORT); // <--------- This needs to be config (ans not my port), but for testing we will use the defined MYPORT
		// webserver.init();
		// webserver.run();
	} catch (const std::exception& e) {
		std::cout << "Error: " << e.what() << std::endl;
	
	}	
	return 0;
}