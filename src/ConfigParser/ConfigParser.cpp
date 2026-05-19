// #include "ConfigParser.hpp"
// #include <fstream>
// #include <stdexcept>
// #include <iostream> 
// #include <sstream>
// #include <cstdlib>

// ConfigParser::ConfigParser(const std::string& filename) : _filename(filename) {}

// ConfigParser::~ConfigParser() {}


// // Helper function to trim whitespace from a string
// std::string trim(const std::string&  str)
// {
// 	size_t start = str.find_first_not_of(" \t\r\n");
// 	size_t end = str.find_last_not_of(" \t\r\n");
// 	if (start == std::string::npos)
// 		return "";
// 	return str.substr(start, end - start + 1);
// }

// // Parse a single directive and update config
// void ConfigParser::parseDirective(const std::string& line, ServerConfig& config)
// {
// 	// Tokenize the line into key and value
// 	std::istringstream iss(line);
// 	std::string key, value;
	
// 	if(!(iss >> key >> value))
// 		throw std::runtime_error("Invalid directive format: " + line);
// 	if (key == "listen")
// 	{
// 		// Check for duplicate listen directive
// 		if (config.port != 0)
// 			throw std::runtime_error("Duplicate directive: listen");
// 		config.port = atoi(value.c_str());
// 	}
// 	else if (key == "host")
// 		config.host = value;
// 	else if (key == "root")
// 		config.root = value;
// 	else if (key == "index")
// 		config.index = value;
// 	else if (key == "client_max_body_size")
// 		config.client_max_body_size = strtoul(value.c_str(), NULL, 10);
// 	else
// 		throw std::runtime_error("Unknown directive: " + key);
// }

// // Validate that all required fields are set
// void ConfigParser::requiredFieldsValidation(const ServerConfig& config, bool foundServer, int bracketCount)
// {
// 	if (!foundServer)
//     	throw std::runtime_error("No server block found");
// 	if (config.port <= 0 || config.port > 65535)
// 		throw std::runtime_error("Missing required directive: listen");
// 	if (config.host.empty())
// 		throw std::runtime_error("Missing required directive: host");
// 	if (config.root.empty())
// 		throw std::runtime_error("Missing required directive: root");
// 	if (config.index.empty()) 
// 		throw std::runtime_error("Missing required directive: index");
// 	if (bracketCount != 0)
// 		throw std::runtime_error("Unbalanced brackets in config file");
// }

// ServerConfig ConfigParser::parse()
// {
// 	ServerConfig config;
// 	std::ifstream file(_filename.c_str());
// 	std::string line;
// 	bool insideServerBlock = false;
// 	int bracketCount = 0;
// 	bool foundServer = false;

// 	// Step 1 - Open the config file
// 	if (!file.is_open())
// 		throw std::runtime_error("Could not open config file: " + _filename);

// 	// Step 2 - Read the file line by line
// 	while (std::getline(file, line))
// 	{
// 		// Step 3 - Trim whitespace and ignore comments
// 		line = trim(line);
// 		if (line.empty() || line[0] == '#')
// 			continue; // Skip empty lines and comments
		
// 		// Step 4 - Handle server block
// 		if(!insideServerBlock)
// 		{
// 			if (line == "server {")
// 			{
// 				insideServerBlock = true;
// 				foundServer = true;
// 				bracketCount = 1; // We found the opening bracket
// 				continue;
// 			}
// 			else 
// 			{
// 				throw std::runtime_error("Only server block supported");
// 			}
// 		}
// 		// Step 5 - Parse directives inside server block
// 		if (insideServerBlock)
// 		{
// 			// Case A - Close bracket
// 			if (line == "}")
// 			{
// 				bracketCount--;
// 				if (bracketCount == 0)
// 					insideServerBlock = false; // End of server block
// 				continue;
// 			}
// 			// Case B - Opening bracket (nested block, not supported in this simple parser)

// 			// Case C - Directive (tokenize and store in config)
// 			if (line[line.length() - 1] != ';' || line.empty())
// 			{
// 				throw std::runtime_error("Directive must end with ';': " + line);
// 			}
// 			line = line.substr(0, line.length() - 1); // Remove the trailing ';'
// 			parseDirective(line, config);
// 		}
// 	}
// 	// Step 6 - Final validation
// 	requiredFieldsValidation(config, foundServer, bracketCount);
// 	return config;
// }

