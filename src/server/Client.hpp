/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:17:41 by mknoll            #+#    #+#             */
/*   Updated: 2026/06/27 11:24:31 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../http/HttpHandler.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <cstddef>
#include <string>
#include "../cgi/Cgi.hpp"

class Client
{
  public:
  	enum ProcState
    {
        CLIENT_RECV = 0,
        CLIENT_WAIT_CGI,
        CLIENT_SEND,
        CLIENT_CLOSE
    };
	Client(int fd, const ServerConfig& config);
	~Client();

	ProcState getProcState() const;
    bool hasRunningCgi() const;
    CgiContext& cgi();
    const CgiContext& cgi() const;
	
	
    bool beginCgiIfNeeded(); // startet cgi_, wenn Route CGI ist
    void finalizeCgiToResponse(); // buildResponse -> send_buffer_
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
	private:
		const ServerConfig& config_;
		int                 fd_;
		std::string         recv_buffer_;
		std::string         send_buffer_;
		bool                request_complete_;
		bool                keep_alive_;
		ProcState proc_state_;
    	bool cgi_active_;
    	CgiContext cgi_;

		HttpRequest         request;
		HttpResponse        response;
		HttpHandler         handler;

		Client();
		Client(const Client& other);
		Client& operator=(const Client& other);
};


/*
processRequest:
parse
wenn Request komplett:
wenn CGI-Route: beginCgiIfNeeded(), proc_state_ = CLIENT_WAIT_CGI, noch kein request_complete_
sonst wie bisher response bauen, send_buffer_ setzen, request_complete_ = true, proc_state_ = CLIENT_SEND
finalizeCgiToResponse:
response = cgi_.buildResponse()
send_buffer_ = response.toString()
request_complete_ = true
proc_state_ = CLIENT_SEND
*/