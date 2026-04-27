/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:17:41 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/27 11:25:57 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include "../ConfigParser/ServerConfig.hpp"

class Client
{
  public:
	int         clientFd;             // file descriptor for the client socket
	std::string requestBuffer;  // buffer to store incoming request data
	std::string responseBuffer; // buffer to store response data to be sent back
	                            // to the client
	bool        isReadyToWrite; // flag to indicate if the full request has been
	                            // received
	ServerConfig *config;

	Client(int clientFd, ServerConfig *cfg) : clientFd(clientFd), isReadyToWrite(false), config(cfg) {}
	~Client() {}

	void clearBuffers()
	{
		requestBuffer.clear();
		responseBuffer.clear();
		isReadyToWrite = false;
	}
};
