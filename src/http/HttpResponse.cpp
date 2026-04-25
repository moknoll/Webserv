#include "HttpResponse.hpp"
#include "../lib/ws.hpp"
#include "HttpRequest.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <sys/stat.h>

HttpResponse::HttpResponse() : _status(200), _statusText("OK"), _httpVersion("HTTP/1.1")
{
}

HttpResponse::HttpResponse(const std::string& status_text, int status)
    : _status(status), _statusText(status_text), _body("")
{
}

HttpResponse::HttpResponse(const HttpResponse& other)
    : _status(other._status), _statusText(other._statusText),
      _headers(other._headers), _body(other._body)
{
}

HttpResponse::~HttpResponse() {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other)
{
	if (this != &other)
	{
		_status = other._status;
		_statusText = other._statusText;
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
 * <html><body>Some code</body></html>
 */

std::string HttpResponse::build_response(const std::string& path)
{
	std::string response = "HTTP/1.1 ";

	response += ws::to_string(_status);
	response += " " + _statusText + "\r\n";
	response += "Server: webserv\r\n";

	std::string extension = ws::getFileExtension(path);
	std::string Content_type;

	Content_type = _getMimeType(extension);

	response += "Content-Type: " + Content_type + "\r\n";

	std::string   body;

	std::ifstream file(path.c_str(), std::ios::binary);
	if (file.is_open())
	{
		std::string body_file((std::istreambuf_iterator< char >(file)),
		                      std::istreambuf_iterator< char >());

		response +=
		    "Content-Length: " + ws::to_string(body_file.size()) + "\r\n\r\n";
		// std::string bb = std::string(body_file.begin(), body_file.end());
		response += body_file;
		std::cout << "BODY size: " << body_file.size();
	}
	else
	{
		// response error 5xx
		size_t body_size = build_err_page(HTTP_INTERNAL_SERVER_ERROR);
		response +=
		    "Content-Length: " + ws::to_string(body_size) + "\r\n\r\n";
		response += body;
	}

	return response;
}


const char*	HttpResponse::getStatusStr(int status)
{
	switch (status)
	{
		case HTTP_OK:
			return "200 OK";
		case HTTP_CREATED:
			return "201 Created";
		case HTTP_NO_CONTENT:
			return "204 No Content";
		case HTTP_FORBIDDEN:
			return "403 Forbidden";
		case HTTP_NOT_FOUND:
			return  "404 Not Found";
		case HTTP_REQUEST_URI_TOO_LARGE:
			return  "414 URI Too Long";
		case HTTP_REQUEST_ENTITY_TOO_LARGE:
			return  "413 Content Too Large";
		case HTTP_INTERNAL_SERVER_ERROR:
			return  "500 Internal Server Error";
		case HTTP_NOT_IMPLEMENTED:
			return  "501 Not Implemented";
		case HTTP_BAD_GATEWAY:
			return  "502 Bad Gateway";
		default:
			return "500 Internal Server Error";
	}
}

size_t	HttpResponse::build_err_page(int err_status)
{
	this->_body = "<html><head><title>";
	
	this->_body += getStatusStr(err_status);
	this->_body += "</title></head><body><center><h1>";
	this->_body += getStatusStr(err_status);
	this->_body += "</h1></center><hr><center>webserv</center></body></html>";

	return this->_body.size();
}

std::string HttpResponse::_getMimeType(const std::string& extension)
{
	std::map< std::string, std::string > m;
	m["3gp"] = "video/3gpp";
	m["3gpp"] = "video/3gpp";
	m["adp"] = "audio/adpcm";
	m["apng"] = "image/apng";
	m["bmp"] = "image/bmp";
	m["conf"] = "text/plain";
	m["css"] = "text/css";
	m["csv"] = "text/csv";
	m["deb"] = "application/octet-stream";
	m["doc"] = "application/msword";
	m["dot"] = "application/msword";
	m["epub"] = "application/epub+zip";
	m["gif"] = "image/gif";
	m["gz"] = "application/gzip";
	m["h261"] = "video/h261";
	m["h263"] = "video/h263";
	m["h264"] = "video/h264";
	m["heic"] = "image/heic";
	m["heics"] = "image/heic-sequence";
	m["heif"] = "image/heif";
	m["htm"] = "text/html";
	m["html"] = "text/html";
	m["img"] = "application/octet-stream";
	m["in"] = "text/plain";
	m["ini"] = "text/plain";
	m["ink"] = "application/inkml+xml";
	m["iso"] = "application/octet-stream";
	m["jpeg"] = "image/jpeg";
	m["jpf"] = "image/jpx";
	m["jpg"] = "image/jpeg";
	m["jpx"] = "image/jpx";
	m["js"] = "application/javascript";
	m["json"] = "application/json";
	m["json5"] = "application/json5";
	m["jsonld"] = "application/ld+json";
	m["jsonml"] = "application/jsonml+json";
	m["jsx"] = "text/jsx";
	m["kar"] = "audio/midi";
	m["list"] = "text/plain";
	m["log"] = "text/plain";
	m["m1v"] = "video/mpeg";
	m["m2a"] = "audio/mpeg";
	m["m2v"] = "video/mpeg";
	m["m3a"] = "audio/mpeg";
	m["m4a"] = "audio/mp4";
	m["m4p"] = "application/mp4";
	m["map"] = "application/json";
	m["mar"] = "application/octet-stream";
	m["markdown"] = "text/markdown";
	m["md"] = "text/markdown";
	m["mjs"] = "application/javascript";
	m["mov"] = "video/quicktime";
	m["mp2"] = "audio/mpeg";
	m["mp2a"] = "audio/mpeg";
	m["mp3"] = "audio/mpeg";
	m["mp4"] = "video/mp4";
	m["mp4a"] = "audio/mp4";
	m["mp4s"] = "application/mp4";
	m["mp4v"] = "video/mp4";
	m["mpe"] = "video/mpeg";
	m["mpeg"] = "video/mpeg";
	m["mpg"] = "video/mpeg";
	m["mpg4"] = "video/mp4";
	m["mpga"] = "audio/mpeg";
	m["msh"] = "model/mesh";
	m["oga"] = "audio/ogg";
	m["ogg"] = "audio/ogg";
	m["ogv"] = "video/ogg";
	m["ogx"] = "application/ogg";
	m["owl"] = "application/rdf+xml";
	m["pdf"] = "application/pdf";
	m["png"] = "image/png";
	m["ps"] = "application/postscript";
	m["sgi"] = "image/sgi";
	m["shf"] = "application/shf+xml";
	m["shtml"] = "text/html";
	m["so"] = "application/octet-stream";
	m["spx"] = "audio/ogg";
	m["sru"] = "application/sru+xml";
	m["srx"] = "application/sparql-results+xml";
	m["ssdl"] = "application/ssdl+xml";
	m["svg"] = "image/svg+xml";
	m["svgz"] = "image/svg+xml";
	m["text"] = "text/plain";
	m["txt"] = "text/plain";
	m["uri"] = "text/uri-list";
	m["wav"] = "audio/wav";
	m["webm"] = "video/webm";
	m["webp"] = "image/webp";
	m["xml"] = "application/xml";

	if (m.find(extension) == m.end())
		return m["txt"];
	return m[extension];
}
