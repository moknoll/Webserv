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

#include <cctype>
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
// WIP
HttpResponse HttpHandler::redirect(const std::string& location) const
{
	HttpResponse res(HTTP_MOVED_PERMANENTLY);

	res.setHeader("Location", location);

	res.setFullResponse(res.getErrorPage(HTTP_MOVED_PERMANENTLY), "html");

	return res;
}

HttpResponse HttpHandler::handleGET(const HttpRequest& req)
{
	const std::string& uri = req.getURI();

	const Location*    loc = findMatchUri(uri, config_.locations);

	if (req.getRequestStatus() != HTTP_OK)
		return makeError(req.getRequestStatus(), loc);

	if (loc == NULL)
		return makeError(HTTP_NOT_FOUND, loc);

	// TODO
	// getMethod if method not allowed
	// return makeError(HTTP_NOT_ALLOWED);

	// build path WIP
	std::cout << "URI: " << uri << std::endl;
	std::string path = buildPath(uri, *loc);

	std::cout << "path: " << path << std::endl;

	if (ws::isDirectory(path))
	{
		if (path[path.length() - 1] != '/')
			std::cout << req.getHeader("Host") + "/" + uri + "/";
		// return redirect(req.getHeader("Host") + "/" + uri + "/");
		std::string index_file_path = path;

		if (!loc->index.empty())
			index_file_path += loc->index;
		else
			index_file_path += "index.html";

		std::cout << "index_file_path: " << index_file_path << '\n';
		// TODO makeDirectoryPage
		if (checkFile(index_file_path.c_str()) == FILE_OK)
			return buildFileResponse(index_file_path, loc);

		std::cout << "autoindex: " << loc->autoindex << '\n';
		std::cout << "checkFile: " << checkFile(index_file_path.c_str())
		          << '\n';
		if (loc->autoindex)
		{
			if (uri[uri.length() - 1] != '/')
				return redirect("http://" + req.getHeader("Host") + uri + "/");
			return makeDirectoryPage(path, uri);
		}

		return makeError(HTTP_FORBIDDEN, loc);
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
	switch (checkFile(path.c_str()))
	{
		case FILE_OK:        break;
		case ERR_NOT_FOUND:  return makeError(HTTP_NOT_FOUND, loc);
		case ERR_PERMISSION: return makeError(HTTP_FORBIDDEN, loc);
		case ERR_IS_DIR:     return makeError(HTTP_FORBIDDEN, loc);
		default:             return makeError(HTTP_INTERNAL_SERVER_ERROR, loc);
	}

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
	std::string sub_uri = uri.substr(loc.path.length());
	std::string path = loc.root;

	if (!path.empty() && path[path.length() - 1] != '/')
		path += "/";

	if (!sub_uri.empty() && sub_uri[0] == '/')
		path += sub_uri.substr(1);
	else
		path += sub_uri;

	return path;
}

/*
<html>
<head><title>Index of /</title></head>
<body>
<h1>Index of /</h1><hr><pre><a href="../">../</a>
<a href="dir1/">dir1/</a> 03-May-2026 10:24                   - <a
href="dir2/">dir2/</a>                                              03-May-2026
10:24                   - <a href="file1">file1</a> 03-May-2026 10:25 0 <a
href="file2">file2</a>                                              03-May-2026
10:25                   0 <a href="file3">file3</a> 03-May-2026 10:25 0
</pre><hr></body>
</html>
*/

// WIP
std::string
HttpHandler::buildDirectoryPage(const std::vector< std::string >& list_of_files,
                                const std::string&                path)
{
	std::string content;

	content += "<html>\n";
	content += "<head><title>Index of " + path + "</title></head>\n";
	content += "<body>\n<h1>Index of " + path
	         + "</h1><hr><pre>"; //<a href=\"../\">../</a>\n";

	for (size_t i = 0; i < list_of_files.size(); ++i)
		content +=
		    "<a href=" + list_of_files[i] + ">" + list_of_files[i] + "</a>\n";

	content += "</pre><hr></body></html>";

	return content;
}

HttpResponse HttpHandler::makeDirectoryPage(const std::string& path,
                                            const std::string& uri)
{
	HttpResponse               res(HTTP_OK);

	std::vector< std::string > list_of_files = getListOfFiles(path);

	if (this->error_ != HTTP_OK)
		return makeError(error_, NULL);
	std::string content = buildDirectoryPage(list_of_files, uri);

	res.setFullResponse(content, getMimeType("html"));

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

	for (size_t i = 0; i < list_of_files.size(); ++i)
	{
		if (ws::isDirectory(path + list_of_files[i]))
			list_of_files[i] += "/";
	}

	return list_of_files;
}

int HttpHandler::checkFile(const char* path) const
{
	int fd = open(path, O_RDONLY);
	if (fd != -1)
	{
		close(fd);
		return FILE_OK;
	}

	switch (errno)
	{
		case ENOENT: return ERR_NOT_FOUND;
		case EACCES: return ERR_PERMISSION;
		case EISDIR: return ERR_IS_DIR;
		default:     return ERR_UNKNOWN;
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

