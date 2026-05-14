#include "client.hpp"

Client::Client(int clientFd, ServerConfig* cfg)
    : clientFd(clientFd), isReadyToWrite(false), config(cfg),
      handler(config[0]), request(), response()
{
	std::cout << "CALL Client contructor\n";
	std::cout << request.getRequestStatus();
}

Client::Client(const Client& other)
    : clientFd(other.clientFd), requestBuffer(other.requestBuffer),
      responseBuffer(other.responseBuffer),
      isReadyToWrite(other.isReadyToWrite), config(other.config),
      handler(other.handler), request(other.request), response(other.response)
{
}

