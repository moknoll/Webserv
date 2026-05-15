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
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

HttpResponse::HttpResponse() : status_(0), status_line_(""), body_("") {}

HttpResponse::HttpResponse(const int status) : status_(status) {}

HttpResponse::HttpResponse(const HttpResponse& other)
    : status_(other.status_), status_line_(other.status_line_),
      headers_(other.headers_), body_(other.body_)
{
}

HttpResponse::~HttpResponse() {}

void HttpResponse::reset()
{
	status_ = HTTP_OK;
	status_line_.clear();
	headers_.clear();
	body_.clear();
}

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
	if (status_ == 0)
		return "";

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

void HttpResponse::setHeader(const std::string& name, const std::string& v)
{
	headers_[name] = v;
}

void HttpResponse::setFullResponse(const std::string& content,
                                   const std::string& extention)
{
	setHeader("Content-Type", getMimeType(extention));
	setBody(content);
	setHeader("Date", getHttpTime());
	setHeader("Server", SERVER_NAME_STR);
}

void HttpResponse::setBody(const std::string& content)
{
	body_ = content;
	setHeader("Content-Length", ws::to_string(content.size()));
}

std::string HttpResponse::buildErrorPage(int err_status) const
{
	const std::string  status = getStatusMsg(err_status);

	std::ostringstream html_page;
	html_page << "<html><head><title>" << status << "</title></head>"
	          << "<body><center><h1>" << status << "</h1></center>"
	          << "<hr><center>" SERVER_NAME_STR "</center></body></html>";

	return html_page.str();
}

std::string HttpResponse::truncateName(const std::string& name)
{
	if (name.size() > 51)
		return name.substr(0, 47) + "..&gt;";
	return name;
}

std::string HttpResponse::formatEntry(const std::string& name,
                                      const std::string& time,
                                      const std::string& size)
{
	std::ostringstream ss;
	std::string        dispay_name = truncateName(name) + "</a>";

	ss << std::left << std::setw(55) << dispay_name << std::right
	   << std::setw(11) << time << std::right << std::setw(20) << size;

	return ss.str();
}

std::string HttpResponse::buildEntry(const std::string& path,
                                     const std::string& name)
{
	std::string time = ws::getFileModificationTime(path + name);
	std::string size;

	if (ws::isDirectory(path + name))
		size = "-";
	else
		size = ws::to_string(ws::getFileSize(path + name));
	std::cout << "SIZE: " << size << std::endl;
	return "<a href=" + name + ">" + formatEntry(name, time, size) + "\n";
}

std::string
HttpResponse::buildDirectoryPage(const std::vector< std::string >& files,
                                 const std::string&                path,
                                 const std::string&                uri)
{
	std::ostringstream page;

	page << "<html>\n"
	     << "<head><title>Index of " << uri << "</title></head>\n"
	     << "<body>\n"
	     << "<h1>Index of " << uri << "</h1>"
	     << "<hr><pre><a href=\"../\">../</a>\n";

	for (size_t i = 0; i < files.size(); ++i)
		page << buildEntry(path, files[i]);

	page << "</pre><hr></body></html>";

	return page.str();
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
		case HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE:
			return "431 Request Header Fields Too Large";
		case HTTP_INTERNAL_SERVER_ERROR: return "500 Internal Server Error";
		case HTTP_NOT_IMPLEMENTED:       return "501 Not Implemented";
		case HTTP_BAD_GATEWAY:           return "502 Bad Gateway";
		case HTTP_SERVICE_UNAVAILABLE:   return "503 Service Unavailable";
		case HTTP_GATEWAY_TIME_OUT:      return "504 Gateway Timeout";
		default:                         return "500 Internal Server Error";
	}
}

// clang-format off
const std::string HttpResponse::getMimeType(const std::string& ext) const
{
	if (ext == "htm" || ext == "html")	return "text/html; charset=utf-8";
	if (ext == "css")					return "text/css";
	if (ext == "js")					return "application/javascript";
	if (ext == "xml")					return "application/xml";
	if (ext == "txt")					return "text/plain";
	if (ext == "jpeg" || ext == "jpg")	return "image/jpeg";
	if (ext == "png")					return "image/png";
	if (ext == "gif")					return "image/gif";
	if (ext == "ico")					return "image/x-icon";
	if (ext == "svg")					return "image/svg+xml";
	if (ext == "webp")					return "image/webp";
	if (ext == "webm")					return "video/webm";
	if (ext == "mp4")					return "video/mp4";
	if (ext == "mp3")					return "audio/mpeg";
	if (ext == "gz")					return "application/gzip";
	if (ext == "zip")					return "application/zip";

	return "application/octet-stream";
}
// clang-format on

