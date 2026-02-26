/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sockets.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/25 09:16:08 by mknoll            #+#    #+#             */
/*   Updated: 2026/02/26 14:50:44 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <sys/socket.h>	// socket()
#include <netinet/in.h>	// 
#include <arpa/inet.h> 
#include <iostream> 
#include <sys/types.h>
#include <cstring> // memset
#include <unistd.h> // close() 
#include <signal.h> 
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h> 
#include <vector> 
#include <map>
#include <poll.h>

#define MYPORT 16000 // Port users will be connecting to 
#define LOCALHOST "127.0.0.1"
#define SOCKET_ERROR -1
#define BACKLOG 10 // How may pendiing connections queue will hold 
#define MAXDATASIZE 100 // max number ob bytes we can get at once 

#endif