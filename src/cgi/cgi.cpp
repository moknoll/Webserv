#include "cgi.hpp"
#include "../constants.hpp"
#include "../http/HttpRequest.hpp"
#include "../lib/ws.hpp"

#include <algorithm>
#include <csignal>
#include <cstddef>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <ios>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

CgiContext::CgiContext()
{
	pid = -1;
	stdin_pipe = -1;
	stdout_pipe = -1;
	exit_status = 0;
	start_time = 0;
	request_body_fd = -1;
}

CgiContext::CgiContext(const CgiContext& other) :
        pid(other.pid),
        stdin_pipe(other.stdin_pipe),
        stdout_pipe(other.stdout_pipe),
        request_body_fd(other.request_body_fd)

{
}
static std::string normalizeHeader(std::string name)
{
	std::replace(name.begin(), name.end(), '-', '_');
	return ws::toUpperCase(name);
}

static std::vector< std::string > buildCGIHeaders(const HttpRequest& req)
{
	const std::map< std::string, std::string >& headers = req.getHeaders();
	std::vector< std::string >                  result;

	for (std::map< std::string, std::string >::const_iterator it =
	         headers.begin();
	     it != headers.end();
	     ++it)
	{
		if (ws::toUpperCase(it->first) == "CONTENT-LENGTH"
		    || ws::toUpperCase(it->first) == "CONTENT-TYPE")
			continue;
		std::string n = "HTTP_" + normalizeHeader(it->first);
		std::string h = n + "=" + it->second;
		result.push_back(h);
	}
	return result;
}

std::string getPATH_INFO(const HttpRequest& req)
{
	std::string uri = req.getURI();
	size_t      ext_pos = uri.find(".py");
	size_t      query_pos = uri.find("?");
	std::string path_info = "";

	if (query_pos != std::string::npos)
		path_info = uri.substr(ext_pos + 3, query_pos - ext_pos - 3);
	else
		path_info = uri.substr(ext_pos + 3);

	return path_info;
}

std::string getPATH_STRANSLATED(const std::string& root,
                                const std::string& path_info)
{
	std::string ret = root;
	if (!ret.empty() && ret[ret.size() - 1] == '/')
		ret.substr(ret.size() - 1);

	ret += path_info;

	return ret;
}

static std::vector< std::string > buildEnv(const HttpRequest& req)
{
	const Location             loc = req.getLocation();
	std::string                uri = req.getURI();
	size_t                     ext = uri.find(".py");
	size_t                     q = uri.find("?");

	std::vector< std::string > env = buildCGIHeaders(req);

	if (req.getMethod() == "POST")
	{
		env.push_back("CONTENT_LENGTH="
		              + ws::to_string(req.getContentLenght()));
		env.push_back("CONTENT_TYPE=" + req.getHeader("Content-Type"));
	}
	env.push_back("REQUEST_METHOD=" + req.getMethod());
	env.push_back("SERVER_NAME=" + req.getConfig().server_name);
	env.push_back("SERVER_PORT=" + ws::to_string(req.getConfig().port));
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");

	if (ext != std::string::npos)
	{
		std::string path_info = getPATH_INFO(req);

		env.push_back("SCRIPT_NAME=" + req.getURI().substr(0, ext + 3));
		env.push_back("PATH_INFO=" + getPATH_INFO(req));

		if (!path_info.empty())
		{
			std::string pt = getPATH_STRANSLATED(loc.root, path_info);
			env.push_back("PATH_TRANSLATED=" + pt);
		}
	}

	if (q != std::string::npos)
		env.push_back("QUERY_STRING=" + uri.substr(q + 1));
	else
		env.push_back("QUERY_STRING=");

	return env;
}

std::vector< std::string > buildCgiArgv(const HttpRequest& req, CgiContext& ctx)
{
	std::string path = req.getPath();
	size_t      ext_pos = path.find(".py");
	if (ext_pos != std::string::npos)
		path = path.substr(0, ext_pos + 3);

	PathInfo path_info = ws::checkPath(path);
	PathInfo cgi_path_info = ws::checkPath(req.getLocation().cgi_path);
	std::vector< std::string > argv;
	if (!path_info.exists && !path_info.readable)
	{
		ctx.exit_status = HTTP_NOT_FOUND;
	}
	if (!cgi_path_info.exists && !cgi_path_info.readable
	    && !cgi_path_info.executable)
	{
		ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;
	}

	argv.push_back(req.getLocation().cgi_path);
	argv.push_back(path);
	return argv;
}

bool executeChild(CgiContext&                       ctx,
                  const std::vector< std::string >& args,
                  const std::vector< std::string >& env)
{
	std::vector< char* > envp;
	std::vector< char* > argv;
	argv.push_back(const_cast< char* >(args[0].c_str()));
	argv.push_back(const_cast< char* >(args[1].c_str()));
	argv.push_back(NULL);

	for (size_t i = 0; i < env.size(); ++i)
	{
		envp.push_back(const_cast< char* >(env[i].c_str()));
	}
	envp.push_back(NULL);

	ctx.start_time = std::time(NULL);
	int pipe_in[2];
	int pipe_out[2];

	if (pipe(pipe_in) == -1)
	{
		ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;
		return false;
	}
	if (pipe(pipe_out) == -1)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;
		return false;
	}
	pid_t pid = fork();
	if (pid < 0)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;
		return false;
	}

	if (pid == 0)
	{
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);

		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);

		execve(argv[0], argv.data(), envp.data());
		_exit(127);
	}
	close(pipe_in[0]);
	close(pipe_out[1]);

	ctx.pid = pid;
	ctx.stdin_pipe = pipe_in[1];
	ctx.stdout_pipe = pipe_out[0];

	return true;
}

void executeCGI(const HttpRequest& req, CgiContext& ctx)
{
	std::vector< std::string > args = buildCgiArgv(req, ctx);
	if (ctx.exit_status)
		return;

	std::vector< std::string > env = buildEnv(req);
	if (ctx.exit_status)
		return;

	if (!executeChild(ctx, args, env))
		return;

	fcntl(ctx.stdout_pipe, F_SETFL, O_NONBLOCK);
	if (req.getMethod() == "POST")
	{
		// ctx.request_body_fd = open(req.getBodyTempFileName().c_str(),
		// O_RDONLY);
		ctx.request_body.open(req.getBodyTempFileName().c_str(),
		                      std::ios::in | std::ios::binary);
		if (!ctx.request_body.is_open())
		{
			close(ctx.stdin_pipe);
			close(ctx.stdout_pipe);
			kill(ctx.pid, SIGTERM);
			ctx.pid = -1;
			ctx.stdin_pipe = -1;
			ctx.stdout_pipe = -1;
			ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;
			return;
		}

		// if (ctx.request_body_fd == -1)
		// {
		// 	close(ctx.stdin_pipe);
		// 	close(ctx.stdout_pipe);
		// 	kill(ctx.pid, SIGTERM);
		// 	ctx.pid = -1;
		// 	ctx.stdin_pipe = -1;
		// 	ctx.stdout_pipe = -1;
		// 	ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;
		// 	return;
		// }
		fcntl(ctx.stdin_pipe, F_SETFL, O_NONBLOCK);
	}
	else
	{
		close(ctx.stdin_pipe);
		ctx.stdin_pipe = -1;
	}
}

