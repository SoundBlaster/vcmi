/*
 * DiagnosticCommandChannelTest.cpp, part of VCMI engine
 * License: GNU General Public License v2.0 or later
 */
#include "StdInc.h"

#if defined(__unix__) || defined(__APPLE__)
#include "../../client/gui/DiagnosticCommandChannel.h"

TEST(DiagnosticCommandChannelTest, HandlesBoundedRequestsAndPreservesExistingFiles)
{
	char dir[] = "/tmp/vcmi-channel-test-XXXXXX";
	ASSERT_NE(mkdtemp(dir), nullptr);
	const std::string path = std::string(dir) + "/socket";
	const char * oldValue = getenv("VCMI_DIAGNOSTIC_SOCKET");
	const std::optional<std::string> previous = oldValue ? std::optional<std::string>(oldValue) : std::nullopt;
	ASSERT_EQ(setenv("VCMI_DIAGNOSTIC_SOCKET", path.c_str(), 1), 0);
	int dispatched = 0;
	{
		DiagnosticChannel channel;
		struct stat st{};
		ASSERT_EQ(lstat(path.c_str(), &st), 0);
		EXPECT_TRUE(S_ISSOCK(st.st_mode));
		EXPECT_EQ(st.st_mode & 0777, 0600);
		auto request = [&](const std::string & input)
		{
			const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
			EXPECT_GE(fd, 0);
			sockaddr_un address{};
			address.sun_family = AF_UNIX;
			std::copy(path.begin(), path.end(), address.sun_path);
			EXPECT_EQ(connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)), 0);
			EXPECT_EQ(send(fd, input.data(), input.size(), 0), input.size());
			channel.poll([&](const std::string & command) { ++dispatched; return command == "valid"; });
			channel.poll([&](const std::string & command) { ++dispatched; return command == "valid"; });
			char response[128]{};
			const auto count = recv(fd, response, sizeof(response), 0);
			close(fd);
			return count > 0 ? std::string(response, count) : std::string();
		};
		EXPECT_EQ(request("valid\n"), "accepted\n");
		EXPECT_EQ(request("unknown\n"), "error invalid_command\n");
		EXPECT_EQ(request("valid\nvalid\n"), "error invalid_request\n");
		EXPECT_EQ(request(std::string(257, 'x')), "error invalid_request\n");
		EXPECT_EQ(dispatched, 2);
		const int idle = socket(AF_UNIX, SOCK_STREAM, 0);
		ASSERT_GE(idle, 0);
		sockaddr_un address{};
		address.sun_family = AF_UNIX;
		std::copy(path.begin(), path.end(), address.sun_path);
		ASSERT_EQ(connect(idle, reinterpret_cast<sockaddr *>(&address), sizeof(address)), 0);
		ASSERT_EQ(send(idle, "partial", 7, 0), 7);
		channel.poll([&](const std::string &) { ++dispatched; return true; });
		std::this_thread::sleep_for(std::chrono::milliseconds(2100));
		channel.poll([&](const std::string &) { ++dispatched; return true; });
		char byte;
		EXPECT_EQ(recv(idle, &byte, 1, 0), 0);
		close(idle);
		EXPECT_EQ(dispatched, 2);
		EXPECT_EQ(request("valid\n"), "accepted\n");
		EXPECT_EQ(dispatched, 3);
	}
	EXPECT_NE(access(path.c_str(), F_OK), 0);
	const int existing = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
	ASSERT_GE(existing, 0);
	close(existing);
	{ DiagnosticChannel channel; }
	struct stat st{};
	EXPECT_EQ(lstat(path.c_str(), &st), 0);
	EXPECT_TRUE(S_ISREG(st.st_mode));
	unlink(path.c_str());
	rmdir(dir);
	if(previous) setenv("VCMI_DIAGNOSTIC_SOCKET", previous->c_str(), 1);
	else unsetenv("VCMI_DIAGNOSTIC_SOCKET");
}

TEST(DiagnosticCommandChannelTest, RejectsSharedDirectory)
{
	char dir[] = "/tmp/vcmi-channel-shared-XXXXXX";
	ASSERT_NE(mkdtemp(dir), nullptr);
	ASSERT_EQ(chmod(dir, 0755), 0);
	const std::string path = std::string(dir) + "/socket";
	const char * oldValue = getenv("VCMI_DIAGNOSTIC_SOCKET");
	const std::optional<std::string> previous = oldValue ? std::optional<std::string>(oldValue) : std::nullopt;
	ASSERT_EQ(setenv("VCMI_DIAGNOSTIC_SOCKET", path.c_str(), 1), 0);
	{ DiagnosticChannel channel; }
	EXPECT_NE(access(path.c_str(), F_OK), 0);
	rmdir(dir);
	if(previous) setenv("VCMI_DIAGNOSTIC_SOCKET", previous->c_str(), 1);
	else unsetenv("VCMI_DIAGNOSTIC_SOCKET");
}
#endif