#include "ConfigParser.hpp"
#include <stdexcept>
#include <cstdlib>
#include <cctype>

// ---------------- OCF ----------------

ConfigParser::ConfigParser() : tokenizer() {}

ConfigParser::ConfigParser(const std::string &filename)
    : tokenizer(filename) {}

ConfigParser::ConfigParser(const ConfigParser &other)
    : tokenizer(other.tokenizer), servers(other.servers) {}

ConfigParser &ConfigParser::operator=(const ConfigParser &other)
{
    if (this != &other)
    {
        tokenizer = other.tokenizer;
        servers = other.servers;
    }
    return *this;
}

ConfigParser::~ConfigParser() {}


// ---------------- Utility ----------------

void ConfigParser::expect(const std::string &tok)
{
    std::string t = tokenizer.next();
    if (t != tok)
        throw std::runtime_error("expected: " + tok + ", got: " + t);
}

void ConfigParser::validateNumber(const std::string &s)
{
    for (size_t i = 0; i < s.size(); i++)
        if (!isdigit(s[i]))
            throw std::runtime_error("expected number: " + s);
}

void ConfigParser::validateIPv4(const std::string &s)
{
    int dots = 0;
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] == '.') dots++;

    if (dots != 3)
        throw std::runtime_error("wrong IPv4: " + s);

    size_t start = 0;
    for (int i = 0; i < 4; i++)
    {
        size_t end = s.find('.', start);
        if (end == std::string::npos) end = s.size();

        std::string part = s.substr(start, end - start);
        validateNumber(part);

        int num = std::atoi(part.c_str());
        if (num < 0 || num > 255)
            throw std::runtime_error("wrong IPv4 segment: " + part);

        start = end + 1;
    }
}

size_t ConfigParser::parseSize(const std::string &s)
{
    if (s.empty())
        throw std::runtime_error("client_max_body_size: empty value");

    // Якщо останній символ — літера (суфікс)
    char last = s[s.size() - 1];

    size_t multiplier = 1;
    std::string number = s;

    if (last == 'K' || last == 'M' || last == 'G')
    {
        number = s.substr(0, s.size() - 1); // відкидаємо суфікс

        if (last == 'K') multiplier = 1024;
        else if (last == 'M') multiplier = 1024 * 1024;
        else if (last == 'G') multiplier = 1024 * 1024 * 1024;
    }

    // Перевіряємо, що number — це чисте число
    for (size_t i = 0; i < number.size(); i++)
        if (!isdigit(number[i]))
            throw std::runtime_error("client_max_body_size: invalid number: " + s);

    size_t base = static_cast<size_t>(std::atoi(number.c_str()));
    return base * multiplier;
}


void ConfigParser::validateMethod(const std::string &m)
{
    if (m != "GET" && m != "POST" && m != "DELETE")
        throw std::runtime_error("wrong HTTP method: " + m);
}

void ConfigParser::validateAutoindex(const std::string &s)
{
    if (s != "on" && s != "off")
        throw std::runtime_error("autoindex must be 'on/off', recieved: " + s);
}

void ConfigParser::validateRedirectCode(int code)
{
    if (code != 301 && code != 302 && code != 307 && code != 308)
        throw std::runtime_error("wrong redirect code");
}

void ConfigParser::validateErrorCode(int code)
{
    if (code < 300 || code > 599)
        throw std::runtime_error("Wrong error_page код");
}

void ConfigParser::validatePath(const std::string &p)
{
    if (p.empty())
        throw std::runtime_error("Path has to exist");

    if (p[0] != '/' && p[0] != '.')
        throw std::runtime_error("path has to start from '/','.'");
}

void ConfigParser::validateExtension(const std::string &ext)
{
    if (ext.empty() || ext[0] != '.')
        throw std::runtime_error("Wrong CGI: " + ext);
}


// ---------------- Main parse() ----------------

void ConfigParser::parse()
{
    while (tokenizer.hasMore())
    {
        std::string tok = tokenizer.next();

        if (tok == "server")
            parseServer();
        else
            throw std::runtime_error("unknown token: " + tok);
    }
}


// ---------------- Parse server block ----------------

