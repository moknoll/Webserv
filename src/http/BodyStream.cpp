#include "BodyStream.hpp"
#include "../lib/ws.hpp"
#include "constants.hpp"
#include <iostream>
#include <string>

BodyStream::BodyStream() : state_(sw_start) {}

// private:
// std::string boundary;
// std::string delimeter_;
// std::string end_delimeter_;
// std::string name;
// std::string filename;
// State       state_;
// std::string data;

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

void setBoundary(const std::string& boundary);

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
			case sw_start:
			{
				std::cout << "IN PARS BODY\n";
				std::string::size_type p = raw_data.find(delimeter_);
				if (p == std::string::npos)
					return;
				state_ = sw_header;
				raw_data.erase(0, p + delimeter_.size());
				break;
			}
			case sw_header:
			{
				std::string::size_type p = raw_data.find(CRLF CRLF);
				if (p == std::string::npos)
				{
					if (raw_data.size() > 8192)
						state_ = sw_end; // error BadRequest

					return;
				}
				if (!parseHeaders_(raw_data.substr(0, p)))
				{
					state_ = sw_end; // error BadRequest
					return;
				}
				raw_data.erase(0, p + 4);
				state_ = sw_data;
				break;
			}
			case sw_data:
			{
				std::string::size_type p = raw_data.find(end_delimeter_);
				if (p != std::string::npos)
				{
					data_ = raw_data.substr(0, p - 2);
					state_ = sw_end;
					return;
				}
				data_ = raw_data;
				raw_data.clear();
				return;
			}
			case sw_end: return;
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
	if (state_ == sw_end)
		return true;
	return false;
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
	state_ = sw_start;
	data_.clear();
}

const std::string& BodyStream::getFileName() const
{
	return filename_;
}
