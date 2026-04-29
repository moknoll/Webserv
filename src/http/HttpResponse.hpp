#pragma once

#include <map>
#include <string>

class HttpResponse
{
  public:
	HttpResponse();
	HttpResponse(const int status);
	HttpResponse(const HttpResponse& other);
	~HttpResponse();

	HttpResponse& operator=(const HttpResponse& other);

	std::string   buildResponse() const;

	void setHeader(const std::string& header_name, const std::string& v);
	void setBody(const std::string& content, const std::string& content_type);
	void setStatus(int status);

	std::string getHeader(const std::string& header_name) const;
	std::string get_error_page(int err_status) const;
	const char* getStatusStr(int status) const;

  private:
	int                                  _status;
	std::string                          _status_line;
	std::map< std::string, std::string > _headers;
	std::string                          _body;

	std::string _getMimeType(const std::string& extension);
};

