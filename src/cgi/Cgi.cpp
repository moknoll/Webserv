#include "Cgi.hpp"
#include "../http/constants.hpp"
#include "../lib/ws.hpp"

#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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
	response_ = HttpResponse(HTTP_OK);
	cgi_output_.reserve(1024 * 1024);
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

	return path;
}

int CgiContext::buildCgiEnvp(const HttpRequest& req)
{
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
		envp_.push_back(const_cast< char* >(env[i].c_str()));
	envp_.push_back(NULL);

	return HTTP_OK;
}

int CgiContext::buildCgiArgv(const HttpRequest& req, const Location& loc)
{
	std::string            uri = req.getURI();
	std::string::size_type p = uri.find('?');
	std::string            path = buildPath(uri.substr(0, p), loc);

	PathInfo               path_info = ws::checkPath(path);
	PathInfo               cgi_path_info = ws::checkPath(loc.cgi_path);
	if ((!path_info.exists && !path_info.readable) || path_info.type == PATH_IS_DIR)
	{ 
		return HTTP_NOT_FOUND;
	}
	
	if (!cgi_path_info.exists && !cgi_path_info.readable)
		{
			return HTTP_INTERNAL_SERVER_ERROR;
		}
	if (!path_info.executable)
		return HTTP_FORBIDDEN;
	args.push_back(loc.cgi_path);
	args.push_back(path);
	// args.push_back(path);
	for (size_t i = 0; i < args.size(); ++i)
		argv_.push_back(const_cast< char* >(args[i].c_str()));
	argv_.push_back(NULL);
	for (size_t i = 0; i < argv_.size(); ++i)
		std::cout << "args:" << this->argv_[i] << std::endl;
	return HTTP_OK;
}

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
		std::exit(127);
	}
	close(stdin_pipe[0]);
	close(stdout_pipe[1]);

	return true;
}

// bool CgiContext::writeRequestBody(const HttpRequest& req)
// {
// 	std::string body_temp_file = req.getBodyTempFileName();
// 	int         fd = open(body_temp_file.c_str(), O_RDONLY);
// 	if (fd == -1)
// 		return false;
//
// 	char    buf[4096];
// 	ssize_t r;
// 	while ((r = read(fd, buf, sizeof(buf))) > 0)
// 		write(stdin_pipe[1], buf, r);
// 	close(stdin_pipe[1]);
//
// 	return true;
// }
//
// bool CgiContext::readChildOutput()
// {
// 	char    buf[4096];
// 	ssize_t r;
// 	while ((r = read(stdout_pipe[0], buf, sizeof(buf))) > 0)
// 	{
// 		if (r + cgi_output_.size() > MAX_CGI_BUFFER)
// 			return false;
// 		cgi_output_.append(buf, r);
// 	}
//
// 	close(stdout_pipe[0]);
// 	return true;
// }

int CgiContext::buildResponse()
{
	if (cgi_output_.empty())
	{
		return HTTP_BAD_GATEWAY;
	}

	std::string            raw_headers;
	std::string            body;

	std::string::size_type sep = cgi_output_.find("\r\n\r\n");
	if (sep != std::string::npos)
	{
		raw_headers = cgi_output_.substr(0, sep);
		body = cgi_output_.substr(sep + 4);
	}
	else
	{
		sep = cgi_output_.find("\n\n");
		if (sep == std::string::npos)
			return HTTP_BAD_GATEWAY;

		raw_headers = cgi_output_.substr(0, sep);
		body = cgi_output_.substr(sep + 2);
	}

	std::stringstream ss(raw_headers);
	std::string       line;
	std::size_t       content_length = 0;
	bool              have_content_type = false;
	bool              have_content_length = false;

	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue;

		size_t c = line.find(':');
		if (c == std::string::npos)
			return HTTP_BAD_GATEWAY;

		std::string name = line.substr(0, c);
		std::string value = line.substr(c + 1);
		size_t      start = value.find_first_not_of(" \t");
		if (start != std::string::npos)
			value = value.substr(start);

		if (ws::toUpperCase(name) == "STATUS")
			exit_status_ = std::atoi(value.c_str());
		else if (ws::toUpperCase(name) == "LOCATION" && exit_status_ == 0)
		{
			exit_status_ = 302;
			response_.setHeader(name, value);
		}
		else
			response_.setHeader(name, value);

		if (ws::toUpperCase(name) == "CONTENT-TYPE")
			have_content_type = true;

		if (ws::toUpperCase(name) == "CONTENT-LENGTH")
		{
			have_content_length = true;
			content_length = ws::stosize(value);
		}
	}
	if (!have_content_type || !have_content_length
	    || content_length != body.size())
		return HTTP_BAD_GATEWAY;

	if (exit_status_ != 0)
		response_.setStatus(exit_status_);
	response_.setBody(body);
	return HTTP_OK;
}

int CgiContext::executeCGI(const HttpRequest& req, const Location& loc)
{
	int ret = HTTP_OK;

	ret = buildCgiEnvp(req);
	if (ret != HTTP_OK)
		return ret;

	ret = buildCgiArgv(req, loc);
	if (ret != HTTP_OK)
		return ret;
	if (!executeChild())
	{
		std::cout << "here" << std::endl;
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
	if (req.getMethod() == "POST")
		fcntl(stdin_pipe[1], F_SETFL, O_NONBLOCK);
	else
	{
		close(stdin_pipe[1]);
		stdin_pipe[1] = -1;
	}

	// if (req.getMethod() == "POST" && !writeRequestBody(req))
	// 	return HTTP_INTERNAL_SERVER_ERROR;

	// if (!readChildOutput())
	// 	return HTTP_INTERNAL_SERVER_ERROR;

	// int status = waitChildProc(5);
	//
	// if (WIFEXITED(status))
	// 	;
	// else if (WIFSIGNALED(status))
	// 	return HTTP_GATEWAY_TIME_OUT;
	// else
	// 	return HTTP_INTERNAL_SERVER_ERROR;
	//
	// exit_status_ = buildResponse();
	// if (exit_status_ != HTTP_OK)
	// 	return exit_status_;

	return HTTP_OK;
}

HttpResponse CgiContext::getResponse() const
{
	return response_;
}

int CgiContext::getFdCGI_out() const
{
	return stdout_pipe[0];
}

int CgiContext::getFdCGI_in() const
{
	return stdin_pipe[1];
}

int CgiContext::getStatus() const
{
	return exit_status_;
}

int CgiContext::getCgiPid() const
{
	return pid_;
}

void CgiContext::appendCgiOutput(const char* buf, size_t size)
{
	cgi_output_.append(buf, size);
	std::cout << cgi_output_;
}

