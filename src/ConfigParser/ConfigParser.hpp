#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ServerConfig.hpp"
#include <string>

class ConfigParser {
	private:
		std::string _filename;
		void parseDirective(const std::string& line, ServerConfig& config);
		void requiredFieldsValidation(const ServerConfig& config, bool foundServer, int bracketCount);
	public:
		ConfigParser(const std::string& filename);
		ServerConfig parse();
		~ConfigParser();

};

#endif