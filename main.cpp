/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 12:14:52 by mknoll            #+#    #+#             */
/*   Updated: 2026/02/26 14:20:37 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <iostream>

int main()
{
	try {
		Server webserver(MYPORT);
		webserver.init();
		webserver.run();
	} catch (const std::exception& e) {
		std::cout << "Error: " << e.what() << std::endl;
	
	}	
	return 0;
}