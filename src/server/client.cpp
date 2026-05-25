#include "client.hpp"
#include <unistd.h>

Client::Client(int fd, const ServerConfig& config) : fd_(fd), handler(config) {}

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
		send_buffer_ = response.buildResponse();
	}
}

std::string Client::serialize()
{
	if (send_buffer_.empty())
		send_buffer_ = handler.getFileChunk();

	return send_buffer_;
}

void Client::appendBuffer(const std::string& buffer)
{
	recv_buffer_ += buffer;
}

void Client::clearBuffers()
{
	recv_buffer_.clear();
	send_buffer_.clear();
}

void Client::reset()
{
	handler.reset();
	request.reset();
	response.reset();
	recv_buffer_.clear();
	send_buffer_.clear();
}

void Client::setRequestBuffer(const std::string& buffer)
{
	this->recv_buffer_ = buffer;
}

void Client::setResponseBuffer(const std::string& buffer)
{
	this->send_buffer_ = buffer;
}

bool Client::isComplete() const
{
	return request.isComplete() || request.getbodyStream().eof();
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

