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
	HttpHandler(const HttpHandler& other);
	~HttpHandler();

	HttpResponse handle(const HttpRequest& req);
	HttpResponse makeStatusResponse(int status);
	// std::string  getFileChunk();
	void         reset();

  private:
	HttpHandler&        operator=(const HttpHandler& other);

	const ServerConfig& config_;
	Location            loc_;
	// int                 error_;
	// std::string         upload_file_path_;

	HttpResponse        handleGET(const HttpRequest& req);
	HttpResponse        handlePOST(const HttpRequest& req);
	HttpResponse        handleDELETE(const HttpRequest& req);

	// std::vector< std::string > getListOfFiles(const std::string& path);
	// HttpResponse makeDirectoryListingResponse(const std::string& path,
	//                                           const std::string& uri);

	bool                isAllowedMethod(const std::string&) const;
	std::string         buildUploadPath(const std::string&);
	std::string         sanitizeFileName(const std::string&);
	bool                validateUploadPath(const std::string&);
	int                 openUploadFile(const std::string&, HttpResponse&);
	bool                savePlainBody(const HttpRequest&, HttpResponse&);
	bool saveUploadedFileFromTemp(const HttpRequest&, HttpResponse&);
};

