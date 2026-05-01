/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:19:31 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:19:33 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "../lib/ws.hpp"
#include "constants.hpp"
#include <string>
#include <vector>

HttpRequest::HttpRequest(const std::string& req_message)
{
	this->err_status_ = _parser(req_message);
}

HttpRequest::HttpRequest(const HttpRequest& other)
    : method_(other.method_), uri_(other.uri_),
      http_version_(other.http_version_), request_line_(other.request_line_),
      host_(other.host_), body_(other.body_), headers_(other.headers_)
{
}

HttpRequest::~HttpRequest() {}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
	if (this != &other)
	{
		this->method_ = other.method_;
		this->uri_ = other.uri_;
		this->http_version_ = other.http_version_;
		this->request_line_ = other.request_line_;
		this->host_ = other.host_;
		this->body_ = other.body_;
		this->headers_ = other.headers_;
	}
	return *this;
}

int HttpRequest::_parser(const std::string& req_message)
{
	if (_parse_request_line(req_message) != HTTP_OK)
		return HTTP_BAD_REQUEST;

	size_t start_header = req_message.find("\r\n");
	size_t end_header = req_message.find("\r\n\r\n");
	if (start_header == std::string::npos || end_header == std::string::npos)
		return HTTP_BAD_REQUEST;

	std::string headers =
	    req_message.substr(start_header + 2, end_header - start_header + 2);

	_parse_headers(headers);

	this->body_ = req_message.substr(end_header + 4);

	return HTTP_OK;
}

int HttpRequest::_parse_request_line(const std::string& req_message)
{
	size_t end_start_line = req_message.find("\r\n");

	if (end_start_line == std::string::npos)
	{
		this->err_status_ = HTTP_BAD_REQUEST;
		return HTTP_BAD_REQUEST;
	}

	this->request_line_ = req_message.substr(0, end_start_line);

	std::vector< std::string > toks = ws::strSplit(request_line_, " ");
	if (toks.size() != 3)
	{
		this->err_status_ = HTTP_BAD_REQUEST;
		return HTTP_BAD_REQUEST;
	}

	if (toks[0] == "GET")
		method_ = HTTP_GET;
	else if (toks[0] == "POST")
		method_ = HTTP_POST;
	else if (toks[0] == "DELETE")
		method_ = HTTP_DELETE;
	else
	{
		method_ = HTTP_UNKNOWN;
		this->err_status_ = HTTP_BAD_REQUEST;
		return HTTP_BAD_REQUEST;
	}

	this->uri_ = ws::strip(toks[1]);
	this->http_version_ = ws::strip(toks[2]);

	return HTTP_OK;
}

void HttpRequest::_parse_headers(const std::string& header)
{
	std::vector< std::string >           toks = ws::strSplit(header, "\r\n");

	std::vector< std::string >::iterator it = toks.begin();
	for (; it != toks.end(); ++it)
	{
		size_t      colonChar_pos = it->find(":");
		std::string key = it->substr(0, colonChar_pos);
		std::string value = it->substr(colonChar_pos + 1);

		this->headers_.insert(std::make_pair(ws::strip(key), ws::strip(value)));
	}
	if (this->headers_.find("Host") == this->headers_.end())
	{
		this->err_status_ = HTTP_BAD_REQUEST;
		return;
	}
	this->host_ = this->headers_.at("Host");
}

std::string HttpRequest::getURI() const
{
	return this->uri_;
}

int HttpRequest::getRequestStatus() const
{
	return this->err_status_;
}

std::string HttpRequest::getHeader(const std::string& name) const
{
	if (headers_.find(name) != headers_.end())
		return headers_.at(name);

	return "";
}
