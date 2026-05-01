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

class HttpRequest
{
  public:
	HttpRequest();
	HttpRequest(const std::string& req_message);
	HttpRequest(const HttpRequest& other);
	~HttpRequest();

	HttpRequest& operator=(const HttpRequest& other);

	std::string  getURI() const;
	std::string  getHeader(const std::string& name) const;
	int          getRequestStatus() const;

  private:
	int                                  err_status_;
	int                                  method_;
	size_t                               Content_length_;
	// std::string                          method_;
	std::string                          uri_;
	std::string                          http_version_;
	std::string                          request_line_;
	std::string                          host_;
	std::string                          body_;
	std::string                          extension_;
	std::string                          unparsed_uri_;
	std::map< std::string, std::string > headers_;

	int  _parser(const std::string& req_message);
	int  _parse_request_line(const std::string& line);
	void _parse_headers(const std::string& header);
};

