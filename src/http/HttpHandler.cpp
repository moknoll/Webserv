/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:19:14 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:19:16 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpHandler.hpp"
#include "../lib/ws.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "constants.hpp"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

HttpHandler::HttpHandler(const ServerConfig& cfg)
    : config_(cfg), error_(HTTP_OK)
{
}

// HttpHandler::HttpHandler(const HttpHandler& other): config(other.config),
// error(other.error)  {}
HttpHandler::~HttpHandler() {}

// HttpHandler&	HttpHandlerHt::operator=(const HttpHandler& other) {}

HttpResponse HttpHandler::handleGET(const HttpRequest& req)
{
	const std::string& uri = req.getURI();

	const Location*    loc = findMatchUri(uri, config_.locations);

	if (req.getRequestStatus() != HTTP_OK)
		return makeError(req.getRequestStatus(), loc);

	if (loc == NULL)
		return makeError(HTTP_NOT_FOUND, loc);

	// getMethod if method not allowed
	// return makeError(HTTP_NOT_ALLOWED);

	// build path
	std::string path = buildPath(uri, *loc);

	std::cout << "HIIIIIIII";
	if (ws::isDirectory(path))
	{
		std::string index_file_path = path + loc->index;

		// TODO makeDirectoryPage
		if (checkFile(index_file_path.c_str()) != HTTP_OK && loc->autoindex)
			return makeDirectoryPage(path);
		// ;

		return buildFileResponse(index_file_path, loc);
	}

	return buildFileResponse(path, loc);
}

HttpResponse HttpHandler::makeError(int status, const Location* loc)
{
	HttpResponse res(status);
	std::string  content;

	if (!loc || loc->error_pages.find(status) == loc->error_pages.end())
	{
		res.setFullResponse(res.getErrorPage(status), getMimeType("html"));
		return res;
	}

	std::map< int, std::string >::const_iterator error_page_path_it;
	error_page_path_it = loc->error_pages.find(status);

	content = readFile(error_page_path_it->second.c_str());
	if (this->error_ == HTTP_OK)
	{
		res.setFullResponse(content, getMimeType("html"));
	}
	else
	{
		res.setFullResponse(res.getErrorPage(error_), getMimeType("html"));
	}

	return res;
}

std::string HttpHandler::readFile(const char* path)
{
	this->error_ = checkFile(path);

	if (error_ != HTTP_OK)
		return "";

	std::ifstream file(path);

	if (!file.is_open())
	{
		this->error_ = HTTP_INTERNAL_SERVER_ERROR;
		return "";
	}

	std::string content((std::istreambuf_iterator< char >(file)),
	                    std::istreambuf_iterator< char >());
	error_ = HTTP_OK;
	return content;
}

HttpResponse HttpHandler::buildFileResponse(const std::string& path,
                                            const Location*    loc)
{
	std::string content = readFile(path.c_str());

	if (error_ != HTTP_OK)
		return makeError(error_, loc);

	HttpResponse res(HTTP_OK);

	std::string  extention = ws::getFileExtension(path);
	res.setFullResponse(content, getMimeType(extention));

	return res;
}

const Location*
HttpHandler::findMatchUri(const std::string&             uri,
                          const std::vector< Location >& locations) const
{
	const Location* best_loc = NULL;
	size_t          len_best_loc = 0;

	for (size_t i = 0; i < locations.size(); ++i)
	{
		const std::string& path = locations[i].path;

		if (uri.find(path) == 0)
		{
			if (path.size() > len_best_loc)
			{
				best_loc = &locations[i];
				len_best_loc = path.size();
			}
		}
	}

	return best_loc;
}

std::string HttpHandler::buildPath(const std::string& uri, const Location& loc)
{
	return loc.root + uri;
}

std::string buildDirectoryPage(const std::vector< std::string >& list_of_files,
                               const std::string&                path)
{
	std::string content;

	content += "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>";
	content += "<title>Index of " + path + "</title></head><body>";
	content += "<h1>Index of " + path + "</h1> 	<hr>	<pre>";

	for (size_t i = 0; i < list_of_files.size(); ++i)
		content +=
		    "<a href=" + list_of_files[i] + ">" + list_of_files[i] + "</a>";

	content += "</pre><hr></body></html>";

	return content;

	/*


<a href="../">../</a>
<a href="document.txt">document.txt</a>
<a href="photo.jpg">photo.jpg</a>
<a href="video.mp4">video.mp4</a>
<a href="archive.zip">archive.zip</a>
<a href="script.js">script.js</a>
<a href="folder/">folder/</a>
<a href="backup.tar.gz">backup.tar.gz</a>
	</pre>
	<hr>
</body>
</html>
	    */
}

HttpResponse HttpHandler::makeDirectoryPage(const std::string& path)
{
	HttpResponse               res(HTTP_OK);

	std::vector< std::string > list_of_files = getListOfFiles(path);

	if (this->error_ != HTTP_OK)
		return makeError(error_, NULL);
	std::string content = buildDirectoryPage(list_of_files, path);

	res.setFullResponse(content, "html");

	return res;
}

std::vector< std::string > HttpHandler::getListOfFiles(const std::string& path)
{
	std::vector< std::string > list_of_files;

	DIR*                       dir = opendir(path.c_str());
	if (dir == NULL)
	{
		this->error_ = HTTP_INTERNAL_SERVER_ERROR;
		return list_of_files;
	}

	struct dirent* entry;

	while ((entry = readdir(dir)) != NULL)
	{
		if (std::strcmp(entry->d_name, ".") == 0)
			continue;
		list_of_files.push_back(entry->d_name);
	}

	return list_of_files;
}

int HttpHandler::checkFile(const char* path) const
{
	int fd = open(path, O_RDONLY);
	if (fd != -1)
	{
		close(fd);
		return HTTP_OK;
	}

	switch (errno)
	{
		case ENOENT: return HTTP_NOT_FOUND;
		case EACCES: return HTTP_FORBIDDEN;
		default:     return HTTP_INTERNAL_SERVER_ERROR;
	}
}

// clang-format off
std::string HttpHandler::getMimeType(const std::string& ext) const
{
	if (ext == "htm" || ext == "html")	return "text/html; charset=utf-8";
	if (ext == "css")					return "text/css";
	if (ext == "js")					return "application/javascript";
	if (ext == "xml")					return "application/xml";
	if (ext == "txt")					return "text/plain";
	if (ext == "jpeg" || ext == "jpg")	return "image/jpeg";
	if (ext == "png")					return "image/png";
	if (ext == "gif")					return "image/gif";
	if (ext == "ico")					return "image/x-icon";
	if (ext == "svg")					return "image/svg+xml";
	if (ext == "webp")					return "image/webp";
	if (ext == "webm")					return "video/webm";
	if (ext == "mp4")					return "video/mp4";
	if (ext == "mp3")					return "audio/mpeg";
	if (ext == "gz")					return "application/gzip";
	if (ext == "zip")					return "application/zip";

	return "application/octet-stream";
}
// clang-format on

