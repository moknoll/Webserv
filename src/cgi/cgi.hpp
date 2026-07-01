#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"

#include <ctime>
#include <string>
#include <sys/types.h>
#include <vector>

struct CgiContext
{
	// static const size_t  MAX_CGI_BUFFER;
	pid_t       pid;
	int         stdin_pipe;
	int         stdout_pipe;
	int         exit_status;
	time_t      start_time;
	std::string cgi_output;

	CgiContext();
};

void
executeCGI(const HttpRequest& req, CgiContext& ctx, const ServerConfig& cfg);
std::vector< std::string > buildCgiArgv(const HttpRequest& req,
                                        CgiContext&        ctx);
void                       buildCgiEnvp(const HttpRequest&    req,
                                        const ServerConfig&   cfg,
                                        std::vector< char* >& envp);
bool                       executeChild(CgiContext&          ctx,
                                        std::vector< char* > envp,
                                        std::vector< char* > argv);

