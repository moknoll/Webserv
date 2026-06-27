#include "Client.hpp"
#include <cstddef>
#include <unistd.h>

Client::Client(int fd, const ServerConfig& config) :
        config_(config),
        fd_(fd),
        keep_alive_(true),
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
	request.parse(recv_buffer_, config_);

	// if (request.isComplete() || request.isAlmostDone())
	if (request.isComplete())
	{
		response = handler.handle(request);
		if (response.isReady()) // ???
		{
			if (handler.getState() == HttpHandler::HTTP_CLOSE)
				keep_alive_ = false;
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

