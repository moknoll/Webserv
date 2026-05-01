#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>

class HttpHandler
{
  public:
	HttpHandler(const ServerConfig& cfg);
	// HttpHandler(const HttpHandler& other);
	~HttpHandler();

	// HttpHandler& operator=(const HttpHandler& other);

	HttpResponse handle(const HttpRequest& req);
	HttpResponse handleGET(const HttpRequest& req);

  private:
	const ServerConfig& config;
	int                 error;

	HttpResponse        makeError(int status);

	std::string         readFile(const char* path);
	HttpResponse        buildFileResponse(const std::string& path);
	std::string         getMimeType(const std::string& path);

	const Location*     findMatchUri(const std::string&             uri,
	                                 const std::vector< Location >& locations);

	std::string         buildPath(const std::string& uri, const Location& loc);
};

