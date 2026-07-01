#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

#include <ctime>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

struct CgiContext
{
	static const size_t        MAX_CGI_BUFFER;
	pid_t                      pid;
	int                        stdin_pipe;
	int                        stdout_pipe;
	int                        exit_status;
	time_t                     start_time;
	std::vector< std::string > env;
	std::vector< char* >       envp;
	std::vector< char* >       argv;
	std::string                cgi_output_buf;
	std::string                cgi_input_buf;

	CgiContext();
};

CgiContext buildCgiContext();
exeect()
bool       buildCgiEnvp(CgiContext& ctx);
bool       buildCgiArgv(CgiContext& ctx);
bool
buildCgiEnv(const HttpRequest& req, const Location& loc, const CgiContext& ctx);
bool         executeChild(CgiContext& ctx);
bool         writeRequestBody(CgiContext& ctx);
bool         readChildOutput(CgiContext& ctx);
HttpResponse buildResponse(const CgiContext& ctx);
void         cleanup(CgiContext& ctx);
