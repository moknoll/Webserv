#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host("0.0.0.0"),
      port(8080),
      server_name("www.example.com"),
      root("./www"),
      index("index.html"),
      client_max_body_size(0),
      redirect(std::make_pair(-1, ""))
{}

ServerConfig::ServerConfig(const ServerConfig& other)
    : host(other.host),
      port(other.port),
      server_name(other.server_name),
      root(other.root),
      index(other.index),
      client_max_body_size(other.client_max_body_size),
      redirect(other.redirect),
      error_pages(other.error_pages),
      locations(other.locations)
{}

ServerConfig& ServerConfig::operator=(const ServerConfig& other)
{
    if (this != &other)
    {
        host = other.host;
        port = other.port;
        server_name = other.server_name;
        root = other.root;
        index = other.index;
        client_max_body_size = other.client_max_body_size;
        redirect = other.redirect;
        error_pages = other.error_pages;
        locations = other.locations;
    }
    return *this;
}

ServerConfig::~ServerConfig() {}


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
{}


Location::Location(ServerConfig &srv)
    : path("/"),
      root(srv.root),
      index(srv.index),
      autoindex(false),
      client_max_body_size(srv.client_max_body_size),
      redirect(srv.redirect),
      upload_path(""),
      cgi_extension(""),
      cgi_path(""),
      has_redirect(false),
      has_cgi(false)
{}

Location::Location(const Location& other)
    : path(other.path),
      root(other.root),
      index(other.index),
      autoindex(other.autoindex),
      client_max_body_size(other.client_max_body_size),
      allowed_methods(other.allowed_methods),
      error_pages(other.error_pages),
      redirect(other.redirect),
      upload_path(other.upload_path),
      cgi_extension(other.cgi_extension),
      cgi_path(other.cgi_path),
      has_redirect(other.has_redirect),
      has_cgi(other.has_cgi)
{}

Location& Location::operator=(const Location& other)
{
    if (this != &other)
    {
        path = other.path;
        root = other.root;
        index = other.index;
        autoindex = other.autoindex;
        client_max_body_size = other.client_max_body_size;
        allowed_methods = other.allowed_methods;
        error_pages = other.error_pages;
        redirect = other.redirect;
        upload_path = other.upload_path;
        cgi_extension = other.cgi_extension;
        cgi_path = other.cgi_path;
        has_redirect = other.has_redirect;
        has_cgi = other.has_cgi;
    }
    return *this;
}

Location::~Location() {}

