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
#include "../constants.hpp"
#include "../lib/ws.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

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

// const size_t HttpHandler::FILE_CHUNK_SIZE = 512 * 1024;

HttpHandler::HttpHandler(const ServerConfig& cfg) :
        config_(cfg),
        loc_(),
        error_(0)
// fd_(-1)
{
}

HttpHandler::HttpHandler(const HttpHandler& other) :
        config_(other.config_),
        loc_(other.loc_),
        error_(other.error_),
        upload_file_path_(other.upload_file_path_)
// fd_(other.fd_)
{
}

HttpHandler::~HttpHandler()
{
	// closeFile();
}

HttpResponse HttpHandler::handle(const HttpRequest& req)
{
	loc_ = req.getLocation();

	if (this->loc_.has_redirect)
		return redirect(loc_.redirect.first, loc_.redirect.second);

	const std::string& method = req.getMethod();

	if (!isAllowedMethod(method))
		return HttpResponse::error(req, HTTP_NOT_ALLOWED);

	if (method == "GET")
		return handleGET(req);
	else if (method == "POST")
		return handlePOST(req);
	else if (method == "DELETE")
		return handleDELETE(req);

	return HttpResponse::error(req, HTTP_INTERNAL_SERVER_ERROR);
}

HttpResponse HttpHandler::handleGET(const HttpRequest& req)
{
	std::string path = req.getPath();
	std::cout << "PATH: " << path << std::endl;

	if (ws::isDirectory(path))
	{
		if (req.getURI() != "/" && req.getURI()[req.getURI().size() - 1] != '/')
			return redirect(HTTP_MOVED_PERMANENTLY, req.getURI() + "/");

		std::string index_file = path + loc_.index;

		if (ws::checkFile(index_file.c_str()) == FILE_OK)
			return HttpResponse::file(req, index_file);

		if (loc_.autoindex)
			return HttpResponse::directory(req);

		return HttpResponse::error(req, HTTP_FORBIDDEN);
	}

	return HttpResponse::file(req, path);
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
		safe_name =
		    "upload_" + ws::randString(5); // generate file name or with time

	return safe_name;
}

std::string HttpHandler::buildUploadPath(const std::string& filename)
{
	std::string path = loc_.upload_path;

	if (!path.empty() && path[path.size() - 1] != '/')
		path += '/';

	if (!filename.empty())
	{
		path += sanitizeFileName(filename);
	}
	else
		path += "upload_" + ws::to_string(std::time(NULL));

	return path;
}

int HttpHandler::openUploadFile(const std::string& path, HttpResponse& err_res)
{
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
	if (fd == -1)
	{
		if (errno == EEXIST)
			err_res = makeStatusResponse(HTTP_CONFLICT);
		else if (errno == EACCES || errno == EISDIR)
			err_res = makeStatusResponse(HTTP_FORBIDDEN);
		else if (errno == ENOSPC)
			err_res = makeStatusResponse(HTTP_INSUFFICIENT_STORAGE);
		else
			err_res = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);

		return fd;
	}

	return fd;
}

static const char* find_bytes_(const char* ext_start,
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
                                           HttpResponse&      err_res)
{
	std::string   temp_file = req.getBodyTempFileName();
	std::ifstream in(temp_file.c_str(), std::ios::binary);

	if (!in.is_open())
	{
		err_res = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
		return false;
	}

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
		{
			err_res = makeStatusResponse(HTTP_BAD_REQUEST);
			return false;
		}
		p += 10;
		filename = line.substr(p, e_p - p);
	}

	std::string path = buildUploadPath(filename);
	if (!validateUploadPath(path))
	{
		err_res = makeStatusResponse(HTTP_FORBIDDEN);
		return false;
	}

	int fd = openUploadFile(path.c_str(), err_res);
	if (fd == -1)
	{
		return false;
	}

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
				to_write = total_in_buf - end_boundary.size();

			if (to_write > 0)
				write(fd, buf_start, to_write);

			leftover = total_in_buf - to_write;
			std::memmove(&buffer[0], buf_start + to_write, leftover);
		}
	}
	close(fd);
	return true;
}

