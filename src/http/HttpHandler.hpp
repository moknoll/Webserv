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
#include "constants.hpp"

#include <cstddef>
#include <string>

class HttpHandler
{
  public:
	enum HTTP_STATE
	{
		HTTP_READING_STATE = 0,
		HTTP_WRITING_STATE,
		HTTP_KEEPALIVE_STATE,
		HTTP_CLOSE
	};

	HttpHandler(const ServerConfig& cfg);
	// HttpHandler(const HttpHandler& other);
	~HttpHandler();

	// HttpHandler& operator=(const HttpHandler& other);

	HttpResponse handle(const HttpRequest& req);
	int          getState() const;
	void         reset();

  private:
	const ServerConfig& config_;
	Location*           loc_;
	int                 error_;
	int                 state_;
	int                 fd_;
	bool                uploading;
	size_t              writen_bytes;

	HttpResponse        handleGET(const HttpRequest& req, const Location& loc);
	HttpResponse        handlePOST(const HttpRequest& req, const Location& loc);
	HttpResponse        makeError(int status, const Location* loc);

	// WIP
	HttpResponse        makeDirectoryPage(const std::string& path,
	                                      const std::string& uri,
	                                      const Location*    loc);

	std::vector< std::string > getListOfFiles(const std::string& path);
	HttpResponse makeFileResponse(const std::string& path, const Location* loc);

	// WIP
	HttpResponse
	redirect(int status, const std::string& location, const std::string& host);

	const Location*
	            findMatchUri(const std::string&             uri,
	                         const std::vector< Location >& locations) const;

	// WIP
	std::string buildPath(const std::string& uri, const Location& loc);

	bool isAllowedMethod(const std::string& method, const Location& loc) const;

	bool writeToFile(const std::string& data);
};

