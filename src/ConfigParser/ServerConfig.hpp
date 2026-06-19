#pragma once

#include <map>
#include <string>
#include <vector>

struct ServerConfig;

struct Location
{
	std::string path;                              // "/uploads", "/cgi-bin/py"
	std::string root;                              // "./www", "./cgi-bin/py"
	std::string index;                             // "index.html"
	bool        autoindex;                         // on/off
	size_t      client_max_body_size;              // override for this location
	std::vector< std::string >    allowed_methods; // GET, POST, DELETE
	std::map< int, std::string >  error_pages;   // 404->"/error_pages/404.html"
	std::pair< int, std::string > redirect;      // "301 /new-path"
	std::string                   upload_path;   // "./uploads"
	std::string                   cgi_extension; // ".py", ".php"
	std::string                   cgi_path;      // "/usr/bin/python3"
	bool                          has_redirect;  //
	bool                          has_cgi;       //
	Location();
	Location(ServerConfig& srv);
};

struct ServerConfig
{
	std::string                   host;        // "127.0.0.1"
	int                           port;        // 8080
	std::string                   server_name; // "webserv"
	std::string                   root;        // "./www"
	std::string                   index;       // "index.html"
	size_t                        client_max_body_size;
	std::pair< int, std::string > redirect;    // "301 /new-path"
	std::map< int, std::string >  error_pages; // 404 -> "/error_pages/404.html"
	std::vector< Location >       locations;   // all location blocks

	ServerConfig();
};
