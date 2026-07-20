#pragma once

#include <map>
#include <string>
#include <vector>

struct ServerConfig;

struct Location
{
	std::string path;
	std::string root;
	std::string index;
	bool        autoindex;
	size_t      client_max_body_size;
	std::vector< std::string >    allowed_methods;
	std::map< int, std::string >  error_pages;
	std::pair< int, std::string > redirect;
	std::string                   upload_path;
	std::string                   cgi_extension;
	std::string                   cgi_path;
	bool                          has_redirect;
	bool                          has_cgi;

	Location();
	Location(ServerConfig& srv);
	Location(const Location& other);
	Location& operator=(const Location& other);
	~Location();
};

struct ServerConfig
{
	std::string                   host;
	int                           port;
	std::string                   server_name;
	std::string                   root;
	std::string                   index;
	size_t                        client_max_body_size;
	std::pair< int, std::string > redirect;
	std::map< int, std::string >  error_pages;
	std::vector< Location >       locations;

	ServerConfig();
	ServerConfig(const ServerConfig& other);
	ServerConfig& operator=(const ServerConfig& other);
	~ServerConfig();
};
