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
	const ServerConfig& config_;
	int                 error_;

	HttpResponse        makeError(int status, const Location* loc);

	// TODO
	HttpResponse        makeDirectoryPage(const std::string& path);

	int                 checkFile(const char* path) const;
	std::string         readFile(const char* path);
	HttpResponse        buildFileResponse(const std::string& path,
	                                      const Location*    loc);
	std::string         getMimeType(const std::string& path) const;

	const Location*
	            findMatchUri(const std::string&             uri,
	                         const std::vector< Location >& locations) const;

	std::string buildPath(const std::string& uri, const Location& loc);
};

