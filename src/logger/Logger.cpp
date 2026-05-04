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
#include <cstddef>
#include <ctime>
#include <iostream>
#include <string>

Logger::~Logger() {}

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

Logger::Logger() : currentLevel(INFO) {}

void Logger::setLevel(LogLevel newLevel)
{
	currentLevel = newLevel;
}

void Logger::log(LogLevel           level,
                 const std::string& message,
                 const std::string& context)
{
	if (level < this->currentLevel)
		return;

	std::string timestamp = _getTimestamp();
	std::string strLevel = _levelToString(level);

	if (context == "")
	{
		std::cout << "[ " << strLevel << " ] - [" << timestamp << "] - "
		          << message << std::endl;
	}
	else
	{
		// std::string st = "\"" + getmethod() + " " + geturi() + " "
		//                + gethttpversion() + "\"";
		//
		// std::cout << "[ " << strLevel << " ] - [" << timestamp << "] - ["
		//           << gethost() << "] - " << st << " - " << message
		//           << std::endl;
	}
}

const std::string Logger::_levelToString(LogLevel level)
{
	switch (level)
	{
		case DEBUG:    return "DEBUG";
		case INFO:     return "INFO";
		case WARNING:  return "WARNING";
		case ERROR:    return "ERROR";
		case CRITICAL: return "CRITICAL";
		default:       return "UNKNOWN";
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

