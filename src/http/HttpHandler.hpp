/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:19:26 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:19:27 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>

class HttpHandler
{
  public:
	HttpHandler(const ServerConfig& cfg);
	// HttpHandler(const HttpHandler& other);
	~HttpHandler();

	// HttpHandler& operator=(const HttpHandler& other);

	HttpResponse handle(const HttpRequest& req);
	HttpResponse handleGET(const HttpRequest& req);

  private:
	const ServerConfig&        config_;
	int                        error_;

	HttpResponse               makeError(int status, const Location* loc);

	// WIP
	HttpResponse               makeDirectoryPage(const std::string& path,
	                                             const std::string& uri);

	int                        checkFile(const char* path) const;
	std::string                readFile(const char* path);
	std::string                getMimeType(const std::string& path) const;
	std::vector< std::string > getListOfFiles(const std::string& path);
	HttpResponse               buildFileResponse(const std::string& path,
	                                             const Location*    loc);
	// WIP
	HttpResponse               redirect(const std::string& location) const;

	// WIP
	std::string
	buildDirectoryPage(const std::vector< std::string >& list_of_files,
	                   const std::string&                path);

	    const Location* findMatchUri(
	        const std::string&             uri,
	        const std::vector< Location >& locations) const;

	// WIP
	std::string buildPath(const std::string& uri, const Location& loc);
};

