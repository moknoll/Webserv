#include "Client.hpp"
#include "../cgi/cgi.hpp"
#include "../constants.hpp"
#include "../http/HttpHandler.hpp"
#include "../lib/ws.hpp"
#include "../logger/Logger.hpp"

#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <ios>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const size_t Client::MAX_CGI_BUFFER = 105 * 1024 * 1024;

Client::Client(int fd, const ServerConfig& config) :
        config_(config),
        fd_(fd),
        keep_alive_(true),
        last_activity_(std::time(NULL)),
        state_(HTTP_RECV),
        request(config),
        response()
{
}

Client::~Client()
{
	close(fd_);
	safeClosePipeFds_();
	if (state_ == CGI_STATE && cgi_ctx_.pid != -1)
	{
		kill(cgi_ctx_.pid, SIGKILL);
	}
}

void Client::processRequest()
{
	if (request.getStatus())
	{
		response = HttpResponse::error(request, request.getStatus());
		keep_alive_ = false;
		state_ = HTTP_SEND;
		return;
	}

	if (!isAllowedMethod(request.getLocation(), request.getMethod()))
	{
		response = HttpResponse::error(request, HTTP_NOT_ALLOWED);
		state_ = HTTP_SEND;
		return;
	}

	if (request.getLocation().has_cgi)
	{
		executeCGI(request, cgi_ctx_);

		if (!cgi_ctx_.error)
		{
			state_ = CGI_STATE;
			return;
		}
		if (cgi_ctx_.error != ERR_CGI_SCRIPT_NOT_FOUND)
		{
			response = HttpResponse::error(request, cgi_ctx_.error);
			keep_alive_ = false;
			state_ = HTTP_SEND;
			return;
		}
	}

	HttpHandler h(config_);
	response = h.handle(request);
	state_ = HTTP_SEND;
}

std::string Client::serialize()
{
	if (send_buffer_.empty())
	{
		send_buffer_ = response.nextChunk();
	}

	return send_buffer_;
}

void Client::buildCGIResponse()
{
	std::string            raw_headers;
	std::string            body;
	int                    status = HTTP_OK;

	std::string::size_type sep = cgi_output_buf.find("\r\n\r\n");
	if (sep != std::string::npos)
	{
		raw_headers = cgi_output_buf.substr(0, sep);
		body = cgi_output_buf.substr(sep + 4);
	}
	else
	{
		sep = cgi_output_buf.find("\n\n");
		if (sep == std::string::npos)
		{
			response = HttpResponse::error(request, HTTP_BAD_GATEWAY);
			return;
		}

		raw_headers = cgi_output_buf.substr(0, sep);
		body = cgi_output_buf.substr(sep + 2);
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
		{
			response = HttpResponse::error(request, HTTP_BAD_GATEWAY);
			return;
		}

		std::string name = line.substr(0, c);
		std::string value = line.substr(c + 1);
		size_t      start = value.find_first_not_of(" \t");
		if (start != std::string::npos)
			value = value.substr(start);

		if (ws::toUpperCase(name) == "STATUS")
			status = std::atoi(value.c_str());
		if (ws::toUpperCase(name) == "LOCATION" && status == HTTP_OK)
		{
			status = 302;
			response.setHeader(name, value);
		}
		else
			response.setHeader(name, value);

		if (ws::toUpperCase(name) == "CONTENT-TYPE")
			have_content_type = true;

		if (ws::toUpperCase(name) == "CONTENT-LENGTH")
		{
			have_content_length = true;
			content_length = ws::stosize(value);
		}
	}

	if (!body.empty() && !have_content_type)
	{
		response = HttpResponse::error(request, HTTP_BAD_GATEWAY);
		return;
	}
	if (have_content_length && content_length != body.size())
	{
		body = body.substr(0, content_length);
	}

	response.setStatus(status);
	response.setBody(body);
	send_buffer_ = response.toString();
}

void Client::parseRequest(const char* buffer, size_t size)
{
	recv_buffer_.append(buffer, size);
	request.parse(recv_buffer_);
}

void Client::reset()
{
	resetCgiContext(cgi_ctx_);
	request.reset();
	response.reset();
	recv_buffer_.clear();
	send_buffer_.clear();
	state_ = HTTP_RECV;
	cgi_output_buf.clear();
}

static bool checkTimeOut(std::time_t start)
{
	std::time_t now = std::time(NULL);

	if (now - start > CGI_TIMEOUT_SEC)
		return true;

	return false;
}

void Client::tryFinalizeCGI_()
{
	if (!cgi_ctx_.pipe_stdout_eof || !cgi_ctx_.procese_reaped)
		return;

	if (cgi_ctx_.cgi_timed_out)
		response = HttpResponse::error(request, HTTP_GATEWAY_TIME_OUT);
	else if (cgi_ctx_.exit_ok)
		buildCGIResponse();
	else
		response = HttpResponse::error(request, HTTP_INTERNAL_SERVER_ERROR);

	cgi_ctx_.pid = -1;
	safeClosePipeFds_();
	state_ = HTTP_SEND;
}

