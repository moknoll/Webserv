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

	HttpResponse& operator=(const HttpResponse& other);

	std::string   buildResponse() const;

	void        setHeader(const std::string& header_name, const std::string& v);
	void        setBody(const std::string& content);
	void        setFullResponse(const std::string& content,
	                            const std::string& extention);
	void        setStatus(int status);

	std::string getHeader(const std::string& header_name) const;
	std::string buildErrorPage(int err_status) const;

	std::string buildDirectoryPage(const std::vector< std::string >& files,
	                               const std::string&                path,
	                               const std::string&                uri);
	const char* getStatusMsg(int status) const;
	void        clear();

  private:
	int                                  status_;
	std::string                          status_line_;
	std::map< std::string, std::string > headers_;
	std::string                          body_;

	const std::string                    getHttpTime() const;
	const std::string getMimeType(const std::string& ext) const;
	std::string       truncateName(const std::string& name);
	std::string buildEntry(const std::string& path, const std::string& name);
	std::string formatEntry(const std::string& name,
	                        const std::string& time,
	                        const std::string& size);
};

