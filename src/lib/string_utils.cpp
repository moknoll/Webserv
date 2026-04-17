#include "ws.hpp"

std::string ws::strip(const std::string& s)
{
	const char* whilespaces = " \t\n\r\f\v";
	size_t      begin = s.find_first_not_of(whilespaces);

	if (begin == std::string::npos)
		return "";

	size_t end = s.find_last_not_of(whilespaces);

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
