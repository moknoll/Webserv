#pragma once

#include <map>
#include <string>
#include <sstream>

class HttpHeader
{
  public:
	HttpHeader();
	HttpHeader(const std::string& request);
	HttpHeader(const HttpHeader& other);
	~HttpHeader();

	HttpHeader& operator=(const HttpHeader& other);

	std::string gethost();
	std::string getmethod();
	std::string geturi();
	std::string gethttpversion();

  private:
	std::string                          _method;
	std::string                          _uri;
	std::string                          _http_version;
	std::string                          _host;
	std::string                          _ContentType;
	std::string                          _ContentLength;
	std::string                          _Connection;
	size_t                               _Content_Length;
	std::map< std::string, std::string > _headers;
	std::string                          _body;

	std::string                          _get_header(const std::string& buffer);
	std::string                          _get_body(const std::string& buffer);
	void _parse_start_line(const std::string& start_line);
	void _parse_header(const std::string& header);
};

template < typename T >
std::string to_string(const T& value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}
