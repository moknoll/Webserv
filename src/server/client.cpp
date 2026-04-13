/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/25 12:56:51 by mknoll            #+#    #+#             */
/*   Updated: 2026/02/26 14:51:51 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sockets.hpp"

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in their_addr; // connectors address

	if (argc != 2)
	{
		std::cout << "usage: getip address" << std::endl;
		exit(1);
	}

	if((he=gethostbyname(argv[1])) == NULL) // get the host info
	{
		std::cout << "gethostname failed" << std::endl;
		exit(1);
	}

	std::cout << "Host name: " << he->h_name << std::endl;
	std::cout << "IP Adress: " << inet_ntoa(*((struct in_addr *)he->h_addr)) << std::endl;

	// create client socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
	{
		std::cout << "Socket error client" << std::endl;
		exit(1);
	}
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(MYPORT);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);

	if((connect(sockfd, (struct sockaddr *)&their_addr, sizeof(their_addr))) == SOCKET_ERROR)
	{	
		std::cout << "connect failed" << std::endl; 
		exit(1);
	}
	else 
		std::cout << "conncetion succesful" << std::endl;

	// Trying to send a message
	std::cout << "Enter a message: " << std::endl;
	std::string msg;
	std::getline(std::cin, msg);	
	
	int numSend = send(sockfd, msg.c_str(), msg.size(), 0);
	if(numSend < 0)
		std::cout << "Error send in client" << std::endl; 

		
	// recv() Daten
	if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';
	printf("Received: %s",buf);
 	close(sockfd);

	return 0;
}