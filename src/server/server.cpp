/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:48 by mknoll            #+#    #+#             */
/*   Updated: 2026/04/29 08:44:54 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "../ConfigParser/ServerConfig.hpp"
#include "../logger/Logger.hpp"
#include "client.hpp"
#include <sstream>
// #include "../http/request/HttpHeader.hpp"

Server::Server(std::vector<ServerConfig> configs): _configs(configs){}

Server::~Server()
{
	// for(size_t i = 0; i < _serverfds.size(); i++)
	// {
	// 	if(_serverfds[i] != -1)
	// 		close(_serverfds[i]);
	// }
	//Check later on how to close? 
}

/**
 * @brief Initializes the server by setting up listening sockets.
 * 
 * Creates and configures a socket for each serve	// Logger::log(DEBUG, "client disconnected");r configuration in _configs.
 * For each socket, this method:
 * - Creates an IPv4 TCP socket
 * - Enables socket address reuse to avoid TIME_WAIT issues
 * - Sets the socket to non-blocking mode for event-driven I/O
 * - Binds the socket to the configured port
 * - Initiates listening for incoming client connections
 * 
 * @throws std::runtime_error if socket creation, configuration, binding, or
 *         listening fails
 * 
 * @see Server::_configs for the list of configurations to initialize
 */
