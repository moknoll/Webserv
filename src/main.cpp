 /* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: moritzknoll <moritzknoll@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:52 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/17 16:57:07 by moritzknoll      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser/ConfigParser.hpp"
#include "ConfigParser/ServerConfig.hpp"
#include "server/server.hpp"
#include "logger/Logger.hpp"

#include <iostream>

int checkArguments(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: ./websrerv <config_file>" << std::endl;
		return 0;
	}
	if (std::string(argv[1]) != "config.conf")
	{
		std::cout << "Error: Config file must be named 'config.conf'"
		          << std::endl;
		return 0;
	}
	return 1;
}

std::vector< ServerConfig > setupConfigDefaultToTest()
{
	std::vector< ServerConfig > configs;
	ServerConfig                config1, config2;
	config1.port = MYPORT;
	config1.host = "localhost";
	config1.root = "./www";
	config1.index = "index.html";
	config1.client_max_body_size = 1024 * 1024; // 1MB
	configs.push_back(config1);

	config2.port = MYPORT + 1;
	config2.host = "localhost";
	config2.root = "./www2";
	config2.index = "index.html";
	config2.client_max_body_size = 1024 * 1024; // 1MB
	configs.push_back(config2);
	return configs;
}

int main(int argc, char* argv[])
{
	// if (checkArguments(argc, argv) == 0)
	// 	return 1;
	// std::string filename = argv[1];
	(void) argc;
	(void) argv;
	Logger::getInstance().setLevel(DEBUG);
	
	try
	{
		ConfigParser parser(argv[1]);
        parser.parse();
		std::vector< ServerConfig > configs = parser.getServers();

		std::cout << "Parsed Configurations:" << std::endl;
		for (size_t i = 0; i < configs.size(); i++)
		{
			std::cout << "Server " << i + 1 << ":" << std::endl;
			std::cout << "  Port: " << configs[i].port << std::endl;
			std::cout << "  Host: " << configs[i].host << std::endl;
			std::cout << "  Root: " << configs[i].root << std::endl;
			std::cout << "  Index: " << configs[i].index << std::endl;
		}
		// TODO: Use the parsed config to initialize and run the server

		Server webserver(
		    configs); // <--------- This needs to be config (ans not my port),
		              // but for testing we will use the defined MYPORT
		webserver.init();
		webserver.run();
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}
	return 0;
}
