#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host("127.0.0.1"),
      port(8080),
      server_name("webserv"),
      root("./www"),
      index("index.html"),
      client_max_body_size(0),
      redirect(std::make_pair(-1, ""))
{
}

Location::Location()
    : path("/"),
      root(""),
      index("index.html"),
      autoindex(false),
      client_max_body_size(0),
      redirect(std::make_pair(-1, "")),
      upload_path(""),
      cgi_extension(""),
      cgi_path(""),
      has_redirect(false),
      has_cgi(false)
{
}
