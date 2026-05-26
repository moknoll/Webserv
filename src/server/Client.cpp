#include "Client.hpp"
#include <cstddef>
#include <unistd.h>

Client::Client(int fd, const ServerConfig& config)
    : fd_(fd), request_complete_(false), keep_elive_(true), handler(config)
{
}

Client::~Client()
{
	close(fd_);
}

void Client::processRequest()
{
	request.parse(recv_buffer_);

	if (request.isComplete() || request.isAlmostDone())
	{
		response = handler.handle(request);
		if (response.isReady())
		{
			request_complete_ = true;
			if (handler.getState() == HttpHandler::HTTP_CLOSE)
				keep_elive_ = false;
			send_buffer_ = response.toString();
		}
	}
}

std::string Client::serialize()
{
	if (send_buffer_.empty())
		send_buffer_ = handler.getFileChunk();

	return send_buffer_;
}

void Client::appendRecvBuffer(const char* buffer, size_t size)
{
	recv_buffer_.append(buffer, size);
}

void Client::reset()
{
	handler.reset();
	request.reset();
	response.reset();
	recv_buffer_.clear();
	send_buffer_.clear();
	request_complete_ = false;
}

void Client::setRecvBuffer(const std::string& buffer)
{
	this->recv_buffer_ = buffer;
}

void Client::setSendBuffer(const std::string& buffer)
{
	this->send_buffer_ = buffer;
}

bool Client::isComplete() const
{
	return request_complete_;
}

bool Client::isKeepElive() const
{
	return keep_elive_;
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

