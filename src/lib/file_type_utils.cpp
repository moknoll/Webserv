#include "ws.hpp"

#include <cstddef>
#include <string>
#include <sys/stat.h>

bool ws::is_file(const std::string& path)
{
	struct stat filestat;
	if (stat(path.c_str(), &filestat) == 0)
	{
		if (S_ISREG(filestat.st_mode))
			return true;
		return false;
	}

	return false;
}

bool ws::is_dir(const std::string& path)
{
	struct stat filestat;
	if (stat(path.c_str(), &filestat) == 0)
	{
		if (S_ISDIR(filestat.st_mode))
			return true;
		return false;
	}

	return false;
}

size_t ws::get_file_size(const std::string& path)
{
	size_t      file_size = 0;

	struct stat filestat;
	if (stat(path.c_str(), &filestat) == 0)
	{
		file_size = filestat.st_size;
	}

	return file_size;
}

const std::string getFileExtension(const std::string& path)
{
	size_t lastSlash = path.find_last_of('/');

	size_t lastDot = path.find_last_of('.');

	if (lastDot == std::string::npos
	    || (lastSlash != std::string::npos && lastDot < lastSlash))
		return "";

	std::string extension = path.substr(lastDot + 1);

	ws::toLowerCase(extension);
	return extension;
}

