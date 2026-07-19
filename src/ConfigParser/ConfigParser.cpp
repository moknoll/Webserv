#include "ConfigParser.hpp"
#include <cctype>
#include <cstdlib>
#include <stdexcept>

// ---------------- OCF ----------------

ConfigParser::ConfigParser() :
        tokenizer()
{
}

ConfigParser::ConfigParser(const std::string& filename) :
        tokenizer(filename)
{
}

ConfigParser::ConfigParser(const ConfigParser& other) :
        tokenizer(other.tokenizer),
        servers(other.servers)
{
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
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

void ConfigParser::expect(const std::string& tok)
{
	std::string t = tokenizer.next();
	if (t != tok)
		throw std::runtime_error("expected: " + tok + ", got: " + t);
}

void ConfigParser::validateNumber(const std::string& s)
{
	for (size_t i = 0; i < s.size(); i++)
		if (!isdigit(s[i]))
			throw std::runtime_error("expected number: " + s);
}

void ConfigParser::validateIPv4(const std::string& s)
{
	int dots = 0;
	for (size_t i = 0; i < s.size(); i++)
		if (s[i] == '.')
			dots++;

	if (dots != 3)
		throw std::runtime_error("wrong IPv4: " + s);

	size_t start = 0;
	for (int i = 0; i < 4; i++)
	{
		size_t end = s.find('.', start);
		if (end == std::string::npos)
			end = s.size();

		std::string part = s.substr(start, end - start);
		validateNumber(part);

		int num = std::atoi(part.c_str());
		if (num < 0 || num > 255)
			throw std::runtime_error("wrong IPv4 segment: " + part);

		start = end + 1;
	}
}

void ConfigParser::validateInterfacePort(std::string& s)
{
	std::string::size_type p = s.find(':');
	if (p == std::string::npos)
	{
		validateNumber(s);
	}
	else
	{
		std::string host = s.substr(0, p);
		std::string port = s.substr(p + 1);
		validateIPv4(host);
		validateNumber(port);
	}
}

size_t ConfigParser::parseSize(const std::string& s)
{
	if (s.empty())
		throw std::runtime_error("client_max_body_size: empty value");

	char        last = s[s.size() - 1];

	size_t      multiplier = 1;
	std::string number = s;

	if (last == 'K' || last == 'M' || last == 'G')
	{
		number = s.substr(0, s.size() - 1);

		if (last == 'K')
			multiplier = 1024;
		else if (last == 'M')
			multiplier = 1024 * 1024;
		else if (last == 'G')
			multiplier = 1024 * 1024 * 1024;
	}

	for (size_t i = 0; i < number.size(); i++)
		if (!isdigit(number[i]))
			throw std::runtime_error("client_max_body_size: invalid number: "
			                         + s);

	size_t base = static_cast< size_t >(std::atoi(number.c_str()));
	return base * multiplier;
}

void ConfigParser::validateMethod(const std::string& m)
{
	if (m != "GET" && m != "POST" && m != "DELETE" && m != "HEAD" && m != "PUT" && m != "CONNECT"
	    && m != "OPTIONS" && m != "TRACE" && m != "PATCH"
	    && m != "MOVE")
		throw std::runtime_error("wrong HTTP method: " + m);
}

void ConfigParser::validateAutoindex(const std::string& s)
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

void ConfigParser::validatePath(const std::string& p)
{
	if (p.empty())
		throw std::runtime_error("Path has to exist");

	if (p[0] != '/' && p[0] != '.')
		throw std::runtime_error("path has to start from '/','.': " + p);
}

void ConfigParser::validateRedirectPath(const std::string& p)
{
	// Empty path is always invalid
	if (p.empty())
		throw std::runtime_error("Path cannot be empty");
	if (p[0] != '/' && p[0] != '.' && p.rfind("http://", 0) == 0
	    && p.rfind("https://", 0) == 0)
		throw std::runtime_error("path has to start from '/','.'");
}

void ConfigParser::validateExtension(const std::string& ext)
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
			server.server_name = h;
			expect(";");
		}
		else if (tok == "listen")
		{
			std::string p = tokenizer.next();
			validateInterfacePort(p);
			std::string::size_type pos = p.find(':');
			if (pos != std::string::npos)
			{
				server.host = p.substr(0, pos);
				server.port = std::atoi(p.substr(pos + 1).c_str());
			}
			else
				server.port = std::atoi(p.c_str());
			if (server.port < 1 || server.port > 65535)
				throw std::runtime_error("Wrong port: " + p);
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
			validateRedirectPath(path);

			server.redirect = std::make_pair(code, path);
			expect(";");
		}
		else
		{
			throw std::runtime_error("unknown directive in server: " + tok);
		}
	}

	if (!validateRootLocation(server))
		throw std::runtime_error(
		    "No root path location. Check and fix your .conf file");

	servers.push_back(server);

	// debugPrint();
}

