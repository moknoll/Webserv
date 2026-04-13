/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:17:41 by mknoll            #+#    #+#             */
/*   Updated: 2026/02/26 14:01:27 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
	public:
		int 		fd; // file descriptor for the client socket
		std::string requestBuffer; // buffer to store incoming request data
		std::string responseBuffer; // buffer to store response data to be sent back to the client
		bool		isReadyToWrite; // flag to indicate if the full request has been received

		Client(int clientFD) : fd(clientFD), isReadyToWrite(false) {}
		~Client() {}

		void clearBuffers() {
			requestBuffer.clear();
			responseBuffer.clear();
			isReadyToWrite = false;
		}
};

#endif
