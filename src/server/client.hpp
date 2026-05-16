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

#include "../ConfigParser/ServerConfig.hpp"
#include "../http/HttpHandler.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <iostream>
#include <string>

class Client
{
  public:
	enum HTTP_STATE
	{
		HTTP_READING_STATE = 0,
		HTTP_WRITING_STATE,
		HTTP_KEEPALIVE_STATE,
		HTTP_CLOSE
	};

	int         clientFd;       // file descriptor for the client socket
	std::string requestBuffer;  // buffer to store incoming request data
	std::string responseBuffer; // buffer to store response data to be sent back
	                            // to the client
	bool        isReadyToWrite; // flag to indicate if the full request has been
	                            // received
	ServerConfig* config;
	HttpHandler   handler;
	HttpRequest   request;
	HttpResponse  response;

	Client(int clientFd, ServerConfig* cfg);
	//     : clientFd(clientFd), isReadyToWrite(false), config(cfg),
	//       handler(config[0]), request(), response()
	// {
	// 	std::cout << "CALL Client contructor\n";
	// 	std::cout << request.getRequestStatus();
	//
	// }

	Client(const Client& other);
	~Client() {}

	void clearBuffers()
	{
		requestBuffer.clear();
		responseBuffer.clear();
		isReadyToWrite = false;
	}
};
