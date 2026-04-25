#pragma once

#include <cstddef>
#include <map>
#include <string>

class HttpResponse
{
  public:
	HttpResponse();
	HttpResponse(const std::string& status_text, int status);
	HttpResponse(const HttpResponse& other);
	~HttpResponse();

	HttpResponse& operator=(const HttpResponse& other);

	void          Get(const std::string& content);
	std::string   build_response(const std::string& path);

	std::string   getHeader(const std::string& header_name) const;
	void setHeader(const std::string& header_name, const std::string& v);

  private:
	int                                  _status;
	std::string                          _statusText;
	std::string                          _httpVersion;
	std::string                          _contentType;
	size_t                               _contentLength;
	std::map< std::string, std::string > _headers;
	std::string                          _body;

	size_t                               build_err_page(int err_status);

	std::string        _getMimeType(const std::string& extension);
	static const char* getStatusStr(int status);
};