bool Client::CGIProcessFinished()
{
	if (cgi_ctx_.pid == -1)
		return cgi_ctx_.pipe_stdout_eof && cgi_ctx_.procese_reaped;

	int   status;
	pid_t ret = waitpid(cgi_ctx_.pid, &status, WNOHANG);

	if (ret == cgi_ctx_.pid)
	{
		LOG_DEBUG("CGI process FINISH");
		cgi_ctx_.procese_reaped = true;
		cgi_ctx_.exit_ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
		cgi_ctx_.pid = -1;

		tryFinalizeCGI_();
		return cgi_ctx_.pipe_stdout_eof && cgi_ctx_.procese_reaped;
	}

	if (ret == -1)
	{
		LOG_DEBUG("CGI process error");
		cgi_ctx_.procese_reaped = true;
		cgi_ctx_.pipe_stdout_eof = true;
		cgi_ctx_.exit_ok = false;
		tryFinalizeCGI_();
		return true;
	}

	if (ret == 0 && checkTimeOut(cgi_ctx_.start_time))
	{
		kill(cgi_ctx_.pid, SIGKILL);
		cgi_ctx_.cgi_timed_out = true;

		pid_t ret = waitpid(cgi_ctx_.pid, &status, WNOHANG);
		if (ret == cgi_ctx_.pid)
		{
			cgi_ctx_.pipe_stdout_eof = true;
			cgi_ctx_.procese_reaped = true;
			cgi_ctx_.exit_ok = false;
			tryFinalizeCGI_();
			return true;
		}
	}
	return false;
}

bool Client::readCgiOutput_(int pipe_fd)
{
	char    buf[PIPE_BUF_SIZE];
	ssize_t ret = read(pipe_fd, buf, sizeof(buf));

	if (ret > 0)
		cgi_output_buf.append(buf, ret);
	else if (ret == 0)
	{
		LOG_DEBUG("EOF Reading from stdout pipe");
		cgi_ctx_.pipe_stdout_eof = true;
		tryFinalizeCGI_();
		return true;
	}
	return false;
}

bool Client::writeCgiInput(int pipe_fd)
{
	char buf[PIPE_BUF_SIZE];

	if (!cgi_ctx_.request_body.is_open())
	{
		if (cgi_ctx_.stdin_pipe != -1)
		{
			close(cgi_ctx_.stdin_pipe);
			cgi_ctx_.stdin_pipe = -1;
		}
		return true;
	}

	cgi_ctx_.request_body.read(buf, sizeof(buf));
	// if (cgi_ctx_.request_body_.fail()) ????
	std::streamsize read_bytes = cgi_ctx_.request_body.gcount();

	ssize_t         sent_bytes = write(pipe_fd, buf, read_bytes);
	if (sent_bytes < 0)
	{
		if (cgi_ctx_.request_body.eof())
			cgi_ctx_.request_body.clear();
		cgi_ctx_.request_body.seekg(-(read_bytes), std::ios::cur);
	}
	else if (sent_bytes > 0 && sent_bytes < read_bytes)
	{
		if (cgi_ctx_.request_body.eof())
			cgi_ctx_.request_body.clear();
		ssize_t rest_bytes = read_bytes - sent_bytes;
		cgi_ctx_.request_body.seekg(-(rest_bytes), std::ios::cur);
	}

	if (cgi_ctx_.request_body.eof())
	{
		cgi_ctx_.request_body.close();
		close(cgi_ctx_.stdin_pipe);
		cgi_ctx_.stdin_pipe = -1;
		return true;
	}

	return false;
}

bool Client::isAllowedMethod(const Location&    loc,
                             const std::string& method) const
{
	for (size_t i = 0; i < loc.allowed_methods.size(); i++)
	{
		if (method == loc.allowed_methods[i])
			return true;
	}
	return false;
}

void Client::setRecvBuffer(const std::string& buffer)
{
	this->recv_buffer_ = buffer;
}

void Client::setSendBuffer(const std::string& buffer)
{
	this->send_buffer_ = buffer;
}

bool Client::isRequestComplete() const
{
	return request.isComplete();
}

bool Client::isKeepAlive() const
{
	return keep_alive_;
}

std::string Client::getResponseBuffer() const
{
	return this->send_buffer_;
}

std::string Client::getRequestBuffer() const
{
	return this->recv_buffer_;
}

int Client::getClientFd() const
{
	return this->fd_;
}

int Client::getHttpState() const
{
	return state_;
}

CgiContext Client::getCGIContext() const
{
	return cgi_ctx_;
}

void Client::setState(enum STATE state)
{
	state_ = state;
}

void Client::safeClosePipeFds_()
{
	if (cgi_ctx_.stdin_pipe != -1)
	{
		close(cgi_ctx_.stdin_pipe);
		cgi_ctx_.stdin_pipe = -1;
	}
	if (cgi_ctx_.stdout_pipe != -1)
	{
		close(cgi_ctx_.stdout_pipe);
		cgi_ctx_.stdout_pipe = -1;
	}
}

void Client::updateLastActivity()
{
	last_activity_ = std::time(NULL);
}

time_t Client::getLastActivity() const
{
	return last_activity_;
}
