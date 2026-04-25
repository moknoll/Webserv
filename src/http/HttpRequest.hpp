#pragma once

#include <map>
#include <string>

#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_NO_CONTENT 204
#define HTTP_PARTIAL_CONTENT 206

#define HTTP_SPECIAL_RESPONSE 300
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_MOVED_TEMPORARILY 302
#define HTTP_NOT_MODIFIED 304

#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_NOT_ALLOWED 405
#define HTTP_REQUEST_TIME_OUT 408
#define HTTP_CONFLICT 409
#define HTTP_LENGTH_REQUIRED 411
#define HTTP_PRECONDITION_FAILED 412
#define HTTP_REQUEST_ENTITY_TOO_LARGE 413
#define HTTP_REQUEST_URI_TOO_LARGE 414
#define HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_RANGE_NOT_SATISFIABLE 416

#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_NOT_IMPLEMENTED 501
#define HTTP_BAD_GATEWAY 502
#define HTTP_SERVICE_UNAVAILABLE 503
#define HTTP_GATEWAY_TIME_OUT 504
#define HTTP_INSUFFICIENT_STORAGE 507

class HttpRequest
{
  public:
	HttpRequest();
	HttpRequest(const std::string& req_message);
	HttpRequest(const HttpRequest& other);
	~HttpRequest();

	HttpRequest& operator=(const HttpRequest& other);

	void         Get(const std::string& content);

	std::string  get_uri();

  private:
	int                                  _err_status;
	std::string                          _method;
	std::string                          _uri;
	std::string                          _http_version;
	std::string                          _start_line;
	std::string                          _host;
	std::string                          _body;
	std::string                          _ContentLenght;
	std::map< std::string, std::string > _headers;

	int  _parser(const std::string& req_message);
	int  _parse_start_line(const std::string& line);
	void _parse_header(const std::string& header);
};

