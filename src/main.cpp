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
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

bool g_running = true;

void signal_handler(int sig)
{
	if (sig == SIGINT || sig == SIGQUIT)
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

	Logger::getInstance().setLevel(LOG_INFO);

	try
	{
		ConfigParser cfg_parser(filename);
		cfg_parser.parse();
		std::vector< ServerConfig > configs = cfg_parser.getServers();

		LOG_INFO("Web Server Starting...");
		LOG_INFO("Press Ctrl+C to stop");
		Core webserv(configs);
		webserv.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return 0;
}