void ConfigParser::parseServer()
{
    ServerConfig server;
    expect("{");

    while (tokenizer.hasMore())
    {
        std::string tok = tokenizer.next();

        if (tok == "}")
            break;

        if (tok == "location")
        {
            parseLocation(server);
        }
        else if (tok == "server_name")
        {
            std::string h = tokenizer.next();
            //validateIPv4(h);
            server.host = h;
            expect(";");
        }
        else if (tok == "listen")
        {
            std::string p = tokenizer.next();
            validateNumber(p);
            int port = std::atoi(p.c_str());
            if (port < 1 || port > 65535)
                throw std::runtime_error("Wrong port: " + p);
            server.port = port;
            expect(";");
        }
        else if (tok == "root")
        {
            std::string r = tokenizer.next();
            validatePath(r);
            server.root = r;
            expect(";");
        }
        else if (tok == "index")
        {
            std::string idx = tokenizer.next();
            server.index = idx;
            expect(";");
        }
        else if (tok == "client_max_body_size")
        {
            std::string s = tokenizer.next();

            server.client_max_body_size = parseSize(s.c_str());
            expect(";");
        }
        else if (tok == "error_page")
        {
            std::string codeStr = tokenizer.next();
            validateNumber(codeStr);
            int code = std::atoi(codeStr.c_str());
            validateErrorCode(code);

            std::string path = tokenizer.next();
            validatePath(path);

            server.error_pages[code] = path;
            expect(";");
        }
        else if (tok == "redirect" || tok == "return")
        {
            std::string codeStr = tokenizer.next();
            validateNumber(codeStr);
            int code = std::atoi(codeStr.c_str());
            validateRedirectCode(code);
        
            std::string path = tokenizer.next();
            validatePath(path);
        
            server.redirect = std::make_pair(code, path);
            expect(";");
        }
        else
        {
            throw std::runtime_error("unknown directive in server: " + tok);
        }
    }

    servers.push_back(server);
}


// ---------------- Parse location block ----------------

void ConfigParser::parseLocation(ServerConfig &server)
{
    Location loc;

    loc.path = tokenizer.next();
    validatePath(loc.path);

    expect("{");

    while (tokenizer.hasMore())
    {
        std::string tok = tokenizer.next();

        if (tok == "}")
            break;

        if (tok == "root")
        {
            std::string r = tokenizer.next();
            validatePath(r);
            loc.root = r;
            expect(";");
        }
        else if (tok == "index")
        {
            loc.index = tokenizer.next();
            expect(";");
        }
        else if (tok == "autoindex")
        {
            std::string val = tokenizer.next();
            validateAutoindex(val);
            loc.autoindex = (val == "on");
            expect(";");
        }
        else if (tok == "client_max_body_size")
        {
            std::string s = tokenizer.next();
            validateNumber(s);
            loc.client_max_body_size = std::atoi(s.c_str());
            expect(";");
        }
        else if (tok == "allowed_methods")
        {
            while (tokenizer.peek() != ";")
            {
                std::string m = tokenizer.next();
                validateMethod(m);
                loc.allowed_methods.push_back(m);
            }
            expect(";");
        }
        else if (tok == "error_page")
        {
            std::string codeStr = tokenizer.next();
            validateNumber(codeStr);
            int code = std::atoi(codeStr.c_str());
            validateErrorCode(code);

            std::string path = tokenizer.next();
            validatePath(path);

            loc.error_pages[code] = path;
            expect(";");
        }
        else if (tok == "redirect" || tok == "return")
        {
            std::string codeStr = tokenizer.next();
            validateNumber(codeStr);
            int code = std::atoi(codeStr.c_str());
            validateRedirectCode(code);
        
            std::string path = tokenizer.next();
            validatePath(path);
        
            server.redirect = std::make_pair(code, path);
            expect(";");
        }
        else if (tok == "upload_path")
        {
            std::string p = tokenizer.next();
            validatePath(p);
            loc.upload_path = p;
            expect(";");
        }
        else if (tok == "cgi_extension")
        {
            std::string ext = tokenizer.next();
            validateExtension(ext);
            loc.cgi_extension = ext;
            loc.has_cgi = true;
            expect(";");
        }
        else if (tok == "cgi_path")
        {
            std::string p = tokenizer.next();
            validatePath(p);
            loc.cgi_path = p;
            loc.has_cgi = true;
            expect(";");
        }
        else
        {
            throw std::runtime_error("unknown directive in location: " + tok);
        }
    }

    server.locations.push_back(loc);
}


// ---------------- Accessor ----------------

const std::vector<ServerConfig> &ConfigParser::getServers() const
{
    return servers;
}
