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
#include <iostream>

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

static std::string normalizeHeader(std::string name)
{
	for(int i = 0; i < name.size(); i++)
	{
		if(name[i] == '-')
			name[i] = '_';
		else 
			name[i] = std::toupper(static_cast<unsigned char>(name[i]));
	}
	return name;
}

std::map< std::string, std::string> buildCGIHeaders(const HttpRequest &req)
{
	const std::map<std::string, std::string>& headers = req.getHeaders();
	std::map<std::string, std::string> result;

	for(std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		if(it->first == "Content-Length" || it->first == "Content-Type")
			continue;
		std::string header_name = "HTTP_" + normalizeHeader(it->first);
		result[header_name] = it->second;
	}
}


std::map< std::string, std::string> buildEnv(const HttpRequest &req, const ServerConfig &cfg)
{
	std::string uri = req.getURI();
	size_t ext = uri.find(".py");
	size_t q = uri.find("?");
	std::map<std::string, std::string> env;

	env = buildCGIHeaders(req);
	if(req.getMethod() == "POST")
	{
		env["CONTENT_LENGTH"] = ws::to_string(req.getContentLenght());
		env["CONTENT_TYPE"] = req.getHeader("Content-Type");
	}
	env["REQUEST_METHOD"] = req.getMethod();

	env["SERVER_NAME"] = cfg.server_name;
	env["SERVER_PORT"] = ws::to_string(cfg.port);
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["GATEWAY_INTERFACE"] = "CGI/1.1";

	if(ext != std::string::npos)
	{
		env["SCRIPT_NAME"] = req.getPath().substr(0, ext);
		env["PATH_INFO"] = req.getPath().substr(ext);

		if(!env["PATH_INFO"].empty())
			env["PATH_TRANSLATED"] = cfg.root + env["PATH_INFO"];
		else
			env["PATH_TRANSLATED"] = "";
	}
	if( q != std::string::npos)
		env["QUERY_STRING"] = uri.substr(q+1);
	else
		env["QUERY_STRING"] = "";
	
	return env;
}

std::vector<std::string> buildCgiEnvp(const HttpRequest& req, CgiContext& ctx, ServerConfig &cfg)
{
	std::vector< std::string > envp;
	std::map<std::string, std::string> env = buildEnv(req, cfg);


	for(std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it)
	{
		std::string env_var = it->first + "=" + it->second;
		envp.push_back(const_cast<char *>(env_var.c_str()));		
	}
	envp.push_back(NULL);

	for(int i = 0; i < envp.size(); ++i)
		std::cout << "envp" << i << envp[i] << std::endl;

	ctx.exit_status = HTTP_OK;
	return envp;
}




std::vector < char *>buildCgiArgv(const HttpRequest& req, CgiContext& ctx)
{
	std::string            uri = req.getURI();
	std::string::size_type p = uri.find('?');
	std::string            path = req.getPath();
	PathInfo               path_info = ws::checkPath(path);
	PathInfo               cgi_path_info = ws::checkPath(req.getLocation().cgi_path);
	std::vector < char *>  argv;
	if (!path_info.exists && !path_info.readable)
	{
		ctx.exit_status = HTTP_NOT_FOUND;
	}
	if (!cgi_path_info.exists && !cgi_path_info.readable
	    && !cgi_path_info.executable)
	{
		ctx.exit_status = HTTP_INTERNAL_SERVER_ERROR;;
	}

	argv.push_back(const_cast< char* >(req.getLocation().cgi_path.c_str()));
	argv.push_back(const_cast< char* >(path.c_str()));
	argv.push_back(NULL);
	ctx.exit_status = HTTP_OK;
	return argv;
}

bool executeChild(CgiContext& ctx, std::vector<char *> envp, std::vector< char *>argv)
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

int executeCGI(const HttpRequest &req, CgiContext &ctx, ServerConfig &cfg)
{
	std::vector< char *> argv = buildCgiArgv(req, ctx);
	std::vector< std::string > envp = buildCgiEnvp(req, ctx, cfg);

	if (ctx.exit_status != HTTP_OK)
		return ctx.exit_status;
	if(!executeChild(ctx))
		return HTTP_INTERNAL_SERVER_ERROR;
	fcntl(ctx.stdout_pipe, F_SETFL, O_NONBLOCK);
	if(req.getMethod() == "POST")
		fcntl(ctx.stdin_pipe, F_SETFL, O_NONBLOCK);
	else 
	{
		close(ctx.stdin_pipe);
		ctx.stdin_pipe = -1;
	}
	return HTTP_OK;
}