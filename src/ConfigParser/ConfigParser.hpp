#pragma once

#include "ServerConfig.hpp"
#include "Tokenizer.hpp"
#include <string>
#include <vector>

class ConfigParser
{
  private:
	Tokenizer                   tokenizer;
	std::vector< ServerConfig > servers;

	void                        expect(const std::string& tok);
	void                        validateNumber(const std::string& s);
	void                        validateIPv4(const std::string& s);
	void                        validateMethod(const std::string& m);
	void                        validateAutoindex(const std::string& s);
	void                        validateRedirectCode(int code);
	void                        validateErrorCode(int code);
	void                        validatePath(const std::string& p);
	void                        validateExtension(const std::string& ext);
	bool                        validateRootLocation(ServerConfig& server);
	void                        validateInterfacePort(std::string& s);
	size_t                      parseSize(const std::string& s);

	void                        parseServer();
	void                        parseLocation(ServerConfig& server);

  public:
	ConfigParser();
	ConfigParser(const std::string& filename);
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);
	~ConfigParser();

	void                               parse();
	const std::vector< ServerConfig >& getServers() const;
};
