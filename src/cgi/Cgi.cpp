#include "Cgi.hpp"
#include <unistd.h>
#include <string.h>
#include "../lib/ws.hpp"

CgiContext buildCgiContext()
{
	CgiContext ctx;

	ctx.pid = -1;
	ctx.stdin_pipe[0] = -1;
	ctx.stdin_pipe[1] = -1;
	ctx.stdout_pipe[0] = -1;
	ctx.stdout_pipe[1] = -1;
	ctx.argv = NULL;
	ctx.deadline = 0;
	ctx.envp = NULL;
	ctx.exit_status = 0;

	return ctx;
}

void buildCgiEnv(const HttpRequest &req, const Location &loc, const CgiContext &ctx)
{
	std::map<std::string, std::string> env;
	std::string uri = req.getURI();
	size_t pos = uri.find('?');

	env["REQUEST_METHOD"] = req.getMethod();

	if (pos != std::string::npos)
		env["QUERY_STRING"] = uri.substr(pos + 1);
	else
		env["QUERY_STRING"] = "";
	
	env["CONTENT_LENGTH"] = req.getContentLenght();
	// env["CONTENT_TYPE"] = ;



	std::string script_name = uri;
	if (pos != std::string::npos)
		script_name = uri.substr(0, pos);
	env["SCRIPT_NAME"] = "";


	env["PATH_INFO"] = script_name;
	env["PATH_TRANSLATED"] = loc.cgi_path;
	env["REMOTE_ADDR"] = "";
	//env["SERVER_NAME"] = req.;
	//env["SERVER_PORT"] = req.;
	env["SERVER_PROTOCOL"] = "HTTP/1.1";

	// Required for PHP-CGI security
	env["REDIRECT_STATUS"] = "200";

	// Parse all HTTPs in Header and replace - with _ 
}

void buildCgiEnvp(CgiContext& ctx)
{
	char **_envp;

	ctx.envp = _envp;
}
void buildCgiArgv(CgiContext &ctx)
{
	
}


bool executeChild(CgiContext &ctx)
{
	if(pipe(ctx.stdin_pipe) == -1)
		return false;
	if(pipe(ctx.stdout_pipe) == -1)
		return false;
	pid_t pid = fork();
	if(pid < 0)
		return false;
	
	if(pid == 0)
	{
		dup2(ctx.stdin_pipe[0], STDIN_FILENO);
		dup2(ctx.stdout_pipe[1], STDOUT_FILENO);

		close(ctx.stdin_pipe[0]);
		close(ctx.stdin_pipe[1]);
		close(ctx.stdout_pipe[0]);
		close(ctx.stdout_pipe[1]);

		buildCgiEnvp(ctx);
		buildCgiArgv(ctx);
		execve(ctx.argv[0], ctx.argv, ctx.envp);
		_exit(127);
	}
	close(ctx.stdin_pipe[0]);
	close(ctx.stdout_pipe[1]);

	ctx.pid = pid;
	return true;
}

CgiContext::CgiContext()
	: pid(-1), exit_status(0), deadline(0), response()
{
	stdin_pipe[0] = -1;
	stdin_pipe[1] = -1;
	stdout_pipe[0] = -1;
	stdout_pipe[1] = -1;
}
