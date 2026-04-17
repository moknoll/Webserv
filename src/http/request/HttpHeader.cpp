#include "HttpHeader.hpp"
#include "../../lib/ws.hpp"
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

HttpHeader::HttpHeader()
    : _method("GET"), _uri(""), _http_version("HTTP/1.1"), _host(""),
      _ContentType("text/html"), _ContentLength(""), _Connection("close"),
      _Content_Length(0), _headers(), _body("")
{
}

HttpHeader::HttpHeader(const std::string& request)
{
	_parse_start_line(request);

	std::string header = _get_header(request);
	_parse_header(header);

	this->_body = _get_body(request);
}

HttpHeader::HttpHeader(const HttpHeader& other)
    : _method(other._method), _uri(other._uri),
      _http_version(other._http_version), _host(other._host),
      _ContentType(other._ContentType), _Connection(other._Connection),
      _Content_Length(other._Content_Length), _headers(other._headers),
      _body(other._body)

{
}

HttpHeader::~HttpHeader() {}

HttpHeader& HttpHeader::operator=(const HttpHeader& other)
{
	if (this != &other)
	{
		this->_method = other._method;
		this->_uri = other._uri;
		this->_http_version = other._http_version;
		this->_host = other._host;
		this->_ContentType = other._ContentType;
		this->_ContentLength = other._ContentLength;
		this->_Connection = other._Connection;
		this->_Content_Length = other._Content_Length;
		this->_headers = other._headers;
		this->_body = other._body;
	}
	return *this;
}

std::string HttpHeader::gethost()
{
	return this->_host;
}

std::string HttpHeader::geturi()
{
	return this->_uri;
}

std::string HttpHeader::getmethod()
{
	return this->_method;
}

std::string HttpHeader::gethttpversion()
{
	return this->_http_version;
}

void HttpHeader::_parse_start_line(const std::string& buffer)
{
	size_t end_startl_pos = buffer.find("\r\n");

	if (end_startl_pos == std::string::npos)
	{
		std::cout << "BAD request with message '400 Bad Request'\n";
		return;
	}

	std::string                start_line = buffer.substr(0, end_startl_pos);

	std::vector< std::string > toks = ws::strSplit(start_line, " ");
	if (toks.size() != 3)
	{
		std::cout << "BAD request with message '400 Bad Request'\n";
		return;
	}

	this->_method = ws::strip(toks[0]);
	this->_uri = ws::strip(toks[1]);
	this->_http_version = ws::strip(toks[2]);
}

void HttpHeader::_parse_header(const std::string& header)
{
	typedef std::vector< std::string >::iterator VSit;

	std::vector< std::string > toks = ws::strSplit(header, "\r\n");

	for (VSit it = toks.begin(); it != toks.end(); ++it)
	{
		size_t      colonChar_pos = it->find(":");
		std::string key = it->substr(0, colonChar_pos);
		std::string value = it->substr(colonChar_pos + 1);

		this->_headers.insert(std::make_pair(ws::strip(key), ws::strip(value)));
	}

	this->_host = this->_headers["Host"];
	this->_Connection = this->_headers["Connection"];
	this->_ContentType = this->_headers["Content-Type"];
	this->_ContentLength = this->_headers["Content-Length"];
	this->_Content_Length = std::strtol(this->_ContentLength.c_str(), NULL, 10);
}

std::string HttpHeader::_get_header(const std::string& buffer)
{
	size_t start_header = buffer.find("\r\n");
	size_t end_header = buffer.find("\r\n\r\n");
	if (start_header == std::string::npos || end_header == std::string::npos)
	{
		std::cout << "BAD request with message '400 Bad Request'\n";
		return "";
	}

	std::string headers =
	    buffer.substr(start_header + 2, end_header - start_header + 2);
	return ws::strip(headers);
}

std::string HttpHeader::_get_body(const std::string& buffer)
{
	size_t start_body_pos = buffer.find("\r\n\r\n");

	if (start_body_pos + 4 == buffer.size())
		return "";
	return buffer.substr(start_body_pos + 4);
}

