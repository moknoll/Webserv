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
	pid_t 								 pid;
	int   								 stdin_pipe[2];
	int   								 stdout_pipe[2];
	char	       			 			 **argv;
	std::map< std::string, std::string > env_map;
	int                        			 exit_status;
	time_t                     			 deadline;
	HttpResponse               			 response;
	char								 **envp;

	CgiContext();
};

bool        isCgiRequest(const HttpRequest& req, const Location& loc);
CgiContext  buildCgiContext(const HttpRequest& req, const Location& loc);
void        buildCgiEnvp(CgiContext& ctx);
void        buildCgiArgv(CgiContext& ctx);
std::map< std::string, std::string >
			buildCgiEnv(const HttpRequest& req,
						const Location&    loc,
						const CgiContext&  ctx);
bool        executeChild(CgiContext& ctx);
bool        writeRequestBody(CgiContext& ctx);
bool        readChildOutput(CgiContext& ctx);
HttpResponse buildResponse(const CgiContext& ctx);
void        cleanup(CgiContext& ctx);