bool HttpHandler::savePlainBody(const HttpRequest& req, HttpResponse& err_res)
{
	std::string filename = req.getHeader("X-Filename");
	if (filename.empty())
	{
		err_res = makeStatusResponse(HTTP_BAD_REQUEST);
		return false;
	}

	std::string path = buildUploadPath(filename);
	if (!validateUploadPath(path))
	{
		err_res = makeStatusResponse(HTTP_FORBIDDEN);
		return false;
	}

	std::string   temp_file = req.getBodyTempFileName();
	std::ifstream in(temp_file.c_str(), std::ios::binary);
	if (!in.is_open())
	{
		err_res = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
		return false;
	}

	int fd = openUploadFile(path.c_str(), err_res);
	if (fd == -1)
		return false;

	const size_t        buf_size = 4096;
	std::vector< char > buffer(buf_size);
	while (in)
	{
		in.read(&buffer[0], buf_size);
		size_t bytes_read = in.gcount();
		if (bytes_read == 0)
			break;
		if (write(fd, &buffer[0], bytes_read) < 0)
		{
			close(fd);
			err_res = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
			return false;
		}
	}
	close(fd);
	return true;
}

// bool HttpHandler::exceedsBodySizeLimit(const HttpRequest& req) const
// {
// 	const size_t mbs = loc_->client_max_body_size;
// 	const size_t cl = req.getContentLenght();
// 	const size_t rb = req.getReceivedBytes();
//
// 	return (cl > mbs || (req.isChunked() && rb > mbs));
// }

// bool HttpHandler::saveBodyToTempFile(const HttpRequest& req,
//                                      HttpResponse&      err_res)
// {
// 	const std::string data = req.getbody();
//
// 	if (state_ == HTTP_INIT)
// 	{
// 		temp_file_ = "/tmp/wsload_" + ws::randString();
//
// 		fd_ = open(
// 		    temp_file_.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
// 		if (fd_ == -1)
// 		{
// 			err_res = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
// 			return false;
// 		}
// 		state_ = HTTP_RECV;
// 	}
//
// 	if (!data.empty())
// 	{
// 		ssize_t n = write(fd_, data.data(), data.size());
// 		if (n == -1)
// 		{
// 			resetUpload();
// 			err_res = makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
// 			return false;
// 		}
// 	}
//
// 	return true;
// }

HttpResponse HttpHandler::handlePOST(const HttpRequest& req)
{
	if (loc_.upload_path.empty())
	{
		std::string path = req.getPath();
		PathInfo    info = ws::checkPath(path);
		if (info.exists)
		{
			if (info.type == PATH_IS_DIR)
				return makeStatusResponse(HTTP_FORBIDDEN);
			if (info.type == PATH_IS_FILE)
				return makeStatusResponse(HTTP_NOT_ALLOWED);
		}
		else
			return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
	}

	HttpResponse error_respons;

	bool         success = req.isMultipart() ?
	                           saveUploadedFileFromTemp(req, error_respons) :
	                           savePlainBody(req, error_respons);

	if (!success)
		return error_respons;
	return makeStatusResponse(HTTP_CREATED);
	// // if (!saveBodyToTempFile(req, error_respons))
	// // 	return error_respons;
	//
	// if (req.isComplete())
	// {
	// 	closeFile();
	// 	state_ = HTTP_COMPLETE;
	//
	// 	bool success = req.isMultipart() ?
	// 	                   saveUploadedFileFromTemp(req, error_respons) :
	// 	                   savePlainBody(req, error_respons);
	//
	// 	std::remove(temp_file_.c_str());
	// 	if (!success)
	// 		return error_respons;
	//
	// 	return makeStatusResponse(HTTP_CREATED);
	// }

	return HttpResponse();
}

