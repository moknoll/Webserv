/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:17:41 by mknoll            #+#    #+#             */
/*   Updated: 2026/05/05 13:06:24 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include "../ConfigParser/ServerConfig.hpp"

class Client
{
	private:
		int         clientFd;
		std::string requestBuffer;
		std::string responseBuffer;
		bool        requestComplete;
		//httpshandler *h;

	public: 
		Client(int clientFd, Httphandler h);
		~Client() {}
	
		void clearBuffers();
		void requestComplete();
};