// ---------------- Parse location block ----------------

void ConfigParser::parseLocation(ServerConfig& server)
{
	Location loc(server);

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
			loc.client_max_body_size = parseSize(s.c_str());

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
			validateRedirectPath(path);

			loc.redirect = std::make_pair(code, path);
			loc.has_redirect = true;
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
	
	if (server.redirect.first != -1) {
		loc.has_redirect = true;
		loc.redirect = server.redirect;
	}

	server.locations.push_back(loc);
}

// ---------------- Accessor ----------------

const std::vector< ServerConfig >& ConfigParser::getServers() const
{
	return servers;
}

bool ConfigParser::validateRootLocation(ServerConfig& server)
{
	for (size_t i = 0; i < server.locations.size(); i++)
	{
		if (server.locations[i].path == "/")
			return true;
	}
	return false;
}

////////////// debug func for visual only. delete after use /////////////////////

// void ConfigParser::debugPrint() const
// {
//     std::cout << "==================== PARSED CONFIG ====================\n";

//     for (size_t i = 0; i < servers.size(); i++)
//     {
//         const ServerConfig& s = servers[i];

//         std::cout << "\n-------------------- SERVER " << i << " --------------------\n";
//         std::cout << "server_name: " << s.server_name << "\n";
//         std::cout << "host:        " << s.host << "\n";
//         std::cout << "port:        " << s.port << "\n";
//         std::cout << "root:        " << s.root << "\n";
//         std::cout << "index:       " << s.index << "\n";
//         std::cout << "client_max_body_size: " << s.client_max_body_size << "\n";

//         if (!s.redirect.first)
//             std::cout << "redirect:    none\n";
//         else
//             std::cout << "redirect:    " << s.redirect.first
//                       << " -> " << s.redirect.second << "\n";

//         // Error pages
//         std::cout << "error_pages:\n";
//         for (std::map<int, std::string>::const_iterator it = s.error_pages.begin();
//              it != s.error_pages.end(); ++it)
//         {
//             std::cout << "  " << it->first << " => " << it->second << "\n";
//         }

//         // Locations
//         std::cout << "\n  ---- LOCATIONS ----\n";
//         for (size_t j = 0; j < s.locations.size(); j++)
//         {
//             const Location& loc = s.locations[j];

//             std::cout << "\n  Location " << j << ":\n";
//             std::cout << "    path:        " << loc.path << "\n";
//             std::cout << "    root:        " << loc.root << "\n";
//             std::cout << "    index:       " << loc.index << "\n";
//             std::cout << "    autoindex:   " << (loc.autoindex ? "on" : "off") << "\n";
//             std::cout << "    client_max_body_size: " << loc.client_max_body_size << "\n";

//             // Allowed methods
//             std::cout << "    allowed_methods: ";
//             if (loc.allowed_methods.empty())
//                 std::cout << "(none)";
//             else
//             {
//                 for (size_t k = 0; k < loc.allowed_methods.size(); k++)
//                     std::cout << loc.allowed_methods[k] << " ";
//             }
//             std::cout << "\n";

//             // Error pages
//             std::cout << "    error_pages:\n";
//             for (std::map<int, std::string>::const_iterator it = loc.error_pages.begin();
//                  it != loc.error_pages.end(); ++it)
//             {
//                 std::cout << "      " << it->first << " => " << it->second << "\n";
//             }

//             // Redirect
        
//             std::cout << "    redirect: " << loc.redirect.first
//                       << " -> " << loc.redirect.second << "\n";

//             // Upload path
//             std::cout << "    upload_path: ";
//             if (loc.upload_path.empty())
//                 std::cout << "(none)\n";
//             else
//                 std::cout << loc.upload_path << "\n";

//             // CGI
//             std::cout << "    CGI:\n";
//             std::cout << "      has_cgi: " << (loc.has_cgi ? "yes" : "no") << "\n";
//             std::cout << "      cgi_extension: " << loc.cgi_extension << "\n";
//             std::cout << "      cgi_path:      " << loc.cgi_path << "\n";
//         }
//     }

//     std::cout << "\n================== END PARSED CONFIG ==================\n";
// }
