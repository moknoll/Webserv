#include <client.hpp>

Client::Client(){}

Client::Client(){}

void Client::clearBuffers()
{
	requestBuffer.clear();
	responseBuffer.clear();
	isReadyToWrite = false;
}