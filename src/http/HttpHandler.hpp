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

#include <cstddef>
#include <string>

class HttpHandler
{
  public:
	HttpHandler(const ServerConfig& cfg);
	HttpHandler(const HttpHandler& other);
	~HttpHandler();

	HttpResponse handle(const HttpRequest& req);
	HttpResponse makeStatusResponse(int status);
	std::string  getFileChunk();
	void         reset();

  private:
	HttpHandler&               operator=(const HttpHandler& other);

	// static const size_t        FILE_CHUNK_SIZE;

	const ServerConfig&        config_;
	Location                   loc_;
	int                        error_;
	std::string                upload_file_path_;
	// std::string                temp_file_;
	// int                        fd_;

	HttpResponse               handleGET(const HttpRequest& req);
	HttpResponse               handlePOST(const HttpRequest& req);
	HttpResponse               handleDELETE(const HttpRequest& req);
	// HttpResponse               makeFileResponse(const std::string& path);

	std::vector< std::string > getListOfFiles(const std::string& path);
	HttpResponse makeDirectoryListingResponse(const std::string& path,
	                                          const std::string& uri);
	// WIP
	HttpResponse redirect(int status, const std::string& target);

	// const Location*
	//             findMatchUri(const std::string&             uri,
	//                          const std::vector< Location >& locations) const;

	// WIP
	// std::string buildPath(const std::string& uri) const;
	bool         isAllowedMethod(const std::string& method) const;

	std::string  buildUploadPath(const std::string& filename);
	std::string  sanitizeFileName(const std::string& filename);
	bool         validateUploadPath(const std::string& path);
	int          openUploadFile(const std::string& path, HttpResponse& err_res);
	// bool        exceedsBodySizeLimit(const HttpRequest& req) const;
	// bool saveBodyToTempFile(const HttpRequest& req, HttpResponse& err_res);
	// bool writeToFile(const std::string& data);
	// void resetUpload();
	// void         closeFile();
	bool         savePlainBody(const HttpRequest& req, HttpResponse& err_res);
	bool         saveUploadedFileFromTemp(const HttpRequest& req,
	                                      HttpResponse&      err_res);
};

