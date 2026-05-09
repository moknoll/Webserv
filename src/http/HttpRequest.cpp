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
#include <cstring>
#include <iostream>
#include <string>

HttpRequest::HttpRequest()
    : err_status_(HTTP_OK), content_length_(0), current_pos_(0),
      state_(sw_start)
{
}

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
	std::cout << "Content_length_:" << content_length_ << '\n';
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

void HttpRequest::parseHeaderLine(const std::string& header_line)
{
	size_t p = header_line.find(':');
	if (p == std::string::npos)
	{
		fail(HTTP_BAD_REQUEST);
		return;
	}
	std::string name = header_line.substr(0, p);

	size_t      start_value = p + 1;
	size_t      end_line = header_line.find(CRLF);
	std::string value = header_line.substr(start_value, end_line - start_value);
	headers_[name] = ws::strip(value);
	if (name == "Content-Lenght")
		content_length_ = ws::stosize(headers_[name]);
	if (name == "Host")
		host_ = headers_[name];
}

void HttpRequest::parse(const std::string& raw)
{
	size_t pos = current_pos_;
	while (pos < raw.size())
	{
		switch (state_)
		{
			case sw_done: return;
			case sw_start:
			{
				std::string::size_type p = raw.find(' ', pos);
				if (p == std::string::npos)
				{
					if (raw.size() > MAX_METHOD_LEN)
						fail(HTTP_BAD_REQUEST);
					return;
				}
				this->method_str_ = raw.substr(0, p);
				pos = p + 1;
				state_ = sw_uri;
				break;
			}
			case sw_uri:
			{
				std::string::size_type p = raw.find(' ', pos);
				if (p == std::string::npos)
				{
					if (raw.size() - MAX_METHOD_LEN > MAX_METHOD_LEN)
						fail(HTTP_REQUEST_URI_TOO_LARGE);
					current_pos_ = pos;
					return;
				}

				this->uri_ = raw.substr(pos, p - pos);
				pos = p + 1;
				state_ = sw_version;
				break;
			}
			case sw_version:
			{
				std::string::size_type p = raw.find(CRLF, pos);
				if (p == std::string::npos)
				{
					if (raw.size() - pos > std::strlen(HTTP_VERSION))
						fail(HTTP_BAD_REQUEST);
					current_pos_ = pos;
					return;
				}
				this->http_version_ = raw.substr(pos, p - pos);
				pos = p + 2;
				state_ = sw_headers;
				break;
			}
			case sw_headers:
			{
				std::string::size_type p = raw.find(CRLF, pos);
				if (p == std::string::npos)
				{
					if (raw.size() > MAX_HEADER_SIZE)
						fail(HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE);
					current_pos_ = pos;
					return;
				}

				if (raw.compare(p, 4, CRLF CRLF) == 0)
				{
					state_ = sw_done;
					current_pos_ = pos + 4;
					this->body_ = raw.substr(pos, p - pos);
					return;
				}
				std::string header_line = raw.substr(pos, p - pos);
				parseHeaderLine(header_line);
				pos = p + 2;
				break;
			}
		}
	}
	current_pos_ = pos;
}

void HttpRequest::fail(int status)
{
	err_status_ = status;
	state_ = sw_done;
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
