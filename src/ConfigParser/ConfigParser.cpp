#include "ConfigParser.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream> 
#include <sstream>
#include <cstdlib>

ConfigParser::ConfigParser(const std::string& filename) : _filename(filename) {}

ConfigParser::~ConfigParser() {}


// Helper function to trim whitespace from a string
std::string trim(const std::string&  str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	size_t end = str.find_last_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	return str.substr(start, end - start + 1);
}

// Parse a single directive and update config
void ConfigParser::parseDirective(const std::string& line, ServerConfig& config)
{
	// Tokenize the line into key and value
	std::istringstream iss(line);
	std::string key, value;
	
	if(!(iss >> key >> value))
		throw std::runtime_error("Invalid directive format: " + line);
	if (key == "listen")
	{
		// Check for duplicate listen directive
		if (config.port != 0)
			throw std::runtime_error("Duplicate directive: listen");
		config.port = atoi(value.c_str());
	}
	else if (key == "host")
		config.host = value;
	else if (key == "root")
		config.root = value;
	else if (key == "index")
		config.index = value;
	else if (key == "client_max_body_size")
		config.client_max_body_size = strtoul(value.c_str(), NULL, 10);
	else
		throw std::runtime_error("Unknown directive: " + key);
}

// Validate that all required fields are set
void ConfigParser::requiredFieldsValidation(const ServerConfig& config, bool foundServer, int bracketCount)
{
	if (!foundServer)
    	throw std::runtime_error("No server block found");
	if (config.port <= 0 || config.port > 65535)
		throw std::runtime_error("Missing required directive: listen");
	if (config.host.empty())
		throw std::runtime_error("Missing required directive: host");
	if (config.root.empty())
		throw std::runtime_error("Missing required directive: root");
	if (config.index.empty()) 
		throw std::runtime_error("Missing required directive: index");
	if (bracketCount != 0)
		throw std::runtime_error("Unbalanced brackets in config file");
}

ServerConfig ConfigParser::parse()
{
	ServerConfig config;
	std::ifstream file(_filename.c_str());
	std::string line;
	bool insideServerBlock = false;
	int bracketCount = 0;
	bool foundServer = false;

	// Step 1 - Open the config file
	if (!file.is_open())
		throw std::runtime_error("Could not open config file: " + _filename);

	// Step 2 - Read the file line by line
	while (std::getline(file, line))
	{
		// Step 3 - Trim whitespace and ignore comments
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue; // Skip empty lines and comments
		
		// Step 4 - Handle server block
		if(!insideServerBlock)
		{
			if (line == "server {")
			{
				insideServerBlock = true;
				foundServer = true;
				bracketCount = 1; // We found the opening bracket
				continue;
			}
			else 
			{
				throw std::runtime_error("Only server block supported");
			}
		}
		// Step 5 - Parse directives inside server block
		if (insideServerBlock)
		{
			// Case A - Close bracket
			if (line == "}")
			{
				bracketCount--;
				if (bracketCount == 0)
					insideServerBlock = false; // End of server block
				continue;
			}
			// Case B - Opening bracket (nested block, not supported in this simple parser)

			// Case C - Directive (tokenize and store in config)
			if (line[line.length() - 1] != ';' || line.empty())
			{
				throw std::runtime_error("Directive must end with ';': " + line);
			}
			line = line.substr(0, line.length() - 1); // Remove the trailing ';'
			parseDirective(line, config);
		}
	}
	// Step 6 - Final validation
	requiredFieldsValidation(config, foundServer, bracketCount);
	return config;
}