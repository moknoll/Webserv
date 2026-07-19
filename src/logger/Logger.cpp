/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:20:13 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:20:14 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

Logger::~Logger() {}

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

Logger::Logger() :
        currentLevel(LOG_INFO)
{
}

void Logger::setLevel(LogLevel newLevel)
{
	currentLevel = newLevel;
}

void Logger::log(LogLevel           level,
                 const std::string& message,
                 const char*        file,
                 int                line,
                 const char*        function)
{
	if (level < this->currentLevel)
		return;

	std::string        timestamp = _getTimestamp();
	std::string        strLevel = _levelToString(level);

	std::ostringstream oss;
	oss << "[" << timestamp << "]  [" << strLevel << "]";

	if (level == LOG_DEBUG || level == LOG_ERROR)
	{
		oss << " [" << file << ":" << line << ":" << function << "]";
	}

	oss << " " << message;

	std::cout << oss.str() << std::endl;
}

const std::string Logger::_levelToString(LogLevel level)
{
	switch (level)
	{
		case LOG_DEBUG:    return "DEBUG";
		case LOG_INFO:     return "INFO";
		case LOG_WARNING:  return "WARNING";
		case LOG_ERROR:    return "ERROR";
		case LOG_CRITICAL: return "CRITICAL";
		default:           return "UNKNOWN";
	}
}

const std::string Logger::_getTimestamp()
{
	char        buf[50];
	const char* fmt = "%Y-%m-%d %H:%M:%S";

	std::time_t now = std::time(0);
	if (now == -1)
		return "";

	tm* timeinfo = std::localtime(&now);

	if (!timeinfo || std::strftime(buf, sizeof(buf), fmt, timeinfo) == 0)
		return "";

	return std::string(buf);
}

