#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host("0.0.0.0"),
      port(8080),
      server_name("www.example.com"),
      root("./www"),
      index("index.html"),
      client_max_body_size(0),
      redirect(std::make_pair(-1, ""))
{
}

Location::Location()
    : path("/"),
      root("./"),
      index("index.html"),
      autoindex(false),
      client_max_body_size(1000 * 1000),
      redirect(std::make_pair(-1, "")),
      upload_path(""),
      cgi_extension(""),
      cgi_path(""),
      has_redirect(false),
      has_cgi(false)
{
}


Location::Location(ServerConfig &srv)
    : path("/"),
      root(srv.root),
      index(srv.index),
      autoindex(false),
      client_max_body_size(srv.client_max_body_size),
      redirect(std::make_pair(-1, "")),
      upload_path(""),
      cgi_extension(""),
      cgi_path(""),
      has_redirect(false),
      has_cgi(false)
{
}

