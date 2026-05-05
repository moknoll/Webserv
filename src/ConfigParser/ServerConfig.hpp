#pragma once
#include <string>
#include <vector>
#include <map> 

struct Location
{
  std::string  path; // "/"  "/upload"
  std::string  root;  // "./www"
  std::string  index; // "index.html"
  bool   autoindex;  // on off (from subject:  Enabling or disabling directory listing.)
  size_t   client_max_body_size;
  std::map< int, std::string > error_pages; // 404 ->
  std::vector< std::string > allowed_methods;
  std::string redirect; // return 301 (from subject: HTTP redirection)
};

class ServerConfig
{
  public:
  int            port;
  std::string    host;
  std::string    server_name;
  std::string    root;
  std::string    index;
  size_t  client_max_body_size;
  std::map< int, std::string > error_pages; // 404 ->
  std::vector< Location > locations;

  ServerConfig();
  int _getPort();
  std::string getHost();
};