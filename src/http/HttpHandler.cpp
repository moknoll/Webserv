#include "HttpHandler.hpp"
#include "../lib/ws.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "http.hpp"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

HttpHandler::HttpHandler(const ServerConfig& cfg) : config(cfg), error(0) {}

// HttpHandler::HttpHandler(const HttpHandler& other): config(other.config),
// error(other.error)  {}
HttpHandler::~HttpHandler() {}

// HttpHandler&	HttpHandlerHt::operator=(const HttpHandler& other) {}


HttpResponse HttpHandler::handleGET(const HttpRequest& req)
{
	const std::string& uri = req.get_uri();

	if (req.hasError())
		return makeError(req.hasError());

	const Location* loc = findMatchUri(uri, config.locations);

	if (loc == NULL)
		return makeError(HTTP_NOT_FOUND);

	// getMethod if method not allowed
	// return makeError(HTTP_NOT_ALLOWED);

	// build path
	//
	std::string path = buildPath(uri, *loc);

	if (ws::isDirectory(path))
		path += loc->index;

	// if path is dir add index.html from location.index
	if (ws::isDirectory(path))
	{
		std::string index_path = path + loc->index;

		// if (loc->autoindex && kk
		; // return makeDirPage(path);
		  // p += loc.index;
	}

	return buildFileResponse(path);
}



HttpResponse HttpHandler::makeError(int status)
{
	HttpResponse res;
	std::string  content;
	res.setStatus(status);

	// if in config exist eror page  page = config.get_eror_page
	// ws::checkFile(confing.error_page_path
	// content = readFile(config.error_page.path);
	if (content == "")
	{
		res.setBody(res.get_error_page(status), "text/html");
	}
	else
	{
		res.setHeader("Content-Type", "text/html");
		res.setHeader("Content-Length", ws::to_string(content.size()));
		res.setHeader("Server", "webserv");
		// res.setBody(content, getMimeType(error_page_path);
	}
	return res;
}

std::string HttpHandler::readFile(const char* path)
{
	error = ws::checkFile(path);

	if (error != HTTP_OK)
		return "";

	std::ifstream file(path);

	if (!file.is_open())
	{
		error = HTTP_INTERNAL_SERVER_ERROR;
		return "";
	}

	std::string content((std::istreambuf_iterator< char >(file)),
	                    std::istreambuf_iterator< char >());
	error = HTTP_OK;
	return content;
}

HttpResponse HttpHandler::buildFileResponse(const std::string& path)
{
	std::string content = readFile(path.c_str());

	if (error != HTTP_OK)
		return makeError(error);

	HttpResponse res;

	res.setStatus(HTTP_OK);

	res.setBody(content, getMimeType(path));
	return res;
}

const Location*
HttpHandler::findMatchUri(const std::string&             uri,
                          const std::vector< Location >& locations)
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

// clang-format off
std::string HttpHandler::getMimeType(const std::string& path)
{
	std::string ext = ws::getFileExtension(path);

	if (ext == "htm" || ext == "html")	return "text/html";
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

