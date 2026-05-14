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
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

HttpRequest::HttpRequest()
    : err_status_(HTTP_OK), content_length_(0), chunked(false), temp_file(""),
      state_(sw_start)
{
}

HttpRequest::HttpRequest(const HttpRequest& other)
    : err_status_(other.err_status_), content_length_(other.content_length_),
      method_(other.method_), uri_(other.uri_),
      http_version_(other.http_version_), request_line_(other.request_line_),
      host_(other.host_), body_(other.body_), headers_(other.headers_),
      chunked(other.chunked), state_(other.state_)
{
}

HttpRequest::~HttpRequest() {}

void HttpRequest::clear()
{
	err_status_ = HTTP_OK;
	content_length_ = 0;
	method_.clear();
	uri_.clear();
	http_version_.clear();
	request_line_.clear();
	host_.clear();
	body_.clear();
	headers_.clear();
	chunked = false;
	temp_file.clear();
	state_ = sw_start;
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
	if (this != &other)
	{
		err_status_ = other.err_status_;
		this->uri_ = other.uri_;
		this->http_version_ = other.http_version_;
		this->request_line_ = other.request_line_;
		this->host_ = other.host_;
		this->body_ = other.body_;
		this->headers_ = other.headers_;
		state_ = other.state_;
	}
	return *this;
}

void HttpRequest::parseHeaderLine(const std::string& header_line)
{
	std::string::size_type p = header_line.find(':');
	if (p == std::string::npos)
	{
		fail(HTTP_BAD_REQUEST);
		return;
	}
	std::string name = header_line.substr(0, p);
	if (name.find(' ') != std::string::npos)
	{
		fail(HTTP_BAD_REQUEST);
		return;
	}

	std::string value = header_line.substr(p + 1);
	headers_[name] = ws::strip(value);

	if (name == "Content-Length")
		content_length_ = ws::stosize(headers_[name]);
	else if (name == "Transfer-Encoding" && headers_[name] == "chunked")
		chunked = true;
	if (name == "Host")
		host_ = headers_[name];
}

void HttpRequest::parseBody(const std::string& raw)
{
	if (temp_file == "")
		temp_file = "/tmp/wstemp_" + ws::randString();

	std::ofstream fout(temp_file.c_str(), std::ios::binary | std::ios::app);
	if (!fout.is_open())
		return;
	std::cout << "IN: parsbody\n";

	if (!body_.empty())
		fout.write(body_.c_str(), body_.size());
	fout.write(raw.c_str(), raw.size());
	fout.close();
}

bool HttpRequest::isValidMethod(const std::string& method)
{
	if (method == "GET" || method == "POST" || method == "DELETE")
		return true;
	if (method == "HEAD" || method == "PUT" || method == "CONNECT"
	    || method == "OPTIONS" || method == "TRACE" || method == "PATCH"
	    || method == "MOVE")
	{
		fail(HTTP_NOT_ALLOWED);
		return false;
	}
	fail(HTTP_BAD_REQUEST);
	return false;
}

// void parseBody(const std::string& raw) {}

bool HttpRequest::isAlmostDone() const
{
	if (state_ == sw_almost_done)
		return true;
	return false;
}

bool HttpRequest::isComplete() const
{
	// if (err_status_ != HTTP_OK)
	// 	return true;
	if (state_ == sw_done)
		return true;
	return false;
}

void HttpRequest::setStatus(int status)
{
	this->err_status_ = status;
}

bool HttpRequest::isChunked() const
{
	return this->chunked;
}

std::string extractBoundary(const std::string& content_type)
{
	std::string::size_type p = content_type.find("boundary=");
	if (p == std::string::npos)
		return "";
	return "--" + content_type.substr(p + 9);
}

void HttpRequest::parse(std::string& raw)
{
	static size_t CRLF_LEN = 2;
	size_t        pos = 0;

	while (pos < raw.size())
	{
		switch (state_)
		{
			case sw_done: return;
			case sw_almost_done:
			{
				body_ = raw;
				raw.clear();
				return;
			}
			case sw_start:
			{
				std::string::size_type p = raw.find(' ', pos);
				if (p == std::string::npos)
				{
					if (raw.size() > MAX_METHOD_LEN)
						fail(HTTP_BAD_REQUEST);
					return;
				}
				this->method_ = raw.substr(0, p);
				pos = p + 1;
				state_ = sw_uri;
				isValidMethod(method_);
				break;
			}
			case sw_uri:
			{
				std::string::size_type p = raw.find(' ', pos);
				if (p == std::string::npos)
				{
					if (raw.size() - MAX_METHOD_LEN > MAX_METHOD_LEN)
						fail(HTTP_REQUEST_URI_TOO_LARGE);
					raw.erase(0, pos);
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
					raw.erase(0, pos);
					return;
				}
				this->http_version_ = raw.substr(pos, p - pos);
				pos = p + CRLF_LEN;
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
					raw.erase(0, pos);
					return;
				}

				parseHeaderLine(raw.substr(pos, p - pos));

				if (raw.compare(p, 4, CRLF CRLF) == 0)
				{
					state_ = sw_almost_done;
					pos = p + CRLF_LEN + CRLF_LEN;
					this->body_ = raw.substr(pos);
					raw.clear();
					return;
				}
				pos = p + CRLF_LEN;
				break;
			}
		}
	}
}

void HttpRequest::fail(int status)
{
	err_status_ = status;
	state_ = sw_done;
}

std::string HttpRequest::getMethod() const
{
	return this->method_;
}

std::string HttpRequest::getURI() const
{
	return this->uri_;
}

const std::string HttpRequest::getbody() const
{
	return body_;
}

int HttpRequest::getRequestStatus() const
{
	return this->err_status_;
}

size_t HttpRequest::getContentLenght() const
{
	return content_length_;
}

std::string HttpRequest::getHeader(const std::string& name) const
{
	if (headers_.find(name) != headers_.end())
		return headers_.at(name);

	return "";
}
