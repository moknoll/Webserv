#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

#include <ctime>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

class CgiContext
{
  public:
	CgiContext();

	HttpResponse handle(const HttpRequest& req, const Location& loc, const ServerConfig& cfg);

  private:
	static const size_t                  MAX_CGI_BUFFER;
	pid_t                                pid_;
	int                                  stdin_pipe[2];
	int                                  stdout_pipe[2];
	std::map< std::string, std::string > env_map;
	int                                  exit_status_;
	time_t                               deadline;
	HttpResponse                         response;
	std::vector< char* >                 envp_;
	std::vector< char* >                 argv_;
	std::map<std::string, std::string>   env;
	std::vector< std::string >           args;
	std::string                          cgi_output_;


	std::string  buildPath(const std::string& uri, const Location& loc) const;
	void         buildCgiEnvp(const HttpRequest& req, const ServerConfig& cfg, const Location& loc);
	void         buildCgiArgv(const HttpRequest& req, const Location& loc);
	void		 buildHttpHeaders(const HttpRequest& req);
	void		 buildEnv(const HttpRequest& req, const std::string& script_path, const ServerConfig& cfg);

	bool         executeChild();
	bool         readChildOutput();
	bool         writeRequestBody(const HttpRequest& req);
	HttpResponse buildResponse();
};
