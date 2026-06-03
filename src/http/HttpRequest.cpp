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
#include "BodyStream.hpp"
#include "constants.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

HttpRequest::HttpRequest() :
        err_status_(HTTP_OK),
        content_length_(0),
        chunked_(false),
        multipart_(false),
        recv_bytes_(0),
        state_(sw_start),
        body_data()
{
}

HttpRequest::HttpRequest(const HttpRequest& other) :
        err_status_(other.err_status_),
        content_length_(other.content_length_),
        method_(other.method_),
        uri_(other.uri_),
        http_version_(other.http_version_),
        body_(other.body_),
        headers_(other.headers_),
        chunked_(other.chunked_),
        boundary_(other.boundary_),
        recv_bytes_(other.recv_bytes_),
        state_(other.state_),
        body_data(other.body_data)
{
}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset()
{
	err_status_ = HTTP_OK;
	content_length_ = 0;
	method_.clear();
	uri_.clear();
	http_version_.clear();
	body_.clear();
	headers_.clear();
	chunked_ = false;
	state_ = sw_start;
	boundary_.clear();
	multipart_ = false;
	recv_bytes_ = 0;
	body_data.reset();
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
	if (this != &other)
	{
		err_status_ = other.err_status_;
		this->uri_ = other.uri_;
		this->http_version_ = other.http_version_;
		this->body_ = other.body_;
		this->headers_ = other.headers_;
		state_ = other.state_;
	}
	return *this;
}

std::string extractBoundary(const std::string& content_type)
{
	std::string::size_type p = content_type.find("boundary=");
	if (p == std::string::npos)
		return "";
	return content_type.substr(p + 9);
}

void HttpRequest::parseHeaders(const std::string& headers)
{
	size_t pos = 0;
	while (pos < headers.size())
	{
		std::string::size_type p = headers.find(CRLF, pos);
		if (p == std::string::npos)
		{
			fail(HTTP_BAD_REQUEST);
			return;
		}
		parseHeaderLine(headers.substr(pos, p - pos));
		pos = p + 2;
	}

	std::map< std::string, std::string >::iterator it = headers_.begin();
	for (; it != headers_.end(); ++it)
	{
		if (it->first == "Content-Length")
			content_length_ = ws::stosize(it->second);
		if (it->first == "Transfer-Encoding" && it->second == "chunked")
		{
			body_data.setChunked(true);
			chunked_ = true;
		}
		if (it->first == "Content-Type"
		    && it->second.find("multipart/form-data; boundary=") == 0)
		{
			boundary_ = extractBoundary(it->second);
			multipart_ = true;
			body_data.setBoundary(boundary_);
		}
	}
	if (headers_.find("Content-Length") != headers_.end() && chunked_)
		fail(HTTP_BAD_REQUEST);
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

	p += 1;

	std::string value = header_line.substr(p + 1);
	headers_[name] = ws::strip(value);
}

bool HttpRequest::isValidMethod(const std::string& method)
{
	if (method == "GET" || method == "POST" || method == "DELETE")
		return true;
	if (method == "HEAD" || method == "PUT" || method == "CONNECT"
	    || method == "OPTIONS" || method == "TRACE" || method == "PATCH"
	    || method == "MOVE")
	{
		fail(HTTP_NOT_IMPLEMENTED);
		return false;
	}
	fail(HTTP_BAD_REQUEST);
	return false;
}

void HttpRequest::parceChunked(std::string& raw_data)
{
	size_t      pos = 0;
	std::string body;
	while (true)
	{
		std::string::size_type p = raw_data.find(CRLF);
		if (p == std::string::npos)
			break;
		std::string chunk_size = raw_data.substr(0, p);
		size_t      n = std::strtol(chunk_size.c_str(), NULL, 16);

		if (n == 0)
		{
			state_ = sw_done;
			break;
		}
		pos = p + 2;
		if (pos + n + 2 > raw_data.size())
		{
			break;
		}

		body.append(&raw_data[pos], n);
		pos += n + 2;
		raw_data.erase(0, pos);
	}

	body_ = body;
}

void HttpRequest::parse(std::string& raw_data)
{
	static size_t CRLF_LEN = 2;

	while (true)
	{
		switch (state_)
		{
			case sw_done: return;
			case sw_almost_done:
			{
				recv_bytes_ += raw_data.size();
				if (chunked_)
				{
					parceChunked(raw_data);
				}
				else
				{
					body_ = raw_data;
					raw_data.clear();
					if (recv_bytes_ >= content_length_)
						state_ = sw_done;
				}
				// body_data.parse(raw_data);
				// if (body_data.eof())
				// state_ = sw_done;
				// if (recv_bytes >= content_length_)
				// state_ = sw_done;
				return;
			}
			case sw_start:
			{
				std::string::size_type p = raw_data.find(' ');
				if (p == std::string::npos)
				{
					if (raw_data.size() > MAX_METHOD_LEN)
						fail(HTTP_BAD_REQUEST);
					return;
				}
				this->method_ = raw_data.substr(0, p);
				p += 1;
				raw_data.erase(0, p);
				state_ = sw_uri;
				isValidMethod(method_);
				break;
			}
			case sw_uri:
			{
				std::string::size_type p = raw_data.find(' ');
				if (p == std::string::npos)
				{
					if (raw_data.size() - MAX_METHOD_LEN > MAX_URL_LEN)
						fail(HTTP_URI_TOO_LONG);
					return;
				}

				this->uri_ = raw_data.substr(0, p);
				p += 1;
				raw_data.erase(0, p);
				state_ = sw_version;
				break;
			}
			case sw_version:
			{
				std::string::size_type p = raw_data.find(CRLF);
				if (p == std::string::npos)
				{
					if (raw_data.size() > std::strlen(HTTP_VERSION))
						fail(HTTP_BAD_REQUEST);
					return;
				}
				this->http_version_ = raw_data.substr(0, p);
				raw_data.erase(0, p + CRLF_LEN);
				state_ = sw_headers;
				break;
			}
			case sw_headers:
			{
				std::string::size_type p = raw_data.find(CRLF CRLF);
				if (p == std::string::npos)
				{
					if (raw_data.size() > MAX_HEADER_SIZE)
						fail(HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE);
					return;
				}

				parseHeaders(raw_data.substr(0, p + CRLF_LEN));
				raw_data.erase(0, p + CRLF_LEN + CRLF_LEN);
				state_ = method_ != "POST" ? sw_done : sw_almost_done;
			}
			break;
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

size_t	HttpRequest::getReceivedBytes() const
{
	return this->recv_bytes_;

}

std::string HttpRequest::getHeader(const std::string& name) const
{
	if (headers_.find(name) != headers_.end())
		return headers_.at(name);

	return "";
}

std::string HttpRequest::getBoundary() const
{
	return this->boundary_;
}

bool HttpRequest::isAlmostDone() const
{
	return state_ == sw_almost_done;
}

bool HttpRequest::isComplete() const
{
	return state_ == sw_done;
}

void HttpRequest::setStatus(int status)
{
	this->err_status_ = status;
}

bool HttpRequest::isChunked() const
{
	return this->chunked_;
}

bool HttpRequest::isMultipart() const
{
	return this->multipart_;
}

BodyStream HttpRequest::getbodyStream() const
{
	return body_data;
}
