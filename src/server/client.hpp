/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:17:41 by mknoll            #+#    #+#             */
/*   Updated: 2026/05/05 16:05:38 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include "../ConfigParser/ServerConfig.hpp"

class Client
{
	private:
		int         _clientFd;
		std::string _requestBuffer;
		std::string _responseBuffer;
		bool        _requestComplete;
		//httpshandler *h;

	public: 
		Client(int clientFd);
		Client();
		~Client() {}
	
		void clearBuffers();
		void requestComplete();

		std::string getResponseBuffer()const;
		std::string getRequestBuffer()const;
		bool getComplete()const;

		void setResponseBuffer(const std::string &response);
		void setRequestBuffer(const std::string &request);
};
