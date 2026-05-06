#include "client.hpp"

Client::Client(){}

Client::Client(int clientFD) : _clientFd(clientFD) {}

void Client::clearBuffers()
{
	_requestBuffer.clear();
	_responseBuffer.clear();
	//this->requestComplete = false;
}

void Client::setRequestBuffer(const std::string &requestBuffer)
{
	this->_requestBuffer = requestBuffer; 
}

void Client::setResponseBuffer(const std::string &response)
{
	this->_responseBuffer = response;
}

bool Client::getComplete()const
{
	return this->_requestComplete;
}


std::string Client::getResponseBuffer()const
{
	return this->_responseBuffer;
}


std::string Client::getRequestBuffer()const
{
	return this->_requestBuffer;
}