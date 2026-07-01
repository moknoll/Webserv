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
	start_time_ = 0;
	response_ = HttpResponse(HTTP_OK);
	cgi_output_buf_.reserve(1024 * 1024);
	request_body_fd_ = -1;
	cgi_input_buf_.resize(512 * 1024, '\0');
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

		for (size_t i = 0; i < env.size(); ++i)
			envp_.push_back(const_cast< char* >(env[i].c_str()));
		envp_.push_back(NULL);

		return HTTP_OK;
	}
	return HTTP_OK;
}

int CgiContext::buildCgiArgv(const HttpRequest& req, const Location& loc)
{
	std::string            uri = req.getURI();
	std::string::size_type p = uri.find('?');
	std::string            path = buildPath(uri.substr(0, p), loc);

	PathInfo               path_info = ws::checkPath(path);
	PathInfo               cgi_path_info = ws::checkPath(loc.cgi_path);
	if (!path_info.exists && !path_info.readable)
	{
		return HTTP_NOT_FOUND;
	}
	if (!cgi_path_info.exists && !cgi_path_info.readable
	    && !cgi_path_info.executable)
		return HTTP_INTERNAL_SERVER_ERROR;

	args.push_back(loc.cgi_path);
	args.push_back(path);
	// args.push_back(path);
	for (size_t i = 0; i < args.size(); ++i)
		argv_.push_back(const_cast< char* >(args[i].c_str()));
	argv_.push_back(NULL);
	return HTTP_OK;
}

bool CgiContext::executeChild()
{
	if (pipe(stdin_pipe) == -1)
		return false;
	if (pipe(stdout_pipe) == -1)
	{
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		return false;
	}
	pid_ = fork();
	if (pid_ < 0)
	{
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);

		return false;
	}

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

int CgiContext::buildResponse()
{
	if (cgi_output_buf_.empty())
	{
		return HTTP_BAD_GATEWAY;
	}

	std::string            raw_headers;
	std::string            body;

	std::string::size_type sep = cgi_output_buf_.find("\r\n\r\n");
	if (sep != std::string::npos)
	{
		raw_headers = cgi_output_buf_.substr(0, sep);
		body = cgi_output_buf_.substr(sep + 4);
	}
	else
	{
		sep = cgi_output_buf_.find("\n\n");
		if (sep == std::string::npos)
			return HTTP_BAD_GATEWAY;

		raw_headers = cgi_output_buf_.substr(0, sep);
		body = cgi_output_buf_.substr(sep + 2);
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
		return HTTP_INTERNAL_SERVER_ERROR;

	fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
	if (req.getMethod() == "POST")
	{
		fcntl(stdin_pipe[1], F_SETFL, O_NONBLOCK);
	}
	else
	{
		close(stdin_pipe[1]);
		stdin_pipe[1] = -1;
	}

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

bool CgiContext::writeRequestBody()
{
	static const size_t FILE_CHUNK_SIZE = 512 * 1024;

	if (cgi_input_buf_.empty() && request_body_fd_ != -1)
	{
		ssize_t n = read(request_body_fd_, &cgi_input_buf_[0], FILE_CHUNK_SIZE);
		if (n <= 0)
		{
			closeFile();
		}

		if (n > 0 && static_cast< size_t >(n) < FILE_CHUNK_SIZE)
		{
			cgi_input_buf_.resize(static_cast< size_t >(n));
			closeFile();
		}
	}

	if (cgi_input_buf_.empty())
		return true;

	std::cout << cgi_input_buf_ << '\n';
	ssize_t r =
	    write(stdin_pipe[1], cgi_input_buf_.data(), cgi_input_buf_.size());
	if (r <= 0)
	{
		return true;
	}

	size_t sent = static_cast< size_t >(r);
	cgi_input_buf_.erase(0, sent);

	return false;
}

void CgiContext::appendCgiOutput(const char* buf, size_t size)
{
	cgi_output_buf_.append(buf, size);
}

// void CgiContext::reset()
// {
// 	pid_ = -1;
// 	if (stdin_pipe[0] != -1)
// 	{
// 		close(stdin_pipe[0]);
// 		stdin_pipe[0] = -1;
// 	}
//
// 	int                                  stdin_pipe[1] = -1;
// 	int                                  stdout_pipe[2];
// 	std::map< std::string, std::string > env_map;
// 	int                                  exit_status_;
// 	time_t                               start_time_;
// 	HttpResponse                         response_;
// 	int                                  request_body_fd_;
// 	std::string                          cgi_input_buf_;
//
// 	std::vector< char* >                 envp_;
// 	std::vector< char* >                 argv_;
// 	std::vector< std::string >           env;
// 	std::vector< std::string >           args;
// }

void CgiContext::closeFile()
{
	if (request_body_fd_ != -1)
		close(request_body_fd_);
	request_body_fd_ = -1;
}

// void CgiContext::ClosePipes() {
// 	if (c
//
// }
