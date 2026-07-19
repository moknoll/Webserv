/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagomad <nmagomad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/01 13:20:17 by nmagomad          #+#    #+#             */
/*   Updated: 2026/05/01 13:20:18 by nmagomad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>

#define LOG_DEBUG(msg)    Logger::getInstance().log(LOG_DEBUG, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg)     Logger::getInstance().log(LOG_INFO, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg)  Logger::getInstance().log(LOG_WARNING, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg)    Logger::getInstance().log(LOG_ERROR, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_CRITICAL(msg) Logger::getInstance().log(LOG_ERROR, msg, __FILE__, __LINE__, __FUNCTION__)

enum LogLevel
{
	LOG_DEBUG = 0,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_CRITICAL
};

class Logger
{
  public:
	static Logger& getInstance();

	void           setLevel(LogLevel newLevel);

	void           log(LogLevel           level,
	                   const std::string& message,
	                   const char*        file,
	                   int                line,
	                   const char*        function);

	~Logger();

  private:
	LogLevel                 currentLevel;

	static const std::string _levelToString(LogLevel level);
	static const std::string _getTimestamp();

	Logger();
	Logger(const Logger& other);

	Logger& operator=(const Logger& other);
};

