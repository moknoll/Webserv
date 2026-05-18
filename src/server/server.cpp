#include "server.hpp"
#include "socket.hpp"

Server::Server(){}

Server::Server(const ServerConfig &config) : _socket(config){}

Server::~Server(){}