HttpResponse HttpHandler::handleDELETE(const HttpRequest& req)
{
	const std::string path = req.getPath();

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
	HttpResponse                                 res(status);
	std::string                                  content;

	std::map< int, std::string >::const_iterator it =
	    config_.error_pages.find(status);
	if (it != config_.error_pages.end())
	{
		if (ws::readFile(it->second.c_str(), content))
		{
			res.setFullResponse(content, "html");
			return res;
		}
	}

	res.setFullResponse(res.buildErrorPage(status), "html");
	return res;
}

HttpResponse HttpHandler::redirect(int status, const std::string& target)
{
	HttpResponse res(status);
	std::string  t = target;

	res.setHeader("Location", t);

	res.setFullResponse(res.buildErrorPage(status), "html");

	return res;
}

// HttpResponse HttpHandler::makeFileResponse(const std::string& path)
// {
// 	switch (ws::checkFile(path.c_str()))
// 	{
// 		case FILE_OK:        break;
// 		case ERR_NOT_FOUND:  return makeStatusResponse(HTTP_NOT_FOUND);
// 		case ERR_PERMISSION: return makeStatusResponse(HTTP_FORBIDDEN);
// 		case ERR_IS_DIR:     return makeStatusResponse(HTTP_FORBIDDEN);
// 		default:             return
// makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
// 	}
//
// 	this->fd_ = open(path.c_str(), O_RDONLY);
// 	if (fd_ == -1)
// 		return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
//
// 	HttpResponse res(HTTP_OK);
// 	size_t       cl = ws::getFileSize(path.c_str());
// 	res.setFullResponse("", ws::getFileExtension(path));
// 	res.setHeader("Content-Length", ws::to_string(cl));
// 	return res;
// }

// std::string HttpHandler::getFileChunk()
// {
// 	if (fd_ == -1)
// 		return "";
//
// 	std::string buffer(FILE_CHUNK_SIZE, '\0');
// 	ssize_t     n = read(fd_, &buffer[0], FILE_CHUNK_SIZE);
// 	if (n <= 0)
// 	{
// 		closeFile();
// 		return "";
// 	}
//
// 	if (static_cast< size_t >(n) < FILE_CHUNK_SIZE)
// 	{
// 		buffer.resize(static_cast< size_t >(n));
// 		closeFile();
// 	}
//
// 	return buffer;
// }

// HttpResponse HttpHandler::makeDirectoryListingResponse(const std::string&
// path,
//                                                        const std::string&
//                                                        uri)
// {
// 	DIR* dir = opendir(path.c_str());
// 	if (dir == NULL)
// 		return makeStatusResponse(HTTP_INTERNAL_SERVER_ERROR);
//
// 	std::vector< std::string > files;
// 	struct dirent*             entry;
//
// 	while ((entry = readdir(dir)) != NULL)
// 	{
// 		std::string name = entry->d_name;
// 		if (name == "." || name == "..")
// 			continue;
// 		if (ws::isDirectory(path + name) && name[name.size() - 1] != '/')
// 			name += '/';
// 		files.push_back(name);
// 	}
//
// 	closedir(dir);
//
// 	HttpResponse res(HTTP_OK);
// 	std::string  content = res.buildDirectoryPage(files, path, uri);
// 	res.setFullResponse(content, "html");
// 	return res;
// }

bool HttpHandler::isAllowedMethod(const std::string& method) const
{
	for (size_t i = 0; i < loc_.allowed_methods.size(); i++)
	{
		if (method == loc_.allowed_methods[i])
			return true;
	}
	return false;
}

void HttpHandler::reset()
{
	loc_ = Location();
	// closeFile();
	error_ = 0;
	upload_file_path_.clear();
}

// void HttpHandler::closeFile()
// {
// 	if (fd_ != -1)
// 	{
// 		close(fd_);
// 		fd_ = -1;
// 	}
// }
