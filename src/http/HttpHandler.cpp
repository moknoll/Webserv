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
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const size_t HttpHandler::FILE_CHUNK_SIZE = 512 * 1024;

HttpHandler::HttpHandler(const ServerConfig& cfg) :
        config_(cfg),
        loc_(NULL),
        error_(0),
        state_(HTTP_INIT),
        fd_(-1)
{
}

HttpHandler::HttpHandler(const HttpHandler& other) :
        config_(other.config_),
        loc_(other.loc_),
        error_(other.error_),
        state_(other.state_),
        upload_file_path_(other.upload_file_path_),
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
	else if (method == "DELETE")
		return handleDELETE(req);

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

std::string HttpHandler::buildUploadPath(const HttpRequest& req,
                                         const std::string& filename)
{
	std::string path = loc_->upload_path;

	if (loc_->upload_path.empty())
		path = loc_->root;

	if (!filename.empty())
	{
		std::string safe_name = sanitizeFileName(filename);

		if (!path.empty() && path[path.size() - 1] != '/')
			path += '/';
		path += safe_name;
	}
	else
	{
		path = buildPath(req.getURI());
	}

	return path;
}

int HttpHandler::openUploadFile(const std::string& path, HttpResponse& err_resp)
{
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
	if (fd == -1)
	{
		if (errno == EEXIST)
			err_resp = makeStatusResponse(HTTP_CONFLICT);
		else if (errno == EACCES || errno == EISDIR)
			err_resp = makeStatusResponse(HTTP_FORBIDDEN);
		else if (errno == ENOSPC)
			err_resp = makeStatusResponse(HTTP_INSUFFICIENT_STORAGE);
		else
			err_resp = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);

		return fd;
	}

	return fd;
}

const char* HttpHandler::find_bytes_(const char* ext_start,
                                     const char* ext_end,
                                     const char* s_start,
                                     const char* s_end)
{
	size_t s_len = s_end - s_start;
	if (s_len == 0)
		return ext_start;
	for (const char* p = ext_start; p <= ext_end - s_len; ++p)
	{
		if (std::memcmp(p, s_start, s_len) == 0)
			return p;
	}
	return NULL;
}

bool HttpHandler::saveUploadedFileFromTemp(const HttpRequest& req,
                                           HttpResponse&      err_resp)
{
	std::ifstream in(temp_file_.c_str(), std::ios::binary);

	if (!in.is_open())
		return false;

	std::string line;
	std::string filename;
	while (std::getline(in, line))
	{
		if (line == "\r")
			break;
		std::string::size_type p = line.find("filename=\"");
		if (p == std::string::npos)
			continue;

		std::string::size_type e_p = line.find("\"\r", p);
		if (e_p == std::string::npos)
			return false;
		p += 10;
		filename = line.substr(p, e_p - p);
	}

	std::string path = buildUploadPath(req, filename);
	if (!validateUploadPath(path))
	{
		err_resp = makeStatusResponse(HTTP_FORBIDDEN);
		return false;
	}

	int fd = openUploadFile(path.c_str(), err_resp);
	if (fd == -1)
		return false;

	std::string         end_boundary = CRLF "--" + req.getBoundary() + "--";
	const size_t        buf_size = 4096;
	std::vector< char > buffer(buf_size + end_boundary.size());

	size_t              leftover = 0;

	while (in)
	{
		in.read(&buffer[leftover], buf_size);
		size_t bytes_read = in.gcount();
		size_t total_in_buf = leftover + bytes_read;

		if (total_in_buf == 0)
			break;

		const char* buf_start = &buffer[0];
		const char* buf_end = buf_start + total_in_buf;
		const char* found =
		    find_bytes_(buf_start,
		                buf_end,
		                end_boundary.data(),
		                end_boundary.data() + end_boundary.size());

		if (found != NULL)
		{
			size_t bytes_to_write = found - buf_start;
			if (bytes_to_write > 0)
				write(fd, buf_start, bytes_to_write);
			break;
		}
		else
		{
			size_t to_write = 0;
			if (total_in_buf > end_boundary.size())
			{
				to_write = total_in_buf - end_boundary.size();
			}

			if (to_write > 0)
			{
				write(fd, buf_start, to_write);
			}

			leftover = total_in_buf - to_write;
			std::memmove(&buffer[0], buf_start + to_write, leftover);
		}
	}
	close(fd);
	return true;
}

HttpResponse HttpHandler::handlePOST(const HttpRequest& req)
{
	// if (req.getContentLenght() > loc_->client_max_body_size)
	// return makeStatusResponse(HTTP_CONTENT_TOO_LARGE);

	// const std::string data = req.getbodyStream().getData();
	const std::string data = req.getbody();

	if (state_ == HTTP_INIT && !data.empty())
	{
		temp_file_ = "./wsload_" + ws::randString();

		fd_ = open(
		    temp_file_.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
		if (fd_ == -1)
			return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
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

	if (req.isComplete())
	{
		HttpResponse error_respons;
		closeFile();
		if (req.isMultipart())

		{
			if (!saveUploadedFileFromTemp(req, error_respons))
			{
				std::remove(temp_file_.c_str());
				return error_respons;
			}
		}
		else
		{
			std::string path = buildUploadPath(req, "");
			int         fd = openUploadFile(path, error_respons);
			if (fd == -1)
				return error_respons;
			close(fd);
			if (std::rename(temp_file_.c_str(), path.c_str()) != 0)
				return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
		}
		std::remove(temp_file_.c_str());
		state_ = HTTP_COMPLETE;
		return makeStatusResponse(HTTP_CREATED);
	}

	return HttpResponse();
}

HttpResponse HttpHandler::handleDELETE(const HttpRequest& req)
{
	const std::string& uri = req.getURI();
	std::string        path = buildPath(uri);

	if (ws::isDirectory(path.c_str()))
		return makeStatusResponse(HTTP_FORBIDDEN);

	switch (ws::checkFile(path.c_str()))
	{
		case FILE_OK:        break;
		case ERR_IS_DIR:     return makeStatusResponse(HTTP_FORBIDDEN);
		case ERR_PERMISSION: return makeStatusResponse(HTTP_FORBIDDEN);
		case ERR_NOT_FOUND:  return makeStatusResponse(HTTP_NOT_FOUND);
		default:             return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
	}

	if (std::remove(path.c_str()) != 0)
		return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
	HttpResponse resp(HTTP_NO_CONTENT);
	resp.setFullResponse();
	return resp;
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
				if (uri.size() == path.size() || uri[path.size()] == '/'
				    || uri[path.size() - 1] == '/')
				{
					best_loc = &locations[i];
					len_best_loc = path.size();
				}
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
	{
		path += sub_uri;
	}

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
	temp_file_.clear();
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

