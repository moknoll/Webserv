#pragma once

#include <map>
#include <string>
#include <vector>

struct Location
{
	std::string                  path;                 // "/"  "/upload"
	std::string                  root;                 // "./www" or /tmp/www
	std::string                  index;                // "index.html"
	bool                         autoindex;            // default false
	size_t                       client_max_body_size; // default 1M
	std::map< int, std::string > error_pages;          // 404 ->
	std::vector< std::string >   allowed_methods;      // default GET, POST
	std::string redirect; // return 301 (from subject: HTTP redirection)
};

class ServerConfig
{
  public:
	int                          port;
	std::string                  host;
	std::string                  server_name;
	std::string                  root;
	std::string                  index;
	bool                         autoindex;            // default false
	size_t                       client_max_body_size; // default 1M
	std::map< int, std::string > error_pages;          // 404 ->
	std::vector< Location >      locations;

	ServerConfig();
	int _getPort();
};

/*
struct Location
{
    std::string					path;					// "/"  "/upload"
    std::string					root;					// "./www"
    std::string					index;					// "index.html"
    bool						autoindex;
(from subject: Enabling or disabling directory listing) size_t
client_max_body_size std::vector<std::string>	allowed_methods;		// GET
POST DELETE (i think if not have this conf. allowed all,) (from subject: List of
accepted HTTP methods for the route.) std::string					redirect;
// if we need (i don't sure) return 301 (from subject: HTTP redirection)
};

struct ServerConfig
{
    std::string					host;
    int							port;
    std::string					server_name;
    std::map<int,std::string>	error_pages;			// 404 ->
"./www/404.html" std::vector<Location>		locations;
}; */
