#include "cgi.hpp"
#include "../http/constants.hpp"
#include "../lib/ws.hpp"
#include <cstddef>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

CgiContext::CgiContext()
{
	const size_t MAX_CGI_BUFFER = 10 * 1024 * 1024;
	pid = -1;
	stdin_pipe = -1;
	stdout_pipe = -1;
	exit_status = 0;
	start_time = 0;
	envp.clear();
	argv.clear();
	cgi_output.reserve(1024 * 1024);
}

bool buildCgiEnvp(const HttpRequest& req, CgiContext& ctx)
{
	std::vector< std::string > env;

	env.push_back("REQUEST_METHOD=" + req.getMethod());

	std::map< std::string, std::string >           h = req.getHeaders();

	std::map< std::string, std::string >::iterator it = h.begin();
	for (; it != h.end(); ++it)
	{
		std::string name = it->first;
		if (ws::toUpperCase(name) == "CONTENT-TYPE")
			env.push_back("CONTENT_TYPE=" + it->second);
		else if (ws::toUpperCase(name) == "CONTENT-LENGTH")
			env.push_back("CONTENT_LENGTH=" + it->second);
		else if (ws::toUpperCase(name) == "HOST")
			env.push_back("SERVER_PROTOCOL=" + it->second);
		else
			env.push_back("HTTP_" + it->first + it->second);
	}

	for (size_t i = 0; i < env.size(); ++i)
		ctx.envp.push_back(const_cast< char* >(env[i].c_str()));
	ctx.envp.push_back(NULL);

	return true;
}

std::string buildPath(const std::string& uri, const Location& loc)
{
	std::string sub_uri = uri.substr(loc.path.length());
	std::string path = loc.root;

	if (!path.empty() && path[path.length() - 1] != '/')
		path += '/';

	if (!sub_uri.empty() && sub_uri[0] == '/')
		path += sub_uri.substr(1);
	else
		path += sub_uri;

	return path;
}

bool buildCgiArgv(const HttpRequest& req, const Location& loc, CgiContext& ctx)
{
	std::string            uri = req.getURI();
	std::string::size_type p = uri.find('?');
	std::string            path = buildPath(uri.substr(0, p), loc);

	PathInfo               path_info = ws::checkPath(path);
	PathInfo               cgi_path_info = ws::checkPath(loc.cgi_path);
	if (!path_info.exists && !path_info.readable)
	{
		ctx.exit_status = HTTP_NOT_FOUND;
		return false;
	}
	if (!cgi_path_info.exists && !cgi_path_info.readable
	    && !cgi_path_info.executable)
	{
		ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;
		return false;
	}

	ctx.argv.push_back(const_cast< char* >(loc.cgi_path.c_str()));
	ctx.argv.push_back(const_cast< char* >(path.c_str()));
	ctx.argv.push_back(NULL);
	return HTTP_OK;
}

bool executeChild(CgiContext& ctx)
{
	int pipe_in[2];
	int pipe_out[2];

	if (pipe(pipe_in) == -1)
		return false;
	if (pipe(pipe_out) == -1)
		return false;
	pid_t pid = fork();
	if (pid < 0)
		return false;

	if (pid == 0)
	{
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);

		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);

		execve(ctx.argv[0], &ctx.argv[0], &ctx.envp[0]);
		std::exit(127);
	}
	close(pipe_in[0]);
	close(pipe_out[1]);

	ctx.pid = pid;
	ctx.stdin_pipe = pipe_in[1];
	ctx.stdout_pipe = pipe_out[0];

	fcntl(ctx.stdin_pipe, F_SETFL, O_NONBLOCK);
	fcntl(ctx.stdout_pipe, F_SETFL, O_NONBLOCK);

	return true;
}

