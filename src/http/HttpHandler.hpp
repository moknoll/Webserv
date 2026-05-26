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
	enum STATE
	{
		HTTP_INIT = 0,
		HTTP_RECV,
		SEND_STATE,
		HTTP_COMPLETE,
		HTTP_CLOSE
	};

	HttpHandler(const ServerConfig& cfg);
	~HttpHandler();

	HttpHandler(const HttpHandler& other);
	HttpHandler& operator=(const HttpHandler& other);

	HttpResponse handle(const HttpRequest& req);
	int          getState() const;
	std::string  getFileChunk();
	bool         hasMoreData() const;
	void         reset();

  private:
	static const size_t FILE_CHUNK_SIZE = 512 * 1024;

	const ServerConfig& config_;
	Location*           loc_;
	int                 error_;
	STATE               state_;
	std::string         upload_file_path_;
	int                 fd_;

	HttpResponse        handleGET(const HttpRequest& req, const Location& loc);
	HttpResponse        handlePOST(const HttpRequest& req, const Location& loc);
	HttpResponse        makeStatusResponse(int status, const Location* loc);
	HttpResponse makeFileResponse(const std::string& path, const Location* loc);

	std::vector< std::string > getListOfFiles(const std::string& path);
	// WIP
	HttpResponse               makeDirectoryPage(const std::string& path,
	                                             const std::string& uri,
	                                             const Location*    loc);

	// WIP
	HttpResponse
	redirect(int status, const std::string& location, const std::string& host);

	const Location*
	            findMatchUri(const std::string&             uri,
	                         const std::vector< Location >& locations) const;

	// WIP
	std::string buildPath(const std::string& uri, const Location& loc);
	bool isAllowedMethod(const std::string& method, const Location& loc) const;

	std::string buildUploadPath(const HttpRequest& req, const Location& loc);
	std::string sanitizeFileName(const std::string& filename);
	bool        validateUploadPath(const std::string& path);
	bool        openUploadFile(const std::string& path,
	                           HttpResponse&      erro_resp,
	                           const Location&    loc);

	bool        writeToFile(const std::string& data);
	void        resetUpload();
	void        closeFile();
};

