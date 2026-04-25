#include "HttpRequest.hpp"
#include "../lib/ws.hpp"
#include <iostream>
#include <string>
#include <vector>

HttpRequest::HttpRequest(const std::string& req_message)
{
	this->_err_status = _parser(req_message);
}

HttpRequest::HttpRequest(const HttpRequest& other)
    : _method(other._method), _uri(other._uri),
      _http_version(other._http_version), _start_line(other._start_line),
      _host(other._host), _body(other._body), _headers(other._headers)
{
}

HttpRequest::~HttpRequest() {}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
	if (this != &other)
	{
		this->_method = other._method;
		this->_uri = other._uri;
		this->_http_version = other._http_version;
		this->_start_line = other._start_line;
		this->_host = other._host;
		this->_body = other._body;
		this->_headers = other._headers;
	}
	return *this;
}

int HttpRequest::_parser(const std::string& req_message)
{
	if (_parse_start_line(req_message) != HTTP_OK)
		return HTTP_BAD_REQUEST;

	size_t start_header = req_message.find("\r\n");
	size_t end_header = req_message.find("\r\n\r\n");
	if (start_header == std::string::npos || end_header == std::string::npos)
		return HTTP_BAD_REQUEST;

	std::string headers =
	    req_message.substr(start_header + 2, end_header - start_header + 2);

	_parse_header(headers);

	if (this->_method == "POST" || this->_method == "DELETE")
		this->_body = end_header + 4;

	return HTTP_OK;
}

int HttpRequest::_parse_start_line(const std::string& req_message)
{
	size_t end_start_line = req_message.find("\r\n");

	if (end_start_line == std::string::npos)
		return HTTP_BAD_REQUEST;

	std::string start_line = req_message.substr(0, end_start_line);

	std::vector< std::string > toks = ws::strSplit(start_line, " ");
	if (toks.size() != 3)
		return HTTP_BAD_REQUEST;

	this->_method = ws::strip(toks[0]);

	if (_method != "GET" || _method != "POST" || _method != "DELETE")
		return HTTP_BAD_REQUEST;

	std::cout << this->_err_status;
	std::cout << this->_method;
	this->_uri = ws::strip(toks[1]);
	this->_http_version = ws::strip(toks[2]);
	this->_start_line = start_line;

	return HTTP_OK;
}

void HttpRequest::_parse_header(const std::string& header)
{
	std::vector< std::string >           toks = ws::strSplit(header, "\r\n");

	std::vector< std::string >::iterator it = toks.begin();
	for (; it != toks.end(); ++it)
	{
		size_t      colonChar_pos = it->find(":");
		std::string key = it->substr(0, colonChar_pos);
		std::string value = it->substr(colonChar_pos + 1);

		this->_headers.insert(std::make_pair(ws::strip(key), ws::strip(value)));
	}
	this->_host = this->_headers.at("Host");
}

std::string HttpRequest::get_uri()
{
	return this->_uri;
}
