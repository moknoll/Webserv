#include "HttpResponse.hpp"
#include "../lib/ws.hpp"
#include "http.hpp"
#include <string>

HttpResponse::HttpResponse() {}

HttpResponse::HttpResponse(const int status) : _status(status) {}

// HttpResponse::HttpResponse(int status) : _status(status) {]

HttpResponse::HttpResponse(const HttpResponse& other)
    : _status(other._status), _status_line(other._status_line),
      _headers(other._headers), _body(other._body)
{
}

HttpResponse::~HttpResponse() {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other)
{
	if (this != &other)
	{
		_status = other._status;
		_status_line = other._status_line;
		_headers = other._headers;
		_body = other._body;
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
	res += getStatusStr(_status);
	res += CRLF;

	std::map< std::string, std::string >::const_iterator it;

	for (it = _headers.begin(); it != _headers.end(); ++it)
		res += it->first + ": " + it->second + CRLF;

	res += CRLF;
	res += _body;

	return res;
}

void HttpResponse::setStatus(int status)
{
	this->_status = status;
}

void HttpResponse::setHeader(const std::string& header_name,
                             const std::string& v)
{
	_headers[header_name] = v;
}

void HttpResponse::setBody(const std::string& content,
                           const std::string& content_type)
{
	_body = content;
	setHeader("Content-Length", ws::to_string(content.size()));
	setHeader("Content-Type", content_type);
	setHeader("Server", "Webserv");
}

const char* HttpResponse::getStatusStr(int status) const
{
	switch (status)
	{
		case HTTP_OK:                       return "200 OK";
		case HTTP_CREATED:                  return "201 Created";
		case HTTP_NO_CONTENT:               return "204 No Content";
		case HTTP_FORBIDDEN:                return "403 Forbidden";
		case HTTP_NOT_FOUND:                return "404 Not Found";
		case HTTP_REQUEST_URI_TOO_LARGE:    return "414 URI Too Long";
		case HTTP_REQUEST_ENTITY_TOO_LARGE: return "413 Content Too Large";
		case HTTP_INTERNAL_SERVER_ERROR:    return "500 Internal Server Error";
		case HTTP_NOT_IMPLEMENTED:          return "501 Not Implemented";
		case HTTP_BAD_GATEWAY:              return "502 Bad Gateway";
		default:                            return "500 Internal Server Error";
	}
}

std::string getMimeType(const std::string& path)
{
	std::string ext = ws::getFileExtension(path);

	if (ext == "htm" || ext == "html") return "text/html";
	if (ext == "css") return "text/css";
	if (ext == "js") return "application/javascript";
	if (ext == "xml") return "application/xml";
	if (ext == "txt") return "text/plain";
	if (ext == "jpeg" || ext == "jpg") return "image/jpeg";
	if (ext == "png") return "image/png";
	if (ext == "gif") return "image/gif";
	if (ext == "ico") return "image/x-icon";
	if (ext == "svg") return "image/svg+xml";
	if (ext == "webp") return "image/webp";
	if (ext == "webm") return "video/webm";
	if (ext == "mp4") return "video/mp4";
	if (ext == "mp3") return "audio/mpeg";
	if (ext == "gz") return "application/gzip";
	if (ext == "zip") return "application/zip";

	return "application/octet-stream";
}

std::string HttpResponse::get_error_page(int err_status) const
{
	std::string ret = "<html><head><title>";
	ret += getStatusStr(err_status);
	ret += "</title></head><body><center><h1>";
	ret += getStatusStr(err_status);
	ret += +"</h1></center><hr><center>webserv</center></body></html>";

	return ret;
}

