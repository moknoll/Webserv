#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

#include <ctime>
#include <map>
#include <string>
#include <sys/types.h>

class CgiContext
{
  public:
	CgiContext();

	HttpResponse handle(const HttpRequest& req, const Location& loc);

  private:
	pid_t                                pid_;
	int                                  stdin_pipe[2];
	int                                  stdout_pipe[2];
	std::map< std::string, std::string > env_map;
	int                                  exit_status_;
	time_t                               deadline;
	HttpResponse                         response;
	char**                               envp;
	char**                               argv;

	std::string resolveScriptPath(const Location&    loc,
	                              const std::string& uri) const;

	void        buildCgiEnv(const HttpRequest& req, const Location& loc);

	std::string buildPath(const std::string& uri, const Location& loc) const;
	void        buildCgiEnvp();
	void        buildCgiArgv();
	bool        executeChild();
};

CgiContext   buildCgiContext();
void         buildCgiEnv(const HttpRequest& req, const Location& loc);
bool         executeChild(CgiContext& ctx);
bool         writeRequestBody(CgiContext& ctx);
bool         readChildOutput(CgiContext& ctx);
HttpResponse buildResponse(const CgiContext& ctx);
void         cleanup(CgiContext& ctx);
