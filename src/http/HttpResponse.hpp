#pragma once

#include <map>
#include <string>

class HttpResponse
{
  public:
	HttpResponse(const std::string& status_text, int status);
	HttpResponse(const HttpResponse& other);
	~HttpResponse();

	HttpResponse& operator=(const HttpResponse& other);

	void          Get(const std::string& content);
	std::string   build_response(const std::string& path);

  private:
	int                                  _status;
	std::string                          _status_text;
	std::map< std::string, std::string > _headers;
	std::string                          _body;

	std::string                          build_err_page(int err_status);

	std::string _getMimeType(const std::string& extension);
};

