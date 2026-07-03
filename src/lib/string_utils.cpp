/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   string_utils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:20:01 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:20:02 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ws.hpp"
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

bool ws::has_suffix(const std::string& s, const std::string& suffix)
{
	return s.size() > suffix.size()
	    && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string ws::trim(const std::string& s, char c)
{
	std::string::size_type begin = s.find_first_not_of(c);
	if (begin == std::string::npos)
		return "";

	std::string::size_type end = s.find_last_not_of(c);
	if (end == std::string::npos)
		return "";

	return s.substr(begin, end - begin + 1);
}

std::string ws::strip(const std::string& s)
{
	const char* whitespaces = " \t\n\r\f\v";
	size_t      begin = s.find_first_not_of(whitespaces);

	if (begin == std::string::npos)
		return "";

	size_t end = s.find_last_not_of(whitespaces);

	if (end == std::string::npos)
		return "";

	return s.substr(begin, end - begin + 1);
}

std::vector< std::string > ws::strSplit(const std::string& s,
                                        const std::string& delim)
{
	std::vector< std::string > tokens;
	size_t                     start = 0;
	size_t                     end = s.find(delim);

	while (end != std::string::npos)
	{
		tokens.push_back(s.substr(start, end - start));
		start = end + 1;
		end = s.find(delim, start);
	}
	tokens.push_back(s.substr(start));

	return tokens;
}

size_t ws::stosize(const std::string& s)
{
	size_t             n = 0;
	std::istringstream ss(s);
	ss >> n;
	return n;
}

std::string ws::toLowerCase(const std::string& s)
{
	std::string out;

	for (size_t i = 0; i < s.size(); ++i)
	{
		unsigned char c = static_cast< unsigned char >(s[i]);
		out += std::tolower(c);
	}
	return out;
}

std::string ws::toUpperCase(const std::string& s)
{
	std::string out;

	for (size_t i = 0; i < s.size(); ++i)
	{
		unsigned char c = static_cast< unsigned char >(s[i]);
		out += std::toupper(c);
	}
	return out;
}

std::string ws::randString(size_t len)
{
	std::srand(std::time(0));

	const char  char_set[] = "abcdefghijklmnopqrstuvwxyz"
	                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	const int   char_set_size = sizeof(char_set) - 1;

	std::string rand_string;

	for (size_t i = 0; i < len; ++i)
	{
		size_t rand_index = std::rand() % char_set_size;
		rand_string += char_set[rand_index];
	}
	return rand_string;
}
