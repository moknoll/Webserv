/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: moritzknoll <moritzknoll@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:17:41 by mknoll            #+#    #+#             */
/*   Updated: 2026/05/16 12:25:45 by moritzknoll      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../http/HttpHandler.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <string>

class Client
{
  private:
	int          fd_;
	std::string  recv_buffer_;
	std::string  send_buffer_;
	bool         request_complete_;

	HttpRequest  request;
	HttpResponse response;
	HttpHandler  handler;

	Client();
	Client(const Client& other);
	Client& operator=(const Client& other);

  public:
	Client(int fd, const ServerConfig& config);

	~Client();

	void        clearBuffers(); // ???????
	void        reset();
	void        requestComplete();

	void        processRequest(); // buildResponse()
	std::string serialize();
	void        appendBuffer(const std::string& buffer);

	std::string getResponseBuffer() const; // ??????????
	std::string getRequestBuffer() const;  // ???????????
	bool        isComplete() const;
	int         getClientFd() const;

	void        setResponseBuffer(const std::string& response); // ???????
	void        setRequestBuffer(const std::string& request);   // ??????????
};
