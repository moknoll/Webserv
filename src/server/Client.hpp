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

#include "../cgi/cgi.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <cstddef>
#include <ctime>
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

	std::string cgi_output_buf;
	bool        writeCgiInput(int fd);
	bool        readCgiOutput_(int fd);
	void        buildCGIResponse();
	bool        CGIProcessFinished();
	void        tryFinalizeCGI_();

	void        reset();

	void        processRequest(); // buildResponse()
	std::string serialize();
	void        parseRequest(const char* buffer, size_t size);

	std::string getResponseBuffer() const;                  // ??????????
	std::string getRequestBuffer() const;                   // ???????????
	bool        isRequestComplete() const;
	bool        isKeepAlive() const;
	void        setSendBuffer(const std::string& response); // ???????
	void        setRecvBuffer(const std::string& request);  // ??????????
	void        setState(enum STATE state);

	int         getClientFd() const;
	int         getHttpState() const;
	CgiContext  getCGIContext() const;
	time_t      getLastActivity() const;
	void        updateLastActivity();

  private:
	static const size_t MAX_CGI_BUFFER;

	const ServerConfig& config_;
	int                 fd_;
	std::string         recv_buffer_;
	std::string         send_buffer_;
	bool                keep_alive_;
	time_t              last_activity_;
	STATE               state_;
	CgiContext          cgi_ctx_;

	HttpRequest         request;
	HttpResponse        response;
	// HttpHandler         handler;
	// time_t last_activity_;

	Client();
	Client(const Client& other);
	Client& operator=(const Client& other);

	void    safeClosePipeFds_();
};

