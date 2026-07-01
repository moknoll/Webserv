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

	HttpResponse getResponse() const;
	int          executeCGI(const HttpRequest& req, const Location& loc);
	int          getFdCGI_out() const;
	int          getFdCGI_in() const;
	int          getStatus() const;
	int          getCgiPid() const;
	void         appendCgiOutput(const char* buf, size_t size);
	bool         writeRequestBody();
	int          buildResponse();
	int          waitChildProc();

	std::string  cgi_output_buf_;

  private:
	static const size_t                  MAX_CGI_BUFFER;
	pid_t                                pid_;
	int                                  stdin_pipe[2];
	int                                  stdout_pipe[2];
	std::map< std::string, std::string > env_map;
	int                                  exit_status_;
	time_t                               start_time_;
	HttpResponse                         response_;
	int                                  request_body_fd_;
	std::string                          cgi_input_buf_;

	std::vector< char* >                 envp_;
	std::vector< char* >                 argv_;
	std::vector< std::string >           env;
	std::vector< std::string >           args;

	std::string resolveScriptPath(const Location&    loc,
	                              const std::string& uri) const;

	void        buildCgiEnv(const HttpRequest& req, const Location& loc);

	std::string buildPath(const std::string& uri, const Location& loc) const;
	int         buildCgiEnvp(const HttpRequest& req);
	int         buildCgiArgv(const HttpRequest& req, const Location& loc);
	bool        executeChild();

	void        closeFile();
	void        ClosePipes();
};

CgiContext buildCgiContext();
void       buildCgiEnv(const HttpRequest& req, const Location& loc);
void       cleanup(CgiContext& ctx);
