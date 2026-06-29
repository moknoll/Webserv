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

// #include "ConfigParser/ConfigParser.hpp"
#include "ConfigParser/ConfigParser.hpp"
#include "ConfigParser/ServerConfig.hpp"
#include "logger/Logger.hpp"
#include "server/Core.hpp"

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

void printConfig(std::vector< ServerConfig > cfg)
{
	for (size_t i = 0; i < cfg.size(); ++i)
	{
		std::cout << "SERVER CONFIG\n";
		std::cout << "host: " << cfg[i].host << '\n';
		std::cout << "port: " << cfg[i].port << '\n';
		std::cout << "server_name: " << cfg[i].server_name << '\n';
		std::cout << "root: " << cfg[i].root << '\n';
		std::cout << "index: " << cfg[i].index << '\n';
		std::map< int, std::string >::iterator it = cfg[i].error_pages.begin();
		for (; it != cfg[i].error_pages.end(); ++it)
			std::cout << "error_pages: " << it->first << " -> " << it->second << '\n';
		std::cout << "client_max_body_size: " << cfg[i].client_max_body_size
		          << '\n';

		std::cout
		    << "-------------------------------------------------------\n";
		for (size_t j = 0; j < cfg[i].locations.size(); ++j)
		{
			std::cout << "________Location________\n";
			std::cout << "LOC_path: " << cfg[i].locations[j].path << '\n';
			std::cout << "LOC_root: " << cfg[i].locations[j].root << '\n';
			std::cout << "LOC_index: " << cfg[i].locations[j].index << '\n';
			std::cout << "LOC_autoindex: " << cfg[i].locations[j].autoindex
			          << '\n';
			std::cout << "LOC_client_max_body_size: "
			          << cfg[i].locations[j].client_max_body_size << '\n';
			// std::cout << "allowed_methods: "
			//           << ss[i].locations[j].allowed_methods[0] << '\n';
			std::cout << "LOC_upload path: " << cfg[i].locations[j].upload_path
			          << '\n';
			std::cout << "LOC_cgi_extension: "
			          << cfg[i].locations[j].cgi_extension << '\n';
			std::cout << "LOC_cgi_path: " << cfg[i].locations[j].cgi_path
			          << '\n';
			std::cout << "LOC_has redirect: "
			          << cfg[i].locations[j].has_redirect << '\n';

			std::cout << "LOC_has_cgi: " << cfg[i].locations[j].has_cgi << '\n';
		}

		std::cout << "======================================================\n";
	}
}

int checkArguments(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: ./websrerv <config_file>" << std::endl;
		return 0;
	}
	(void)argv;
	// if (std::string(argv[1]) != "config.conf")
	// {
	// 	std::cout << "Error: Config file must be named 'config.conf'"
	// 	          << std::endl;
	// 	return 0;
	// }
	return 1;
}

ServerConfig
getConfig(const std::string& host, int port, const std::string& s_name)
{
	// listen 0.0.0.0:8080
	ServerConfig server;
	server.host = host;
	server.port = port;
	server.server_name = s_name;

	// Error pages
	server.error_pages[404] = "./www/404.html";
	server.error_pages[500] = "./www/500.html";

	// Location: /
	Location root_loc;
	root_loc.path = "/";
	root_loc.root = "./www";
	root_loc.index = "index.html";
	root_loc.error_pages[404] = "./www/404.html";
	root_loc.error_pages[500] = "./www/500.html";
	root_loc.allowed_methods.push_back("GET"); // = {"GET", "POST"};
	root_loc.allowed_methods.push_back("POST");
	// root_loc.client_max_body_size = 1024 * 1024; // 1MB
	root_loc.client_max_body_size = 10;
	root_loc.autoindex = false;
	root_loc.redirect = std::make_pair(-1, "");

	// Location: /upload
	Location upload_loc;
	upload_loc.path = "/upload";
	upload_loc.root = "./www/uploads";
	upload_loc.index = "";
	upload_loc.allowed_methods.push_back("GET");  // = {"GET"};
	upload_loc.allowed_methods.push_back("POST"); // = {"POST"};
	upload_loc.client_max_body_size = 2000000000; // 2GB
	upload_loc.upload_path = "./www/uploads";
	upload_loc.redirect = std::make_pair(-1, "");

	// Location: /old (redirect)
	Location redirect_loc;
	redirect_loc.path = "/old";
	redirect_loc.root = "";
	redirect_loc.index = "";
	redirect_loc.allowed_methods.push_back("GET");             // = {"GET"};
	redirect_loc.client_max_body_size = 0;
	redirect_loc.has_redirect = true;
	redirect_loc.redirect = std::make_pair(301, "google.com"); // 301 -> /new

	// Location: /autoindex
	Location autoindex_loc;
	autoindex_loc.path = "/autoin";
	autoindex_loc.root = "./src";
	autoindex_loc.autoindex = true;
	autoindex_loc.index = "";
	autoindex_loc.allowed_methods.push_back("GET");  // = {"GET"};
	autoindex_loc.redirect = std::make_pair(-1, ""); // 301 ->
	//
	//
	Location cgi_loc;
	cgi_loc.path = "/cgi-bin";
	cgi_loc.root = "./www";
	cgi_loc.autoindex = false;
	cgi_loc.cgi_extension = "py";
	cgi_loc.cgi_path = "/usr/bin/python3";
	cgi_loc.has_cgi = true;
	cgi_loc.allowed_methods.push_back("GET");  // = {"GET"};
	cgi_loc.allowed_methods.push_back("POST");
	cgi_loc.allowed_methods.push_back("DELETE");
	cgi_loc.redirect = std::make_pair(-1, ""); // 301 ->

	// server.locations = {root_loc, upload_loc, redirect_loc};
	server.locations.push_back(root_loc);
	server.locations.push_back(upload_loc);
	server.locations.push_back(redirect_loc);
	server.locations.push_back(autoindex_loc);
	server.locations.push_back(cgi_loc);

	return server;
}

std::vector< ServerConfig > setupConfigDefaultToTest()
{
	ServerConfig                s1 = getConfig("0.0.0.0", 8081, "example.com");
	ServerConfig                s2 = getConfig("0.0.0.0", 8082, "example2.com");
	std::vector< ServerConfig > configs;

	configs.push_back(s1);
	configs.push_back(s2);
	return configs;
}

int main(int argc, char* argv[])
{
	if (checkArguments(argc, argv) == 0)
		return 1;
	std::string filename = argv[1];
	(void) argc;
	(void) argv;
	Logger::getInstance().setLevel(LOG_DEBUG);

	try
	{
		// std::vector< ServerConfig > configs = setupConfigDefaultToTest();
		ConfigParser cfg_parser(filename);
		cfg_parser.parse();
		printConfig(cfg_parser.getServers());
		std::vector< ServerConfig > configs = cfg_parser.getServers();

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

		// Server webserver(
		//     configs); // <--------- This needs to be config (ans not my
		//     port),
		// but for testing we will use the defined MYPORT
		Core webserv(configs);
		webserv.run();
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}
	return 0;
}
