#include "Cgi.hpp"
#include "../lib/ws.hpp"
#include <cstddef>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

CgiContext::CgiContext()
{
	pid = -1;
	stdin_pipe[0] = -1;
	stdin_pipe[1] = -1;
	stdout_pipe[0] = -1;
	stdout_pipe[1] = -1;
	exit_status = 0;
	deadline = 0;
	response = HttpResponse();
	envp = NULL;
	argv = NULL;
}

std::string CgiContext::buildPath(const std::string& uri,
                                  const Location&    loc) const
{
	std::string sub_uri = uri.substr(loc.path.length());
	std::string path = loc.root;

	if (!path.empty() && path[path.length() - 1] != '/')
		path += '/';

	if (!sub_uri.empty() && sub_uri[0] == '/')
		path += sub_uri.substr(1);
	else
	{
		path += sub_uri;
	}

	return path;
}

// void CgiContext::buildCgiEnv(const HttpRequest& req, const Location& loc) {}

// bool executeChild(CgiContext &ctx)


HttpResponse CgiContext::handle(const HttpRequest& req, const Location& loc)
{
	const std::string uri = req.getURI();
	const std::string path = buildPath(uri, loc);

	std::cout << uri << '\n';
	std::cout << path << '\n';

	pipe(stdin_pipe);
	pipe(stdout_pipe);

	pid_t pid = fork();

	if (pid == 0)
	{
		dup2(stdin_pipe[0], STDIN_FILENO);
		dup2(stdout_pipe[1], STDOUT_FILENO);
		close(stdin_pipe[1]); // закрываем ненужные концы
		close(stdout_pipe[0]);
		close(stdin_pipe[0]);
		close(stdout_pipe[1]);

		// char* argv[] = {const_cast< char* >(path.c_str()), NULL};
		std::string          sa = "/usr/bin/python3";
		std::string          sb = "./www/helloCGI.py";

		std::vector< char* > argv;
		argv.push_back(const_cast< char* >(sa.c_str()));
		argv.push_back(const_cast< char* >(sb.c_str()));
		argv.push_back(NULL);
		execve(loc.cgi_path.c_str(), argv.data(), envp);
		_exit(1); // если execve вернулся
	}

	close(stdin_pipe[0]);
	close(stdout_pipe[1]);

	// Запись тела запроса в stdin_pipe[1]
	write(stdin_pipe[1], req.getbody().c_str(), req.getbody().size());
	close(stdin_pipe[1]);

	// Чтение ответа
	std::string cgi_output;
	char        buf[4096];
	ssize_t     r;
	while ((r = read(stdout_pipe[0], buf, sizeof(buf))) > 0)
		cgi_output.append(buf, r);

	close(stdout_pipe[0]);

	int status;
	waitpid(pid, &status, 0);

	HttpResponse           res(200);
	std::string::size_type header_end = cgi_output.find("\r\n\r\n");
	size_t                 body_start = header_end + 4;

	res.setBody(cgi_output.substr(body_start));

	std::stringstream ss(cgi_output);
	std::string       line;
	while (std::getline(ss, line))
	{
		if (line == "\r")
			break;
		size_t p = line.find(':');
		std::cout << line << '\n';
		if (line.substr(0, p) == "Content-length")

			continue;
		res.setHeader(line.substr(0, p), line.substr(p + 1));
	}

	return res;
}
