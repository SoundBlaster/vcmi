/*
 * DiagnosticCommandChannel.h, part of VCMI engine
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#if defined(__unix__) || defined(__APPLE__)
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

class DiagnosticChannel
{
	int listener = -1;
	int client = -1;
	std::string path;
	std::string input;
	std::chrono::steady_clock::time_point connected;
	dev_t device = 0;
	ino_t inode = 0;

	static bool nonblocking(int fd)
	{
		const int flags = fcntl(fd, F_GETFL);
		const int fdFlags = fcntl(fd, F_GETFD);
		return flags >= 0 && fdFlags >= 0 && fcntl(fd, F_SETFD, fdFlags | FD_CLOEXEC) == 0
			&& fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
	}

	void closeClient()
	{
		if(client >= 0)
			close(client);
		client = -1;
		input.clear();
	}

	void respond(const char * message)
	{
#if defined(SO_NOSIGPIPE)
		int noSigpipe = 1;
		setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(noSigpipe));
		send(client, message, strlen(message), 0);
#else
		send(client, message, strlen(message), MSG_NOSIGNAL);
#endif
		closeClient();
	}

public:
	DiagnosticChannel(const DiagnosticChannel &) = delete;
	DiagnosticChannel & operator=(const DiagnosticChannel &) = delete;

	DiagnosticChannel()
	{
		const char * configured = std::getenv("VCMI_DIAGNOSTIC_SOCKET");
		if(!configured || configured[0] != '/')
			return;
		path = configured;
		if(path.size() >= sizeof(sockaddr_un::sun_path))
			return;
		const auto slash = path.find_last_of('/');
		const auto parent = slash == 0 ? "/" : path.substr(0, slash);
		struct stat st{};
		if(stat(parent.c_str(), &st) || !S_ISDIR(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 0077))
			return;
		listener = socket(AF_UNIX, SOCK_STREAM, 0);
		if(listener < 0)
			return;
		sockaddr_un address{};
		address.sun_family = AF_UNIX;
		std::copy(path.begin(), path.end(), address.sun_path);
		if(!nonblocking(listener) || bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)))
		{
			close(listener);
			listener = -1;
			return;
		}
		if(chmod(path.c_str(), 0600) || lstat(path.c_str(), &st) || listen(listener, 4))
		{
			unlink(path.c_str());
			close(listener);
			listener = -1;
			return;
		}
		device = st.st_dev;
		inode = st.st_ino;
	}

	~DiagnosticChannel()
	{
		closeClient();
		if(listener >= 0)
		{
			close(listener);
			struct stat st{};
			if(!lstat(path.c_str(), &st) && st.st_dev == device && st.st_ino == inode)
				unlink(path.c_str());
		}
	}

	template<typename Dispatch>
	void poll(Dispatch dispatch)
	{
		if(listener < 0)
			return;
		if(client < 0)
		{
			client = accept(listener, nullptr, nullptr);
			if(client < 0)
				return;
			if(!nonblocking(client))
			{
				closeClient();
				return;
			}
			connected = std::chrono::steady_clock::now();
		}
		char buffer[256];
		const auto count = recv(client, buffer, sizeof(buffer), 0);
		if(count > 0)
			input.append(buffer, count);
		else if(count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
		{
			closeClient();
			return;
		}
		const auto newline = input.find('\n');
		if(input.size() > 256 || (newline != std::string::npos && newline + 1 != input.size()))
			respond("error invalid_request\n");
		else if(newline != std::string::npos)
			respond(dispatch(input.substr(0, newline)) ? "accepted\n" : "error invalid_command\n");
		else if(std::chrono::steady_clock::now() - connected > std::chrono::seconds(2))
			closeClient();
	}
};
#endif
