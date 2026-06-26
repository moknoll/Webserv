#include "Cgi.hpp"
#include "../lib/ws.hpp"
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "../http/constants.hpp"

const size_t CgiContext::MAX_CGI_BUFFER = 10 * 1024 * 1024;

CgiContext::CgiContext()
{
	pid_ = -1;
	stdin_pipe[0] = -1;
	stdin_pipe[1] = -1;
	stdout_pipe[0] = -1;
	stdout_pipe[1] = -1;
	exit_status_ = 0;
	deadline = 0;
	response = HttpResponse();
}

std::string CgiContext::buildPath(const std::string& uri,
                                  const Location&    loc) const
{
	std::string sub_uri = uri.substr(loc.path.length());
	std::string path = loc.root;
	if (!path.empty() && path[path.length() - 1] != '/')
		path += '/';

	if (!sub_uri.empty() && sub_uri[0] == '/')
		path += sub_uri.substr(1);
	else
		path += sub_uri;
	std::cout << path << std::endl;
	return path;
}

static std::string normalizeHeader(std::string name)
{
    for (size_t i = 0; i < name.size(); i++)
    {
        if (name[i] == '-')
            name[i] = '_';
        else
            name[i] = std::toupper(static_cast<unsigned char>(name[i]));
    }
    return name;
}

void CgiContext::buildHttpHeaders(const HttpRequest& req)
{
	const std::map<std::string, std::string>& headers = req.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		if (it->first == "Content-Length" || it->first == "Content-Type")
			continue;
		std::string header_name = "HTTP_" + normalizeHeader(it->first);
		env[header_name] = it->second;
	}
}

void CgiContext::buildEnv(const HttpRequest& req, const std::string& script_path, const ServerConfig& cfg)
{
	std::string uri = req.getURI();
	size_t py = uri.find(".py");
	size_t q = uri.find("?");
	
	if (req.getMethod() == "POST")
	{
		env["CONTENT_LENGTH"] = ws::to_string(req.getContentLenght());
		env["CONTENT_TYPE"] = req.getHeader("Content-Type");
	}	
	env["REQUEST_METHOD"] = req.getMethod();

	env["SERVER_NAME"] = cfg.server_name;
	env["SERVER_PORT"] = ws::to_string(cfg.port);
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_SOFTWARE"] = SERVER_NAME_STR;
	env["GATEWAY_INTERFACE"] = "CGI/1.1";

	std::cout << "test" << std::endl;
	if (py != std::string::npos)
	{
		env["SCRIPT_NAME"] = script_path.substr(0, py);
		env["PATH_INFO"] = script_path.substr(py);

		if(!env["PATH_INFO"].empty())
			env["PATH_TRANSLATED"] = cfg.root + env["PATH_INFO"];
		else
			env["PATH_TRANSLATED"] = "";
	}

	if (q != std::string::npos)
		env["QUERY_STRING"] = uri.substr(q + 1);
	else 
		env["QUERY_STRING"] = "";
}

void CgiContext::buildCgiEnvp(const HttpRequest& req, const ServerConfig& cfg, const Location& loc)
{
	env.clear();
	envp_.clear();
	std::string script = buildPath(req.getURI(), loc);

	buildEnv(req, script, cfg);
	buildHttpHeaders(req);
	
	for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it)
	{
		std::string env_var = it->first + "=" + it->second;
		env_map[it->first] = env_var;
		envp_.push_back(const_cast<char*>(env_map[it->first].c_str()));
	}
	envp_.push_back(NULL);
	for (size_t i = 0; i < envp_.size(); ++i)
		std::cout << "envp_[" << i << "] = " << envp_[i] << std::endl;
}

void CgiContext::buildCgiArgv(const HttpRequest& req, const Location& loc)
{
	std::string            uri = req.getURI();
	std::string::size_type p = uri.find('?');
	std::string            path = buildPath(uri.substr(0, p), loc);

	PathInfo               path_info = ws::checkPath(path);
	PathInfo               cgi_path_info = ws::checkPath(loc.cgi_path);
	if (!path_info.exists && !path_info.readable && !path_info.executable)
		return;
	if (!cgi_path_info.exists && !cgi_path_info.readable
	    && !cgi_path_info.executable)
		return;

	args.push_back(loc.cgi_path);
	args.push_back(path);
	for (size_t i = 0; i < args.size(); ++i)
		argv_.push_back(const_cast< char* >(args[i].c_str()));
	argv_.push_back(NULL);
}

//--------------------------------------------------------------------------

bool CgiContext::executeChild()
{
	if (pipe(stdin_pipe) == -1)
		return false;
	if (pipe(stdout_pipe) == -1)
		return false;
	pid_ = fork();
	if (pid_ < 0)
		return false;

	if (pid_ == 0)
	{
		dup2(stdin_pipe[0], STDIN_FILENO);
		dup2(stdout_pipe[1], STDOUT_FILENO);

		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);

		execve(argv_[0], &argv_[0], &envp_[0]);
		_exit(127);
	}
	close(stdin_pipe[0]);
	close(stdout_pipe[1]);

	return true;
}

bool CgiContext::writeRequestBody(const HttpRequest& req)
{
	std::string body_temp_file = req.getBodyTempFileName();
	int         fd = open(body_temp_file.c_str(), O_RDONLY);
	if (fd == -1)
		return false;

	char    buf[4096];
	ssize_t r;
	while ((r = read(fd, buf, sizeof(buf))) > 0)
		write(stdin_pipe[1], buf, r);
	close(stdin_pipe[1]);

	return true;
}

bool CgiContext::readChildOutput()
{
	char    buf[4096];
	ssize_t r;
	while ((r = read(stdout_pipe[0], buf, sizeof(buf))) > 0)
	{
		if (r + cgi_output_.size() > MAX_CGI_BUFFER)
			return false;
		cgi_output_.append(buf, r);
	}

	close(stdout_pipe[0]);
	return true;
}

HttpResponse CgiContext::buildResponse()
{
	HttpResponse           res(200);
	std::string::size_type header_end = cgi_output_.find("\r\n\r\n");
	size_t                 body_start = header_end + 4;

	res.setBody(cgi_output_.substr(body_start));

	std::stringstream ss(cgi_output_);
	std::string       line;
	while (std::getline(ss, line))
	{
		if (line == "\r")
			break;
		size_t p = line.find(':');
		std::cout << line << '\n';
		if (line.substr(0, p) == "Content-length")

			continue;
		res.setHeader(line.substr(0, p), line.substr(p + 1));
	}
	return res;
}

HttpResponse CgiContext::handle(const HttpRequest& req, const Location& loc, const ServerConfig& cfg)
{
	buildCgiEnvp(req, cfg, loc);
	buildCgiArgv(req, loc);

	if (!executeChild())
		return HttpResponse(500);

	if (req.getMethod() == "POST" && !writeRequestBody(req))
		return HttpResponse(500);

	readChildOutput();
	int status = 0;
	waitpid(pid_, &status, 0);

	return buildResponse();
}
