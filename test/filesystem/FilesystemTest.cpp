/*
 * FilesystemTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/filesystem/CFilesystemLoader.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/ExceptionsCommon.h"

namespace test
{

class FilesystemTest : public ::testing::Test
{
	boost::filesystem::path directory;

protected:
	const boost::filesystem::path & getDirectory() const
	{
		return directory;
	}

	void SetUp() override
	{
		directory = boost::filesystem::current_path()
			/ boost::filesystem::unique_path("vcmi-filesystem-%%%%-%%%%-%%%%-%%%%");
		ASSERT_TRUE(boost::filesystem::create_directory(directory));
	}

	void TearDown() override
	{
		boost::filesystem::remove_all(directory);
	}
};

TEST_F(FilesystemTest, RemovesResourceAndUpdatesIndex)
{
	CFilesystemLoader loader("Test/", getDirectory());
	const ResourcePath resource("Test/example.txt");

	ASSERT_TRUE(loader.createResource("Test/example.txt"));
	ASSERT_TRUE(loader.existsResource(resource));
	ASSERT_TRUE(boost::filesystem::exists(getDirectory() / "example.txt"));

	EXPECT_TRUE(loader.removeResource(resource));
	EXPECT_FALSE(loader.existsResource(resource));
	EXPECT_FALSE(boost::filesystem::exists(getDirectory() / "example.txt"));
}

TEST_F(FilesystemTest, DoesNotCreateResourceOutsideOfDirectory)
{
	const auto loaderDirectory = getDirectory() / "loader";
	ASSERT_TRUE(boost::filesystem::create_directory(loaderDirectory));
	CFilesystemLoader loader("Test/", loaderDirectory);

	EXPECT_FALSE(loader.createResource("Test/../example.txt"));
	EXPECT_FALSE(loader.createResource("Test/folder/../../example.txt"));
	EXPECT_FALSE(loader.createResource("Test/" + (getDirectory() / "example.txt").string()));
	EXPECT_FALSE(boost::filesystem::exists(getDirectory() / "example.txt"));

	EXPECT_TRUE(loader.createResource("Test/folder/example.txt"));
	EXPECT_TRUE(boost::filesystem::exists(loaderDirectory / "folder" / "example.txt"));
}

TEST_F(FilesystemTest, FindsFileWithRepeatedExtensionByItsName)
{
	CFilesystemLoader loader("Test/", getDirectory());
	ASSERT_TRUE(loader.createResource("Test/example.h3m.h3m"));

	const ResourcePath storedName(ResourcePath("Test/example.h3m.h3m").getOriginalName(), EResType::MAP);
	EXPECT_TRUE(loader.existsResource(storedName));
}

#ifndef _WIN32
TEST_F(FilesystemTest, ReportsDirectoryPathWhenScanningFails)
{
	const auto loop = getDirectory() / "cyclic-link";
	boost::filesystem::create_symlink(loop, loop);
	try
	{
		CFilesystemLoader loader("Test/", loop);
		FAIL() << "Expected a filesystem error";
	}
	catch(const DataLoadingException & error)
	{
		EXPECT_NE(std::string(error.what()).find("Failed to load content of '" + loop.string() + "'. Reason:"), std::string::npos);
	}
}
#endif

TEST(ResourcePathTest, KeepsExtensionOfDifferentType)
{
	EXPECT_EQ(ResourcePath("Fonts/CALLI10R.FNT.txt").getName(), "FONTS/CALLI10R.FNT");
}

}
