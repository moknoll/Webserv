/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:19:39 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:19:40 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../ConfigParser/ServerConfig.hpp"
#include <cstddef>
#include <map>
#include <string>

#define MAX_URL_LEN     4048
#define MAX_METHOD_LEN  7
#define MAX_HEADER_SIZE 1024 * 32

class HttpRequest
{
  public:
	enum State
	{
		sw_start = 0,
		sw_uri,
		sw_version,
		sw_headers,
		sw_almost_done,
		sw_done
	};

	HttpRequest();
	HttpRequest(const HttpRequest& other);
	~HttpRequest();

	HttpRequest&      operator=(const HttpRequest& other);

	void              parse(std::string& raw, const ServerConfig& cfg);

	std::string       getURI() const;
	std::string       getHeader(const std::string& name) const;
	std::string       getMethod() const;
	const std::string getbody() const; // ?
	int               getRequestStatus() const;
	size_t            getContentLenght() const;
	size_t            getReceivedBytes() const;
	std::string       getBoundary() const;
	std::string       getBodyTempFileName() const;
	std::map< std::string, std::string > getHeaders() const;

	void                                 setStatus(int status);

	bool                                 isChunked() const;
	bool                                 isMultipart() const;
	bool                                 isComplete() const;
	bool                                 isAlmostDone() const;

	void                                 reset();

  private:
	int                                  err_status_;
	size_t                               content_length_;
	std::string                          method_;
	std::string                          uri_;
	std::string                          http_version_;
	std::string                          body_;
	std::map< std::string, std::string > headers_;
	bool                                 chunked_;
	bool                                 multipart_;
	std::string                          boundary_;
	size_t                               recv_bytes_;
	State                                state_;
	std::string                          body_temp_file_;
	int                                  fd_;

	void parse_request_line(const std::string& raw);
	void parseBody(std::string& raw_data, const ServerConfig& cfg);
	void parseHeaderLine(const std::string& header_line);
	void parseHeaderLines(const std::string& headers);
	void parseHeaders(const std::string& headers);
	void processHeaderFields();
	void parseChunked(std::string& raw_data);
	bool isValidMethod(const std::string& method);
	void fail(int status);
	bool saveBodyToTempFile(std::string& raw_data);
	void closeFile_();
};

