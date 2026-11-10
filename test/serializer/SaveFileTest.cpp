/*
 * SaveFileTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/CSaveFile.h"

class SaveFileTest : public ::testing::Test
{
protected:
	boost::filesystem::path directory;
	boost::filesystem::path saveFile;

	const std::vector<std::byte> metadata = makeData(100, 1);
	const std::vector<std::byte> header = makeData(5000, 2);
	const std::vector<std::byte> gameState = makeData(200000, 3);

	static std::vector<std::byte> makeData(size_t size, int seed)
	{
		std::vector<std::byte> result(size);
		for(size_t i = 0; i < size; ++i)
			result[i] = static_cast<std::byte>((i * seed + i / 7) % 251);
		return result;
	}

	void SetUp() override
	{
		directory = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-savefile-%%%%-%%%%-%%%%-%%%%");
		ASSERT_TRUE(boost::filesystem::create_directory(directory));
		saveFile = directory / "test.vsgm1";

		CSaveFile save;
		save.addChunk(SaveFileFormat::CHUNK_METADATA, metadata);
		save.addChunk(SaveFileFormat::CHUNK_HEADER, header);
		save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, gameState);
		save.write(saveFile);
	}

	void TearDown() override
	{
		boost::filesystem::remove_all(directory);
	}

	std::vector<char> readSaveFile() const
	{
		std::ifstream stream(saveFile.c_str(), std::ios::binary);
		return std::vector<char>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	}

	void writeSaveFile(const std::vector<char> & content) const
	{
		std::ofstream stream(saveFile.c_str(), std::ios::binary | std::ios::trunc);
		stream.write(content.data(), content.size());
	}

	void setRawVersion(uint32_t version) const
	{
		auto content = readSaveFile();
		for(size_t i = 0; i < sizeof(version); ++i)
			content[SaveFileFormat::FILE_MAGIC.size() + i] = static_cast<char>((version >> (8 * i)) & 0xff);
		writeSaveFile(content);
	}
};

TEST_F(SaveFileTest, chunksAreRestored)
{
	EXPECT_EQ(CLoadFile::readVersion(saveFile), ESerializationVersion::CURRENT);

	CLoadFile loader(saveFile);
	// chunks can be read in any order, so that list of saves only needs to read metadata
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_GAMESTATE), gameState);
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_METADATA), metadata);
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_HEADER), header);
}

TEST_F(SaveFileTest, toroidalVersionHasAnExplicitOnDiskMarker)
{
	const auto content = readSaveFile();
	const auto rawVersion = SaveFileFormat::readUInt32(reinterpret_cast<const std::byte *>(content.data()) + SaveFileFormat::FILE_MAGIC.size());
	EXPECT_EQ(rawVersion & 0xffff0000u, 0x54520000u);
	EXPECT_GT(rawVersion, static_cast<uint32_t>(ESerializationVersion::CURRENT));
	EXPECT_EQ(CLoadFile::readVersion(saveFile), ESerializationVersion::CURRENT);
}

TEST_F(SaveFileTest, boundedSaveUsesPreviousUnmarkedVersion)
{
	const auto previous = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	CSaveFile save(previous);
	save.addChunk(SaveFileFormat::CHUNK_METADATA, metadata);
	save.addChunk(SaveFileFormat::CHUNK_HEADER, header);
	save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, gameState);
	save.write(saveFile);
	const auto content = readSaveFile();
	const auto rawVersion = SaveFileFormat::readUInt32(reinterpret_cast<const std::byte *>(content.data()) + SaveFileFormat::FILE_MAGIC.size());
	EXPECT_EQ(rawVersion, static_cast<uint32_t>(previous));
	EXPECT_EQ(CLoadFile::readVersion(saveFile), previous);
	CLoadFile loader(saveFile);
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_GAMESTATE), gameState);
}

TEST_F(SaveFileTest, taggedVersionBelowTopologyIsRejected)
{
	setRawVersion(SaveFileFormat::TOROIDAL_VERSION_TAG | static_cast<uint32_t>(ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER));
	EXPECT_THROW(CLoadFile::readVersion(saveFile), std::runtime_error);
	EXPECT_THROW(CLoadFile::readMetadata(saveFile), std::runtime_error);
}

TEST_F(SaveFileTest, newerTaggedSaveOnlyAllowsMetadata)
{
	setRawVersion(SaveFileFormat::TOROIDAL_VERSION_TAG | (static_cast<uint32_t>(ESerializationVersion::CURRENT) + 1));
	EXPECT_THROW(CLoadFile loader(saveFile), SaveTooNewException);
	EXPECT_EQ(CLoadFile::readMetadata(saveFile), metadata);
}

TEST_F(SaveFileTest, truncatedFileIsRejected)
{
	auto content = readSaveFile();
	content.resize(content.size() - 100);
	writeSaveFile(content);

	EXPECT_THROW(CLoadFile loader(saveFile), std::runtime_error);
}

TEST_F(SaveFileTest, dataAfterLastChunkIsRejected)
{
	auto content = readSaveFile();
	content.resize(content.size() + 100);
	writeSaveFile(content);

	EXPECT_THROW(CLoadFile loader(saveFile), std::runtime_error);
}

TEST_F(SaveFileTest, corruptedChunkIsRejected)
{
	auto content = readSaveFile();
	content[content.size() - 100] ^= 0x55;
	writeSaveFile(content);

	CLoadFile loader(saveFile);
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_METADATA), metadata);
	EXPECT_THROW(loader.readChunk(SaveFileFormat::CHUNK_GAMESTATE), std::runtime_error);
}

TEST_F(SaveFileTest, unsupportedVersionsAreReported)
{
	setRawVersion(1);
	EXPECT_THROW(CLoadFile::readVersion(saveFile), SaveTooOldException);

	setRawVersion(static_cast<uint32_t>(ESerializationVersion::CURRENT) + 1);
	EXPECT_THROW(CLoadFile::readVersion(saveFile), SaveTooNewException);
	EXPECT_THROW(CLoadFile loader(saveFile), SaveTooNewException);
}

TEST_F(SaveFileTest, metadataOfNewerSaveIsReadable)
{
	setRawVersion(static_cast<uint32_t>(ESerializationVersion::CURRENT) + 1);
	auto content = readSaveFile();
	// newer versions may change all chunks after metadata
	content.resize(content.size() + 100);
	writeSaveFile(content);

	EXPECT_EQ(CLoadFile::readMetadata(saveFile), metadata);
}
