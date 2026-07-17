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
#include "lib/ws.hpp"
#include "logger/Logger.hpp"
#include "server/Core.hpp"

#include <csignal>
#include <cstddef>
#include <cstdio>
// #include <ctime>
// #include <fcntl.h>
#include <iostream>
#include <map>     // ?????
#include <string>
#include <utility> // ?????
#include <vector>  // ?????

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
			std::cout << "error_pages: " << it->first << " -> " << it->second
			          << '\n';
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

bool g_running = true;

void signal_handler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM || sig == SIGQUIT)
		g_running = false;
}

int main(int argc, char* argv[])
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);

	if (argc != 2)
	{
		std::cout << "Usage: ./websrerv <config_file>" << std::endl;
		return 1;
	}

	std::string filename = argv[1];

	if (!ws::has_suffix(filename, ".conf"))
	{
		std::cerr << "Error: Config file extention must be `.conf'\n";
		return 1;
	}

	Logger::getInstance().setLevel(LOG_DEBUG);

	try
	{
		ConfigParser cfg_parser(filename);
		cfg_parser.parse();

		// printConfig(cfg_parser.getServers()); //
		// ???????????????????????????????????????????????????????
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

		Core webserv(configs);
		webserv.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return 0;
}
