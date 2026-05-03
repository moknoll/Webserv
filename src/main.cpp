/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:52 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/14 13:16:43 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser/ConfigParser.hpp"
#include "ConfigParser/ServerConfig.hpp"
#include "logger/Logger.hpp"
#include "server/server.hpp"
#include "server/sockets.hpp"

#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <string>

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
	// listen 0.0.0.0:8080
	ServerConfig server;
	server.host = "0.0.0.0";
	server.port = 16000;
	// server.server_name = "example.com";

	// Error pages
	server.error_pages[404] = "./www/404.html";
	server.error_pages[500] = "./www/500.html";

	// Location: /
	Location root_loc;
	root_loc.path = "/";
	root_loc.root = "./www";
	root_loc.index = "index.htm";
	root_loc.error_pages[404] = "./www/404.html";
	root_loc.error_pages[500] = "./www/500.html";
	root_loc.allowed_methods.push_back("GET"); // = {"GET", "POST"};
	root_loc.allowed_methods.push_back("POST");
	root_loc.client_max_body_size = 1024 * 1024; // 1MB
	root_loc.autoindex = false;
	root_loc.redirect = "";

	// Location: /upload
	Location upload_loc;
	upload_loc.path = "/upload";
	upload_loc.root = "./www/uploads";
	upload_loc.index = "";
	upload_loc.allowed_methods.push_back("POST"); // = {"POST"};
	upload_loc.client_max_body_size = 10 * 1024 * 1024; // 10MB
	upload_loc.redirect = "";

	// Location: /old (редирект)
	Location redirect_loc;
	redirect_loc.path = "/old";
	redirect_loc.root = "";
	redirect_loc.index = "";
	redirect_loc.allowed_methods.push_back("GET"); // = {"GET"};
	redirect_loc.client_max_body_size = 0;
	redirect_loc.redirect = "/new"; // 301 -> /new or http://google.com
	
	// Location: /autoindex
	Location autoindex_loc;
	autoindex_loc.path = "/autoin";
	autoindex_loc.root = "./src";
	autoindex_loc.autoindex = true;
	autoindex_loc.index = "";
	autoindex_loc.allowed_methods.push_back("GET"); // = {"GET"};


	//server.locations = {root_loc, upload_loc, redirect_loc};
	server.locations.push_back(root_loc);
	server.locations.push_back(upload_loc);
	server.locations.push_back(redirect_loc);
	server.locations.push_back(autoindex_loc);
	
	std::vector< ServerConfig > configs;
/*
	ServerConfig                config1, config2;
	config1.port = MYPORT;
	config1.host = "server1.com";
	config1.root = "./www";
	config1.index = "index.html";
	config1.client_max_body_size = 1024 * 1024; // 1MB
	config1.locations;
	configs.push_back(config1);

	config2.port = MYPORT + 1;
	config2.host = "server2.org";
	config2.root = "./www2";
	config2.index = "index.html";
	config2.client_max_body_size = 1024 * 1024; // 1MB
	*/
	
	configs.push_back(server);
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
		std::vector< ServerConfig > configs = setupConfigDefaultToTest();

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
