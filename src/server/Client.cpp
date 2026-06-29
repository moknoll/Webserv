#include "Client.hpp"
#include "../cgi/Cgi.hpp"
#include "../http/constants.hpp"
#include "../lib/ws.hpp"

#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

Client::Client(int fd, const ServerConfig& config) :
        cgi(),
        config_(config),
        fd_(fd),
        keep_alive_(true),
        state_(HTTP_INIT),
        request(),
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
		    makeStatusResponse(request.getRequestStatus()).toString();
		return;
	}
	const Location* loc = FindMatchingUri(request.getURI(), config_);
	if (loc == NULL)
		return; // HTTP_NOT_FOUND

	if (loc->has_cgi)
	{
		int status = cgi.executeCGI(request, *loc);
		if (status != HTTP_OK)
		{
			send_buffer_ = makeStatusResponse(status).toString();
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
			// if (handler.getState() == HttpHandler::HTTP_CLOSE)
			// 	keep_alive_ = false;
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
	HttpResponse res;
	int          status = cgi.buildResponse();
	if (status != HTTP_OK)
		send_buffer_ = makeStatusResponse(status).toString();
	else
		send_buffer_ = cgi.getResponse().toString();
}

void Client::parseRequest(const char* buffer, size_t size)
{
	recv_buffer_.append(buffer, size);
	request.parse(recv_buffer_, config_);
}

void Client::reset()
{
	handler.reset();
	request.reset();
	response.reset();
	recv_buffer_.clear();
	send_buffer_.clear();
	state_ = HTTP_RECV;
	// cgi.reset()
}

bool Client::CGIProcessFinished()
{
	int   status;
	pid_t pid = cgi.getCgiPid();

	pid_t ret = waitpid(pid, &status, WNOHANG);
	if (ret == 0)
		return false;
	else if (ret == pid)
	{
		if (WIFEXITED(status))
		{
			int code = WEXITSTATUS(status);
			if (code == 0)
			{
				int cgi_status = cgi.buildResponse();
				if (cgi_status == HTTP_OK)
					send_buffer_ = cgi.getResponse().toString();
				else
					send_buffer_ = makeStatusResponse(cgi_status).toString();
			}
			else
			{
				send_buffer_ =
				    makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR).toString();
			}
		}
		else if (WIFSIGNALED(status))
			send_buffer_ = makeStatusResponse(HTTP_GATEWAY_TIME_OUT).toString();
	}
	else
	{
		send_buffer_ =
		    makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR).toString();
	}
	state_ = HTTP_SEND;
	return true;
}

static bool matchPrefix(const std::string& uri, const std::string& loc)
{
	if (uri.compare(0, loc.size(), loc) != 0)
		return false;
	if (uri.size() == loc.size())
		return true;
	if (loc[loc.size() - 1] == '/')
		return true;

	return uri[loc.size()] == '/';
}

const Location* Client::FindMatchingUri(const std::string&  uri,
                                        const ServerConfig& cfg)
{
	const std::vector< Location >& locations = cfg.locations;
	const Location*                best_loc = NULL;
	size_t                         len_best_loc = 0;

	for (size_t i = 0; i < locations.size(); ++i)
	{
		const std::string& path = locations[i].path;

		if (matchPrefix(uri, path) && path.size() > len_best_loc)
		{
			best_loc = &locations[i];
			len_best_loc = path.size();
		}
	}

	return best_loc;
}

HttpResponse Client::makeStatusResponse(int status)
{
	HttpResponse                                 res(status);
	std::string                                  content;

	std::map< int, std::string >::const_iterator it =
	    config_.error_pages.find(status);
	if (it != config_.error_pages.end())
	{
		if (ws::readFile(it->second.c_str(), content))
		{
			res.setFullResponse(content, "html");
			return res;
		}
	}

	res.setFullResponse(res.buildErrorPage(status), "html");
	return res;
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
	return cgi.getFdCGI_in();
}

int Client::getFdCGI_out() const
{
	return cgi.getFdCGI_out();
}

int Client::getHttpState() const
{
	return state_;
}
