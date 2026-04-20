#pragma once

#include <string.h>

class HttpRequest
{
  public:
	HttpRequest();
	HttpRequest(const HttpRequest& other);
	~HttpRequest();

	HttpRequest& operator=(const HttpRequest& other);

	void         Get(const std::string& content);

  private:
};

