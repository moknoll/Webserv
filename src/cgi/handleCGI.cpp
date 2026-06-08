#include "handleCGI.hpp"
#include <unistd.h>
#include <string.h>
#include "../lib/ws.hpp"

CgiContext buildCgiContext(const HttpRequest &req, const Location &loc)
{
	CgiContext ctx;

	ctx.pid = -1;
	ctx.stdin_pipe[0] = -1;
	ctx.stdin_pipe[1] = -1;
	ctx.stdout_pipe[0] = -1;
	ctx.stdout_pipe[1] = -1;

	return ctx;
}

std::map< std::string, std::string >
buildCgiEnv(const HttpRequest &req, const Location &loc, const CgiContext &ctx)
{
	(void)loc;
	std::map< std::string, std::string > env;

	//finish all setups
	env["REQUEST_METHOD"] = req.getMethod();
	env["CONTENT_LENGTH"] = ws::to_string(req.getContentLenght());
	env["CONTENT_TYPE"] = req.getHeader("Content-Type");
	env["REQUEST_URI"] = req.getURI();
	//env["QUERY_STRING"] = req.getQuery();
	//env[""]
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_SOFTWARE"] = "webserv";
	env["REDIRECT_STATUS"] = "200";

	return env;
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
