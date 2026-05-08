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
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

HttpRequest::HttpRequest() {}

// HttpRequest::HttpRequest(const std::string& req_message)
// {
// 	// this->err_status_ = _parser(req_message);
// }

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

void HttpRequest::print_parsed()
{
	std::cout << "status:" << err_status_ << '\n';
	std::cout << "method_:" << method_ << '\n';
	std::cout << "Content_length_:" << Content_length_ << '\n';
	std::cout << "method_str_:" << method_str_ << '\n';
	std::cout << "uri_:" << uri_ << '\n';
	std::cout << "http_version_:" << http_version_ << '\n';
	std::map< std::string, std::string >::iterator it = headers_.begin();
	for (; it != this->headers_.end(); ++it)
	{
		std::cout << it->first << ":" << it->second << '\n';
	}

	std::cout << "body_:" << body_ << '\n';
}

// void HttpRequest::parse_request_line(const std::string& raw)
// {
// 	size_t pos = pos_;
// 	for (; pos < raw.size(); ++pos)
// 	{
// 		switch (state_)
// 		{
// 			case sw_start:
// 			{
// 				size_t p = raw.find(' ');
// 				if (p == std::string::npos && raw.size() > 7)
// 				{
// 					err_status_ = HTTP_BAD_REQUEST;
// 					return;
// 				}
// 			}
// 		}
// 	}
// }

void HttpRequest::parseHeaderLine(const std::string& header_line)
{
	size_t p = header_line.find(':');
	if (p == std::string::npos)
	{
		err_status_ = HTTP_BAD_REQUEST;
		state_ = sw_done;
		return;
	}
	std::string name = header_line.substr(0, p);

	size_t      start_value = p + 1;
	size_t      end_line = header_line.find(CRLF);
	std::string value = header_line.substr(start_value, end_line - start_value);
	headers_[name] = value;
}

void HttpRequest::parse(const std::string& raw)
{
	// state_t state = state_;

	std::cout << "parse\n";
	size_t pos = 0;
	while (pos < raw.size())
	{
		switch (state_)
		{
			case sw_start:
			{
				size_t p = raw.find(' ', pos);
				if (p == std::string::npos)
					return;
				this->method_str_ = raw.substr(0, p);
				pos = p + 1;
				state_ = sw_uri;
				break;
			}
			case sw_uri:
			{
				size_t p = raw.find(' ', pos);
				if (p == std::string::npos)
					return;
				this->uri_ = raw.substr(pos, p - pos);
				if (this->uri_.size() > 4048) // in HTTP/1.1 version 8000 octet
				{
					err_status_ = HTTP_REQUEST_URI_TOO_LARGE;
					state_ = sw_done;
					return;
				}
				pos = p + 1;
				state_ = sw_version;
				break;
			}
			case sw_version:
			{
				size_t p = raw.find(CRLF, pos);
				if (p == std::string::npos)
					return;
				this->http_version_ = raw.substr(pos, p - pos);
				pos = p + 2;
				state_ = sw_headers;
				break;
			}
			case sw_headers:
			{
				size_t p = raw.find(CRLF CRLF, pos);
				if (p == std::string::npos)
				{
					size_t pp = raw.find(CRLF, pos);
					if (pp == std::string::npos)
						return;

					std::string header_line = raw.substr(pos, pp - pos);
					std::cout << "HEADER_LINE:" << header_line;
					parseHeaderLine(header_line);
					pos += 2;
					return;
				}
				state_ = sw_done;
				this->body_ = raw.substr(pos, p - pos);
				break;
			}
			case sw_done: return;
		}
	}
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
