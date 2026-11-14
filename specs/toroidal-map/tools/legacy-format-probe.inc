/*
 * Legacy format compatibility probe for VCMI baseline 30e9207.
 * Include temporarily in test/game/SaveGameTest.cpp when building the
 * unmodified baseline engine. The test reads fixtures without modifying them.
 * License: GNU General Public License v2.0 or later.
 */
#include "lib/mapping/CMapService.h"

TEST(LegacyFormatProbe, RejectsTaggedToroidalMapAndSave)
{
	const char * mapPath = std::getenv("VCMI_TAGGED_TOROID_VMAP");
	const char * savePath = std::getenv("VCMI_TAGGED_TOROID_SAVE");
	const char * boundedPath = std::getenv("VCMI_BOUNDED_VMAP");
	ASSERT_NE(mapPath, nullptr);
	ASSERT_NE(savePath, nullptr);
	ASSERT_NE(boundedPath, nullptr);
	const auto readMap = [](const char * path)
	{
		std::ifstream file(path, std::ios::binary);
		return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	};
	const auto bytes = readMap(mapPath);
	const auto bounded = readMap(boundedPath);
	ASSERT_FALSE(bytes.empty());
	ASSERT_FALSE(bounded.empty());
	CMapService maps;
	EXPECT_NO_THROW(maps.loadMapHeader(bounded.data(), static_cast<int>(bounded.size()), "bounded-control", "core", "ASCII"));
	try
	{
		maps.loadMapHeader(bytes.data(), static_cast<int>(bytes.size()), "compatibility-probe", "core", "ASCII");
		FAIL() << "Legacy map loader accepted a tagged toroidal header";
	}
	catch(const std::runtime_error & error)
	{
		EXPECT_STREQ(error.what(), "Unsupported map format version");
	}
	EXPECT_THROW(CLoadFile(boost::filesystem::path(savePath)), SaveTooNewException);
}
