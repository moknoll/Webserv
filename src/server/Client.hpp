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
#include <cstddef>
#include <string>

class Client
{
  private:
	const ServerConfig& config_;
	int                 fd_;
	std::string         recv_buffer_;
	std::string         send_buffer_;
	bool                keep_alive_;

	HttpRequest         request;
	HttpResponse        response;
	HttpHandler         handler;

	Client();
	Client(const Client& other);
	Client& operator=(const Client& other);

  public:
	Client(int fd, const ServerConfig& config);
	~Client();

	void        reset();

	void        processRequest(); // buildResponse()
	std::string serialize();
	void        appendRecvBuffer(const char* buffer, size_t size);

	std::string getResponseBuffer() const; // ??????????
	std::string getRequestBuffer() const;  // ???????????
	bool        isRequestComplete() const;
	bool        isKeepAlive() const;
	int         getClientFd() const;
	void        setSendBuffer(const std::string& response); // ???????
	void        setRecvBuffer(const std::string& request);  // ??????????

	static const Location* FindMatchingUri(const std::string&  uri,
	                                       const ServerConfig& cfg);
};
