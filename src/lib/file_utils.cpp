/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_type_utils.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:18:57 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:19:02 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../http/constants.hpp"
#include "ws.hpp"

#include <cerrno>
#include <cstddef>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

bool ws::isFile(const std::string& path)
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

bool ws::isDirectory(const std::string& path)
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

PathInfo ws::checkPath(const std::string& path)
{
	PathInfo info;

	info.exists = false;
	info.readable = false;
	info.writable = false;
	info.executable = false;
	info.type = PATH_NOT_EXISTS;

	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return info;

	info.exists = true;
	info.readable = (access(path.c_str(), R_OK) == 0);
	info.writable = (access(path.c_str(), W_OK) == 0);
	info.executable = (access(path.c_str(), X_OK) == 0);

	if (S_ISDIR(st.st_mode))
		info.type = PATH_IS_DIR;
	else if (S_ISREG(st.st_mode))
		info.type = PATH_IS_FILE;

	return info;
}

size_t ws::getFileSize(const std::string& path)
{
	size_t      file_size = 0;

	struct stat filestat;
	if (stat(path.c_str(), &filestat) == 0)
	{
		file_size = filestat.st_size;
	}

	return file_size;
}

const std::string ws::getFileExtension(const std::string& path)
{
	size_t lastSlash = path.find_last_of('/');

	size_t lastDot = path.find_last_of('.');

	if (lastDot == std::string::npos
	    || (lastSlash != std::string::npos && lastDot < lastSlash))
		return "";

	std::string extension = path.substr(lastDot + 1);

	return ws::toLowerCase(extension);
}

bool ws::readFile(const char* path, std::string& file_content)
{
	std::ifstream file(path);

	if (!file.is_open())
		return false;

	file_content.assign((std::istreambuf_iterator< char >(file)),
	                    std::istreambuf_iterator< char >());
	return true;
}

int ws::checkFile(const char* path)
{
	int fd = open(path, O_RDONLY);
	if (fd != -1)
	{
		close(fd);
		return FILE_OK;
	}

	switch (errno)
	{
		case ENOENT: return ERR_NOT_FOUND;
		case EACCES: return ERR_PERMISSION;
		case EISDIR: return ERR_IS_DIR;
		default:     return ERR_UNKNOWN;
	}
}

const std::string ws::getFileModificationTime(const std::string& path)
{
	struct stat f;
	char        buf[100];
	const char* fmt = "%d-%b-%Y %H:%M";

	if (stat(path.c_str(), &f) == 0)
	{
		time_t     time = static_cast< time_t >(f.st_mtim.tv_sec);
		struct tm* timeinfo;
		timeinfo = std::localtime(&time);
		if (!timeinfo || std::strftime(buf, sizeof(buf), fmt, timeinfo) == 0)
			return "";
	}
	return buf;
}

