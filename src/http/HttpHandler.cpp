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
#include "BodyStream.hpp"
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
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

HttpHandler::HttpHandler(const ServerConfig& cfg)
    : config_(cfg), error_(0), fd_(-1), uploading(false), writen_bytes(0)
{
}

// HttpHandler::HttpHandler(const HttpHandler& other): config(other.config),
// error(other.error)  {}
HttpHandler::~HttpHandler() {}

// HttpHandler&	HttpHandlerHt::operator=(const HttpHandler& other) {}

HttpResponse HttpHandler::handle(const HttpRequest& req)
{
	std::cout << "IN HANDL";
	const std::string& uri = req.getURI();
	const std::string& host = req.getHeader("Host");
	const std::string& method = req.getMethod();
	const Location*    loc = findMatchUri(uri, config_.locations);
	// const std::string  method = req.getMethod();

	if (req.getRequestStatus() != HTTP_OK)
		return makeError(req.getRequestStatus(), loc);

	if (loc == NULL)
		return makeError(HTTP_NOT_FOUND, loc);

	// if redirect -> redirect(status, location)
	if (loc->redirect.first != -1)
		return redirect(loc->redirect.first, loc->redirect.second, host);

	if (!isAllowedMethod(method, *loc))
		return makeError(HTTP_NOT_ALLOWED, loc);
	// if (!loc->cgi_extension.empty())
	// 		handleCGI();

	if (method == "GET")
		return handleGET(req, *loc);
	else if (method == "POST")
		return handlePOST(req, *loc);
	// else if (method == "DELETE")
	// return handleDELETE(req, loc);

	return makeError(HTTP_INTERNAL_SERVER_ERROR, loc);
}

HttpResponse HttpHandler::handleGET(const HttpRequest& req, const Location& loc)
{
	const std::string& uri = req.getURI();
	const std::string& host = req.getHeader("Host");
	std::string        path = buildPath(uri, loc);

	if (ws::isDirectory(path))
	{
		if (uri != "/" && uri[uri.size() - 1] != '/')
			return redirect(HTTP_MOVED_PERMANENTLY, uri + "/", host);

		std::string index_file =
		    loc.index.empty() ? path + "index.html" : path + loc.index;

		if (ws::checkFile(index_file.c_str()) == FILE_OK)
			return makeFileResponse(index_file, &loc);

		if (loc.autoindex)
			return makeDirectoryPage(path, uri, &loc);

		return makeError(HTTP_FORBIDDEN, &loc);
	}

	return makeFileResponse(path, &loc);
}

bool HttpHandler::writeToFile(const std::string& data)
{
	int n = write(fd_, data.c_str(), data.size());
	if (n == -1)
		return false;
	writen_bytes += n;
	return true;
}

HttpResponse HttpHandler::handlePOST(const HttpRequest& req,
                                     const Location&    loc)
{
	std::string        path;
	const std::string& uri = req.getURI();
	const BodyStream   body = req.getbodyStream();
	const std::string& data = body.getData();
	const std::string  filename = body.getFileName();
	if (!filename.empty())
	{
		path = loc.root;
		if (path[path.size() - 1] != '/')
			path += '/';
		path += filename;
	}
	else
		path = buildPath(uri, loc);

	// const size_t       content_length = req.getContentLenght();

	// if (content_length > loc.client_max_body_size)
	// return makeError(HTTP_REQUEST_ENTITY_TOO_LARGE, &loc);

	// std::cout << "BODY>>" << body << "<<";
	// std::cout << "WRITEN: " << writen_bytes << '\n'
	// << "size body: " << body.size() << '\n';
	// std::cout << "Content Length: " << content_length << '\n';

	std::cout << "WRITE TO FILE\n";
	if (fd_ == -1)
	{
		fd_ =
		    open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0644);
		if (fd_ == -1)
		{
			if (errno == EACCES || errno == EISDIR)
				return makeError(HTTP_FORBIDDEN, &loc);
			return makeError(HTTP_INTERNAL_SERVER_ERROR, &loc);
		}
	}

	if (!writeToFile(data))
		return makeError(HTTP_INTERNAL_SERVER_ERROR, &loc);

	if (req.isComplete() || body.eof())
	{
		state_ = HTTP_WRITING_STATE;
		return makeError(HTTP_CREATED, &loc);
	}

	return HttpResponse();
}

HttpResponse HttpHandler::makeError(int status, const Location* loc)
{
	HttpResponse res(status);
	std::string  content;

	if (loc != NULL)
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
	}

	res.setFullResponse(res.buildErrorPage(status), "html");
	return res;
}

HttpResponse HttpHandler::redirect(int                status,
                                   const std::string& location,
                                   const std::string& host)
{
	HttpResponse res(status);
	std::string  l = location;

	if (location[0] == '/')
		l = "http://" + host + location;

	res.setHeader("Location", l);

	res.setFullResponse(res.buildErrorPage(status), "html");

	return res;
}

HttpResponse HttpHandler::makeFileResponse(const std::string& path,
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

HttpResponse HttpHandler::makeDirectoryPage(const std::string& path,
                                            const std::string& uri,
                                            const Location*    loc)
{
	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
		return makeError(HTTP_INTERNAL_SERVER_ERROR, loc);

	std::vector< std::string > files;
	struct dirent*             entry;

	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		if (ws::isDirectory(path + name) && name[name.size() - 1] != '/')
			name += '/';
		files.push_back(name);
	}

	HttpResponse res(HTTP_OK);
	std::string  content = res.buildDirectoryPage(files, path, uri);
	res.setFullResponse(content, "html");
	return res;
}

bool HttpHandler::isAllowedMethod(const std::string& method,
                                  const Location&    loc) const
{
	for (size_t i = 0; i < loc.allowed_methods.size(); i++)
	{
		if (method == loc.allowed_methods[i])
			return true;
	}
	return false;
}

void HttpHandler::reset()
{
	loc_ = NULL;
	error_ = 0;
	if (fd_ != -1)
	{
		close(fd_);
		fd_ = -1;
	}
	uploading = false;
	writen_bytes = 0;
	state_ = 0;
}

int HttpHandler::getState() const
{
	return state_;
}

