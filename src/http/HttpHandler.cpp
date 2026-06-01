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

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// #include <vector>

const size_t HttpHandler::FILE_CHUNK_SIZE = 512 * 1024;

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

	this->loc_ = findMatchUri(uri, config_.locations);

	if (req.getRequestStatus() != HTTP_OK)
		return makeStatusResponse(req.getRequestStatus());

	if (this->loc_ == NULL)
		return makeStatusResponse(HTTP_NOT_FOUND);

	// if redirect -> redirect(status, location)
	if (this->loc_->has_redirect && this->loc_->redirect.first != -1)
		// if (this->loc_->redirect.first != -1)
		return redirect(loc_->redirect.first, loc_->redirect.second, host);

	if (!isAllowedMethod(method))
		return makeStatusResponse(HTTP_NOT_ALLOWED);

	// if (!loc->cgi_extension.empty())
	// 		handleCGI();

	if (method == "GET")
		return handleGET(req);
	else if (method == "POST")
		return handlePOST(req);
	// else if (method == "DELETE")
	// return handleDELETE(req, loc);

	return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
}

HttpResponse HttpHandler::handleGET(const HttpRequest& req)
{
	if (this->loc_ == NULL)
		return HttpResponse(HTTP_INTERNAL_SERVER_ERROR);

	const std::string& uri = req.getURI();
	const std::string& host = req.getHeader("Host");
	std::string        path = buildPath(uri);

	if (ws::isDirectory(path))
	{
		if (uri != "/" && uri[uri.size() - 1] != '/')
			return redirect(HTTP_MOVED_PERMANENTLY, uri + "/", host);

		std::string index_file =
		    loc_->index.empty() ? path + "index.html" : path + loc_->index;

		if (ws::checkFile(index_file.c_str()) == FILE_OK)
			return makeFileResponse(index_file);

		if (loc_->autoindex)
			return makeDirectoryPage(path, uri);

		return makeStatusResponse(HTTP_FORBIDDEN);
	}

	return makeFileResponse(path);
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

std::string HttpHandler::buildUploadPath(const HttpRequest& req)
{
	const std::string filename = req.getbodyStream().getFileName();
	std::string       path;

	if (!filename.empty())
	{
		std::string safe_name = sanitizeFileName(filename);

		if (!loc_->upload_path.empty())
			path = loc_->upload_path;
		else
			path = loc_->root;
		if (!path.empty() && path[path.size() - 1] != '/')
			path += '/';
		path += safe_name;
	}
	else
		path = buildPath(req.getURI());

	return path;
}

bool HttpHandler::openUploadFile(const std::string& path,
                                 HttpResponse&      err_resp)
{
	if (!validateUploadPath(path))
	{
		err_resp = makeStatusResponse(HTTP_FORBIDDEN);
		return false;
	}

	fd_ = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
	if (fd_ == -1)
	{
		if (errno == EEXIST)
			err_resp = makeStatusResponse(HTTP_CONFLICT);
		else if (errno == EACCES || errno == EISDIR)
			err_resp = makeStatusResponse(HTTP_FORBIDDEN);
		else if (errno == ENOSPC)
			err_resp = makeStatusResponse(HTTP_INSUFFICIENT_STORAGE);
		else
			err_resp = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);

		return false;
	}

	return true;
}

HttpResponse HttpHandler::handlePOST(const HttpRequest& req)
{
	// if (req.getContentLenght() > loc_->client_max_body_size)
	// return makeStatusResponse(HTTP_CONTENT_TOO_LARGE);

	const std::string data = req.getbodyStream().getData();

	if (state_ == HTTP_INIT && !data.empty())
	{
		std::string  path = buildUploadPath(req);

		HttpResponse error_respons;
		if (!openUploadFile(path, error_respons))
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
			return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
		}
	}

	if (req.isComplete() || req.getbodyStream().eof())
	{
		closeFile();
		state_ = HTTP_COMPLETE;
		return makeStatusResponse(HTTP_CREATED);
	}

	return HttpResponse();
}

HttpResponse HttpHandler::makeStatusResponse(int status)
{
	HttpResponse res(status);
	std::string  content;

	if (loc_ != NULL)
	{
		std::map< int, std::string >::const_iterator it =
		    loc_->error_pages.find(status);
		if (it != loc_->error_pages.end())
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

HttpResponse HttpHandler::makeFileResponse(const std::string& path)
{
	switch (ws::checkFile(path.c_str()))
	{
		case FILE_OK:        break;
		case ERR_NOT_FOUND:  return makeStatusResponse(HTTP_NOT_FOUND);
		case ERR_PERMISSION: return makeStatusResponse(HTTP_FORBIDDEN);
		case ERR_IS_DIR:     return makeStatusResponse(HTTP_FORBIDDEN);
		default:             return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
	}

	this->fd_ = open(path.c_str(), O_RDONLY);
	if (fd_ == -1)
		return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);

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

std::string HttpHandler::buildPath(const std::string& uri) const
{
	std::string sub_uri = uri.substr(loc_->path.length());
	std::string path = loc_->root;

	if (!path.empty() && path[path.length() - 1] != '/')
		path += '/';

	if (!sub_uri.empty() && sub_uri[0] == '/')
		path += sub_uri.substr(1);
	else
		path += sub_uri;

	return path;
}

HttpResponse HttpHandler::makeDirectoryPage(const std::string& path,
                                            const std::string& uri)
{
	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
		return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);

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

bool HttpHandler::isAllowedMethod(const std::string& method) const
{
	for (size_t i = 0; i < loc_->allowed_methods.size(); i++)
	{
		if (method == loc_->allowed_methods[i])
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

