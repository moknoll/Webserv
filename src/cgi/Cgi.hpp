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
	enum State
	{
		CGI_IDLE = 0,
		CGI_STARTING,
		CGI_WRITING_STDIN,
		CGI_READING_STDOUT,
		CGI_WAIT_CHILD,
		CGI_DONE,
		CGI_ERROR
	};
	CgiContext();
	~CgiContext();

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

	State state_;
    bool stdin_closed_;
    bool stdout_closed_;

    std::string request_body_;
    size_t body_write_off_;

	bool start(const HttpRequest& req, const Location& loc, const ServerConfig& cfg);
	/*1. Env argv bauen
	Pipes erstellen
	Beide Parent-Enden non-blocking setzen
	Fork/exec
	request_body_ aus temp file einmal in RAM laden (Datei-I/O ist erlaubt)
	state_ = CGI_WRITING_STDIN (wenn Body), sonst direkt CGI_READING_STDOUT */
	// std::string  buildPath(const std::string& uri, const Location& loc) const;
	// void         buildCgiEnvp(const HttpRequest& req, const ServerConfig& cfg, const Location& loc);
	// void         buildCgiArgv(const HttpRequest& req, const Location& loc);
	// void		 buildHttpHeaders(const HttpRequest& req);
	// void		 buildEnv(const HttpRequest& req, const std::string& script_path, const ServerConfig& cfg);
	
	bool onPipeWritable();   // stdin_pipe[1] ist POLLOUT
	/*2. write nur einmalig/teilweise
	bei EAGAIN: nichts tun, weiter warten
	wenn alles geschrieben: stdin schließen, state_ = CGI_READING_STDOUT*/
	bool         executeChild();

	bool onPipeReadable();   // stdout_pipe[0] ist POLLIN
	/*3. onPipeReadable:
	read in Chunks
	bei EAGAIN: warten
	bei 0: stdout schließen, state_ = CGI_WAIT_CHILD
	MAX_CGI_BUFFER beachten*/
	
	bool pollChild();        // waitpid(..., WNOHANG)
	/*4. waitpid(pid_, &status, WNOHANG)
	wenn Child fertig und stdout schon zu: state_ = CGI_DONE
	Exitcode ungleich 0 -> CGI_ERROR*/
	bool setNonBlocking_(int fd);
    void closeStdinWrite_();
    void closeStdoutRead_();
    void cleanupOnError_();
	bool isDone() const;
    bool hasError() const;
    State getState() const;


	bool         readChildOutput();
	bool         writeRequestBody(const HttpRequest& req);
	HttpResponse buildResponse();
};
