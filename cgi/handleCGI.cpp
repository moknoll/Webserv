#include "handleCGI.hpp"



handle_cgi_timeout() 

HttpReponse handle_cgi(const HttpRequest &req, const Location &loc)
{
	std::string cgi_output;

	// pipe parent -> child
	// pipe child -> parent
		// if pipe fails make status reponse 
	// create a child process with fork()
		// 	if fork fails make status repsonse

	// if pid == 0
		// executeCGI

	// **PARENT PROCESS**
	// close pipes 

	// send post body? 

	// fcntl set to non blocking + timeout 

	return cgi_output;
}


execute_CGI() 
{
	// dup 2 () stdin <- parent
	// dup 2() stdout -> parent
	
	// close pipes 

	// setenv

	// create script path 

	// char *Argv[] = python3, script, NULL
	
	// execve (script, argv, env)
	// exit on failure 
	// 
}

set_env(const HttpRequest *req)
{
	// set env 
}

cleanup(const HttpRequest *req, int status)
{
	close();

}