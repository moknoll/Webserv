#pragma once

#include "../http/HttpRequest.hpp"

#include <ctime>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <vector>

struct CgiContext
{
	pid_t        pid;
	int          stdin_pipe;
	int          stdout_pipe;
	int          error;
	time_t       start_time;
	bool         exit_ok;
	bool         pipe_stdout_eof;
	bool         procese_reaped;
	bool         cgi_timed_out;
	std::fstream request_body;

	CgiContext();
	CgiContext(const CgiContext& other);
	~CgiContext();
};

void                       resetCgiContext(CgiContext& ctx);
void                       executeCGI(const HttpRequest& req, CgiContext& ctx);
std::vector< std::string > buildCgiArgv(const HttpRequest&, CgiContext&);
void buildCgiEnvp(const HttpRequest&, std::vector< char* >&);
bool executeChild(CgiContext& ctx,
                  const std::vector< std::string >&,
                  const std::vector< std::string >&);

