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
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
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

HttpResponse HttpHandler::handle(const HttpRequest& req)
{
	const std::string& uri = req.getURI();

	const Location*    loc = findMatchUri(uri, config_.locations);
	(void) loc;

	// if loc->path == CGI
	//  handleCGI();
	//

	return HttpResponse(200);
}

// WIP
HttpResponse HttpHandler::redirect(const std::string& location) const
{
	HttpResponse res(HTTP_MOVED_PERMANENTLY);

	res.setHeader("Location", location);

	res.setFullResponse(res.buildErrorPage(HTTP_MOVED_PERMANENTLY), "html");

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
		if (ws::checkFile(index_file_path.c_str()) == FILE_OK)
			return buildFileResponse(index_file_path, loc);

		std::cout << "autoindex: " << loc->autoindex << '\n';
		std::cout << "checkFile: " << ws::checkFile(index_file_path.c_str())
		          << '\n';
		if (loc->autoindex)
		{
			if (uri[uri.length() - 1] != '/')
				return redirect("http://" + req.getHeader("Host") + uri + "/");
			return makeDirectoryPage(path, uri, loc);
		}

		return makeError(HTTP_FORBIDDEN, loc);
	}

	return buildFileResponse(path, loc);
}

HttpResponse HttpHandler::makeError(int status, const Location* loc)
{
	HttpResponse res(status);
	std::string  content;

	if (!loc)
	{
		std::map< int, std::string >::const_iterator it =
		    loc->error_pages.find(status);
		if (it != loc->error_pages.end())
		{
			if (ws::readFile(it->second.c_str(), content))
			{
				res.setFullResponse(content, "html");
				return res;
			}
		}
		return res;
	}

	res.setFullResponse(res.buildErrorPage(status), "html");
	return res;
}

HttpResponse HttpHandler::buildFileResponse(const std::string& path,
                                            const Location*    loc)
{
	switch (ws::checkFile(path.c_str()))
	{
		case FILE_OK:        break;
		case ERR_NOT_FOUND:  return makeError(HTTP_NOT_FOUND, loc);
		case ERR_PERMISSION: return makeError(HTTP_FORBIDDEN, loc);
		case ERR_IS_DIR:     return makeError(HTTP_FORBIDDEN, loc);
		default:             return makeError(HTTP_INTERNAL_SERVER_ERROR, loc);
	}

	std::string content;
	if (!ws::readFile(path.c_str(), content))
		return makeError(HTTP_INTERNAL_SERVER_ERROR, loc);

	HttpResponse res(HTTP_OK);

	res.setFullResponse(content, ws::getFileExtension(path));
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
		path += '/';

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

std::string
modif(const std::string& name, const std::string& time, const std::string& size)
{
	std::stringstream ss;
	std::string       n = name;

	if (name.length() > 51)
	{
		n = name.substr(0, 47);
		n += "..>";
	}
	ss << std::left << std::setw(55) << n + "</a>";
	ss << std::right << time << std::setw(11);
	ss << std::right << size << std::setw(20);

	return ss.str();
}

// WIP
// std::string
// HttpHandler::buildDirectoryPage(const std::vector< std::string >& list_of_files,
//                                 const std::string&                path,
//                                 const std::string&                uri)
// {
// 	std::string content;
//
// 	content += "<html>\n";
// 	content += "<head><title>Index of " + uri + "</title></head>\n";
// 	content += "<body>\n<h1>Index of " + uri
// 	         + "</h1><hr><pre><a href=\"../\">../</a>\n";
//
// 	for (size_t i = 0; i < list_of_files.size(); ++i)
// 	{
// 		std::string time = ws::getFileModificationTime(path + list_of_files[i]);
// 		std::string size;
// 		if (ws::isDirectory(path + list_of_files[i]))
// 			size = "-";
// 		else
// 			size = ws::to_string(ws::getFileSize(path + list_of_files[i]));
// 		std::cout << modif(list_of_files[i], time, size) << std::endl;
// 		content += "<a href=" + list_of_files[i] + ">"
// 		         + modif(list_of_files[i], time, size) + "\n";
// 	}
// 	// content += +list_of_files[i] + ">" + list_of_files[i] + "</a>\n";
//
// 	content += "</pre><hr></body></html>";
//
// 	return content;
// }

HttpResponse HttpHandler::makeDirectoryPage(const std::string& path,
                                            const std::string& uri,
                                            const Location*    loc)
{
	HttpResponse               res(HTTP_OK);

	std::vector< std::string > list_of_files;

	DIR*                       dir = opendir(path.c_str());
	if (dir == NULL)
		return makeError(HTTP_INTERNAL_SERVER_ERROR, loc);

	struct dirent* entry;

	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		list_of_files.push_back(name);
	}

	for (size_t i = 0; i < list_of_files.size(); ++i)
	{
		if (ws::isDirectory(path + list_of_files[i])
		    && list_of_files[i][list_of_files[i].size() - 1] != '/')
			list_of_files[i] += "/";
	}

	std::string content = res.buildDirectoryPage(list_of_files, path, uri);

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
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		list_of_files.push_back(name);
	}

	for (size_t i = 0; i < list_of_files.size(); ++i)
	{
		if (ws::isDirectory(path + list_of_files[i])
		    && list_of_files[i][list_of_files[i].size() - 1] != '/')
			list_of_files[i] += "/";
	}

	return list_of_files;
}

