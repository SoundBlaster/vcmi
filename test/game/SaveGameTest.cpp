/*
 * SaveGameTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/campaign/CampaignState.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/json/JsonValidator.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/CMemorySerializer.h"
#include "../../lib/serializer/CSaveFile.h"

class SaveGameTest : public GameStateTest
{
protected:
	boost::filesystem::path directory;
	void checkSavedGameIsRestored(bool toroidal, bool untaggedPrototype = false);

	void SetUp() override
	{
		GameStateTest::SetUp();
		directory = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-savegame-%%%%-%%%%-%%%%-%%%%");
		ASSERT_TRUE(boost::filesystem::create_directory(directory));
	}

	void TearDown() override
	{
		boost::filesystem::remove_all(directory);
		GameStateTest::TearDown();
	}
};

void SaveGameTest::checkSavedGameIsRestored(bool toroidal, bool untaggedPrototype)
{
	startTestGame();
	map->toroidal = toroidal;
	const auto hero = std::ranges::find_if(map->objects, [](const auto & object) { return object && object->ID == Obj::HERO; });
	ASSERT_NE(hero, map->objects.end());
	const auto heroID = (*hero)->id;
	const auto heroPosition = (*hero)->visitablePos();
	const auto saveFile = directory / "test.vsgm1";

	{
		const auto version = SaveFileFormat::versionForTopology(toroidal);
		CMemorySerializer header;
		header.oser.version = version;
		gameState->saveHeader(header.oser);
		CMemorySerializer game;
		game.oser.version = version;
		gameState->saveGame(game.oser);

		CSaveFile save(version);
		save.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*gameState).toBytes());
		save.addChunk(SaveFileFormat::CHUNK_HEADER, header.extractBuffer());
		save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, game.extractBuffer());
		save.write(saveFile);
	}
	if(untaggedPrototype)
	{
		std::vector<std::byte> version;
		SaveFileFormat::writeUInt32(version, static_cast<uint32_t>(ESerializationVersion::TOROIDAL_MAP));
		std::fstream file(saveFile.c_str(), std::ios::in | std::ios::out | std::ios::binary);
		file.seekp(SaveFileFormat::FILE_MAGIC.size());
		file.write(reinterpret_cast<const char *>(version.data()), version.size());
		ASSERT_TRUE(file.good());
	}

	CLoadFile loader(saveFile);
	EXPECT_EQ(CLoadFile::readVersion(saveFile), SaveFileFormat::versionForTopology(toroidal));
	JsonValidator validator;

	const JsonNode metadata = SaveMetadata::parse(loader.readChunk(SaveFileFormat::CHUNK_METADATA), "test", validator);
	EXPECT_EQ(metadata["map"]["width"].Integer(), map->width);
	EXPECT_EQ(metadata["game"]["humanPlayers"].Integer(), gameState->getStartInfo()->playerInfos.size());
	EXPECT_NO_THROW(SaveMetadata::verifyMods(metadata));

	CMapHeader loadedHeader;
	StartInfo loadedStartInfo;
	auto headerChunk = loader.openChunk(SaveFileFormat::CHUNK_HEADER, nullptr);
	headerChunk->iser & loadedHeader;
	headerChunk->iser & loadedStartInfo;
	EXPECT_TRUE(headerChunk->isFullyRead());
	EXPECT_EQ(loadedHeader.width, map->width);
	EXPECT_EQ(loadedHeader.toroidal, toroidal);
	EXPECT_EQ(loadedStartInfo.mapname, gameState->getStartInfo()->mapname);

	auto loadedState = std::make_shared<CGameState>();
	loadedState->preInit(&services);
	auto gameChunk = loader.openChunk(SaveFileFormat::CHUNK_GAMESTATE, loadedState.get());
	loadedState->loadGame(gameChunk->iser);
	EXPECT_TRUE(gameChunk->isFullyRead());
	EXPECT_EQ(loadedState->day, gameState->day);
	EXPECT_EQ(loadedState->getMap().width, map->width);
	EXPECT_EQ(loadedState->getMap().toroidal, toroidal);
	ASSERT_NE(loadedState->getMap().getObject(heroID), nullptr);
	EXPECT_EQ(loadedState->getMap().getObject(heroID)->visitablePos(), heroPosition);
	EXPECT_EQ(loadedState->players.size(), gameState->players.size());
}

TEST_F(SaveGameTest, savedGameIsRestored)
{
	checkSavedGameIsRestored(false);
}

TEST_F(SaveGameTest, savedToroidalGameIsRestored)
{
	checkSavedGameIsRestored(true);
}

TEST_F(SaveGameTest, untaggedPrototypeToroidalSaveIsRestored)
{
	checkSavedGameIsRestored(true, true);
}

TEST_F(SaveGameTest, mapTopologyIsVersionedInSavedHeader)
{
	CMapHeader original;
	original.toroidal = true;

	CMemorySerializer current;
	current.oser & original;
	CMapHeader restored;
	current.iser & restored;
	EXPECT_TRUE(restored.toroidal);

	CMemorySerializer legacy;
	legacy.oser.version = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	legacy.iser.version = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	legacy.oser & original;
	CMapHeader restoredLegacy;
	restoredLegacy.toroidal = true;
	legacy.iser & restoredLegacy;
	EXPECT_FALSE(restoredLegacy.toroidal);
}

TEST_F(SaveGameTest, invalidMetadataIsRejected)
{
	startTestGame();

	JsonValidator validator;
	JsonNode metadata = SaveMetadata::create(*gameState);
	metadata["mods"]["testMod"]["name"].String() = "mod without version";
	EXPECT_THROW(SaveMetadata::parse(metadata.toBytes(), "test", validator), std::runtime_error);

	metadata = SaveMetadata::create(*gameState);
	metadata.Struct().erase("map");
	EXPECT_THROW(SaveMetadata::parse(metadata.toBytes(), "test", validator), std::runtime_error);

	metadata = SaveMetadata::create(*gameState);
	metadata.Struct().erase("game");
	EXPECT_THROW(SaveMetadata::parse(metadata.toBytes(), "test", validator), std::runtime_error);
}

TEST_F(SaveGameTest, legacyMetadataIsValid)
{
	startTestGame();

	ModCompatibilityInfo mods;
	const JsonNode metadata = SaveMetadata::createLegacy(gameState->getMap(), *gameState->getStartInfo(), mods);

	JsonValidator validator;
	EXPECT_EQ(validator.check("vcmi:scenarioMetadata", metadata), "");
	EXPECT_EQ(metadata["map"]["width"].Integer(), map->width);
	EXPECT_FALSE(metadata["game"].Struct().contains("day"));
}
