#pragma once

#include <map>
#include <string>
class HttpResponse
{
  public:
	HttpResponse();
	HttpResponse(const HttpResponse& other);
	~HttpResponse();

	HttpResponse& operator=(const HttpResponse& other);

	void          Get(const std::string& content);

  private:
	int                                  status;
	std::string                          status_text;
	std::map< std::string, std::string > headers;
	std::string                          body;
};

