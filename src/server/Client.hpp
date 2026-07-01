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

#include "../cgi/Cgi.hpp"
#include "../http/HttpHandler.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <cstddef>
#include <stack>
#include <string>

class Client
{
  public:
	Client(int fd, const ServerConfig& config);
	~Client();

	enum STATE
	{
		HTTP_RECV = 0,
		CGI_STATE,
		HTTP_SEND,
		HTTP_CLOSE
	};

	// CgiContext  cgi;
	int         cgi_pipe_in;
	int         cgi_pipe_out;
	int         cgi_pid;
	std::string cgi_output_buf;
	std::string cgi_input_buf_;
	int         request_body_fd_;
	bool        writeRequestBody();
	void        buildCGIResponse();
	bool        CGIProcessFinished();

	void        reset();

	void        processRequest(); // buildResponse()
	std::string serialize();
	void        parseRequest(const char* buffer, size_t size);

	std::string getResponseBuffer() const; // ??????????
	std::string getRequestBuffer() const;  // ???????????
	bool        isRequestComplete() const;
	bool        isKeepAlive() const;
	int         getClientFd() const;
	void        setSendBuffer(const std::string& response); // ???????
	void        setRecvBuffer(const std::string& request);  // ??????????
	void        setState(enum STATE state);

	int         getFdCGI_in() const;
	int         getFdCGI_out() const;
	int         getHttpState() const;

	static const Location* FindMatchingUri(const std::string&  uri,
	                                       const ServerConfig& cfg);

  private:
	const ServerConfig& config_;
	int                 fd_;
	std::string         recv_buffer_;
	std::string         send_buffer_;
	bool                keep_alive_;
	STATE               state_;

	HttpRequest         request;
	HttpResponse        response;
	HttpHandler         handler;

	Client();
	Client(const Client& other);
	Client&      operator=(const Client& other);

	HttpResponse makeStatusResponse(int status);
};
