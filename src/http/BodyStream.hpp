/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BodyStream.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/26 19:25:18 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/26 19:25:21 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <cstddef>
#include <string>

class BodyStream
{
  public:
	enum State
	{
		sb_start = 0,
		sb_header,
		sb_data,
		sb_end
	};

	BodyStream();
	// BodyStream(const BodyStream& other);
	~BodyStream();

	// BodyStream& operator=(const BodyStream& other);

	void               setBoundary(const std::string& boundary);
	void               setChunked(bool chunked);
	void               parse(std::string& raw_data);
	// bool               findBoundary(size_t& pos, const std::string& delim);
	bool               eof() const;
	const std::string& getFileName() const;
	const std::string& getData() const;

	void               printBodyStream() const;
	void               reset();

  private:
	std::string boundary_;
	std::string delimeter_;
	std::string end_delimeter_;
	// std::string buf_;
	std::string name_;
	std::string filename_;
	bool        is_chunked;

	State       state_;
	std::string data_;

	bool        parseHeaders_(const std::string& headers);
	void        parseMultiPart(std::string& raw_data);
	void        parseChunked(std::string& raw_data);
};

