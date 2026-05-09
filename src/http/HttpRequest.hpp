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

typedef enum state_e
{
	sw_start = 0,
	// sw_method,
	sw_uri,
	sw_version,
	sw_headers,
	sw_done
} state_t;

typedef struct req_buf_s
{
	size_t  pos;
	state_t state;

} req_buf_t;

#define MAX_URL_LEN     4048
#define MAX_METHOD_LEN  7
#define MAX_HEADER_SIZE 1024 * 32

class HttpRequest
{
  public:
	HttpRequest();
	// HttpRequest(const std::string& req_message);
	HttpRequest(const HttpRequest& other);
	~HttpRequest();

	HttpRequest& operator=(const HttpRequest& other);

	std::string  getURI() const;
	std::string  getHeader(const std::string& name) const;
	int          getRequestStatus() const;

	void         print_parsed();
	void         parse(const std::string& raw);
	bool         isComplete() const
	{
		// if (err_status_ != HTTP_OK)
		// 	return true;
		if (state_ == sw_done)
			return true;
		return false;
	}
	void setStatus(int status)
	{
		err_status_ = status;
	}

  private:
	int                                  err_status_;
	int                                  method_;
	size_t                               content_length_;
	std::string                          method_str_;
	std::string                          uri_;
	std::string                          http_version_;
	std::string                          request_line_;
	std::string                          host_;
	std::string                          body_;
	std::string                          extension_;
	std::string                          unparsed_uri_;
	std::map< std::string, std::string > headers_;

	int     _parser(const std::string& req_message);
	int     _parse_request_line(const std::string& line);
	void    _parse_headers(const std::string& header);

	void    parse_request_line(const std::string& raw);
	void    parseHeaderLine(const std::string& header_line);
	void    fail(int status);

	size_t  current_pos_;
	state_t state_;
};