void Server::init()
{
	int yes = 1;
	for (size_t i = 0; i < _configs.size(); i++)
	{
		int sock_fd = -1;		
		if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
			throw std::runtime_error("Socket Creation failed"); 

		if((setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == SOCKET_ERROR)
			throw std::runtime_error("Setsockopt failed");
	
		if((fcntl(sock_fd, F_SETFL, O_NONBLOCK)) == SOCKET_ERROR)
			throw std::runtime_error("Fcntl failed");

		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(_configs[i].port);
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		bzero(&(serverAddr.sin_zero), 8);
	
		if ((bind(sock_fd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) == SOCKET_ERROR)
			throw std::runtime_error("Bind failed");
	
		if((listen(sock_fd, BACKLOG)) == SOCKET_ERROR)
			throw std::runtime_error("listen failed");
	
		struct pollfd pfd;
		pfd.fd = sock_fd;
		pfd.events = POLLIN;
		_pollSockets.push_back(pfd);

		_serverConfigsByFd[sock_fd] = _configs[i];
		
		// Debug: Log the server config mapping
		std::ostringstream oss;
		oss << "Server Socket FD: " << sock_fd 
			<< " | Port: " << _configs[i].port 
			<< " | Host: " << _configs[i].host 
			<< " | Root: " << _configs[i].root 
			<< " | Index: " << _configs[i].index
			<< " | Max Body Size: " << _configs[i].client_max_body_size;
		LOG_DEBUG(oss.str());
		
	}
}


/**
 * @brief Runs the main event loop for the server.
 *
 * Continuously waits for activity on all registered sockets using poll().
 * When a listening socket becomes readable, it accepts a new client.
 * When a client socket becomes readable, it reads and handles the request.
 * When a client socket is writable, it sends the pending response.
 *
 * @throws std::runtime_error if poll() fails
 */
void Server::run()
{
	while(1) 
	{
		int ret = poll(_pollSockets.data(), _pollSockets.size(), -1);
		
		if(ret == -1)
			throw std::runtime_error("Poll failed");
		
		for (size_t i = 0; i < _pollSockets.size(); i++)
		{
			if(_pollSockets[i].revents & POLLIN)
			{
				if(_serverConfigsByFd.count(_pollSockets[i].fd))
					_acceptNewClient(_pollSockets[i].fd);
				else 
					_handleClientMessage(_pollSockets[i].fd);
			}
			else if(_pollSockets[i].revents & POLLOUT)
				_sendResponseToClient(_pollSockets[i].fd);
		}
	}	
}


/**
 * @brief Checks if the HTTP request in the buffer is complete.
 *
 * Determines whether a received HTTP request has been fully received and is
 * ready for processing. Currently a placeholder that always returns true.
 *
 * @param request Reference to the request string buffer to validate
 * @return true if the request is complete, false otherwise
 */
bool Server::_isCompleteRequest(std::string &request)
{
	(void)request;
	// size_t headerEnd = request.find("\r\n\r\n");
	// if(headerEnd == std::string::npos)
	// 	return false;
	
	// // For GET check content length
	// size_t contentLength = request.find("Content-Length:");
	// if(contentLength == std::string::npos)
	// 	return true;
	
	// // For POST
	
	// // For Delete 
	return true;
}


/**
* @brief Accepts a new incoming connection on a listening server socket.
*
* Performs accept() on the provided server socket file descriptor, looks up
* the associated ServerConfig, sets the new client socket to non-blocking
* mode, registers it for polling, constructs a Client object and stores
* it in the internal client map. Logs debug information about the new
* connection.
*
* @param serverSocketFd File descriptor of the listening server socket to
*                       accept a new client from.
* @return void
*/
void Server::_acceptNewClient(int serverSocketFd)
{
	struct sockaddr_in clientAddr;
	ServerConfig* config = &_serverConfigsByFd[serverSocketFd];
	socklen_t addrLen = sizeof(clientAddr);
	int newClientFd = accept(serverSocketFd,(struct sockaddr *)&clientAddr, &addrLen);
	
	if(newClientFd == SOCKET_ERROR)
		return;
	
	// Debug: Show which config was assigned to the new client
	std::ostringstream oss;
	oss << "New Client FD: " << newClientFd 
		<< " | Accepted from Server Socket FD: " << serverSocketFd 
		<< " | Assigned Config Port: " << config->port 
		<< " | Config Host: " << config->host 
		<< " | Config Root: " << config->root;
	LOG_DEBUG(oss.str());
	
	fcntl(newClientFd, F_SETFL, O_NONBLOCK);

	struct pollfd pfd;
	pfd.fd = newClientFd;
	pfd.events = POLLIN;
	_pollSockets.push_back(pfd);
	
	_clients.insert(std::make_pair(newClientFd, Client(newClientFd, config)));
	
	// std::cout << "New Client connected: " <<  << std::endl;
	//LOG_DEBUG( "New client connected");
}



/**
 * @brief Reads incoming data from a client socket and prepares a response.
 *
 * This function performs the request-handling step for an active client:
 * - calls recv() to read data from the socket
 * - closes and removes the client if the connection was closed or an error
 *   occurred
 * - appends the received bytes to the client's request buffer
 * - logs the received request data for debugging
 * - checks whether the request is complete before generating a response
 * - stores the response in the client object and switches the socket to
 *   writable mode so the response can be sent later
 *
 * Good practice:
 * - always validate socket reads for disconnects and errors
 * - keep request parsing separate from network I/O when possible
 * - avoid assuming one recv() call contains a full HTTP request
 * - update poll events only after the response is ready
 *
 * @param clientSocketFd File descriptor of the connected client socket
 * @return void
 */
void Server::_handleClientMessage(int clientSocketFd)
{
	char buffer[8192];
	int bytes = recv(clientSocketFd, buffer, sizeof(buffer), 0);
	
	if (bytes <= 0)
		_cleanupClient(clientSocketFd);
	else
	{
		_clients.at(clientSocketFd).requestBuffer.append(buffer, bytes);

		Logger::getInstance().log(INFO, "Received", _clients.at(clientSocketFd).requestBuffer);
		
		if (_isCompleteRequest(_clients.at(clientSocketFd).requestBuffer))
		{	 
			std::string response = "Hello from Server"; 
        
       		_clients.at(clientSocketFd).responseBuffer = response;

			// Change Poll event to writing
			for(size_t i = 0; i < _pollSockets.size(); i++)
			{
				if(_pollSockets[i].fd == clientSocketFd)
					{
						_pollSockets[i].events = POLLIN | POLLOUT;
						break;
					}
			}
		}
	}
}


/**
 * @brief Sends the prepared response to a connected client and resets client state.
 *
 * Verifies the client is tracked, obtains a reference to the client's
 * responseBuffer and returns immediately if it's empty. Attempts a single
 * send() of the buffer. If send() returns a non-negative value, the function:
 *  - clears the responseBuffer,
 *  - clears the requestBuffer to prepare for the next request,
 *  - switches the client's poll entry back to POLLIN (ready for reading).
 *
 * Notes:
 * - Currently treats any non-negative send() result as success and does not
 *   handle partial writes or EAGAIN/EWOULDBLOCK retries.
 * - No error logging is performed on send() failure; callers should ensure
 *   the client is cleaned up elsewhere if needed.
 *
 * @param clientSocketFd File descriptor of the connected client socket.
 */
void Server::_sendResponseToClient(int clientSocketFd)
{
	// check that clients exists
	if (_clients.find(clientSocketFd) == _clients.end())
		return;
	
	std::string &msg = _clients.at(clientSocketFd).responseBuffer;
		
	if(msg.empty())
		return;

	int bytesSent = send(clientSocketFd, msg.c_str(), msg.size(), 0);
	
	if(bytesSent >= 0)
	{
		msg.clear();
		_clients.at(clientSocketFd).requestBuffer.clear();
		std::cout << "Response send" << std::endl; 
		for (size_t i = 0; i < _pollSockets.size(); i++)
		{
			if(_pollSockets[i].fd == clientSocketFd)
			{
				_pollSockets[i].events = POLLIN;
				break;
			}
		}
	}
}



/**
 * @brief Clean up and remove a disconnected client.
 *
 * Performs all necessary teardown for a client that has disconnected or
 * encountered an error: closes the socket, removes the file descriptor
 * from the poll vector so it is no longer monitored, erases the client
 * entry from the internal client map, and logs the disconnection.
 *
 * @param clientSocketFd File descriptor of the client to clean up
 */
void Server::_cleanupClient(int clientSocketFd)
{
	close(clientSocketFd);
    
	for (size_t i = 0; i < _pollSockets.size(); i++)
	{
		if(_pollSockets[i].fd == clientSocketFd)
		{
			_pollSockets.erase(_pollSockets.begin() + i);
			break;
		}
	}
	_clients.erase(clientSocketFd);
    
	LOG_DEBUG("Client disconnected");
}
