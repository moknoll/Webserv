/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mknoll <mknoll@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:19:14 by nmagomad          #+#    #+#             */
/*   Updated: 2026/06/09 13:35:55 by mknoll           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpHandler.hpp"
#include "../lib/ws.hpp"
#include "BodyStream.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "constants.hpp"
#include "../cgi/handleCGI.hpp"

#include <iostream>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
// #include <ctime>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
// #include <map>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// #include <vector>

HttpHandler::HttpHandler(const ServerConfig& cfg)
    : config_(cfg), loc_(NULL), error_(0), state_(HTTP_INIT), fd_(-1)
{
}

HttpHandler::HttpHandler(const HttpHandler& other)
    : config_(other.config_), loc_(other.loc_), error_(other.error_),
      state_(other.state_), upload_file_path_(other.upload_file_path_),
      fd_(other.fd_)
{
}

HttpHandler::~HttpHandler()
{
	closeFile();
	if (state_ != HTTP_COMPLETE && !upload_file_path_.empty())
		std::remove(upload_file_path_.c_str());
}

HttpResponse HttpHandler::handle(const HttpRequest& req)
{
	const std::string& uri = req.getURI();
	const std::string& host = req.getHeader("Host");
	const std::string& method = req.getMethod();
	const Location*    loc = findMatchUri(uri, config_.locations);
	// config

	if (req.getRequestStatus() != HTTP_OK)
		return makeStatusResponse(req.getRequestStatus(), loc);

	if (loc == NULL)
		return makeStatusResponse(HTTP_NOT_FOUND, loc);

	// if redirect -> redirect(status, location)
	if (loc->redirect.first != -1)
		return redirect(loc->redirect.first, loc->redirect.second, host);

	if (!isAllowedMethod(method, *loc))
		return makeStatusResponse(HTTP_NOT_ALLOWED, loc);

	std::cout << loc->cgi_extension << " test" << std::endl;
	if (!loc->cgi_extension.empty())
			return handleCGI(req, *loc, config); 

	if (method == "GET")
		return handleGET(req, *loc);
	else if (method == "POST")
		return handlePOST(req, *loc);
	// else if (method == "DELETE")
	// return handleDELETE(req, loc);

	return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR, loc);
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

		return makeStatusResponse(HTTP_FORBIDDEN, &loc);
	}

	return makeFileResponse(path, &loc);
}

bool HttpHandler::writeToFile(const std::string& data)
{
	ssize_t n = write(fd_, data.data(), data.size());
	if (n == -1)
		return false;
	return true;
}

bool HttpHandler::validateUploadPath(const std::string& path)
{
	if (path.empty())
		return false;

	if (path.find("..") != std::string::npos)
		return false;

	return true;
}

std::string HttpHandler::sanitizeFileName(const std::string& filename)
{
	std::string safe_name;

	for (size_t i = 0; i < filename.size(); ++i)
	{
		char c = filename[i];

		if (c == '/' || c == '\\' || c == '\0')
			continue;

		if (c == '.' && (i == 0 || filename[i - 1] == '/'))
			continue;

		safe_name += c;
	}

	if (safe_name.empty())
		safe_name = "upload_" + ws::randString(5); // generate file name

	return safe_name;
}

std::string HttpHandler::buildUploadPath(const HttpRequest& req,
                                         const Location&    loc)
{
	const std::string filename = req.getbodyStream().getFileName();
	std::string       path;

	if (!filename.empty())
	{
		std::string safe_name = sanitizeFileName(filename);

		path = loc.root;
		if (!path.empty() && path[path.size() - 1] != '/')
			path += '/';
		path += safe_name;
	}
	else
		path = buildPath(req.getURI(), loc);

	return path;
}

bool HttpHandler::openUploadFile(const std::string& path,
                                 HttpResponse&      erro_resp,
                                 const Location&    loc)
{
	if (!validateUploadPath(path))
	{
		erro_resp = makeStatusResponse(HTTP_FORBIDDEN, &loc);
		return false;
	}

	fd_ = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
	if (fd_ == -1)
	{
		if (errno == EEXIST)
			erro_resp = makeStatusResponse(HTTP_CONFLICT, &loc);
		else if (errno == EACCES || errno == EISDIR)
			erro_resp = makeStatusResponse(HTTP_FORBIDDEN, &loc);
		else if (errno == ENOSPC)
			erro_resp = makeStatusResponse(HTTP_INSUFFICIENT_STORAGE, &loc);
		else
			erro_resp = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR, &loc);

		return false;
	}

	return true;
}

HttpResponse HttpHandler::handleCGI(const HttpRequest& req,
									const Location&		loc)
{
	(void)req;
	CgiContext ctx = buildCgiContext();
	buildCgiEnv(req, loc, ctx);
	
	// if(!executeChild(ctx))
	// 	return makeStatusResponse(500, &loc);
	// if (!writeRequestBody(ctx))
    //     return makeStatusResponse(500, &loc);

    // if (!readChildOutput(ctx))
    //     return makeStatusResponse(500, &loc);

    // HttpResponse res = buildResponse(ctx);
    // cleanup(ctx);

	
    return makeStatusResponse(504, &loc);
}

HttpResponse HttpHandler::handlePOST(const HttpRequest& req,
                                     const Location&    loc)
{
	// if (req.getContentLenght() > loc.client_max_body_size)
	// return makeError(HTTP_CONTENT_TOO_LARGE, &loc);

	const std::string data = req.getbodyStream().getData();

	if (state_ == HTTP_INIT && !data.empty())
	{
		std::string  path = buildUploadPath(req, loc);

		HttpResponse error_respons;
		if (!openUploadFile(path, error_respons, loc))
		{
			resetUpload();
			return error_respons;
		}

		upload_file_path_ = path;
		state_ = HTTP_RECV;
	}

	if (!data.empty())
	{
		if (!writeToFile(data))
		{
			resetUpload();
			return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR, &loc);
		}
	}

	if (req.isComplete() || req.getbodyStream().eof())
	{
		closeFile();
		state_ = HTTP_COMPLETE;
		return makeStatusResponse(HTTP_CREATED, &loc);
	}

	return HttpResponse();
}

HttpResponse HttpHandler::makeStatusResponse(int status, const Location* loc)
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
		case ERR_NOT_FOUND:  return makeStatusResponse(HTTP_NOT_FOUND, loc);
		case ERR_PERMISSION: return makeStatusResponse(HTTP_FORBIDDEN, loc);
		case ERR_IS_DIR:     return makeStatusResponse(HTTP_FORBIDDEN, loc);
		default:             return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR, loc);
	}

	this->fd_ = open(path.c_str(), O_RDONLY);
	if (fd_ == -1)
		return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR, loc);

	HttpResponse res(HTTP_OK);
	size_t       cl = ws::getFileSize(path.c_str());
	res.setFullResponse("", ws::getFileExtension(path));
	res.setHeader("Content-Length", ws::to_string(cl));
	return res;
}

bool HttpHandler::hasMoreData() const
{
	return this->fd_ != -1;
}

std::string HttpHandler::getFileChunk()
{
	if (fd_ == -1)
		return "";

	std::string buffer(FILE_CHUNK_SIZE, '\0');
	ssize_t     n = read(fd_, &buffer[0], FILE_CHUNK_SIZE);
	if (n <= 0)
	{
		closeFile();
		return "";
	}

	if (static_cast< size_t >(n) < FILE_CHUNK_SIZE)
	{
		buffer.resize(static_cast< size_t >(n));
		closeFile();
	}

	return buffer;
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
		return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR, loc);

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
	closeFile();
	error_ = 0;
	state_ = HTTP_INIT;
	upload_file_path_.clear();
}

int HttpHandler::getState() const
{
	return state_;
}

void HttpHandler::closeFile()
{
	if (fd_ != -1)
	{
		close(fd_);
		fd_ = -1;
	}
}

void HttpHandler::resetUpload()
{
	closeFile();

	if (!upload_file_path_.empty())
		std::remove(upload_file_path_.c_str());

	state_ = HTTP_CLOSE;
}

