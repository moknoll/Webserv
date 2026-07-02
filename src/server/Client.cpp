#include "Client.hpp"
#include "../cgi/cgi.hpp"
#include "../http/constants.hpp"
#include "../lib/ws.hpp"

#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const size_t Client::MAX_CGI_BUFFER = 100 * 1024 * 1024;

Client::Client(int fd, const ServerConfig& config) :
        cgi_pipe_in(-1),
        cgi_pipe_out(-1),
        cgi_pid(-1),
        config_(config),
        fd_(fd),
        keep_alive_(true),
        state_(HTTP_RECV),
        request(config),
        response(),
        handler(config)
{
}

Client::~Client()
{
	close(fd_);
}

void Client::processRequest()
{
	if (request.getRequestStatus() != HTTP_OK)
	{
		keep_alive_ = false;
		state_ = HTTP_SEND;

		send_buffer_ =
		    handler.makeStatusResponse(request.getRequestStatus()).toString();
		return;
	}
	const Location* loc = request.getLocation();
	if (loc == NULL)
	{
		keep_alive_ = false;
		state_ = HTTP_SEND;
		send_buffer_ = handler.makeStatusResponse(HTTP_NOT_FOUND).toString();
		return;
	}

	if (loc->has_cgi)
	{
		// CgiContext cgi;

		executeCGI(request, cgi, config_);
		cgi_pipe_in = cgi.stdin_pipe;
		cgi_pipe_out = cgi.stdout_pipe;
		request_body_fd_ = cgi.request_body_fd;

		cgi_pid = cgi.pid;
		cgi_output_buf.clear();
		cgi_input_buf_.clear();

		if (cgi.exit_status)
		{
			send_buffer_ =
			    handler.makeStatusResponse(cgi.exit_status).toString();
			state_ = HTTP_SEND;
			keep_alive_ = false;
			return;
		}

		state_ = CGI_STATE;
	}
	else
	{
		response = handler.handle(request);
		if (response.isReady()) // ???
		{
			send_buffer_ = response.toString();
			state_ = HTTP_SEND;
		}
	}
}

std::string Client::serialize()
{
	if (send_buffer_.empty())
		send_buffer_ = handler.getFileChunk();

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
			send_buffer_ =
			    handler.makeStatusResponse(HTTP_BAD_GATEWAY).toString();
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
			send_buffer_ =
			    handler.makeStatusResponse(HTTP_BAD_GATEWAY).toString();
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
	if (!have_content_type
	    || (have_content_length && content_length != body.size()))

	{
		send_buffer_ = handler.makeStatusResponse(HTTP_BAD_GATEWAY).toString();
		return;
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
	handler.reset();
	request.reset();
	response.reset();
	recv_buffer_.clear();
	send_buffer_.clear();
	state_ = HTTP_RECV;
}

bool Client::CGIProcessFinished()
{
	int   status;
	pid_t pid = cgi_pid;
	pid_t ret = waitpid(pid, &status, WNOHANG);
	if (ret == 0)
	{
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
				// int cgi_status = cgi.buildResponse();
				// if (cgi_status == HTTP_OK)
				// send_buffer_ = cgi.getResponse().toString();
				// else
				// send_buffer_ = makeStatusResponse(cgi_status).toString();
			}
			else
			{
				send_buffer_ =
				    handler.makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR)
				        .toString();
			}
		}
		else if (WIFSIGNALED(status))
			send_buffer_ =
			    handler.makeStatusResponse(HTTP_GATEWAY_TIME_OUT).toString();
		cgi_pid = -1;
		close(cgi_pipe_in);
		close(cgi_pipe_out);
		cgi_pipe_in = -1;
		cgi_pipe_out = -1;
	}
	else
	{
		send_buffer_ =
		    handler.makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR).toString();
	}
	state_ = HTTP_SEND;
	return true;
}

bool Client::writeRequestBody()
{
	// static const size_t FILE_CHUNK_SIZE = 512 * 1024;
	char buf[1024 * 512];

	std::cout << "I AM HERE\n";

	if (!cgi_input_buf_.empty())
	{
		// std::cout << cgi_input_buf_ << '\n';
		ssize_t r =
		    write(cgi_pipe_in, cgi_input_buf_.data(), cgi_input_buf_.size());
		if (r <= 0)
		{
			return true;
		}

		size_t sent = static_cast< size_t >(r);
		cgi_input_buf_.erase(0, sent);
	}

	if (cgi_input_buf_.empty() && request_body_fd_ != -1)
	{
		// ssize_t n = read(request_body_fd_, &cgi_input_buf_[0],
		// FILE_CHUNK_SIZE);
		ssize_t n = read(request_body_fd_, buf, sizeof(buf));
		if (n <= 0)
		{
			close(request_body_fd_);
			request_body_fd_ = -1;
		}

		cgi_input_buf_.append(buf, n);
		// if (n > 0 && static_cast< size_t >(n) < FILE_CHUNK_SIZE)
		// {
		// 	cgi_input_buf_.resize(static_cast< size_t >(n));
		// 	close(request_body_fd_);
		// 	request_body_fd_ = -1;
		// }
	}

	if (cgi_input_buf_.empty())
	{
		if (request_body_fd_ != -1)
		{
			close(request_body_fd_);
			request_body_fd_ = -1;
		}
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

int Client::getFdCGI_in() const
{
	// return cgi.getFdCGI_in();
	return cgi_pipe_in;
}

int Client::getFdCGI_out() const
{
	// return cgi.getFdCGI_out();
	return cgi_pipe_out;
}

int Client::getHttpState() const
{
	return state_;
}

void Client::setState(enum STATE state)
{
	state_ = state;
}
