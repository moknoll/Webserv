/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BodyStream.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/26 19:25:33 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/26 19:25:35 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "BodyStream.hpp"
#include "../lib/ws.hpp"
#include "constants.hpp"
#include <iostream>
#include <string>

BodyStream::BodyStream()
    : boundary_(""), delimeter_(""), end_delimeter_(""), buf_(""), name_(""),
      filename_(""), state_(sb_start), data_("")
{
}

// BodyStream::BodyStream(const BodyStream& other) {}

BodyStream::~BodyStream() {}
// WIP
// BodyStream& BodyStream::operator=(const BodyStream& other)
// {
// 	if (this != &other)
// 	{
// 	}
// 	return *this;
// }

// FOR DEBUG
void BodyStream::printBodyStream() const
{
	std::cout << "FILENAME: " << filename_ << '\n';
	std::cout << "NAME: " << name_ << '\n';
	std::cout << "DATA: " << data_ << ":\n";
	std::cout << "B1:" << delimeter_ << '\n';
	std::cout << "B2:" << end_delimeter_ << '\n';
	std::cout << "B0:" << boundary_ << '\n';
}


bool BodyStream::parseHeaders_(const std::string& headers)
{
	std::string::size_type p = headers.find("name=");
	if (p == std::string::npos)
		return false;
	p += 5;
	std::string::size_type e_p = headers.find(";", p);
	if (e_p == std::string::npos)
		return false;
	name_ = headers.substr(p, e_p - p);
	name_ = ws::trim(name_, '"');

	p = headers.find("filename=\"", e_p);
	if (p == std::string::npos)
		return false;
	p += 10;
	e_p = headers.find("\"" CRLF, p);
	if (e_p == std::string::npos)
		return false;
	filename_ = headers.substr(p, e_p - p);
	return true;
}

void BodyStream::parseMultiPart(std::string& raw_data)
{
	while (true)
	{
		switch (state_)
		{
			case sb_start:
			{
				std::string::size_type p = raw_data.find(delimeter_);
				if (p == std::string::npos)
					return;
				state_ = sb_header;
				raw_data.erase(0, p + delimeter_.size());
				break;
			}
			case sb_header:
			{
				std::string::size_type p = raw_data.find(CRLF CRLF);
				if (p == std::string::npos)
				{
					if (raw_data.size() > 8192)
						state_ = sb_end; // error BadRequest

					return;
				}
				if (!parseHeaders_(raw_data.substr(0, p)))
				{
					state_ = sb_end; // error BadRequest
					return;
				}
				raw_data.erase(0, p + 4);
				state_ = sb_data;
				break;
			}
			case sb_data:
			{
				std::string::size_type p = raw_data.find(end_delimeter_);
				if (p != std::string::npos)
				{
					data_ = raw_data.substr(0, p - 2);
					state_ = sb_end;
					return;
				}
				data_ = raw_data;
				raw_data.clear();
				return;
			}
			case sb_end: return;
		}
	}
}

void BodyStream::parse(std::string& raw_data)
{
	if (!boundary_.empty())
		parseMultiPart(raw_data);
	// else (chunked)
	// parseChunked(std::string &raw_data)
	else
	{
		data_ = raw_data;
		raw_data.clear();
	}
}

bool BodyStream::findBoundary(size_t& pos, const std::string& delim)
{
	std::string::size_type p = buf_.find(delim);

	if (p != std::string::npos)
	{
		pos += 2;
		return true;
	}
	return false;
}

void BodyStream::setBoundary(const std::string& boundary)
{
	this->boundary_ = boundary;
	delimeter_ = "--" + boundary_ + CRLF;
	end_delimeter_ = "--" + boundary_ + "--";
}

bool BodyStream::eof() const
{
	return state_ == sb_end;
}

const std::string& BodyStream::getData() const
{
	return data_;
}

void BodyStream::reset()
{
	boundary_.clear();
	delimeter_.clear();
	end_delimeter_.clear();
	buf_.clear();
	name_.clear();
	filename_.clear();
	state_ = sb_start;
	data_.clear();
}

const std::string& BodyStream::getFileName() const
{
	return filename_;
}
