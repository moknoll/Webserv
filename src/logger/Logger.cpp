#include "Logger.hpp"
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
	if (level < this->currentLevel) return;

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
	time_t now = time(0);
	tm*    timeinfo = localtime(&now);
	char   timestamp[20];
	std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
	return std::string(timestamp);
}

