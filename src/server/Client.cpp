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

const size_t Client::MAX_CGI_BUFFER = 100 * 1024 * 1024;

Client::Client(int fd, const ServerConfig& config) :
        config_(config),
        fd_(fd),
        keep_alive_(true),
        state_(HTTP_RECV),
        request(config),
        response(),
        // handler(config),
        i(0)
{
}

Client::~Client()
{
	close(fd_);
}

void Client::processRequest()
{
	if (request.getStatus())
	{
		keep_alive_ = false;
		state_ = HTTP_SEND;
		response = HttpResponse::error(request, request.getStatus());
		return;
	}

	if (request.getLocation().has_cgi)
	{
		executeCGI(request, cgi_ctx_);
		// request_body_fd_ = cgi_ctx_.request_body_fd;

		cgi_output_buf.clear();

		if (cgi_ctx_.exit_status)
		{
			response = HttpResponse::error(request, cgi_ctx_.exit_status);
			state_ = HTTP_SEND;
			keep_alive_ = false;
			return;
		}
		state_ = CGI_STATE;
	}
	else
	{
		HttpHandler h(config_);
		response = h.handle(request);
		state_ = HTTP_SEND;
	}
}

std::string Client::serialize()
{
	if (send_buffer_.empty())
	{
		send_buffer_ = response.nextChunk();
	}
	// send_buffer_ = handler.getFileChunk();

	return send_buffer_;
}

void Client::buildCGIResponse()
{
	std::string            raw_headers;
	std::string            body;
	int                    status = 200;

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
		else if (ws::toUpperCase(name) == "LOCATION" && status == 0)
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
	std::cout << "body size: " << body.size()
	          << " content_length: " << content_length << '\n';
	if (!have_content_type)
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
	// handler.reset();
	request.reset();
	response.reset();
	recv_buffer_.clear();
	send_buffer_.clear();
	state_ = HTTP_RECV;
	cgi_output_buf.clear();
}

bool checkTimeOut(std::time_t start)
{
	std::time_t now = std::time(NULL);

	if (now - start > 5)
		return true;

	return false;
}

bool Client::CGIProcessFinished()
{
	int   status;
	pid_t pid = cgi_ctx_.pid;
	pid_t ret = waitpid(pid, &status, WNOHANG);
	if (ret == 0)
	{
		if (checkTimeOut(cgi_ctx_.start_time))
		{
			kill(cgi_ctx_.pid, SIGTERM);
			pid_t r = waitpid(cgi_ctx_.pid, &status, WNOHANG);
			if (r == 0)
			{
				std::cout << "process don't finished\n";
				// kill(cgi_ctx_.pid, SIGINT);
			}
			response = HttpResponse::error(request, HTTP_GATEWAY_TIME_OUT);
			cgi_ctx_.pid = -1;
			safeClosePipeFds_();
			state_ = HTTP_SEND;
			return true;
		}
		return false;
	}
	else if (ret == pid)
	{
		if (WIFEXITED(status))
		{
			int code = WEXITSTATUS(status);
			if (code == 0)
			{
				buildCGIResponse();
			}
			else
			{
				response =
				    HttpResponse::error(request, HTTP_INTERNAL_SERVER_ERROR);
			}
		}
		cgi_ctx_.pid = -1;
		safeClosePipeFds_();
	}
	else
	{
		response = HttpResponse::error(request, HTTP_INTERNAL_SERVER_ERROR);
	}
	state_ = HTTP_SEND;
	return true;
}

void Client::appendCgiOutput(const char* buf, size_t size_of_bytes)
{
	cgi_output_buf.append(buf, size_of_bytes);
}

bool Client::writeRequestBody(int fd)
{
	++i;
	// static const size_t FILE_CHUNK_SIZE = 512 * 1024;
	char buf[1024 * 512];

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

	ssize_t         sent_bytes = write(fd, buf, read_bytes);
	if (sent_bytes <= 0)
	{
		if (cgi_ctx_.request_body.eof())
			cgi_ctx_.request_body.clear();
		cgi_ctx_.request_body.seekg(-(read_bytes), std::ios::cur);
	}
	if (sent_bytes > 0 && sent_bytes < read_bytes)
	{
		if (cgi_ctx_.request_body.eof())
			cgi_ctx_.request_body.clear();
		ssize_t rest_bytes = read_bytes - sent_bytes;
		LOG_DEBUG("rest_bytes: " + ws::to_string(rest_bytes));
		cgi_ctx_.request_body.seekg(-(rest_bytes), std::ios::cur);
	}

	LOG_DEBUG("read: " + ws::to_string(read_bytes));
	LOG_DEBUG("sent: " + ws::to_string(sent_bytes));

	// if (i == 5)
	// 	std::exit(10);
	if (cgi_ctx_.request_body.eof())
	{
		cgi_ctx_.request_body.close();
		close(cgi_ctx_.stdin_pipe);
		cgi_ctx_.stdin_pipe = -1;
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
