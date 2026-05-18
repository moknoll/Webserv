/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: moritzknoll <moritzknoll@student.42.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:17:41 by mknoll            #+#    #+#             */
/*   Updated: 2026/05/16 12:25:45 by moritzknoll      ###   ########.fr       */
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
		int get_Client_FD()const {return _clientFd;}

		void setResponseBuffer(const std::string &response);
		void setRequestBuffer(const std::string &request);
};
