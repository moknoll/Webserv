/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:19:45 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:19:46 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include "../lib/ws.hpp"
#include "constants.hpp"
#include <cstddef>
#include <ctime>
#include <string>

HttpResponse::HttpResponse() {}

HttpResponse::HttpResponse(const int status) : status_(status) {}

HttpResponse::HttpResponse(const HttpResponse& other)
    : status_(other.status_), status_line_(other.status_line_),
      headers_(other.headers_), body_(other.body_)
{
}

HttpResponse::~HttpResponse() {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other)
{
	if (this != &other)
	{
		status_ = other.status_;
		status_line_ = other.status_line_;
		headers_ = other.headers_;
		body_ = other.body_;
	}
	return *this;
}

/*
 * HTTP/1.1 200 OK\r\n
 * Server: webserv\r\n
 * Date: Fri, 21 Apr 2026 12:52:34 GMT\r\n
 * Content-Length: 555\r\n
 * Content-Type: text/html\r\n
 *
 * <html><body>Some text</body></html>
 */

std::string HttpResponse::buildResponse() const
{
	std::string res;

	res = "HTTP/1.1 ";
	res += getStatusMsg(status_);
	res += CRLF;

	std::map< std::string, std::string >::const_iterator it;

	for (it = headers_.begin(); it != headers_.end(); ++it)
		res += it->first + ": " + it->second + CRLF;

	res += CRLF;
	res += body_;

	return res;
}

void HttpResponse::setStatus(int status)
{
	this->status_ = status;
}

void HttpResponse::setHeader(const std::string& header_name,
                             const std::string& v)
{
	headers_[header_name] = v;
}

void HttpResponse::setFullResponse(const std::string& content,
                                   const std::string& content_type)
{
	setHeader("Content-Type", content_type);
	setBody(content);
	setHeader("Date", getHttpTime());
	setHeader("Server", SERVER_NAME_STR);
}

void HttpResponse::setBody(const std::string& content)
{
	body_ = content;
	setHeader("Content-Length", ws::to_string(content.size()));
}

const char* HttpResponse::getStatusMsg(int status) const
{
	switch (status)
	{
		case HTTP_OK:                       return "200 OK";
		case HTTP_CREATED:                  return "201 Created";
		case HTTP_NO_CONTENT:               return "204 No Content";
		case HTTP_PARTIAL_CONTENT:          return "206 Partial Content";
		case HTTP_MOVED_PERMANENTLY:        return "301 Moved Permanently";
		case HTTP_MOVED_TEMPORARILY:        return "302 Moved Temporarily";
		case HTTP_NOT_MODIFIED:             return "304 Not Modified";
		case HTTP_BAD_REQUEST:              return "400 Bad Request";
		case HTTP_FORBIDDEN:                return "403 Forbidden";
		case HTTP_NOT_FOUND:                return "404 Not Found";
		case HTTP_NOT_ALLOWED:              return "405 Method Not Allowed";
		case HTTP_REQUEST_TIME_OUT:         return "408 Request Timeout";
		case HTTP_CONFLICT:                 return "409 Conflict";
		case HTTP_REQUEST_ENTITY_TOO_LARGE: return "413 Content Too Large";
		case HTTP_REQUEST_URI_TOO_LARGE:    return "414 URI Too Long";
		case HTTP_INTERNAL_SERVER_ERROR:    return "500 Internal Server Error";
		case HTTP_NOT_IMPLEMENTED:          return "501 Not Implemented";
		case HTTP_BAD_GATEWAY:              return "502 Bad Gateway";
		case HTTP_SERVICE_UNAVAILABLE:      return "503 Service Unavailable";
		case HTTP_GATEWAY_TIME_OUT:         return "504 Gateway Timeout";
		default:                            return "500 Internal Server Error";
	}
}

std::string HttpResponse::getErrorPage(int err_status) const
{
	std::string ret = "<html><head><title>";

	ret += getStatusMsg(err_status);
	ret += "</title></head><body><center><h1>";
	ret += getStatusMsg(err_status);
	ret +=
	    "</h1></center><hr><center>" SERVER_NAME_STR "</center></body></html>";

	return ret;
}

const std::string HttpResponse::getHttpTime() const
{
	char        buf[100];

	std::time_t now = std::time(NULL);
	if (now == -1)
		return "";

	std::tm*    tm_info = std::gmtime(&now); // For getting time in GMT

	// %a day		(Fri)
	// %d month	day	(1)
	// %b month		(May)
	// %Y year		(2026)
	// %H:%M:%S		(Time)
	const char* fmt = "%a, %d %b %Y %H:%M:%S GM";

	if (!tm_info || std::strftime(buf, sizeof(buf), fmt, tm_info) == 0)
		return "";

	return buf;
}
