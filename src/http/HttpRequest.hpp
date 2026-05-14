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

// #include <cstdint>
#include <cstddef>
#include <map>
#include <string>

#define MAX_URL_LEN     4048
#define MAX_METHOD_LEN  7
#define MAX_HEADER_SIZE 1024 * 32

typedef enum state_e
{
	sw_start = 0,
	// sw_method,
	sw_uri,
	sw_version,
	sw_headers,
	sw_almost_done,
	sw_done
} state_t;

typedef struct req_buf_s
{
	size_t  pos;
	state_t state;

} req_buf_t;

class HttpRequest
{
  public:
	HttpRequest();
	// HttpRequest(const std::string& req_message);
	HttpRequest(const HttpRequest& other);
	~HttpRequest();

	HttpRequest&      operator=(const HttpRequest& other);

	std::string       getURI() const;
	std::string       getHeader(const std::string& name) const;
	std::string       getMethod() const;
	const std::string getbody() const;
	int               getRequestStatus() const;
	size_t            getContentLenght() const;

	void              setStatus(int status);

	void              print_parsed();
	void              parse(std::string& raw);
	bool              isValidMethod(const std::string& method);
	bool              isChunked() const;
	bool              isComplete() const;
	bool              isAlmostDone() const;

	void              reset();

  private:
	int                                  err_status_;
	size_t                               content_length_;
	std::string                          method_;
	std::string                          uri_;
	std::string                          http_version_;
	std::string                          request_line_;
	std::string                          host_;
	std::string                          body_;
	std::map< std::string, std::string > headers_;
	bool                                 chunked;
	std::string                          temp_file;
	state_t                              state_;

	void parse_request_line(const std::string& raw);
	void parseBody(const std::string& raw);
	void parseHeaderLine(const std::string& header_line);
	void fail(int status);
};

