#pragma once

#include <string>
#include <vector>

class ws
{
  public:
	/**
	 * Splits a string into a vector of substrings based on a specific
	 * delimiter.
	 * * @param s The input string to be split.
	 * @param delim The delimiter string used to identify split points.
	 * @return A vector of strings containing the extracted tokens.
	 */
	static std::vector< std::string > strSplit(const std::string& s,
	                                           const std::string& delim);

	/**
	 * Removes leading and trailing whitespace characters from a string.
	 * * @param s The input string to be trimmed.
	 * @return A new string with all leading and trailing whitespaces removed.
	 */
	static std::string                strip(const std::string& s);
};
