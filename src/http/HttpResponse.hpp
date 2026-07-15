/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:19:50 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:19:51 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "HttpRequest.hpp"

#include <map>
#include <string>
#include <vector>

class HttpResponse
{
  public:
	HttpResponse();
	HttpResponse(const int status);
	HttpResponse(const HttpResponse& other);
	~HttpResponse();

	HttpResponse&       operator=(const HttpResponse& other);

	static HttpResponse error(const HttpRequest& req, int status);
	static HttpResponse file(const HttpRequest& req, const std::string& path);
	static HttpResponse directory(const HttpRequest& req);
	static HttpResponse redirect(int status, const std::string& target);

	std::string         nextChunk();
	std::string         toString() const;

	void                setStatus(int status);
	void        setHeader(const std::string& name, const std::string& v);
	void        setBody(const std::string& content);
	void        setFileFd(int fd);
	void        setFullResponse(const std::string& content = "",
	                            const std::string& extention = "");

	std::string getHeader(const std::string& header_name) const;
	std::string buildErrorPage(int err_status) const;
	std::string buildDirectoryPage(const std::vector< std::string >& files,
	                               const std::string&                path,
	                               const std::string&                uri);
	void        reset();

  private:
	static const size_t                  FILE_CHUNK_SIZE;

	int                                  status_;
	std::string                          status_line_;
	std::map< std::string, std::string > headers_;
	std::string                          body_;
	int                                  fd_;
	bool                                 start_send_;

	void                                 closeFile();
	const char*                          getStatusMsg(int status) const;
	const std::string                    getHttpTime() const;
	const std::string getMimeType(const std::string& ext) const;
	std::string       truncateName(const std::string& name);
	std::string buildEntry(const std::string& path, const std::string& name);
	std::string formatEntry(const std::string& name,
	                        const std::string& time,
	                        const std::string& size);
};

