#pragma once

// #include <cstdint>
#include <cstddef>
#include <map>
#include <string>

class HttpRequest
{
  public:
	HttpRequest();
	HttpRequest(const std::string& req_message);
	HttpRequest(const HttpRequest& other);
	~HttpRequest();

	HttpRequest& operator=(const HttpRequest& other);

	std::string  get_uri() const;
	std::string  get_header(const std::string& name);
	int          hasError() const;

  private:
	int                                  _err_status;
	int                                  _method;
	size_t                               _ContentLenght;
	// std::string                          _method;
	std::string                          _uri;
	std::string                          _http_version;
	std::string                          _request_line;
	std::string                          _host;
	std::string                          _body;
	std::string                          _extension;
	std::string                          _unparsed_uri;
	std::map< std::string, std::string > _headers;

	int  _parser(const std::string& req_message);
	int  _parse_request_line(const std::string& line);
	void _parse_headers(const std::string& header);
};

