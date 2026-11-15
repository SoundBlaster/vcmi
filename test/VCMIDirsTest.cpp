/*
 * VCMIDirsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"
#include "../lib/VCMIDirs.h"

#ifdef VCMI_MAC
namespace
{
class ScopedUserDataPath
{
public:
	ScopedUserDataPath()
	{
		if(const char * value = std::getenv("VCMI_USER_DATA_PATH"))
			previous = value;
	}

	~ScopedUserDataPath()
	{
		if(previous)
			setenv("VCMI_USER_DATA_PATH", previous->c_str(), 1);
		else
			unsetenv("VCMI_USER_DATA_PATH");
	}

private:
	std::optional<std::string> previous;
};
}

TEST(VCMIDirsTest, MacUserDataOverrideKeepsRelatedPathsTogether)
{
	ScopedUserDataPath restore;
	unsetenv("VCMI_USER_DATA_PATH");
	const auto & dirs = VCMIDirs::get();
	const auto originalData = dirs.userDataPath();
	const auto originalLogs = dirs.userLogsPath();
	const boost::filesystem::path profile("/tmp/vcmi profile with spaces");
	ASSERT_EQ(setenv("VCMI_USER_DATA_PATH", profile.string().c_str(), 1), 0);
	EXPECT_EQ(dirs.userDataPath(), profile);
	EXPECT_EQ(dirs.userCachePath(), profile);
	EXPECT_EQ(dirs.userConfigPath(), profile / "config");
	EXPECT_EQ(dirs.userSavePath(), profile / "Saves");
	EXPECT_EQ(dirs.userLogsPath(), profile / "logs");
	EXPECT_EQ(setenv("VCMI_USER_DATA_PATH", "relative-path", 1), 0);
	EXPECT_THROW(dirs.userDataPath(), std::runtime_error);
	ASSERT_EQ(setenv("VCMI_USER_DATA_PATH", "", 1), 0);
	EXPECT_EQ(dirs.userDataPath(), originalData);
	EXPECT_EQ(dirs.userLogsPath(), originalLogs);
}
#endif
