#include "cgi.hpp"
#include "../constants.hpp"
#include "../http/HttpRequest.hpp"
#include "../lib/ws.hpp"
#include "../logger/Logger.hpp"

#include <algorithm>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
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
	error = 0;
	start_time = 0;
	exit_ok = false;
	pipe_stdout_eof = false;
	procese_reaped = false;
	cgi_timed_out = false;
}

CgiContext::CgiContext(const CgiContext& other) :
        pid(other.pid),
        stdin_pipe(other.stdin_pipe),
        stdout_pipe(other.stdout_pipe),
        error(other.error),
        start_time(other.start_time),
        exit_ok(other.exit_ok),
        pipe_stdout_eof(other.pipe_stdout_eof),
        procese_reaped(other.procese_reaped),
        cgi_timed_out(other.cgi_timed_out)

{
}

CgiContext::~CgiContext()
{
	if (request_body.is_open())
		request_body.close();
}

void resetCgiContext(CgiContext& ctx)
{
	ctx.pid = -1;
	ctx.stdin_pipe = -1;
	ctx.stdout_pipe = -1;
	ctx.error = 0;
	ctx.start_time = 0;
	ctx.exit_ok = false;
	ctx.pipe_stdout_eof = false;
	ctx.procese_reaped = false;
	ctx.cgi_timed_out = false;
	if (ctx.request_body.is_open())
		ctx.request_body.close();
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
	std::vector< std::string > argv;
	std::string path = req.getPath();
	std::string	extention = req.getLocation().cgi_extension;
	size_t      ext_pos = path.find(extention);
	if (ext_pos == std::string::npos)
	{
		LOG_DEBUG("CGI SCRIPT NOT FOUND: handle like static file");
		ctx.error = ERR_CGI_SCRIPT_NOT_FOUND;
		return argv;

	}
	path = path.substr(0, ext_pos + 3);

	PathInfo path_info = ws::checkPath(path);
	PathInfo cgi_path_info = ws::checkPath(req.getLocation().cgi_path);
	if (!path_info.exists && !path_info.readable)
	{
		LOG_DEBUG("Path to cgi script: " + path);
		ctx.error = HTTP_NOT_FOUND;
	}
	if (!cgi_path_info.exists && !cgi_path_info.readable
	    && !cgi_path_info.executable)
	{
		LOG_DEBUG("cgi_path: " + req.getLocation().cgi_path);
		ctx.error = HTTP_INTERNAL_SERVER_ERROR;
	}

	std::string interp_path = ws::getAbsolutePath(req.getLocation().cgi_path);
	argv.push_back(interp_path);
	argv.push_back(path);
	return argv;
}

bool executeChild(CgiContext&                       ctx,
                  const std::vector< std::string >& args,
                  const std::vector< std::string >& env)
{
	std::string path_to_chdir = "./";
	std::string script_name = args[1];
	size_t      p = args[1].find_last_of('/');
	if (p != std::string::npos)
	{
		path_to_chdir = args[1].substr(0, p);
		script_name = args[1].substr(p + 1);
	}

	std::vector< char* > envp;
	std::vector< char* > argv;
	argv.push_back(const_cast< char* >(args[0].c_str()));
	argv.push_back(const_cast< char* >(script_name.c_str()));
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
		ctx.error = HTTP_INTERNAL_SERVER_ERROR;
		return false;
	}
	if (pipe(pipe_out) == -1)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		ctx.error = HTTP_INTERNAL_SERVER_ERROR;
		return false;
	}
	pid_t pid = fork();
	if (pid < 0)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		ctx.error = HTTP_INTERNAL_SERVER_ERROR;
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

		if (::chdir(path_to_chdir.c_str()) != 0)
			LOG_ERROR("Error: chdir, connot change directory");

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

	if (ctx.error)
	{
		LOG_DEBUG("Error: buildCgiArgv: err=" + ws::to_string(ctx.error));
		return;
	}

	std::vector< std::string > env = buildEnv(req);
	if (ctx.error)
	{
		LOG_DEBUG("Error: buildEnv: err = " + ws::to_string(ctx.error));
		return;
	}

	if (!executeChild(ctx, args, env))
	{
		LOG_DEBUG("Error: executeChild");
		return;
	}

	fcntl(ctx.stdout_pipe, F_SETFL, O_NONBLOCK);
	if (req.getMethod() == "POST")
	{
		ctx.request_body.open(req.getBodyTempFileName().c_str(),
		                      std::ios::in | std::ios::binary);
		if (!ctx.request_body.is_open())
		{
			LOG_DEBUG("Error opening file ctx.request_body");
			close(ctx.stdin_pipe);
			close(ctx.stdout_pipe);
			kill(ctx.pid, SIGTERM);
			ctx.pid = -1;
			ctx.stdin_pipe = -1;
			ctx.stdout_pipe = -1;
			ctx.error = HTTP_INTERNAL_SERVER_ERROR;
			return;
		}

		fcntl(ctx.stdin_pipe, F_SETFL, O_NONBLOCK);
	}
	else
	{
		close(ctx.stdin_pipe);
		ctx.stdin_pipe = -1;
	}
}

