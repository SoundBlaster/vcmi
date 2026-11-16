/*
 * CMapFormatTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/filesystem/Filesystem.h"

#include "../../lib/callback/EditorCallback.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGDwelling.h"
#include "../../lib/mapObjects/CGResource.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/StartInfo.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapTopology.h"
#include "../../lib/mapping/MapFormatH3M.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/modding/IdentifierResolver.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"
#include "../../lib/serializer/CMemorySerializer.h"
#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/CSaveFile.h"
#include "../../lib/serializer/ESerializationVersion.h"
#include "../../lib/serializer/GameConnection.h"
#include "../../lib/network/NetworkInterface.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"

#include "../../lib/VCMIDirs.h"
#include "../../server/CGameHandler.h"
#include "../../server/battles/BattleProcessor.h"
#include "../../server/activities/Activity.h"
#include "../../server/activities/MapActivities.h"
#include "../../server/activities/ActivityProcessor.h"
#include "../../server/processors/TurnOrderProcessor.h"
#include "../mock/GameHandlerTestServer.h"

#include "../JsonComparer.h"
#include "../mock/TinyH3MBuilder.h"
#include "../mock/ToroidalDemoMap.h"

#include <condition_variable>
#include <thread>

#ifdef ENABLE_NULLKILLER2_AI
#include "../../AI/Nullkiller2/AIGateway.h"
#include "../../AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/callback/IClient.h"
#include "../../lib/networkPacks/PacksForServer.h"
#endif

static void saveTestMap(CMemoryBuffer & serializeBuffer, const std::string & filename)
{
	auto path = VCMIDirs::get().userDataPath() / filename;
	boost::filesystem::remove(path);
	std::ofstream tmp(path.c_str(), std::ofstream::binary);

	tmp.write((const char *)serializeBuffer.getBuffer().data(), serializeBuffer.getSize());
	tmp.flush();
	tmp.close();
}

static JsonNode getFromArchive(CZipLoader & archive, const std::string & archiveFilename)
{
	JsonPath resource = JsonPath::builtin(archiveFilename);

	if(!archive.existsResource(resource))
		throw std::runtime_error(archiveFilename + " not found");

	auto data = archive.load(resource)->readAll();

	JsonNode res(reinterpret_cast<const std::byte *>(data.first.get()), data.second, resource.getName());

	return res;
}

// Demo maps are authored by the test executable, whose registry contains
// additional test-only heroes and spells. Those are unavailable in the game.
static void removeTestModEntitiesFromDemo(CMap & map)
{
	const auto testSpell = [](SpellID spell) { return spell.toSpell()->getModScope() == "vcmi-test"; };
	std::erase_if(map.allowedHeroes, [](HeroTypeID hero) { return hero.toHeroType()->getModScope() == "vcmi-test"; });
	std::erase_if(map.allowedSpells, testSpell);
	for(const auto & object : map.objects)
		if(auto * town = dynamic_cast<CGTownInstance *>(object.get()))
		{
			std::erase_if(town->possibleSpells, testSpell);
			std::erase_if(town->obligatorySpells, testSpell);
		}
}

static void expectDemoHasNoTestModReferences(CMemoryBuffer & buffer)
{
	std::shared_ptr<CIOApi> io(new CProxyROIOApi(&buffer));
	CZipLoader archive("", "_", io);
	for(const auto & name : {CMapFormatJson::HEADER_FILE_NAME, CMapFormatJson::OBJECTS_FILE_NAME})
		EXPECT_EQ(getFromArchive(archive, name).toString().find("vcmi-test:"), std::string::npos) << name;
}

static void addToArchive(CZipSaver & saver, const JsonNode & data, const std::string & filename)
{
	auto s = data.toString();
	std::unique_ptr<COutputStream> stream = saver.addFile(filename);

	if(stream->write((const ui8*)s.c_str(), s.size()) != s.size())
		throw std::runtime_error("CMapSaverJson::saveHeader() zip compression failed.");
}

static std::unique_ptr<CMap> loadOriginal(const JsonNode & header, const JsonNode & objects, const JsonNode & surface, const JsonNode & underground)
{
	CMemoryBuffer initialBuffer;

	std::shared_ptr<CIOApi> originalDataIO(new CProxyIOApi(&initialBuffer));

	{
		CZipSaver initialSaver(originalDataIO, "_");

		addToArchive(initialSaver, header, CMapFormatJson::HEADER_FILE_NAME);
		addToArchive(initialSaver, objects, CMapFormatJson::OBJECTS_FILE_NAME);
		addToArchive(initialSaver, surface, "surface_terrain.json");
		addToArchive(initialSaver, underground, "underground_terrain.json");
	}

	initialBuffer.seek(0);

	CMapLoaderJson initialLoader(&initialBuffer);

	// object handlers assert on a non-null callback in their factory
	static EditorCallback cb(nullptr);
	auto map = initialLoader.loadMap(&cb);
	cb.setMap(map.get());
	return map;
}

static void loadActual(CMemoryBuffer * serializeBuffer, const std::unique_ptr<CMap> & originalMap, JsonNode & header, JsonNode & objects, JsonNode & surface, JsonNode & underground)
{
	{
		CMapSaverJson saver(serializeBuffer);
		saver.saveMap(originalMap);
	}

	std::shared_ptr<CIOApi> actualDataIO(new CProxyROIOApi(serializeBuffer));
	CZipLoader actualDataLoader("", "_", actualDataIO);

	header = getFromArchive(actualDataLoader, CMapFormatJson::HEADER_FILE_NAME);
	objects = getFromArchive(actualDataLoader, CMapFormatJson::OBJECTS_FILE_NAME);
	surface = getFromArchive(actualDataLoader, "surface_terrain.json");
	underground = getFromArchive(actualDataLoader, "underground_terrain.json");
}

TEST(MapFormat, Objects)
{
	static const std::string MAP_DATA_PATH = "test/ObjectPropertyTest/";

	const JsonNode expectedHeader(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::HEADER_FILE_NAME));
	const JsonNode expectedObjects(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::OBJECTS_FILE_NAME));

	const JsonNode expectedSurface(JsonPath::builtin(MAP_DATA_PATH + "surface_terrain.json"));
	const JsonNode expectedUnderground(JsonPath::builtin(MAP_DATA_PATH + "underground_terrain.json"));

	std::unique_ptr<CMap> originalMap = loadOriginal(expectedHeader, expectedObjects, expectedSurface, expectedUnderground);

	CMemoryBuffer serializeBuffer;

	JsonNode actualHeader;
	JsonNode actualObjects;
	JsonNode actualSurface;
	JsonNode actualUnderground;

	loadActual(&serializeBuffer, originalMap, actualHeader, actualObjects, actualSurface, actualUnderground);

	saveTestMap(serializeBuffer, "test_object_property.vmap");

	{
		JsonComparer c(false);
		c.compare("hdr", actualHeader, expectedHeader);
		c.compare("obj", actualObjects, expectedObjects);
	}

	{
		JsonComparer c(true);
		c.compare("surface", actualSurface, expectedSurface);
		c.compare("underground", actualUnderground, expectedUnderground);
	}
}

TEST(MapFormat, Terrain)
{
	static const std::string MAP_DATA_PATH = "test/TerrainTest/";

	const JsonNode expectedHeader(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::HEADER_FILE_NAME));
	const JsonNode expectedObjects(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::OBJECTS_FILE_NAME));

	const JsonNode expectedSurface(JsonPath::builtin(MAP_DATA_PATH + "surface_terrain.json"));
	const JsonNode expectedUnderground(JsonPath::builtin(MAP_DATA_PATH + "underground_terrain.json"));

	std::unique_ptr<CMap> originalMap = loadOriginal(expectedHeader, expectedObjects, expectedSurface, expectedUnderground);

	CMemoryBuffer serializeBuffer;

	JsonNode actualHeader;
	JsonNode actualObjects;
	JsonNode actualSurface;
	JsonNode actualUnderground;

	loadActual(&serializeBuffer, originalMap, actualHeader, actualObjects, actualSurface, actualUnderground);

	saveTestMap(serializeBuffer, "test_terrain.vmap");

	{
		JsonComparer c(false);
		c.compare("hdr", actualHeader, expectedHeader);
		c.compare("obj", actualObjects, expectedObjects);
	}

	{
		JsonComparer c(true);
		c.compare("surface", actualSurface, expectedSurface);
		c.compare("underground", actualUnderground, expectedUnderground);
	}
}

TEST(MapFormat, ToroidalTopologyRoundTripsAndRequiresNewFormat)
{
	static const std::string MAP_DATA_PATH = "test/TerrainTest/";
	JsonNode header(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::HEADER_FILE_NAME));
	const JsonNode objects(JsonPath::builtin(MAP_DATA_PATH + CMapFormatJson::OBJECTS_FILE_NAME));
	const JsonNode surface(JsonPath::builtin(MAP_DATA_PATH + "surface_terrain.json"));
	const JsonNode underground(JsonPath::builtin(MAP_DATA_PATH + "underground_terrain.json"));

	const auto bounded = loadOriginal(header, objects, surface, underground);
	EXPECT_FALSE(bounded->toroidal);

	header["topology"].String() = "toroidal";
	EXPECT_THROW(loadOriginal(header, objects, surface, underground), std::runtime_error);
	header["versionMajor"].Float() = CMapFormatJson::VERSION_MAJOR_TOROIDAL;
	const auto toroidal = loadOriginal(header, objects, surface, underground);
	ASSERT_TRUE(toroidal->toroidal);

	CMemoryBuffer buffer;
	JsonNode actualHeader;
	JsonNode actualObjects;
	JsonNode actualSurface;
	JsonNode actualUnderground;
	loadActual(&buffer, toroidal, actualHeader, actualObjects, actualSurface, actualUnderground);
	EXPECT_EQ(actualHeader["versionMajor"].Integer(), CMapFormatJson::VERSION_MAJOR_TOROIDAL_TAGGED);
	EXPECT_EQ(actualHeader["topology"].String(), "toroidal");
	EXPECT_TRUE(loadOriginal(actualHeader, actualObjects, actualSurface, actualUnderground)->toroidal);

	actualHeader["versionMajor"].Integer() = CMapFormatJson::VERSION_MAJOR_TOROIDAL_TAGGED + 1;
	EXPECT_THROW(loadOriginal(actualHeader, actualObjects, actualSurface, actualUnderground), std::runtime_error);
	actualHeader["versionMajor"].Integer() = static_cast<int64_t>(CMapFormatJson::VERSION_MAJOR_TOROIDAL_TAGGED) + (int64_t{1} << 32);
	EXPECT_THROW(loadOriginal(actualHeader, actualObjects, actualSurface, actualUnderground), std::runtime_error);
	actualHeader["versionMajor"].Integer() = -1;
	EXPECT_THROW(loadOriginal(actualHeader, actualObjects, actualSurface, actualUnderground), std::runtime_error);
	actualHeader["versionMajor"].Integer() = CMapFormatJson::VERSION_MAJOR_TOROIDAL_TAGGED;
	actualHeader["topology"].String() = "bounded";
	EXPECT_THROW(loadOriginal(actualHeader, actualObjects, actualSurface, actualUnderground), std::runtime_error);

	header["topology"].String() = "unknown";
	EXPECT_THROW(loadOriginal(header, objects, surface, underground), std::runtime_error);
	header["versionMajor"].Float() = CMapFormatJson::VERSION_MAJOR;
	EXPECT_THROW(loadOriginal(header, objects, surface, underground), std::runtime_error);
	header["topology"].Bool() = true;
	EXPECT_THROW(loadOriginal(header, objects, surface, underground), std::runtime_error);
	header["topology"].String() = "bounded";
	EXPECT_FALSE(loadOriginal(header, objects, surface, underground)->toroidal);
}

TEST(MapFormat, ToroidalHeaderRequiresAtLeastThreeTilesOnEachAxis)
{
	const JsonNode original(JsonPath::builtin("test/TerrainTest/" + CMapFormatJson::HEADER_FILE_NAME));
	auto loadHeader = [](const JsonNode & header)
	{
		CMemoryBuffer buffer;
		std::shared_ptr<CIOApi> io(new CProxyIOApi(&buffer));
		{
			CZipSaver saver(io, "_");
			addToArchive(saver, header, CMapFormatJson::HEADER_FILE_NAME);
		}
		buffer.seek(0);
		CMapLoaderJson loader(&buffer);
		return loader.loadMapHeader();
	};

	JsonNode header = original;
	header["versionMajor"].Float() = CMapFormatJson::VERSION_MAJOR_TOROIDAL;
	header["topology"].String() = "toroidal";
	auto & surface = header["mapLevels"]["surface"];
	surface["width"].Float() = 3;
	surface["height"].Float() = 3;
	const auto minimum = loadHeader(header);
	ASSERT_TRUE(minimum->toroidal);
	EXPECT_EQ(minimum->width, 3);
	EXPECT_EQ(minimum->height, 3);

	surface["width"].Float() = 2;
	EXPECT_THROW(loadHeader(header), std::runtime_error);
	surface["width"].Float() = 3;
	surface["height"].Float() = 2;
	EXPECT_THROW(loadHeader(header), std::runtime_error);
}

TEST(MapFormat, ReadsAndRewritesExternalUntaggedPrototypeVmap)
{
	const char * path = std::getenv("VCMI_TOROID_PROTOTYPE_VMAP");
	if(!path || !*path)
		GTEST_SKIP() << "Set VCMI_TOROID_PROTOTYPE_VMAP to a branch-era map";
	std::ifstream file(path, std::ios::binary);
	ASSERT_TRUE(file.is_open());
	const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	CMemoryBuffer input;
	input.write(reinterpret_cast<const ui8 *>(bytes.data()), static_cast<si64>(bytes.size()));
	input.seek(0);
	{
		std::shared_ptr<CIOApi> io(new CProxyROIOApi(&input));
		CZipLoader archive("", "_", io);
		const JsonNode header = getFromArchive(archive, CMapFormatJson::HEADER_FILE_NAME);
		ASSERT_EQ(header["versionMajor"].Integer(), CMapFormatJson::VERSION_MAJOR_TOROIDAL);
		ASSERT_EQ(header["topology"].String(), "toroidal");
	}
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderJson loader(&input);
	auto map = loader.loadMap(&callback);
	ASSERT_NE(map, nullptr);
	callback.setMap(map.get());
	ASSERT_TRUE(map->toroidal);
	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	{
		std::shared_ptr<CIOApi> io(new CProxyROIOApi(&output));
		CZipLoader archive("", "_", io);
		const JsonNode header = getFromArchive(archive, CMapFormatJson::HEADER_FILE_NAME);
		ASSERT_EQ(header["versionMajor"].Integer(), CMapFormatJson::VERSION_MAJOR_TOROIDAL_TAGGED);
	}
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reopenedLoader(&output);
	auto reopened = reopenedLoader.loadMap(&reopenedCallback);
	ASSERT_NE(reopened, nullptr);
	ASSERT_TRUE(reopened->toroidal);
	EXPECT_EQ(reopened->width, map->width);
	EXPECT_EQ(reopened->height, map->height);
	EXPECT_EQ(reopened->objects.size(), map->objects.size());
}

TEST(MapFormat, LegacyH3MCanBeOptedIntoToroidalVmap)
{
	const PlayerColor player(0);
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, true)
		.name("Legacy topology conversion")
		.playerActive(player)
		.hero(int3(11, 10, 0), HeroTypeID(0), player)
		.town(int3(20, 20, 0), FactionID(0), player)
		.terrain(int3(8, 8, 0), TerrainId::WATER)
		.buildAndDump("LegacyH3MCanBeOptedIntoToroidalVmap");
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("LegacyTopology", "core", "ASCII", &input);
	auto legacy = loader.loadMap(&callback);
	ASSERT_NE(legacy, nullptr);
	callback.setMap(legacy.get());
	ASSERT_FALSE(legacy->toroidal);
	ASSERT_EQ(legacy->getHeroesOnMap().size(), 1u);
	ASSERT_EQ(legacy->getAllTowns().size(), 1u);
	legacy->setToroidalAndRebuild(true);
	CMemoryBuffer output;
	JsonNode header, objects, surface, underground;
	loadActual(&output, legacy, header, objects, surface, underground);
	EXPECT_EQ(header["versionMajor"].Integer(), CMapFormatJson::VERSION_MAJOR_TOROIDAL_TAGGED);
	EXPECT_EQ(header["topology"].String(), "toroidal");
	auto reopened = loadOriginal(header, objects, surface, underground);
	ASSERT_TRUE(reopened->toroidal);
	EXPECT_EQ(reopened->width, legacy->width);
	EXPECT_EQ(reopened->height, legacy->height);
	EXPECT_EQ(reopened->levels(), 2);
	EXPECT_EQ(reopened->getHeroesOnMap().size(), legacy->getHeroesOnMap().size());
	EXPECT_EQ(reopened->getAllTowns().size(), legacy->getAllTowns().size());
	EXPECT_EQ(reopened->getTile(int3(8, 8, 0)).terrainType, TerrainId::WATER);
}

TEST(MapFormat, LoadsExternalLegacyMapForInspection)
{
	const char * path = std::getenv("VCMI_INSPECT_LEGACY_H3M");
	if(!path || !*path)
		GTEST_SKIP() << "Set VCMI_INSPECT_LEGACY_H3M to a legacy map";
	std::ifstream file(path, std::ios::binary);
	ASSERT_TRUE(file.is_open());
	const std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	EditorCallback callback(nullptr);
	CMapService service;
	auto map = service.loadMap(bytes.data(), static_cast<int>(bytes.size()), "ExternalInspectionMap", "core", "CP1251", &callback);
	ASSERT_NE(map, nullptr);
	callback.setMap(map.get());
	std::cout << "External map: " << map->width << "x" << map->height << ", levels=" << static_cast<int>(map->levels())
		<< ", objects=" << map->objects.size() << ", towns=" << map->getAllTowns().size()
		<< ", toroidal=" << map->toroidal << std::endl;
	if(std::getenv("VCMI_INSPECT_EXPECT_TOROIDAL"))
	{
		ASSERT_TRUE(map->toroidal);
		const auto towns = map->getAllTowns();
		for(const auto & player : map->players)
			if(player.hasMainTown)
				EXPECT_TRUE(std::ranges::any_of(towns, [&](const auto & id)
				{
					const auto * town = map->getObject(id);
					return town && town->anchorPos() == player.posOfMainTown;
				})) << "Missing main town at " << player.posOfMainTown.toString();
	}
	for(int z = 0; z < map->levels(); ++z)
	{
		size_t objects = 0, water = 0, road = 0, river = 0;
		for(const auto & object : map->objects)
			if(object && object->anchorPos().z == z)
				++objects;
		for(int y = 0; y < map->height; ++y)
			for(int x = 0; x < map->width; ++x)
			{
				const auto & tile = map->getTile(int3(x, y, z));
				water += tile.isWater();
				road += tile.hasRoad();
				river += tile.hasRiver();
			}
		std::cout << "Level " << z << ": objects=" << objects << ", water=" << water
			<< ", road=" << road << ", river=" << river << std::endl;
	}
	if(std::getenv("VCMI_INSPECT_VMAP_ROUNDTRIP"))
	{
		CMemoryBuffer archive;
		{
			CMapSaverJson saver(&archive);
			saver.saveMap(map);
		}
		archive.seek(0);
		EditorCallback reopenedCallback(nullptr);
		CMapLoaderJson reader(&archive);
		auto reopened = reader.loadMap(&reopenedCallback);
		reopenedCallback.setMap(reopened.get());
		ASSERT_EQ(reopened->width, map->width);
		ASSERT_EQ(reopened->height, map->height);
		ASSERT_EQ(reopened->levels(), map->levels());
		ASSERT_EQ(reopened->getAllTowns().size(), map->getAllTowns().size());
		ASSERT_EQ(reopened->instanceNames.size(), map->instanceNames.size());
		ASSERT_EQ(reopened->mods.size(), map->mods.size());
		for(const auto & [name, info] : map->mods)
		{
			const auto found = reopened->mods.find(name);
			ASSERT_NE(found, reopened->mods.end()) << name;
			EXPECT_EQ(found->second.version, info.version) << name;
		}
		size_t changedObjects = 0, changedTerrain = 0, changedArt = 0, changedBit6 = 0, changedBit7 = 0;
		for(const auto & [name, object] : map->instanceNames)
		{
			const auto found = reopened->instanceNames.find(name);
			if(found == reopened->instanceNames.end() || found->second->anchorPos() != object->anchorPos() || found->second->ID != object->ID)
				++changedObjects;
		}
		for(int z = 0; z < map->levels(); ++z)
			for(int y = 0; y < map->height; ++y)
				for(int x = 0; x < map->width; ++x)
				{
					const int3 pos(x, y, z);
					const auto & before = map->getTile(pos);
					const auto & after = reopened->getTile(pos);
					if(before.terrainType != after.terrainType || before.roadType != after.roadType || before.riverType != after.riverType)
						++changedTerrain;
					if(before.terView != after.terView || (before.extTileFlags & 3) != (after.extTileFlags & 3) ||
					   (before.hasRoad() && (before.roadDir != after.roadDir || (before.extTileFlags & 48) != (after.extTileFlags & 48))) ||
					   (before.hasRiver() && (before.riverDir != after.riverDir || (before.extTileFlags & 12) != (after.extTileFlags & 12))))
						++changedArt;
					if((before.extTileFlags & 64) != (after.extTileFlags & 64))
						++changedBit6;
					if((before.extTileFlags & 128) != (after.extTileFlags & 128))
						++changedBit7;
				}
		EXPECT_EQ(changedObjects, 0u);
		EXPECT_EQ(changedTerrain, 0u);
		EXPECT_EQ(changedArt, 0u);
		EXPECT_EQ(changedBit7, 0u);
		std::cout << "VMAP round trip: changed objects=" << changedObjects << ", changed terrain tiles=" << changedTerrain
			<< ", changed art=" << changedArt << ", changed bit 6=" << changedBit6 << ", changed bit 7=" << changedBit7 << std::endl;
		if(const char * outputDir = std::getenv("VCMI_INSPECT_VMAP_OUTPUT_DIR"); outputDir && *outputDir)
		{
			ASSERT_EQ(changedObjects, 0u);
			ASSERT_EQ(changedTerrain, 0u);
			ASSERT_EQ(changedArt, 0u);
			ASSERT_EQ(changedBit7, 0u);
			const boost::filesystem::path directory(outputDir);
			ASSERT_TRUE(boost::filesystem::create_directory(directory)) << "Output directory must not already exist";
			std::ofstream output((directory / "map.vmap").string(), std::ios::binary);
			ASSERT_TRUE(output.is_open());
			output.write(reinterpret_cast<const char *>(archive.getBuffer().data()), archive.getSize());
			output.close();
			ASSERT_TRUE(output.good());
		}
	}
}

namespace
{
/// Supplies external VMAP bytes to the normal in-process game-state initializer.
class ExternalInspectionMapService final : public IMapService
{
public:
	explicit ExternalInspectionMapService(std::vector<ui8> bytes) : bytes(std::move(bytes)) {}
	std::unique_ptr<CMap> loadMap(const ResourcePath &, IGameInfoCallback * cb) const override
	{
		return service.loadMap(bytes.data(), static_cast<int>(bytes.size()), "ExternalGameInspection", "core", "UTF-8", cb);
	}
	std::unique_ptr<CMapHeader> loadMapHeader(const ResourcePath &, bool) const override
	{
		return service.loadMapHeader(bytes.data(), static_cast<int>(bytes.size()), "ExternalGameInspection", "core", "UTF-8");
	}
	std::unique_ptr<CMap> loadMap(const ui8 *, int, const std::string &, const std::string &, const std::string &, IGameInfoCallback * cb) const override
	{
		return loadMap(ResourcePath(""), cb);
	}
	std::unique_ptr<CMapHeader> loadMapHeader(const ui8 *, int, const std::string &, const std::string &, const std::string &) const override
	{
		return loadMapHeader(ResourcePath(""), false);
	}
	void saveMap(const std::unique_ptr<CMap> &, boost::filesystem::path) const override
	{
		throw std::runtime_error("Unexpected save during external map inspection");
	}
private:
	std::vector<ui8> bytes;
	CMapService service;
};

std::shared_ptr<CGameState> reloadExternalInspectionState(const std::shared_ptr<CGameState> & state)
{
	CMemorySerializer stateBytes, headerBytes;
	state->saveGame(stateBytes.oser);
	state->saveHeader(headerBytes.oser);
	const auto path = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-earth-seam-%%%%-%%%%.vsgm1");
	struct TemporarySave
	{
		boost::filesystem::path path;
		~TemporarySave() { boost::filesystem::remove(path); }
	} temporarySave{path};
	CSaveFile save;
	save.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*state).toBytes());
	save.addChunk(SaveFileFormat::CHUNK_HEADER, headerBytes.extractBuffer());
	save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, stateBytes.extractBuffer());
	save.write(path);
	CLoadFile load(path);
	auto restored = std::make_shared<CGameState>();
	restored->preInit(LIBRARY);
	auto chunk = load.openChunk(SaveFileFormat::CHUNK_GAMESTATE, restored.get());
	restored->loadGame(chunk->iser);
	if(!chunk->isFullyRead())
		throw std::runtime_error("External inspection save was not fully read");
	return restored;
}

std::pair<size_t, size_t> compareExternalInspectionFog(const CGameState & before, const CGameState & after)
{
	if(before.getMapSize() != after.getMapSize())
		throw std::runtime_error("External inspection map dimensions changed");
	size_t compared = 0, mismatches = 0;
	const auto size = before.getMapSize();
	for(const auto & [player, _] : before.players)
	{
		if(!player.isValidPlayer())
			continue;
		if(!after.players.contains(player))
			throw std::runtime_error("External inspection player missing after load");
		const auto * first = before.getPlayerTeam(player);
		const auto * second = after.getPlayerTeam(player);
		if(!first || !second)
			throw std::runtime_error("External inspection team missing after load");
		if(first->id != second->id)
			throw std::runtime_error("External inspection player team changed after load");
		for(int z = 0; z < size.z; ++z)
			for(int x = 0; x < size.x; ++x)
				for(int y = 0; y < size.y; ++y)
				{
					const int3 pos(x, y, z);
					mismatches += first->fogOfWarMap[pos] != second->fogOfWarMap[pos];
					++compared;
				}
	}
	return {compared, mismatches};
}

class ExternalInspectionCapture final : public INetworkConnection
{
public:
	void sendPacket(const std::vector<std::byte> & packet) override { bytes = packet; }
	void setAsyncWritesEnabled(bool) override {}
	void close() override {}
	std::vector<std::byte> bytes;
};

class ExternalInspectionReplicaServer final : public GameHandlerTestServer
{
public:
	ExternalInspectionReplicaServer(const std::shared_ptr<CGameState> & state, PlayerColor player)
		: GameHandlerTestServer(state, player), transport(std::make_shared<ExternalInspectionCapture>()),
		  sender(transport), receiver(transport)
	{
		sender.setSerializationVersion(ESerializationVersion::CURRENT);
		receiver.setSerializationVersion(ESerializationVersion::CURRENT);
		sender.setCallback(*state);
	}

	void applyPack(CPackForClient & pack) override
	{
		if(!replicas.empty())
		{
			sender.sendPack(pack);
			for(const auto & replica : replicas)
			{
				receiver.setCallback(*replica);
				auto decoded = receiver.retrievePack(transport->bytes);
				auto * clientPack = dynamic_cast<CPackForClient *>(decoded.get());
				if(!clientPack)
					throw std::runtime_error("External inspection client packet did not decode");
				replica->apply(*clientPack);
				if(dynamic_cast<TryMoveHero *>(clientPack))
					++deliveredMovements;
			}
		}
		GameHandlerTestServer::applyPack(pack);
	}

	std::vector<std::shared_ptr<CGameState>> replicas;
	int deliveredMovements = 0;

private:
	std::shared_ptr<ExternalInspectionCapture> transport;
	GameConnection sender;
	GameConnection receiver;
};

#ifdef ENABLE_NULLKILLER2_AI
class ExternalInspectionAIServer final : public GameHandlerTestServer
{
public:
	ExternalInspectionAIServer(const std::shared_ptr<CGameState> & state, PlayerColor player)
		: GameHandlerTestServer(state, player), state(state)
	{
	}

	void applyPack(CPackForClient & pack) override
	{
		if(const auto * message = dynamic_cast<const SystemMessage *>(&pack))
			messages.push_back(message->text.toString(LIBRARY->staticTexts()));
		if(gateway)
			if(const auto * removal = dynamic_cast<const RemoveObject *>(&pack))
				if(const auto * object = state->getObjInstance(removal->objectID))
					gateway->objectRemoved(object, removal->initiator);
		GameHandlerTestServer::applyPack(pack);
		if(gateway && stopAtBattle && dynamic_cast<const BattleStart *>(&pack))
		{
			++battleStarts;
			gateway->nullkiller->makingTurnInterruption.interruptThread();
			if(onBattleStart)
				onBattleStart();
		}
		if(gateway && forwardDialogs)
		{
			if(const auto * turn = dynamic_cast<const PlayerStartsTurn *>(&pack); forwardTurn && turn)
			{
				if(turn->player == gateway->playerID)
					gateway->yourTurn(turn->questionID);
			}
			if(const auto * visit = dynamic_cast<const HeroVisit *>(&pack); visit && visit->player == gateway->playerID)
			{
				const auto * hero = state->getHero(visit->heroId);
				const auto * object = state->getObjInstance(visit->objId);
				if(hero && (object || !visit->starting))
				{
					gateway->heroVisit(hero, object, visit->starting);
					if(!visit->starting)
						++completedVisits;
				}
			}
			if(const auto * resolved = dynamic_cast<const QuestionResolved *>(&pack))
				gateway->questionResolved(resolved->questionID);
			if(const auto * dialog = dynamic_cast<const BlockingDialog *>(&pack); dialog && dialog->player == gateway->playerID)
			{
				++blockingDialogs;
				gateway->showBlockingDialog(dialog->text.toString(LIBRARY->staticTexts()), dialog->components,
				                            dialog->questionID, dialog->soundID, dialog->selection(),
				                            dialog->cancel(), dialog->safeToAutoaccept());
			}
			if(const auto * window = dynamic_cast<const OpenWindow *>(&pack))
			{
				if(window->window == EOpenWindowMode::RECRUITMENT_FIRST || window->window == EOpenWindowMode::RECRUITMENT_ALL)
				{
					const auto * dwelling = dynamic_cast<const CGDwelling *>(state->getObjInstance(window->object));
					const auto * visitor = dynamic_cast<const CArmedInstance *>(state->getObjInstance(window->visitor));
					if(dwelling && visitor && visitor->getOwner() == gateway->playerID)
					{
						++recruitmentWindows;
						gateway->showRecruitmentDialog(dwelling, visitor,
							window->window == EOpenWindowMode::RECRUITMENT_FIRST ? 0 : -1, window->questionID);
					}
				}
			}
		}
		if(gateway)
			if(const auto * created = dynamic_cast<const NewObject *>(&pack))
			{
				gateway->invalidatePaths();
				if(state->isVisibleFor(created->newObject.get(), gateway->playerID))
					gateway->newObject(created->newObject.get());
			}
		if(gateway)
			if(const auto * movement = dynamic_cast<const TryMoveHero *>(&pack))
			{
				gateway->invalidatePaths();
				gateway->tileRevealed(movement->fowRevealed);
				gateway->heroMoved(*movement, false);
			}
	}

	void sendPack(CPackForClient & pack, GameConnectionID) override
	{
		if(const auto * applied = dynamic_cast<const PackageApplied *>(&pack))
			response = *applied;
	}

	NK2AI::AIGateway * gateway = nullptr;
	std::optional<PackageApplied> response;
	std::vector<std::string> messages;
	bool forwardDialogs = false;
	bool stopAtBattle = false;
	bool forwardTurn = false;
	std::function<void()> onBattleStart;
	std::atomic<int> battleStarts{0};
	int completedVisits = 0;
	int blockingDialogs = 0;
	int recruitmentWindows = 0;

private:
	std::shared_ptr<CGameState> state;
};

class ExternalInspectionAIClient final : public IClient
{
public:
	ExternalInspectionAIClient(const std::shared_ptr<CGameState> & state, PlayerColor player)
		: server(state, player), handler(server, state)
	{
		if(!std::getenv("VCMI_INSPECT_AI_SERVER_TURN"))
			state->actingPlayers.insert(player);
		server.onBattleStart = [this] { terminal.notify_all(); };
	}

	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &,
	                                                        const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor player, bool) override
	{
		std::lock_guard lock(requestMutex);
		request.player = player;
		request.requestID = ++requestID;
		++requests;
		const auto * movement = dynamic_cast<const MoveHero *>(&request);
		const auto * hero = movement ? handler.gameState().getHero(movement->hid) : nullptr;
		const auto origin = hero ? hero->visitablePos() : int3(-1, -1, -1);
		const auto intended = hero && !movement->path.empty()
			? hero->convertToVisitablePos(movement->path.front()) : int3(-1, -1, -1);
		if(movement)
			++moves;
		if(dynamic_cast<const QuestionAnswer *>(&request))
			gateway->requestSent(&request, requestID);
		server.response.reset();
		server.messages.clear();
		auto copy = CMemorySerializer::deepCopy(request);
		handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, *copy);
		if(!server.response || server.response->requestID != requestID)
			throw std::runtime_error("External inspection AI request was not acknowledged");
		if(dynamic_cast<const QuestionAnswer *>(&request) && server.response->result)
			++acceptedAnswers;
		if(movement && !server.response->result)
		{
			++rejectedMoves;
			std::cout << "Rejected AI movement from " << origin.toString() << ", target "
			          << intended.toString()
			          << ", messages=";
			for(const auto & message : server.messages)
				std::cout << " [" << message << "]";
			std::cout << std::endl;
		}
		gateway->requestRealized(&*server.response);
		if(dynamic_cast<const EndTurn *>(&request) && server.response->result)
		{
			++acceptedEndTurns;
			terminal.notify_all();
		}
		return requestID;
	}

	void notify(NK2AI::AIGateway * value)
	{
		gateway = value;
		server.gateway = value;
		server.forwardTurn = std::getenv("VCMI_INSPECT_AI_SERVER_TURN") != nullptr;
		server.forwardDialogs = std::getenv("VCMI_INSPECT_AI_AUTONOMOUS") != nullptr || server.forwardTurn;
		server.stopAtBattle = server.forwardDialogs;
	}

	void startServerTurn()
	{
		std::lock_guard lock(requestMutex);
		handler.turnOrder->setMinSimturnsDuration(0);
		handler.turnOrder->setMaxSimturnsDuration(0);
		for(const auto & [player, info] : handler.gameState().players)
			if(info.status == EPlayerStatus::INGAME)
				handler.turnOrder->addPlayer(player);
		handler.turnOrder->onGameStarted();
		for(const auto & [player, info] : handler.gameState().players)
		{
			if(player == gateway->playerID || !info.isHuman() || info.status != EPlayerStatus::INGAME)
				continue;
			if(!handler.turnOrder->onPlayerEndsTurn(player))
				throw std::runtime_error("Could not advance the preceding human turn");
		}
	}

	bool waitForServerTurn()
	{
		std::unique_lock lock(requestMutex);
		return terminal.wait_for(lock, std::chrono::seconds(15), [this]
		{
			return acceptedEndTurns > 0 || server.battleStarts.load() > 0;
		});
	}

	struct Result
	{
		int requests;
		int moves;
		int rejectedMoves;
		int battles;
		int completedVisits;
		int blockingDialogs;
		int recruitmentWindows;
		int acceptedAnswers;
		int acceptedEndTurns;
		bool playerStillActing;
		bool anotherPlayerActing;
		bool hasActivities;
	};

	Result result(PlayerColor player)
	{
		std::lock_guard lock(requestMutex);
		const bool anotherPlayerActing = std::ranges::any_of(handler.gameState().players,
			[&](const auto & entry) { return entry.first != player && handler.turnOrder->isPlayerMakingTurn(entry.first); });
		return {requests, moves, rejectedMoves, server.battleStarts.load(), server.completedVisits,
		        server.blockingDialogs, server.recruitmentWindows, acceptedAnswers, acceptedEndTurns,
		        handler.turnOrder->isPlayerMakingTurn(player), anotherPlayerActing,
		        handler.activities->topActivity(player) != nullptr};
	}

	int moves = 0;
	int rejectedMoves = 0;
	int requests = 0;
	int acceptedAnswers = 0;
	int acceptedEndTurns = 0;

private:
	ExternalInspectionAIServer server;
	CGameHandler handler;
	NK2AI::AIGateway * gateway = nullptr;
	int requestID = 0;
	std::recursive_mutex requestMutex;
	std::condition_variable_any terminal;
};
#endif
}

TEST(MapFormat, ChecksExternalMapModRequirements)
{
	const char * path = std::getenv("VCMI_INSPECT_START_VMAP");
	if(!path || !*path)
		GTEST_SKIP() << "Set VCMI_INSPECT_START_VMAP";
	std::ifstream file(path, std::ios::binary);
	ASSERT_TRUE(file.is_open());
	std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	ExternalInspectionMapService service(std::move(bytes));
	const auto header = service.loadMapHeader(ResourcePath("inspection"), false);
	ASSERT_NE(header, nullptr);
	const auto missing = CMapService::verifyMapHeaderMods(*header);
	std::cout << "Declared map mods=" << header->mods.size() << ", missing=" << missing.size() << std::endl;
	for(const auto & [name, _] : missing)
		std::cout << "Missing map mod: " << name << std::endl;
	if(std::getenv("VCMI_INSPECT_EXPECT_NO_MISSING_MODS"))
		EXPECT_TRUE(missing.empty());
	if(const char * expected = std::getenv("VCMI_INSPECT_EXPECT_MISSING_MODS"); expected && *expected)
	{
		std::vector<std::string> names;
		boost::split(names, expected, boost::is_any_of(","));
		for(const auto & name : names)
			EXPECT_TRUE(missing.contains(name)) << name;
	}
}

TEST(MapFormat, InitializesExternalToroidalMapAndChecksPortalPairs)
{
	const char * path = std::getenv("VCMI_INSPECT_START_VMAP");
	const char * pairList = std::getenv("VCMI_INSPECT_PORTAL_PAIRS");
	if(!path || !*path || !pairList || !*pairList)
		GTEST_SKIP() << "Set VCMI_INSPECT_START_VMAP and VCMI_INSPECT_PORTAL_PAIRS";
	std::ifstream file(path, std::ios::binary);
	ASSERT_TRUE(file.is_open());
	std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	ExternalInspectionMapService service(std::move(bytes));
	auto header = service.loadMapHeader(ResourcePath("inspection"), false);
	ASSERT_NE(header, nullptr);
	ASSERT_TRUE(header->toroidal);
	StartInfo start;
	start.mapname = "inspection";
	start.difficulty = static_cast<ui8>(EMapDifficulty::EASY);
	start.mode = EStartMode::NEW_GAME;
	std::optional<int> aiSlot;
	if(const char * configured = std::getenv("VCMI_INSPECT_AI_SLOT"); configured && *configured)
	{
		aiSlot = std::stoi(configured);
		ASSERT_GE(*aiSlot, 0);
		ASSERT_LT(*aiSlot, static_cast<int>(header->players.size()));
		ASSERT_TRUE(header->players[*aiSlot].canComputerPlay);
	}
	for(int index = 0; index < static_cast<int>(header->players.size()); ++index)
	{
		const auto & info = header->players[index];
		if(!info.canHumanPlay && !info.canComputerPlay)
			continue;
		auto & player = start.playerInfos[PlayerColor(index)];
		player.color = PlayerColor(index);
		if(!aiSlot || *aiSlot != index)
			player.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(index));
		player.name = "Inspection";
		player.castle = info.defaultCastle();
		player.hero = info.defaultHero();
		player.bonus = PlayerStartingBonus::GOLD;
	}
	auto state = std::make_shared<CGameState>();
	state->preInit(LIBRARY);
	GameRandomizer randomizer(*state);
	if(const char * seed = std::getenv("VCMI_INSPECT_AI_SEED"); seed && *seed)
		randomizer.setSeed(std::stoi(seed));
	Load::ProgressAccumulator progress;
	state->init(&service, &start, randomizer, progress, false);
	if(aiSlot)
		ASSERT_FALSE(state->players.at(PlayerColor(*aiSlot)).isHuman());
	auto & map = state->getMap();
	ASSERT_TRUE(map.toroidal);
	if(std::getenv("VCMI_INSPECT_EARTH_INTERACTIVE_OVERLAPS"))
	{
		int shared = 0;
		for(int z = 0; z < map.levels(); ++z)
			for(int x = 0; x < map.width; ++x)
				for(int y = 0; y < map.height; ++y)
				{
					const int3 pos(x, y, z);
					std::vector<std::string> names;
					for(const auto & id : map.getTile(pos).visitableObjects)
						if(const auto * object = map.getObject(id); object && object->ID != Obj::HERO && object->ID != Obj::BOAT)
							names.push_back(object->instanceName);
					if(names.size() < 2)
						continue;
					++shared;
					std::cout << "Shared non-hero visitable tile=" << pos.toString();
					for(const auto & name : names)
						std::cout << " " << name;
					std::cout << std::endl;
				}
		std::cout << "Shared non-hero visitable tiles=" << shared << std::endl;
		EXPECT_EQ(shared, 0);
	}
	if(std::getenv("VCMI_INSPECT_EARTH_BANK_OVERLAP"))
	{
		const auto heroEntry = map.instanceNames.find("randomHero_7305");
		const char * firstName = std::getenv("VCMI_INSPECT_EARTH_OVERLAP_TARGET");
		const char * secondName = std::getenv("VCMI_INSPECT_EARTH_OVERLAP_OBSTACLE");
		const auto firstEntry = map.instanceNames.find(firstName ? firstName : "creatureBank_10789");
		const auto secondEntry = map.instanceNames.find(secondName ? secondName : "creatureBank_11365");
		ASSERT_NE(heroEntry, map.instanceNames.end());
		ASSERT_NE(firstEntry, map.instanceNames.end());
		ASSERT_NE(secondEntry, map.instanceNames.end());
		auto * hero = dynamic_cast<CGHeroInstance *>(heroEntry->second.get());
		ASSERT_NE(hero, nullptr);
		const bool reverse = std::getenv("VCMI_INSPECT_EARTH_BANK_SECOND") != nullptr;
		auto * first = (reverse ? secondEntry : firstEntry)->second.get();
		auto * second = (reverse ? firstEntry : secondEntry)->second.get();
		auto * team = state->getPlayerTeam(hero->getOwner());
		ASSERT_NE(team, nullptr);
		for(int x = 0; x < map.width; ++x)
			for(int y = 0; y < map.height; ++y)
				team->fogOfWarMap[int3(x, y, 0)] = 1;
		const auto target = first->visitablePos();
		std::cout << "Bank entrances first=" << target.toString()
		          << " second=" << second->visitablePos().toString() << std::endl;
		GameHandlerTestServer server(state, hero->getOwner());
		CGameHandler handler(server, state);
		if(std::getenv("VCMI_INSPECT_EARTH_REMOVE_OVERLAP_EARLY"))
		{
			ASSERT_NE(std::getenv("VCMI_INSPECT_EARTH_BANK_VISIT"), nullptr);
			ASSERT_FALSE(std::getenv("VCMI_INSPECT_EARTH_REMOVE_OVERLAP_OBSTACLE"));
			ASSERT_TRUE(handler.removeObject(second, hero->getOwner()));
		}
		std::optional<int3> origin;
		for(const auto & dir : int3::getDirs())
		{
			const auto candidate = map.getTopology().canonicalize(target + dir);
			if(candidate && map.getTile(*candidate).isLand() && !map.getTile(*candidate).blocked()
			   && !map.getTile(*candidate).visitable())
			{
				origin = *candidate;
				break;
			}
		}
		if(!origin)
			for(const auto & dir : int3::getDirs())
			{
				const auto candidate = map.getTopology().canonicalize(target + dir);
				if(!candidate)
					continue;
				const auto & tile = map.getTile(*candidate);
				std::cout << "Overlap neighbor=" << candidate->toString() << " land=" << tile.isLand()
				          << " blocked=" << tile.blocked() << " visitable=" << tile.visitable();
				for(const auto & id : tile.blockingObjects)
					if(const auto * object = map.getObject(id))
						std::cout << " blocker=" << object->instanceName;
				std::cout << std::endl;
			}
		ASSERT_TRUE(origin.has_value());
		if(std::getenv("VCMI_INSPECT_EARTH_REMOVE_OVERLAP_EARLY"))
			for(const auto & dir : int3::getDirs())
			{
				const auto neighbor = map.getTopology().canonicalize(*origin + dir);
				if(!neighbor)
					continue;
				const auto & tile = map.getTile(*neighbor);
				std::cout << "Opened approach neighbor=" << neighbor->toString() << " land=" << tile.isLand()
				          << " blocked=" << tile.blocked() << " visitable=" << tile.visitable()
				          << " move=" << map.canMoveBetween(*neighbor, *origin) << std::endl;
			}
		ChangeObjPos place;
		place.objid = hero->id;
		place.nPos = *origin;
		state->apply(place);
		ASSERT_EQ(hero->visitablePos(), *origin);
		hero->setMovementPoints(3000);
		const auto reachable = [&]()
		{
			CPathsInfo paths(state->getMapSize(), hero);
			auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *state, hero);
			CPathfinder(*state, config).calculatePaths();
			return paths.getPathInfo(target, EPathfindingLayer::LAND)->reachable();
		};
		std::cout << "Bank origin=" << origin->toString()
		          << " target blocked=" << map.getTile(target).blocked()
		          << " reachable before removal=" << reachable() << std::endl;
		if(std::getenv("VCMI_INSPECT_EARTH_BANK_VISIT"))
		{
			std::optional<ArtifactID> expectedArtifact;
			if(std::getenv("VCMI_INSPECT_EARTH_EXPECT_ARTIFACT"))
			{
				const auto * artifact = dynamic_cast<const CGArtifact *>(first);
				ASSERT_NE(artifact, nullptr);
				expectedArtifact = artifact->getArtifactType();
				ASSERT_FALSE(hero->hasArt(*expectedArtifact));
			}
			if(std::getenv("VCMI_INSPECT_EARTH_REMOVE_OVERLAP_OBSTACLE"))
				ASSERT_TRUE(handler.removeObject(second, hero->getOwner()));
			const auto targetId = first->id;
			const auto top = map.getTile(target).topVisitableObj();
			std::cout << "Target top object=" << (map.getObject(top) ? map.getObject(top)->instanceName : "none") << std::endl;
			ASSERT_EQ(top, first->id);
			CPathsInfo paths(state->getMapSize(), hero);
			auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *state, hero);
			CPathfinder(*state, config).calculatePaths();
			CGPath route;
			ASSERT_TRUE(paths.getPath(route, target, EPathfindingLayer::LAND));
			ASSERT_GE(route.nodes.size(), 2u);
			state->actingPlayers.insert(hero->getOwner());
			for(size_t index = route.nodes.size() - 1; index > 0; --index)
			{
				const auto pos = route.nodes[index - 1].coord;
				const bool accepted = handler.moveHero(hero->id, hero->convertFromVisitablePos(pos), EMovementMode::STANDARD, false, hero->getOwner(), EPathfindingLayer::LAND);
				std::cout << "Bank route step=" << pos.toString() << " accepted=" << accepted
				          << " hero=" << hero->visitablePos().toString()
				          << " battle=" << (state->getBattle(hero->getOwner()) != nullptr)
				          << " activity=" << (handler.activities->topActivity(hero->getOwner()) != nullptr) << std::endl;
				ASSERT_TRUE(accepted);
				if(index == 1)
				{
					if(firstName)
					{
						std::cout << "Overlap target remains=" << (map.getObject(targetId) != nullptr) << std::endl;
						if(std::getenv("VCMI_INSPECT_EARTH_EXPECT_TARGET_REMOVED"))
							EXPECT_EQ(map.getObject(targetId), nullptr);
						if(expectedArtifact)
							EXPECT_TRUE(hero->hasArt(*expectedArtifact));
						if(std::getenv("VCMI_INSPECT_EARTH_EXPECT_TELEPORT"))
						{
							const auto activity = handler.activities->topActivity(hero->getOwner());
							ASSERT_NE(activity, nullptr);
							EXPECT_EQ(activity->getType(), ActivityType::TeleportDialog);
						}
					}
					else
					{
						EXPECT_EQ(hero->visitablePos(), target);
						EXPECT_NE(handler.activities->topActivity(hero->getOwner()), nullptr);
					}
				}
				if(state->getBattle(hero->getOwner()) || handler.activities->topActivity(hero->getOwner()))
				{
					ASSERT_EQ(index, 1u) << "Unexpected interaction before the bank";
					break;
				}
			}
			if(std::getenv("VCMI_INSPECT_EARTH_OVERLAP_BUILD_BOAT"))
			{
				ASSERT_EQ(hero->visitablePos(), target);
				const auto * shipyard = dynamic_cast<const CGShipyard *>(map.getObject(targetId));
				ASSERT_NE(shipyard, nullptr);
				const auto boatPos = shipyard->bestLocation();
				const auto status = shipyard->shipyardStatus();
				std::cout << "Overlap shipyard boat location=" << boatPos.toString() << " status=" << static_cast<int>(status) << std::endl;
				ASSERT_EQ(status, IBoatGenerator::GOOD);
				ASSERT_TRUE(handler.buildBoat(shipyard->id, hero->getOwner()));
				EXPECT_TRUE(std::ranges::any_of(map.getTile(boatPos).visitableObjects, [&](const auto & id)
				{
					const auto * object = map.getObject(id);
					return object && object->ID == Obj::BOAT;
				}));
			}
		}
		else
		{
			ASSERT_TRUE(handler.removeObject(second, hero->getOwner()));
			std::cout << "Bank target blocked after removal=" << map.getTile(target).blocked()
			          << " reachable after removal=" << reachable() << std::endl;
		}
	}
	if(std::getenv("VCMI_INSPECT_EARTH_START_CONNECTIVITY"))
	{
		const auto entry = map.instanceNames.find("randomHero_7305");
		ASSERT_NE(entry, map.instanceNames.end());
		auto * hero = dynamic_cast<CGHeroInstance *>(entry->second.get());
		ASSERT_NE(hero, nullptr);
		ASSERT_EQ(hero->getOwner(), PlayerColor(0));
		auto * team = state->getPlayerTeam(hero->getOwner());
		ASSERT_NE(team, nullptr);
		for(int x = 0; x < map.width; ++x)
			for(int y = 0; y < map.height; ++y)
				team->fogOfWarMap[int3(x, y, 0)] = 1;
		CPathsInfo paths(state->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *state, hero);
		CPathfinder(*state, config).calculatePaths();
		for(const char * name : {"subterraneanGate_5430", "subterraneanGate_10528", "subterraneanGate_5431", "subterraneanGate_10533"})
		{
			const auto target = map.instanceNames.find(name);
			ASSERT_NE(target, map.instanceNames.end()) << name;
			const auto pos = target->second->visitablePos();
			const auto * node = paths.getNode(pos, EPathfindingLayer::LAND);
			std::cout << "Revealed start connectivity " << name << " at=" << pos.toString()
			          << " reachable=" << node->reachable() << " turns=" << static_cast<int>(node->turns) << std::endl;
		}
		const int3 road(0, 425, 0);
		const auto * node = paths.getNode(road, EPathfindingLayer::LAND);
		std::cout << "Revealed start connectivity seam road at=" << road.toString()
		          << " reachable=" << node->reachable() << " turns=" << static_cast<int>(node->turns) << std::endl;
		int reachableTransport = 0;
		for(const auto & object : map.objects)
		{
			if(!object || (object->ID != Obj::SHIPYARD && object->ID != Obj::BOAT))
				continue;
			const auto pos = object->visitablePos();
			const auto * land = paths.getNode(pos, EPathfindingLayer::LAND);
			const auto * sail = paths.getNode(pos, EPathfindingLayer::SAIL);
			if(land->reachable() || sail->reachable())
			{
				++reachableTransport;
				std::cout << "Revealed start reachable transport " << object->instanceName
				          << " at=" << pos.toString() << " land=" << land->reachable()
				          << " sail=" << sail->reachable() << std::endl;
			}
		}
		std::cout << "Revealed start reachable transport count=" << reachableTransport << std::endl;
		const auto yard = map.instanceNames.find("shipyard_5987");
		ASSERT_NE(yard, map.instanceNames.end());
		const auto yardPos = yard->second->visitablePos();
		std::cout << "Selected shipyard " << yard->first << " at=" << yardPos.toString() << std::endl;
		for(const auto & direction : int3::getDirs())
		{
			const auto neighbor = map.getTopology().canonicalize(yardPos + direction);
			if(!neighbor)
				continue;
			const auto * candidate = paths.getNode(*neighbor, EPathfindingLayer::LAND);
			std::cout << "Shipyard neighbor " << neighbor->toString() << " land=" << map.getTile(*neighbor).isLand()
			          << " blocked=" << map.getTile(*neighbor).blocked() << " reachable=" << candidate->reachable()
			          << " approach=" << map.canMoveBetween(*neighbor, yardPos)
			          << " guard=" << state->guardingCreaturePosition(*neighbor).toString() << std::endl;
		}
		const auto start = hero->visitablePos();
		const auto indexOf = [&](const int3 & pos) { return pos.y * map.width + pos.x; };
		const auto posOf = [&](int index) { return int3(index % map.width, index / map.width, 0); };
		std::vector<int> parent(map.width * map.height, -1);
		std::vector<int> queue{indexOf(start)};
		parent[queue.front()] = queue.front();
		int approach = -1;
		for(size_t index = 0; index < queue.size() && approach < 0; ++index)
		{
			const auto current = posOf(queue[index]);
			for(const auto & direction : int3::getDirs())
			{
				const auto candidate = map.getTopology().canonicalize(current + direction);
				if(!candidate || *candidate == yardPos)
					continue;
				const auto & tile = map.getTile(*candidate);
				const auto next = indexOf(*candidate);
				if(parent[next] != -1 || !tile.isLand() || tile.blocked() || tile.visitable()
				   || !map.canMoveBetween(current, *candidate))
					continue;
				parent[next] = queue[index];
				queue.push_back(next);
				for(const auto & lastStep : int3::getDirs())
					if(map.getTopology().canonicalize(*candidate + lastStep) == yardPos
					   && map.canMoveBetween(*candidate, yardPos))
						approach = next;
			}
		}
		if(approach < 0)
			std::cout << "No free geometric shipyard approach ignoring guards" << std::endl;
		else
		{
			std::vector<int> route;
			for(int at = approach; at != queue.front(); at = parent[at])
				route.push_back(at);
			std::reverse(route.begin(), route.end());
			if(std::getenv("VCMI_INSPECT_EARTH_SHIPYARD_ROUTE"))
			{
				for(const auto & object : map.objects)
					if(auto * creature = dynamic_cast<CGCreature *>(object.get()))
					{
						creature->agression = 127;
						creature->neverFlees = true;
					}
				GameHandlerTestServer routeServer(state, hero->getOwner());
				CGameHandler routeHandler(routeServer, state);
				hero->setMovementPoints(30000);
				state->actingPlayers.insert(hero->getOwner());
				int battles = 0;
				for(const int step : route)
				{
					const auto destination = posOf(step);
					for(int attempt = 0; attempt < 4 && hero->visitablePos() != destination; ++attempt)
					{
						ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, hero->getOwner(), EPathfindingLayer::LAND)) << destination.toString();
						if(state->getBattle(hero->getOwner()))
						{
							++battles;
							routeHandler.battles->cheatBattleVictory(hero->getOwner());
							const auto result = routeHandler.activities->topActivity(hero->getOwner());
							ASSERT_NE(result, nullptr);
							ASSERT_EQ(result->getType(), ActivityType::BattleDialog);
							ASSERT_EQ(routeHandler.activities->submitReply(result->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
							for(int limit = 0; limit < 16; ++limit)
							{
								const auto prompt = routeHandler.activities->topActivity(hero->getOwner());
								if(!prompt || (prompt->getType() != ActivityType::HeroLevelUpDialog && prompt->getType() != ActivityType::CommanderLevelUpDialog))
									break;
								ASSERT_EQ(routeHandler.activities->submitReply(prompt->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
							}
						}
						ASSERT_EQ(routeHandler.activities->topActivity(hero->getOwner()), nullptr) << destination.toString();
					}
					ASSERT_EQ(hero->visitablePos(), destination);
				}
				std::cout << "Shipyard route reached approach=" << hero->visitablePos().toString()
				          << " forced victories=" << battles << std::endl;
				ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(yardPos), EMovementMode::STANDARD, false, hero->getOwner(), EPathfindingLayer::LAND));
				std::cout << "Shipyard visit hero=" << hero->visitablePos().toString()
				          << " activity=" << (routeHandler.activities->topActivity(hero->getOwner()) != nullptr) << std::endl;
				if(std::getenv("VCMI_INSPECT_EARTH_SHIPYARD_BOAT"))
				{
					auto * shipyard = dynamic_cast<CGShipyard *>(yard->second.get());
					ASSERT_NE(shipyard, nullptr);
					const auto boatPos = shipyard->bestLocation();
					std::cout << "Shipyard boat location=" << boatPos.toString()
					          << " status=" << static_cast<int>(shipyard->shipyardStatus()) << std::endl;
					ASSERT_EQ(shipyard->shipyardStatus(), IBoatGenerator::GOOD);
					ASSERT_TRUE(routeHandler.buildBoat(shipyard->id, hero->getOwner()));
					CPathsInfo boatPaths(state->getMapSize(), hero);
					auto boatConfig = std::make_shared<SingleHeroPathfinderConfig>(boatPaths, *state, hero);
					CPathfinder(*state, boatConfig).calculatePaths();
					CGPath boatRoute;
					ASSERT_TRUE(boatPaths.getPath(boatRoute, boatPos, EPathfindingLayer::SAIL));
					ASSERT_GE(boatRoute.nodes.size(), 2u);
					for(size_t index = boatRoute.nodes.size() - 1; index > 0; --index)
					{
						const auto & step = boatRoute.nodes[index - 1];
						ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(step.coord), EMovementMode::STANDARD, false, hero->getOwner(), step.layer));
					}
					ASSERT_TRUE(hero->inBoat());
					std::cout << "Shipyard boat boarded at=" << hero->visitablePos().toString() << std::endl;
					if(std::getenv("VCMI_INSPECT_EARTH_SHIPYARD_SEAM"))
					{
						hero->setMovementPoints(30000);
						CPathsInfo seamPaths(state->getMapSize(), hero);
						auto seamConfig = std::make_shared<SingleHeroPathfinderConfig>(seamPaths, *state, hero);
						CPathfinder(*state, seamConfig).calculatePaths();
						CGPath seamRoute;
						const bool reachable = seamPaths.getPath(seamRoute, road, EPathfindingLayer::LAND);
						std::cout << "Road route from boat reachable=" << reachable
						          << " nodes=" << seamRoute.nodes.size() << std::endl;
						for(const auto layer : {EPathfindingLayer::SAIL, EPathfindingLayer::LAND})
						{
							int closestDistance = std::numeric_limits<int>::max();
							int3 closest(-1, -1, -1);
							for(int x = 0; x < map.width; ++x)
								for(int y = 0; y < map.height; ++y)
								{
									const int3 pos(x, y, 0);
									if(!seamPaths.getPathInfo(pos, layer)->reachable())
										continue;
									const int dx = std::min(std::abs(x - road.x), map.width - std::abs(x - road.x));
									const int dy = std::min(std::abs(y - road.y), map.height - std::abs(y - road.y));
									if(dx * dx + dy * dy < closestDistance)
									{
										closestDistance = dx * dx + dy * dy;
										closest = pos;
									}
								}
							std::cout << "Closest reachable " << (layer == EPathfindingLayer::SAIL ? "water" : "land")
							          << " to road=" << closest.toString() << " distanceSquared=" << closestDistance << std::endl;
						}
						for(int y = 425; y <= 431; ++y)
							for(int x : {251, 0, 1, 2})
							{
								const int3 pos(x, y, 0);
								const auto & tile = map.getTile(pos);
								const auto guard = state->guardingCreaturePosition(pos);
								std::cout << "Road vicinity=" << pos.toString() << " land=" << tile.isLand()
								          << " blocked=" << tile.blocked() << " visitable=" << tile.visitable()
								          << " reachable=" << seamPaths.getPathInfo(pos, EPathfindingLayer::LAND)->reachable()
								          << " guard=" << guard.toString();
								for(const auto & id : tile.blockingObjects)
									if(const auto * object = map.getObject(id))
										std::cout << " blocker=" << object->instanceName;
								if(map.isInTheMap(guard))
									for(const auto & id : map.getTile(guard).visitableObjects)
										if(const auto * creature = dynamic_cast<const CGCreature *>(map.getObject(id)))
											std::cout << " creature=" << creature->instanceName;
								std::cout << std::endl;
							}
						if(std::getenv("VCMI_INSPECT_EARTH_ROAD_GUARD_REMOVAL"))
						{
							for(const char * name : {"mountain_7569", "yuccaTrees_7568"})
							{
								const auto object = map.instanceNames.find(name);
								if(std::getenv("VCMI_INSPECT_EARTH_ROAD_PREPARED"))
									ASSERT_EQ(object, map.instanceNames.end()) << name;
								else
								{
									ASSERT_NE(object, map.instanceNames.end()) << name;
									ASSERT_TRUE(routeHandler.removeObject(object->second.get(), hero->getOwner()));
								}
							}
							CPathsInfo afterDecorations(state->getMapSize(), hero);
							auto decorationConfig = std::make_shared<SingleHeroPathfinderConfig>(afterDecorations, *state, hero);
							CPathfinder(*state, decorationConfig).calculatePaths();
							std::cout << "Road reachable after test decoration removal="
							          << afterDecorations.getPathInfo(road, EPathfindingLayer::LAND)->reachable() << std::endl;
							const auto entry = map.instanceNames.find("randomMonsterLevel3_6689");
							ASSERT_NE(entry, map.instanceNames.end());
							CGPath guardRoute;
							const bool guardReachable = afterDecorations.getPath(guardRoute, entry->second->visitablePos(), EPathfindingLayer::LAND);
							std::cout << "Road guard reachable after decoration removal=" << guardReachable
							          << " nodes=" << guardRoute.nodes.size() << std::endl;
							if(std::getenv("VCMI_INSPECT_EARTH_ROAD_GUARD_ROUTE"))
							{
								ASSERT_TRUE(guardReachable);
								ASSERT_GE(guardRoute.nodes.size(), 2u);
								bool reachedRoad = false;
								for(size_t index = guardRoute.nodes.size() - 1; index > 0; --index)
								{
									const auto & step = guardRoute.nodes[index - 1];
									hero->setMovementPoints(30000);
									const bool accepted = routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(step.coord), EMovementMode::STANDARD, false, hero->getOwner(), step.layer);
									if(!accepted || state->getBattle(hero->getOwner()) || routeHandler.activities->topActivity(hero->getOwner()))
									{
										std::cout << "Guard route stopped at=" << hero->visitablePos().toString()
										          << " requested=" << step.coord.toString() << " accepted=" << accepted
										          << " battle=" << (state->getBattle(hero->getOwner()) != nullptr)
										          << " activity=" << (routeHandler.activities->topActivity(hero->getOwner()) != nullptr)
										          << " remaining nodes=" << index << std::endl;
										const auto * battle = state->getBattle(hero->getOwner());
										ASSERT_NE(battle, nullptr);
										const auto * guard = dynamic_cast<const CGCreature *>(entry->second.get());
										ASSERT_NE(guard, nullptr);
										const auto guardID = guard->id;
										ASSERT_EQ(battle->getSideArmy(BattleSide::DEFENDER), guard);
										routeHandler.battles->cheatBattleVictory(hero->getOwner());
										const auto result = routeHandler.activities->topActivity(hero->getOwner());
										ASSERT_NE(result, nullptr);
										ASSERT_EQ(result->getType(), ActivityType::BattleDialog);
										ASSERT_EQ(routeHandler.activities->submitReply(result->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
										for(int limit = 0; limit < 16; ++limit)
										{
											const auto prompt = routeHandler.activities->topActivity(hero->getOwner());
											if(!prompt || (prompt->getType() != ActivityType::HeroLevelUpDialog && prompt->getType() != ActivityType::CommanderLevelUpDialog))
												break;
											ASSERT_EQ(routeHandler.activities->submitReply(prompt->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
										}
										ASSERT_EQ(map.getObject(guardID), nullptr);
										hero->setMovementPoints(30000);
										CPathsInfo toRoad(state->getMapSize(), hero);
										auto config = std::make_shared<SingleHeroPathfinderConfig>(toRoad, *state, hero);
										CPathfinder(*state, config).calculatePaths();
										CGPath finalRoute;
										ASSERT_TRUE(toRoad.getPath(finalRoute, road, EPathfindingLayer::LAND));
										ASSERT_GE(finalRoute.nodes.size(), 2u);
										for(size_t last = finalRoute.nodes.size() - 1; last > 0; --last)
										{
											const auto & next = finalRoute.nodes[last - 1];
											ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(next.coord), EMovementMode::STANDARD, false, hero->getOwner(), next.layer));
											ASSERT_EQ(hero->visitablePos(), next.coord);
										}
										ASSERT_EQ(hero->visitablePos(), road);
										const int3 across(251, 425, 0);
										ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(across), EMovementMode::STANDARD, false, hero->getOwner(), EPathfindingLayer::LAND));
										ASSERT_EQ(hero->visitablePos(), across);
										ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(road), EMovementMode::STANDARD, false, hero->getOwner(), EPathfindingLayer::LAND));
										ASSERT_EQ(hero->visitablePos(), road);
										if(std::getenv("VCMI_INSPECT_EARTH_ROAD_GATE_CONNECTIVITY"))
										{
											hero->setMovementPoints(30000);
											CPathsInfo fromRoad(state->getMapSize(), hero);
											auto roadConfig = std::make_shared<SingleHeroPathfinderConfig>(fromRoad, *state, hero);
											CPathfinder(*state, roadConfig).calculatePaths();
											std::vector<int> gateParent(map.width * map.height, -1);
											std::vector<int> gateQueue{indexOf(road)};
											gateParent[gateQueue.front()] = gateQueue.front();
											for(size_t i = 0; i < gateQueue.size(); ++i)
											{
												const auto current = posOf(gateQueue[i]);
												for(const auto & direction : int3::getDirs())
												{
													const auto next = map.getTopology().canonicalize(current + direction);
													if(!next || gateParent[indexOf(*next)] != -1)
														continue;
													const auto & tile = map.getTile(*next);
													if(!tile.isLand() || tile.blocked() || tile.visitable() || !map.canMoveBetween(current, *next))
														continue;
													gateParent[indexOf(*next)] = gateQueue[i];
													gateQueue.push_back(indexOf(*next));
												}
											}
											for(const char * name : {"subterraneanGate_5430", "subterraneanGate_10528", "subterraneanGate_5431", "subterraneanGate_10533"})
											{
												const auto target = map.instanceNames.find(name);
												ASSERT_NE(target, map.instanceNames.end()) << name;
												CGPath gateRoute;
												const bool found = fromRoad.getPath(gateRoute, target->second->visitablePos(), EPathfindingLayer::LAND);
												std::cout << "Gate from road " << name << " reachable=" << found << " nodes=" << gateRoute.nodes.size() << std::endl;
												const auto targetPos = target->second->visitablePos();
												int approach = -1;
												for(const auto & direction : int3::getDirs())
												{
													const auto neighbor = map.getTopology().canonicalize(targetPos + direction);
													if(neighbor && gateParent[indexOf(*neighbor)] != -1 && map.canMoveBetween(*neighbor, targetPos))
													{
														approach = indexOf(*neighbor);
														break;
													}
												}
												std::cout << "Gate free geometric approach " << name << " found=" << (approach >= 0) << std::endl;
												if(approach >= 0)
												{
													std::cout << "Gate approach detail " << name << " at=" << posOf(approach).toString()
													          << " reachable=" << fromRoad.getNode(posOf(approach), EPathfindingLayer::LAND)->reachable()
													          << " target guard=" << state->guardingCreaturePosition(targetPos).toString() << std::endl;
													std::vector<int> routeToGate;
													for(int at = approach; at != gateQueue.front(); at = gateParent[at])
														routeToGate.push_back(at);
													std::reverse(routeToGate.begin(), routeToGate.end());
													for(const int step : routeToGate)
														if(!fromRoad.getNode(posOf(step), EPathfindingLayer::LAND)->reachable())
														{
															const auto pos = posOf(step);
															std::cout << "Gate first unreachable geometric step " << name << " at=" << pos.toString()
															          << " guard=" << state->guardingCreaturePosition(pos).toString() << std::endl;
															break;
														}
												}
											}
											if(std::getenv("VCMI_INSPECT_EARTH_ROAD_GATE_ROUTE"))
											{
												const int3 approach(3, 417, 0);
												CGPath approachRoute;
												ASSERT_TRUE(fromRoad.getPath(approachRoute, approach, EPathfindingLayer::LAND));
												ASSERT_GE(approachRoute.nodes.size(), 2u);
												for(size_t index = approachRoute.nodes.size() - 1; index > 0; --index)
												{
													const auto & step = approachRoute.nodes[index - 1];
													ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(step.coord), EMovementMode::STANDARD, false, hero->getOwner(), step.layer));
													std::cout << "Gate approach executed step=" << step.coord.toString() << " hero=" << hero->visitablePos().toString()
													          << " battle=" << (state->getBattle(hero->getOwner()) != nullptr)
													          << " activity=" << (routeHandler.activities->topActivity(hero->getOwner()) != nullptr) << std::endl;
													if(state->getBattle(hero->getOwner()) || routeHandler.activities->topActivity(hero->getOwner()))
														break;
													ASSERT_EQ(hero->visitablePos(), step.coord);
												}
												std::cout << "Gate approach outcome hero=" << hero->visitablePos().toString()
												          << " reached=" << (hero->visitablePos() == approach) << std::endl;
												if(std::getenv("VCMI_INSPECT_EARTH_ROAD_GATE_TRAVERSE"))
										{
											ASSERT_EQ(hero->visitablePos(), approach);
												ASSERT_NE(state->getBattle(hero->getOwner()), nullptr);
												routeHandler.battles->cheatBattleVictory(hero->getOwner());
											const auto result = routeHandler.activities->topActivity(hero->getOwner());
												ASSERT_NE(result, nullptr);
												ASSERT_EQ(result->getType(), ActivityType::BattleDialog);
												ASSERT_EQ(routeHandler.activities->submitReply(result->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
												for(int limit = 0; limit < 16; ++limit)
												{
													const auto prompt = routeHandler.activities->topActivity(hero->getOwner());
													if(!prompt || (prompt->getType() != ActivityType::HeroLevelUpDialog && prompt->getType() != ActivityType::CommanderLevelUpDialog))
														break;
													ASSERT_EQ(routeHandler.activities->submitReply(prompt->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
												}
												const auto entrance = map.instanceNames.find("subterraneanGate_5431");
												const auto exit = map.instanceNames.find("subterraneanGate_10533");
												ASSERT_NE(entrance, map.instanceNames.end());
												ASSERT_NE(exit, map.instanceNames.end());
												hero->setMovementPoints(30000);
												ASSERT_TRUE(routeHandler.moveHero(hero->id, hero->convertFromVisitablePos(entrance->second->visitablePos()), EMovementMode::STANDARD, false, hero->getOwner(), EPathfindingLayer::LAND));
												const auto dialog = routeHandler.activities->topActivity(hero->getOwner());
												ASSERT_NE(dialog, nullptr);
												const auto teleport = std::dynamic_pointer_cast<TeleportDialogActivity>(dialog);
												ASSERT_NE(teleport, nullptr);
												const auto selected = std::ranges::find_if(teleport->td.exits, [&](const auto & item) { return item.first == exit->second->id; });
												ASSERT_NE(selected, teleport->td.exits.end());
												const auto answer = static_cast<ui32>(std::distance(teleport->td.exits.begin(), selected));
												ASSERT_EQ(routeHandler.activities->submitReply(dialog->getActiveQuestionID(), hero->getOwner(), answer), ReplyOutcome::Accepted);
												ASSERT_EQ(hero->visitablePos(), exit->second->visitablePos());
												std::cout << "Reached paired gate exit=" << hero->visitablePos().toString() << std::endl;
											}
											}
										}
										reachedRoad = true;
										std::cout << "Reached and crossed seam road in both directions=" << road.toString() << std::endl;
										break;
									}
								}
								ASSERT_TRUE(reachedRoad);
							}
							else
							{
								ASSERT_TRUE(routeHandler.removeObject(entry->second.get(), hero->getOwner()));
								CPathsInfo after(state->getMapSize(), hero);
								auto config = std::make_shared<SingleHeroPathfinderConfig>(after, *state, hero);
								CPathfinder(*state, config).calculatePaths();
								std::cout << "Road reachable after test decoration and guard removal="
								          << after.getPathInfo(road, EPathfindingLayer::LAND)->reachable() << std::endl;
							}
						}
					}
				}
			}
			else
				for(const int index : route)
				{
					const auto point = posOf(index);
					if(!paths.getNode(point, EPathfindingLayer::LAND)->reachable())
					{
						const auto guard = state->guardingCreaturePosition(point);
						std::cout << "First unreachable geometric step=" << point.toString()
						          << " guard=" << guard.toString() << std::endl;
						if(map.isInTheMap(guard))
							for(const auto & id : map.getTile(guard).visitableObjects)
								if(auto * creature = dynamic_cast<CGCreature *>(map.getObject(id)))
								{
									std::cout << "Guard instance=" << creature->instanceName << std::endl;
									const auto identifiedGuardID = creature->id;
									GameHandlerTestServer server(state, hero->getOwner());
									CGameHandler handler(server, state);
									if(std::getenv("VCMI_INSPECT_EARTH_FIGHT_START_GUARD"))
									{
										creature->agression = 127;
										creature->neverFlees = true;
										hero->setMovementPoints(30000);
										state->actingPlayers.insert(hero->getOwner());
										for(const int step : route)
										{
											const auto destination = posOf(step);
											ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, hero->getOwner(), EPathfindingLayer::LAND));
											if(state->getBattle(hero->getOwner()))
												break;
											ASSERT_EQ(hero->visitablePos(), destination);
										}
										ASSERT_NE(state->getBattle(hero->getOwner()), nullptr);
										handler.battles->cheatBattleVictory(hero->getOwner());
										const auto result = handler.activities->topActivity(hero->getOwner());
										ASSERT_NE(result, nullptr);
										ASSERT_EQ(result->getType(), ActivityType::BattleDialog);
										ASSERT_EQ(handler.activities->submitReply(result->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
										for(int limit = 0; limit < 16; ++limit)
										{
											const auto prompt = handler.activities->topActivity(hero->getOwner());
											if(!prompt || (prompt->getType() != ActivityType::HeroLevelUpDialog && prompt->getType() != ActivityType::CommanderLevelUpDialog))
												break;
											ASSERT_EQ(handler.activities->submitReply(prompt->getActiveQuestionID(), hero->getOwner(), 0), ReplyOutcome::Accepted);
										}
										std::cout << "After battle hero=" << hero->visitablePos().toString()
										          << " movement=" << hero->movementPointsRemaining()
										          << " identified guard remains=" << (map.getObject(identifiedGuardID) != nullptr) << std::endl;
										std::cout << "Battle started from ordinary route; victory forced" << std::endl;
									}
									else
										ASSERT_TRUE(handler.removeObject(creature, hero->getOwner()));
									CPathsInfo after(state->getMapSize(), hero);
									auto afterConfig = std::make_shared<SingleHeroPathfinderConfig>(after, *state, hero);
									CPathfinder(*state, afterConfig).calculatePaths();
									std::cout << "Shipyard reachable after test intervention="
									          << after.getNode(yardPos, EPathfindingLayer::LAND)->reachable() << std::endl;
									break;
								}
						break;
					}
				}
			std::cout << "Free geometric shipyard approach=" << posOf(approach).toString()
			          << " steps=" << route.size() << std::endl;
		}
	}
	if(std::getenv("VCMI_INSPECT_EARTH_SEAM_ROAD"))
	{
		const PlayerColor player(0);
		CGHeroInstance * hero = nullptr;
		for(const auto & id : map.getHeroesOnMap())
			if(auto * candidate = dynamic_cast<CGHeroInstance *>(map.getObject(id)); candidate && candidate->getOwner() == player)
			{
				hero = candidate;
				break;
			}
		ASSERT_NE(hero, nullptr);
		const std::vector<int3> route = {{2, 425, 0}, {1, 425, 0}, {0, 425, 0}, {251, 425, 0}, {250, 425, 0}};
		for(const auto & pos : route)
		{
			ASSERT_NE(map.getTile(pos).getRoadID(), RoadId::NO_ROAD) << pos.toString();
			ASSERT_FALSE(state->guardingCreaturePosition(pos).isValid()) << pos.toString();
		}
		ChangeObjPos place;
		place.objid = hero->id;
		place.nPos = route.front();
		state->apply(place);
		ASSERT_EQ(hero->visitablePos(), route.front());
		hero->setMovementPoints(3000);
		state->actingPlayers.insert(player);
		ExternalInspectionReplicaServer server(state, player);
		CGameHandler handler(server, state);
		for(size_t index = 1; index < route.size(); ++index)
		{
			const auto points = hero->movementPointsRemaining();
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(route[index]), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_EQ(hero->visitablePos(), route[index]);
			EXPECT_LT(hero->movementPointsRemaining(), points);
		}
		for(size_t index = route.size() - 1; index > 0; --index)
		{
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(route[index - 1]), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_EQ(hero->visitablePos(), route[index - 1]);
		}
	}
	if(std::getenv("VCMI_INSPECT_WATER_PORTAL"))
	{
		const PlayerColor player(0);
		CGHeroInstance * hero = nullptr;
		for(const auto & id : map.getHeroesOnMap())
			if(auto * candidate = dynamic_cast<CGHeroInstance *>(map.getObject(id)); candidate
			   && candidate->getOwner() == player && !candidate->isGarrisoned() && !candidate->inBoat())
			{
				hero = candidate;
				break;
			}
		ASSERT_NE(hero, nullptr);
		const auto portal = map.instanceNames.find("monolithTwoWay_5267");
		ASSERT_NE(portal, map.instanceNames.end());
		ASSERT_EQ(portal->second->visitablePos(), int3(239, 429, 0));
		const int3 shore(241, 430, 0), water(240, 430, 0), approach(239, 430, 0);
		ASSERT_TRUE(map.getTile(shore).isLand());
		ASSERT_FALSE(map.getTile(shore).blocked());
		ASSERT_TRUE(map.getTile(water).isWater());
		ASSERT_FALSE(map.getTile(water).blocked());
		ASSERT_TRUE(map.getTile(approach).isWater());
		ASSERT_FALSE(map.getTile(approach).blocked());
		ASSERT_FALSE(state->guardingCreaturePosition(water).isValid());
		ASSERT_FALSE(state->guardingCreaturePosition(approach).isValid());
		state->actingPlayers.insert(player);
		ChangeObjPos place;
		place.objid = hero->id;
		place.nPos = shore;
		state->apply(place);
		GameHandlerTestServer server(state, player);
		CGameHandler handler(server, state);
		handler.createBoat(water, BoatId::CASTLE, player);
		hero->setMovementPoints(3000);
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
		ASSERT_TRUE(hero->inBoat());
		const auto boatId = hero->getBoat()->id;
		hero->setMovementPoints(3000);
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(approach), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(portal->second->visitablePos()), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
		const auto dialog = handler.activities->topActivity(player);
		ASSERT_NE(dialog, nullptr);
		ASSERT_EQ(dialog->getType(), ActivityType::TeleportDialog);
		const auto teleport = std::dynamic_pointer_cast<TeleportDialogActivity>(dialog);
		ASSERT_NE(teleport, nullptr);
		const char * requestedExit = std::getenv("VCMI_INSPECT_WATER_PORTAL_EXIT");
		const auto selected = std::ranges::find_if(teleport->td.exits, [&](const auto & item)
		{
			const auto * object = map.getObject(item.first);
			return object && item.first != portal->second->id
				&& (!requestedExit || !*requestedExit || object->instanceName == requestedExit);
		});
		ASSERT_NE(selected, teleport->td.exits.end());
		const auto exitId = selected->first;
		const auto * exitObject = map.getObject(exitId);
		ASSERT_NE(exitObject, nullptr);
		const auto exit = exitObject->visitablePos();
		ASSERT_TRUE(map.getTile(exit).isWater());
		const auto answer = static_cast<ui32>(std::distance(teleport->td.exits.begin(), selected));
		ASSERT_EQ(handler.activities->submitReply(dialog->getActiveQuestionID(), player, answer), ReplyOutcome::Accepted);
		EXPECT_EQ(hero->visitablePos(), exit);
		ASSERT_TRUE(hero->inBoat());
		EXPECT_EQ(hero->getBoat()->id, boatId);
		std::cout << "Water portal from " << approach.toString() << " to " << exit.toString() << std::endl;
	}
#ifdef ENABLE_NULLKILLER2_AI
	if(const char * input = std::getenv("VCMI_INSPECT_AI_RESOURCE"); input && *input)
	{
		std::vector<std::string> values;
		boost::split(values, input, boost::is_any_of(","));
		ASSERT_EQ(values.size(), 2u);
		const PlayerColor player(std::stoi(values[0]));
		ASSERT_TRUE(player.isValidPlayer());
		if(aiSlot)
			ASSERT_EQ(*aiSlot, player.getNum());
		ASSERT_TRUE(state->players.contains(player));
		ASSERT_NE(state->getPlayerTeam(player), nullptr);
		const auto target = map.instanceNames.find(values[1]);
		ASSERT_NE(target, map.instanceNames.end());
		const auto position = target->second->visitablePos();
		const auto * resource = dynamic_cast<const CGResource *>(target->second.get());
		ASSERT_NE(resource, nullptr);
		ASSERT_EQ(resource->stacksCount(), 0u);
		const auto objectID = resource->id;
		const auto resourceID = resource->resourceID();
		const auto amount = resource->getAmount();
		const auto before = state->players.at(player).resources[resourceID];
		ASSERT_TRUE(map.isInTheMap(position));
		ASSERT_NE(state->getPlayerTeam(player)->fogOfWarMap[position], 0);
		std::unique_ptr<ExternalInspectionAIClient> client;
		if(std::getenv("VCMI_INSPECT_AI_EXECUTE") || std::getenv("VCMI_INSPECT_AI_AUTONOMOUS") || std::getenv("VCMI_INSPECT_AI_SERVER_TURN"))
			client = std::make_unique<ExternalInspectionAIClient>(state, player);
		auto callback = std::make_shared<CCallback>(state, std::optional<PlayerColor>{player}, client.get());
		auto gateway = std::make_unique<NK2AI::AIGateway>();
		gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
		if(client)
			client->notify(gateway.get());
		if(std::getenv("VCMI_INSPECT_AI_SERVER_TURN"))
		{
			ASSERT_TRUE(aiSlot.has_value());
			ASSERT_EQ(*aiSlot, 0);
			client->startServerTurn();
			const bool reachedTerminal = client->waitForServerTurn();
			gateway->finish();
			const auto result = client->result(player);
			EXPECT_TRUE(reachedTerminal) << "AI did not end its turn or start a battle in 15 seconds";
			EXPECT_EQ(result.rejectedMoves, 0);
			EXPECT_GT(result.moves, 0);
			EXPECT_EQ(result.battles + result.acceptedEndTurns, 1);
			EXPECT_EQ(result.playerStillActing, result.battles == 1);
			if(result.acceptedEndTurns)
				EXPECT_TRUE(result.anotherPlayerActing);
			std::cout << "Nullkiller server turn: movement=" << result.moves
			          << ", rejected=" << result.rejectedMoves << ", battles=" << result.battles
			          << ", accepted end turns=" << result.acceptedEndTurns << std::endl;
			return;
		}
		std::shared_lock lock(CGameState::mutex);
		auto & ai = *gateway->nullkiller;
		if(std::getenv("VCMI_INSPECT_AI_AUTONOMOUS"))
		{
			ASSERT_NE(client, nullptr);
			std::atomic<bool> timedOut{false};
			std::jthread watchdog([&](std::stop_token stop)
			{
				std::mutex mutex;
				std::condition_variable_any wake;
				std::unique_lock guard(mutex);
				wake.wait_for(guard, stop, std::chrono::seconds(15), [] { return false; });
				if(!stop.stop_requested())
				{
					timedOut = true;
					ai.makingTurnInterruption.interruptThread();
				}
			});
			bool interrupted = false;
			try { ai.makeTurn(); }
			catch(const InterruptionRequestedException &) { interrupted = true; }
			watchdog.request_stop();
			watchdog.join();
			const auto result = client->result(player);
			EXPECT_FALSE(timedOut) << "Autonomous AI did not finish or reach a battle within 15 seconds";
			EXPECT_GT(result.moves, 0);
			EXPECT_EQ(result.rejectedMoves, 0);
			EXPECT_LE(result.battles, 1);
			EXPECT_EQ(interrupted, result.battles == 1);
			if(result.battles == 0)
			{
				EXPECT_FALSE(result.hasActivities);
				EXPECT_EQ(gateway->status.getQuestionsCount(), 0);
				EXPECT_EQ(gateway->status.getCurrentVisitedObject(), ObjectInstanceID::NONE);
			}
			std::cout << "Nullkiller autonomous planning: requests=" << result.requests
			          << ", movement requests=" << result.moves << ", rejected=" << result.rejectedMoves
			          << ", battles=" << result.battles << ", completed visits=" << result.completedVisits
			          << ", blocking dialogs=" << result.blockingDialogs
			          << ", recruitment windows=" << result.recruitmentWindows
			          << ", accepted answers=" << result.acceptedAnswers << std::endl;
			return;
		}
		else
		{
			ai.heroManager->update();
			NK2AI::PathfinderSettings settings;
			settings.useHeroChain = false;
			settings.useDimensionDoor = false;
			settings.mainTurnDistanceLimit = 1;
			settings.scoutTurnDistanceLimit = 1;
			ai.pathfinder->updatePaths(ai.getHeroesForPathfinding(), settings);
			const auto paths = ai.pathfinder->getPathInfo(position);
			const auto found = std::ranges::find_if(paths, [&](const auto & path)
			{
				return path.targetHero && path.targetHero->getOwner() == player && !path.nodes.empty() && path.turn() == 0;
			});
			ASSERT_NE(found, paths.end()) << values[1];
			std::cout << "Nullkiller planned visible resource " << values[1] << " for player "
			          << player.getNum() << ", path nodes=" << found->nodes.size() << std::endl;
			if(client)
			{
				ASSERT_GT(amount, 0u);
				EXPECT_NO_THROW(NK2AI::Goals::ExecuteHeroChain(*found).accept(gateway.get()));
				EXPECT_GT(client->moves, 0);
				EXPECT_EQ(client->rejectedMoves, 0);
				EXPECT_EQ(map.getObject(objectID), nullptr);
				EXPECT_EQ(state->players.at(player).resources[resourceID], before + amount);
				std::cout << "Nullkiller executed resource plan in " << client->moves << " movement requests" << std::endl;
			}
		}
	}
#endif
	if(std::getenv("VCMI_INSPECT_START_RESOURCE_ROUTES"))
	{
		std::set<PlayerColor> examined;
		int collected = 0;
		for(const auto & id : map.getHeroesOnMap())
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(map.getObject(id));
			if(!hero || !hero->getOwner().isValidPlayer() || hero->isGarrisoned() || hero->inBoat()
			   || !examined.insert(hero->getOwner()).second)
				continue;
			const auto player = hero->getOwner();
			const auto & fog = state->getPlayerTeam(player)->fogOfWarMap;
			CPathsInfo paths(state->getMapSize(), hero);
			auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *state, hero);
			config->options.turnLimit = 0;
			CPathfinder(*state, config).calculatePaths();
			CGPath chosen;
			const CGResource * target = nullptr;
			for(const auto & object : map.objects)
			{
				const auto * resource = dynamic_cast<const CGResource *>(object.get());
				if(!resource || resource->stacksCount() || !resource->getAmount())
					continue;
				const auto position = resource->visitablePos();
				if(!map.isInTheMap(position) || !fog[position] || !map.getTile(position).isLand()
				   || map.getTile(position).topVisitableObj() != resource->id
				   || state->guardingCreaturePosition(position).isValid())
					continue;
				const auto * node = paths.getNode(position, EPathfindingLayer::LAND);
				CGPath route;
				if(!node->reachable() || node->turns != 0 || !paths.getPath(route, position, EPathfindingLayer::LAND)
				   || route.nodes.size() < 3)
					continue;
				bool clear = true;
				for(size_t index = 1; index + 1 < route.nodes.size(); ++index)
				{
					const auto & step = route.nodes[index];
					clear = clear && step.layer == EPathfindingLayer::LAND && step.turns == 0 && fog[step.coord]
						&& !map.getTile(step.coord).visitable() && !state->guardingCreaturePosition(step.coord).isValid();
				}
				if(clear && (!target || route.nodes.size() < chosen.nodes.size()))
				{
					chosen = std::move(route);
					target = resource;
				}
			}
			if(!target)
			{
				std::cout << "No short unguarded resource route for player " << player.getNum() << std::endl;
				continue;
			}
			const auto targetID = target->id;
			const auto resourceID = target->resourceID();
			const auto amount = target->getAmount();
			const auto before = state->players.at(player).resources[resourceID];
			const auto name = target->instanceName;
			state->actingPlayers.insert(player);
			ExternalInspectionReplicaServer server(state, player);
			if(std::getenv("VCMI_INSPECT_REPLICA_RESOURCES"))
				for(int index = 0; index < 2; ++index)
					server.replicas.push_back(reloadExternalInspectionState(state));
			CGameHandler handler(server, state);
			for(size_t index = chosen.nodes.size() - 1; index > 0; --index)
			{
				const auto & step = chosen.nodes[index - 1];
				ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(step.coord),
				                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND))
					<< name << " step " << step.coord.toString();
				if(index > 1)
					ASSERT_EQ(hero->visitablePos(), step.coord);
			}
			EXPECT_EQ(map.getObject(targetID), nullptr);
			EXPECT_EQ(state->players.at(player).resources[resourceID], before + amount);
			for(const auto & replica : server.replicas)
			{
				const auto * replicaHero = dynamic_cast<const CGHeroInstance *>(replica->getMap().getObject(hero->id));
				ASSERT_NE(replicaHero, nullptr);
				EXPECT_EQ(replicaHero->visitablePos(), hero->visitablePos());
				EXPECT_EQ(replicaHero->movementPointsRemaining(), hero->movementPointsRemaining());
				EXPECT_EQ(replica->getMap().getObject(targetID), nullptr);
				EXPECT_EQ(replica->players.at(player).resources[resourceID], before + amount);
				const auto [compared, mismatches] = compareExternalInspectionFog(*state, *replica);
				EXPECT_GT(compared, 0u);
				EXPECT_EQ(mismatches, 0u);
			}
			if(!server.replicas.empty())
				EXPECT_EQ(server.deliveredMovements, 2 * static_cast<int>(chosen.nodes.size() - 1));
			++collected;
			std::cout << "Player " << player.getNum() << " collected " << name << " in "
			          << chosen.nodes.size() - 1 << " planned steps" << std::endl;
		}
		std::cout << "Initial heroes examined=" << examined.size() << ", resource routes=" << collected << std::endl;
		ASSERT_GT(collected, 0);
		if(const char * required = std::getenv("VCMI_INSPECT_REQUIRE_RESOURCE_ROUTES"))
			ASSERT_EQ(collected, std::stoi(required));
	}
	if(std::getenv("VCMI_INSPECT_SEAM_LAND_STEPS"))
	{
		CGHeroInstance * hero = nullptr;
		for(const auto & id : map.getHeroesOnMap())
			if(auto * candidate = dynamic_cast<CGHeroInstance *>(map.getObject(id)); candidate
			   && candidate->getOwner().isValidPlayer() && !candidate->isGarrisoned() && !candidate->inBoat())
			{
				hero = candidate;
				break;
			}
		ASSERT_NE(hero, nullptr);
		const auto player = hero->getOwner();
		state->actingPlayers.insert(player);
		for(const bool horizontal : {true, false})
		{
			std::optional<std::pair<int3, int3>> crossing;
			const int limit = horizontal ? map.height : map.width;
			for(int index = 0; index < limit; ++index)
			{
				const int3 first = horizontal ? int3(0, index, 0) : int3(index, 0, 0);
				const int3 second = horizontal ? int3(map.width - 1, index, 0) : int3(index, map.height - 1, 0);
				const auto clear = [&](const int3 & position)
				{
					const auto & tile = map.getTile(position);
					return tile.isLand() && !tile.blocked() && !tile.visitable()
						&& !state->guardingCreaturePosition(position).isValid();
				};
				if(clear(first) && clear(second) && map.canMoveBetween(first, second))
				{
					crossing = std::pair(first, second);
					break;
				}
			}
			ASSERT_TRUE(crossing) << (horizontal ? "east/west" : "north/south");
			ChangeObjPos place;
			place.objid = hero->id;
			place.nPos = crossing->first;
			state->apply(place);
			hero->setMovementPoints(3000);
			ExternalInspectionReplicaServer server(state, player);
			if(std::getenv("VCMI_INSPECT_REPLICA_SEAMS"))
				for(int index = 0; index < 2; ++index)
					server.replicas.push_back(reloadExternalInspectionState(state));
			CGameHandler handler(server, state);
			const bool destinationWasVisible = state->getPlayerTeam(player)->fogOfWarMap[crossing->second] != 0;
			if(std::getenv("VCMI_INSPECT_EXPECT_HIDDEN_SEAMS"))
				ASSERT_FALSE(destinationWasVisible);
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(crossing->second),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_EQ(hero->visitablePos(), crossing->second);
			const auto checkReplicas = [&](const int3 & expected)
			{
				for(const auto & replica : server.replicas)
				{
					const auto * replicaHero = dynamic_cast<const CGHeroInstance *>(replica->getMap().getObject(hero->id));
					ASSERT_NE(replicaHero, nullptr);
					EXPECT_EQ(replicaHero->visitablePos(), expected);
					EXPECT_EQ(replicaHero->movementPointsRemaining(), hero->movementPointsRemaining());
					const auto [compared, mismatches] = compareExternalInspectionFog(*state, *replica);
					EXPECT_GT(compared, 0u);
					EXPECT_EQ(mismatches, 0u);
				}
			};
			checkReplicas(crossing->second);
			if(!server.replicas.empty())
				std::cout << "Two in-process replicas matched hero and all player fog" << std::endl;
			if(std::getenv("VCMI_INSPECT_SEAM_SAVE"))
			{
				auto restored = reloadExternalInspectionState(state);
				ASSERT_TRUE(restored->getMap().toroidal);
				ASSERT_EQ(restored->getMap().width, map.width);
				ASSERT_EQ(restored->getMap().height, map.height);
				ASSERT_EQ(restored->getMap().objects.size(), map.objects.size());
				ASSERT_EQ(restored->getMap().getAllTowns().size(), map.getAllTowns().size());
				auto * restoredHero = dynamic_cast<CGHeroInstance *>(restored->getMap().getObject(hero->id));
				ASSERT_NE(restoredHero, nullptr);
				ASSERT_EQ(restoredHero->visitablePos(), crossing->second);
				const auto & beforeFog = state->getPlayerTeam(player)->fogOfWarMap;
				ASSERT_NE(beforeFog[crossing->second], 0);
				const auto [comparedFog, mismatchedFog] = compareExternalInspectionFog(*state, *restored);
				EXPECT_GT(comparedFog, 0u);
				EXPECT_EQ(mismatchedFog, 0u);
				GameHandlerTestServer restoredServer(restored, player);
				CGameHandler restoredHandler(restoredServer, restored);
				ASSERT_TRUE(restoredHandler.moveHero(restoredHero->id, restoredHero->convertFromVisitablePos(crossing->first),
				                                     EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
				ASSERT_EQ(restoredHero->visitablePos(), crossing->first);
				std::cout << "Restored seam crossing, compared player fog cells=" << comparedFog
				          << ", destination previously visible=" << destinationWasVisible << std::endl;
			}
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(crossing->first),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_EQ(hero->visitablePos(), crossing->first);
			checkReplicas(crossing->first);
			if(!server.replicas.empty())
				EXPECT_EQ(server.deliveredMovements, 4);
			std::cout << "Land seam crossing in both directions: " << crossing->first.toString()
			          << " <-> " << crossing->second.toString() << std::endl;
		}
	}
	if(std::getenv("VCMI_INSPECT_SEAM_BOAT_STEPS"))
	{
		CGHeroInstance * hero = nullptr;
		for(const auto & id : map.getHeroesOnMap())
			if(auto * candidate = dynamic_cast<CGHeroInstance *>(map.getObject(id)); candidate
			   && candidate->getOwner().isValidPlayer() && !candidate->isGarrisoned() && !candidate->inBoat())
			{
				hero = candidate;
				break;
			}
		ASSERT_NE(hero, nullptr);
		const auto player = hero->getOwner();
		state->actingPlayers.insert(player);
		for(const bool horizontal : {true, false})
		{
			std::optional<int3> source, destination, shore;
			const int limit = horizontal ? map.height : map.width;
			for(int index = 0; index < limit && !source; ++index)
			{
				const int3 first = horizontal ? int3(0, index, 0) : int3(index, 0, 0);
				const int3 second = horizontal ? int3(map.width - 1, index, 0) : int3(index, map.height - 1, 0);
				const auto clearWater = [&](const int3 & position)
				{
					const auto & tile = map.getTile(position);
					return tile.isWater() && !tile.blocked() && !tile.visitable()
						&& !state->guardingCreaturePosition(position).isValid();
				};
				if(!clearWater(first) || !clearWater(second) || !map.canMoveBetween(first, second))
					continue;
				for(const auto & direction : {int3(0, -1, 0), int3(0, 1, 0), int3(-1, 0, 0), int3(1, 0, 0)})
				{
					const auto candidate = map.getTopology().canonicalize(first + direction);
					if(candidate && map.getTile(*candidate).isLand() && !map.getTile(*candidate).blocked()
					   && !map.getTile(*candidate).visitable() && !state->guardingCreaturePosition(*candidate).isValid()
					   && map.canMoveBetween(*candidate, first))
					{
						source = first;
						destination = second;
						shore = *candidate;
						break;
					}
				}
			}
			ASSERT_TRUE(source && destination && shore) << (horizontal ? "east/west" : "north/south");
			ChangeObjPos place;
			place.objid = hero->id;
			place.nPos = *shore;
			state->apply(place);
			hero->setMovementPoints(3000);
			ExternalInspectionReplicaServer server(state, player);
			if(std::getenv("VCMI_INSPECT_REPLICA_BOATS"))
				for(int index = 0; index < 2; ++index)
					server.replicas.push_back(reloadExternalInspectionState(state));
			CGameHandler handler(server, state);
			handler.createBoat(*source, BoatId::CASTLE, player);
			const auto boatID = map.getTile(*source).topVisitableObj();
			ASSERT_NE(dynamic_cast<const CGBoat *>(map.getObject(boatID)), nullptr);
			const auto checkReplicas = [&]
			{
				for(const auto & replica : server.replicas)
				{
					const auto * replicaHero = dynamic_cast<const CGHeroInstance *>(replica->getMap().getObject(hero->id));
					ASSERT_NE(replicaHero, nullptr);
					EXPECT_EQ(replicaHero->visitablePos(), hero->visitablePos());
					EXPECT_EQ(replicaHero->movementPointsRemaining(), hero->movementPointsRemaining());
					ASSERT_EQ(replicaHero->inBoat(), hero->inBoat());
					if(hero->inBoat())
					{
						ASSERT_NE(replicaHero->getBoat(), nullptr);
						ASSERT_NE(hero->getBoat(), nullptr);
						EXPECT_EQ(replicaHero->getBoat()->id, hero->getBoat()->id);
					}
					const auto [compared, mismatches] = compareExternalInspectionFog(*state, *replica);
					EXPECT_GT(compared, 0u);
					EXPECT_EQ(mismatches, 0u);
				}
			};
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(*source),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
			ASSERT_TRUE(hero->inBoat());
			checkReplicas();
			hero->setMovementPoints(3000);
			for(const auto & replica : server.replicas)
			{
				auto * replicaHero = dynamic_cast<CGHeroInstance *>(replica->getMap().getObject(hero->id));
				ASSERT_NE(replicaHero, nullptr);
				replicaHero->setMovementPoints(3000);
			}
			for(const auto & target : {*destination, *source})
			{
				ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(target),
				                             EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
				ASSERT_EQ(hero->visitablePos(), target);
				checkReplicas();
				if(target == *destination && std::getenv("VCMI_INSPECT_BOAT_SAVE"))
				{
					auto restored = reloadExternalInspectionState(state);
					ASSERT_TRUE(restored->getMap().toroidal);
					ASSERT_EQ(restored->getMap().width, map.width);
					ASSERT_EQ(restored->getMap().height, map.height);
					auto * restoredHero = dynamic_cast<CGHeroInstance *>(restored->getMap().getObject(hero->id));
					ASSERT_NE(restoredHero, nullptr);
					ASSERT_EQ(restoredHero->visitablePos(), target);
					ASSERT_TRUE(restoredHero->inBoat());
					ASSERT_NE(restoredHero->getBoat(), nullptr);
					ASSERT_NE(hero->getBoat(), nullptr);
					ASSERT_EQ(restoredHero->getBoat()->id, hero->getBoat()->id);
					const auto [comparedFog, mismatchedFog] = compareExternalInspectionFog(*state, *restored);
					EXPECT_GT(comparedFog, 0u);
					EXPECT_EQ(mismatchedFog, 0u);
					GameHandlerTestServer restoredServer(restored, player);
					CGameHandler restoredHandler(restoredServer, restored);
					ASSERT_TRUE(restoredHandler.moveHero(restoredHero->id, restoredHero->convertFromVisitablePos(*source),
					                                     EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
					ASSERT_EQ(restoredHero->visitablePos(), *source);
					ASSERT_TRUE(restoredHandler.moveHero(restoredHero->id, restoredHero->convertFromVisitablePos(*shore),
					                                     EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
					ASSERT_FALSE(restoredHero->inBoat());
					ASSERT_EQ(restoredHero->visitablePos(), *shore);
					std::cout << "Restored boat and disembark, compared player fog cells=" << comparedFog << std::endl;
				}
			}
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(*shore),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_FALSE(hero->inBoat());
			ASSERT_EQ(hero->visitablePos(), *shore);
			const auto * boat = dynamic_cast<const CGBoat *>(map.getObject(boatID));
			ASSERT_NE(boat, nullptr);
			EXPECT_EQ(boat->visitablePos(), *source);
			checkReplicas();
			for(const auto & replica : server.replicas)
			{
				const auto * replicaBoat = dynamic_cast<const CGBoat *>(replica->getMap().getObject(boatID));
				ASSERT_NE(replicaBoat, nullptr);
				EXPECT_EQ(replicaBoat->visitablePos(), *source);
			}
			if(!server.replicas.empty())
			{
				EXPECT_EQ(server.deliveredMovements, 8);
				std::cout << "Two replicas matched boat, hero and all player fog" << std::endl;
			}
			std::cout << "Embarked at " << shore->toString() << ", sailed " << source->toString()
			          << " <-> " << destination->toString() << " and disembarked" << std::endl;
		}
	}
	if(std::getenv("VCMI_INSPECT_CORNER_BOAT_LANDING"))
	{
		CGHeroInstance * hero = nullptr;
		for(const auto & id : map.getHeroesOnMap())
			if(auto * candidate = dynamic_cast<CGHeroInstance *>(map.getObject(id)); candidate
			   && candidate->getOwner().isValidPlayer() && !candidate->isGarrisoned() && !candidate->inBoat())
			{
				hero = candidate;
				break;
			}
		ASSERT_NE(hero, nullptr);
		const auto player = hero->getOwner();
		state->actingPlayers.insert(player);
		for(const int y : {map.height - 1, 0})
		{
			const int3 shore(0, y, 0);
			const int3 water(map.width - 1, y, 0);
			const int3 landing(0, y == 0 ? map.height - 1 : 0, 0);
			ASSERT_TRUE(map.getTile(shore).isLand());
			ASSERT_TRUE(map.getTile(landing).isLand());
			ASSERT_TRUE(map.getTile(water).isWater());
			for(const auto & position : {shore, water, landing})
			{
				const auto & tile = map.getTile(position);
				const auto isHero = [&](const ObjectInstanceID & id) { return id == hero->id; };
				ASSERT_TRUE(std::ranges::all_of(tile.blockingObjects, isHero)) << position.toString();
				ASSERT_TRUE(std::ranges::all_of(tile.visitableObjects, isHero)) << position.toString();
				ASSERT_FALSE(state->guardingCreaturePosition(position).isValid()) << position.toString();
			}
			ChangeObjPos place;
			place.objid = hero->id;
			place.nPos = shore;
			state->apply(place);
			GameHandlerTestServer server(state, player);
			CGameHandler handler(server, state);
			handler.createBoat(water, BoatId::CASTLE, player);
			hero->setMovementPoints(3000);
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
			ASSERT_TRUE(hero->inBoat());
			hero->setMovementPoints(3000);
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(landing),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_FALSE(hero->inBoat());
			ASSERT_EQ(hero->visitablePos(), landing);
			std::cout << "Corner disembark: " << shore.toString() << " -> " << water.toString()
			          << " -> " << landing.toString() << std::endl;
		}
	}
	if(const char * overlap = std::getenv("VCMI_INSPECT_OVERLAP_RESOURCE"); overlap && *overlap)
	{
		std::vector<std::string> names;
		boost::split(names, overlap, boost::is_any_of(","));
		ASSERT_EQ(names.size(), 2u);
		const auto resourceIt = map.instanceNames.find(names[0]);
		const auto obstacleIt = map.instanceNames.find(names[1]);
		ASSERT_NE(resourceIt, map.instanceNames.end());
		ASSERT_NE(obstacleIt, map.instanceNames.end());
		const auto * resource = dynamic_cast<const CGResource *>(resourceIt->second.get());
		ASSERT_NE(resource, nullptr);
		ASSERT_EQ(resource->stacksCount(), 0u);
		const auto position = resource->visitablePos();
		ASSERT_TRUE(obstacleIt->second->blockingAt(position));
		ASSERT_FALSE(state->guardingCreaturePosition(position).isValid());
		const auto resourceID = resource->resourceID();
		const auto resourceObjectID = resource->id;
		const auto amount = resource->getAmount();
		ASSERT_GT(amount, 0u);
		CGHeroInstance * hero = nullptr;
		for(const auto & id : map.getHeroesOnMap())
			if(auto * candidate = dynamic_cast<CGHeroInstance *>(map.getObject(id)); candidate
			   && candidate->getOwner().isValidPlayer() && !candidate->isGarrisoned() && !candidate->inBoat())
			{
				hero = candidate;
				break;
			}
		ASSERT_NE(hero, nullptr);
		std::optional<int3> approach;
		std::optional<int3> departure;
		for(const auto & direction : int3::getDirs())
		{
			const auto candidate = map.getTopology().canonicalize(position + direction);
			if(candidate && map.getTile(*candidate).isLand() && !map.getTile(*candidate).blocked()
			   && !map.getTile(*candidate).visitable() && !state->guardingCreaturePosition(*candidate).isValid()
			   && map.canMoveBetween(*candidate, position))
			{
				for(const auto & nextDirection : int3::getDirs())
				{
					const auto next = map.getTopology().canonicalize(*candidate + nextDirection);
					if(next && *next != position && map.getTile(*next).isLand() && !map.getTile(*next).blocked()
					   && !map.getTile(*next).visitable() && !state->guardingCreaturePosition(*next).isValid()
					   && map.canMoveBetween(*candidate, *next))
					{
						approach = *candidate;
						departure = *next;
						break;
					}
				}
				if(approach)
					break;
			}
		}
		ASSERT_TRUE(approach);
		ASSERT_TRUE(departure);
		const auto player = hero->getOwner();
		state->actingPlayers.insert(player);
		ChangeObjPos place;
		place.objid = hero->id;
		place.nPos = *approach;
		state->apply(place);
		hero->setMovementPoints(3000);
		const auto before = state->players.at(player).resources[resourceID];
		GameHandlerTestServer server(state, player);
		CGameHandler handler(server, state);
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(position),
		                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
		EXPECT_EQ(state->players.at(player).resources[resourceID], before + amount);
		EXPECT_EQ(map.getObject(resourceObjectID), nullptr);
		ASSERT_EQ(hero->visitablePos(), *approach);
		ASSERT_TRUE(obstacleIt->second->blockingAt(position));
		EXPECT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(*departure),
		                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
		EXPECT_EQ(hero->visitablePos(), *departure);
		std::cout << "Collected resource from adjacent tile and continued to " << departure->toString() << std::endl;
	}
	const char * guardedObjects = std::getenv("VCMI_INSPECT_GUARDED_OBJECTS");
	if(!guardedObjects || !*guardedObjects)
		guardedObjects = std::getenv("VCMI_INSPECT_MINE_GUARDS");
	if(guardedObjects && *guardedObjects)
	{
		std::vector<std::string> groups;
		boost::split(groups, guardedObjects, boost::is_any_of(";"));
		for(const auto & group : groups)
		{
			std::vector<std::string> names;
			boost::split(names, group, boost::is_any_of(","));
			ASSERT_EQ(names.size(), 2u);
			const auto mine = map.instanceNames.find(names[0]);
			const auto guard = map.instanceNames.find(names[1]);
			ASSERT_NE(mine, map.instanceNames.end()) << names[0];
			ASSERT_NE(guard, map.instanceNames.end()) << names[1];
			const auto guardPos = guard->second->visitablePos();
			SCOPED_TRACE(group);
			EXPECT_EQ(guard->second->ID, Obj::MONSTER);
			EXPECT_FALSE(mine->second->blockingAt(guardPos));
			EXPECT_EQ(state->guardingCreaturePosition(mine->second->visitablePos()), guardPos);
		}
		std::cout << "Checked guarded objects=" << groups.size() << std::endl;
	}
	if(std::getenv("VCMI_INSPECT_MOD_SCOPES"))
	{
		std::map<std::string, size_t> objectScopes;
		std::map<std::string, size_t> terrainScopes;
		for(const auto & object : map.objects)
			if(object)
				++objectScopes[object->getObjectHandler()->getModScope()];
		for(int z = 0; z < state->getMapSize().z; ++z)
			for(int x = 0; x < map.width; ++x)
				for(int y = 0; y < map.height; ++y)
					++terrainScopes[map.getTile(int3(x, y, z)).getTerrain()->getModScope()];
		for(const auto & mod : LIBRARY->modh->getActiveMods())
			std::cout << "Active mod: " << mod << std::endl;
		for(const auto & [scope, count] : objectScopes)
			std::cout << "Object handler scope: " << scope << " count=" << count << std::endl;
		for(const auto & [scope, count] : terrainScopes)
			std::cout << "Terrain scope: " << scope << " count=" << count << std::endl;
	}
	if(std::getenv("VCMI_INSPECT_START_ROUTES"))
	{
		std::set<PlayerColor> examined;
		int moved = 0;
		for(const auto & id : map.getHeroesOnMap())
		{
			auto * hero = dynamic_cast<CGHeroInstance *>(map.getObject(id));
			if(!hero || !hero->getOwner().isValidPlayer() || hero->isGarrisoned() || hero->inBoat()
			   || !examined.insert(hero->getOwner()).second)
				continue;
			const auto player = hero->getOwner();
			const auto origin = hero->visitablePos();
			CPathsInfo paths(state->getMapSize(), hero);
			auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *state, hero);
			config->options.turnLimit = 1;
			CPathfinder(*state, config).calculatePaths();
			std::optional<int3> destination;
			int reachableNeighbors = 0;
			for(const auto & direction : int3::getDirs())
			{
				const auto candidate = map.getTopology().canonicalize(origin + direction);
				if(!candidate)
					continue;
				const auto * node = paths.getNode(*candidate, EPathfindingLayer::LAND);
				if(!node->reachable() || node->turns != 0 || node->theNodeBefore == nullptr || node->theNodeBefore->coord != origin)
					continue;
				++reachableNeighbors;
				const auto & tile = map.getTile(*candidate);
				if(!destination && tile.isLand() && !tile.blocked() && !tile.visitable()
				   && !map.isInTheMap(state->guardingCreaturePosition(*candidate)))
					destination = *candidate;
			}
			std::cout << "Starting hero " << hero->instanceName << " owner=" << player.getNum()
			          << " at=" << origin.toString() << " reachable-neighbors=" << reachableNeighbors
			          << " empty-unguarded=" << (destination ? destination->toString() : "none") << std::endl;
			if(destination)
			{
				state->actingPlayers.insert(player);
				GameHandlerTestServer server(state, player);
				CGameHandler handler(server, state);
				ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(*destination),
				                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
				ASSERT_EQ(hero->visitablePos(), *destination);
				++moved;
			}
		}
		std::cout << "Starting heroes examined=" << examined.size() << " moved=" << moved << std::endl;
		ASSERT_GT(moved, 0);
		if(const char * required = std::getenv("VCMI_INSPECT_REQUIRE_START_MOVES"))
		{
			const auto expected = std::stoi(required);
			ASSERT_EQ(examined.size(), static_cast<size_t>(expected));
			ASSERT_EQ(moved, expected);
		}
	}
	std::vector<std::string> groups;
	boost::split(groups, pairList, boost::is_any_of(";"));
	std::set<TeleportChannelID> channels;
	std::vector<std::pair<const CGMonolith *, const CGMonolith *>> portals;
	for(const auto & group : groups)
	{
		std::vector<std::string> names;
		boost::split(names, group, boost::is_any_of(","));
		ASSERT_EQ(names.size(), 2u);
		auto first = map.instanceNames.find(names[0]);
		auto second = map.instanceNames.find(names[1]);
		ASSERT_NE(first, map.instanceNames.end());
		ASSERT_NE(second, map.instanceNames.end());
		const auto * a = dynamic_cast<const CGMonolith *>(first->second.get());
		const auto * b = dynamic_cast<const CGMonolith *>(second->second.get());
		ASSERT_NE(a, nullptr);
		ASSERT_NE(b, nullptr);
		ASSERT_EQ(a->ID, Obj::MONOLITH_TWO_WAY);
		ASSERT_EQ(b->ID, Obj::MONOLITH_TWO_WAY);
		ASSERT_EQ(a->channel, b->channel);
		ASSERT_TRUE(channels.insert(a->channel).second);
		const auto channel = map.teleportChannels.find(a->channel);
		ASSERT_NE(channel, map.teleportChannels.end());
		EXPECT_EQ(channel->second->entrances.size(), 2u);
		EXPECT_EQ(channel->second->exits.size(), 2u);
		EXPECT_TRUE(vstd::contains(channel->second->exits, a->id));
		EXPECT_TRUE(vstd::contains(channel->second->exits, b->id));
		portals.emplace_back(a, b);
	}
	const bool directVisits = std::getenv("VCMI_INSPECT_TRAVERSE_PORTALS") != nullptr;
	const bool approachVisits = std::getenv("VCMI_INSPECT_APPROACH_PORTALS") != nullptr;
	ASSERT_FALSE(directVisits && approachVisits);
	if(directVisits || approachVisits)
	{
		CGHeroInstance * hero = nullptr;
		for(const auto & id : map.getHeroesOnMap())
		{
			auto * candidate = dynamic_cast<CGHeroInstance *>(map.getObject(id));
			if(candidate && candidate->getOwner().isValidPlayer() && !candidate->isGarrisoned() && !candidate->inBoat())
			{
				hero = candidate;
				break;
			}
		}
		ASSERT_NE(hero, nullptr);
		const auto player = hero->getOwner();
		state->actingPlayers.insert(player);
		GameHandlerTestServer server(state, player);
		CGameHandler handler(server, state);
		int battles = 0;
		int levelUps = 0;
		for(const auto & [a, b] : portals)
			for(const auto & [entrance, exit] : {std::pair(a, b), std::pair(b, a)})
			{
				SCOPED_TRACE(entrance->instanceName + " -> " + exit->instanceName);
				int3 startPosition = entrance->visitablePos();
				if(approachVisits)
				{
					std::optional<int3> approach;
					for(const auto & direction : int3::getDirs())
					{
						const auto candidate = map.getTopology().canonicalize(entrance->visitablePos() + direction);
						if(candidate && map.getTile(*candidate).isLand() && !map.getTile(*candidate).blocked()
							&& !map.getTile(*candidate).visitable() && map.canMoveBetween(*candidate, entrance->visitablePos()))
						{
							approach = candidate;
							break;
						}
					}
					ASSERT_TRUE(approach) << "No free neighboring approach to " << entrance->instanceName;
					startPosition = *approach;
					const auto guard = state->guardingCreaturePosition(entrance->visitablePos());
					if(map.isInTheMap(guard))
						for(const auto & id : map.getTile(guard).visitableObjects)
							if(auto * creature = dynamic_cast<CGCreature *>(map.getObject(id)))
							{
								// Keep the test deterministic if the guard would otherwise offer to flee or join.
								creature->agression = 127;
								creature->neverFlees = true;
							}
				}
				ChangeObjPos place;
				place.objid = hero->id;
				place.nPos = startPosition;
				state->apply(place);
				ASSERT_EQ(hero->visitablePos(), startPosition);
				if(approachVisits)
				{
					hero->setMovementPoints(3000);
					ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(entrance->visitablePos()),
					                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
					if(state->getBattle(player))
					{
						++battles;
						handler.battles->cheatBattleVictory(player);
						const auto result = handler.activities->topActivity(player);
						ASSERT_NE(result, nullptr);
						ASSERT_EQ(result->getType(), ActivityType::BattleDialog);
						ASSERT_EQ(handler.activities->submitReply(result->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);
					}
					for(int limit = 0; limit < 16; ++limit)
					{
						const auto prompt = handler.activities->topActivity(player);
						if(!prompt || (prompt->getType() != ActivityType::HeroLevelUpDialog
						               && prompt->getType() != ActivityType::CommanderLevelUpDialog))
							break;
						++levelUps;
						ASSERT_EQ(handler.activities->submitReply(prompt->getActiveQuestionID(), player, 0), ReplyOutcome::Accepted);
					}
				}
				else
					handler.objectVisited(entrance, hero);
				const auto dialog = handler.activities->topActivity(player);
				ASSERT_NE(dialog, nullptr);
				ASSERT_EQ(dialog->getType(), ActivityType::TeleportDialog);
				const auto teleport = std::dynamic_pointer_cast<TeleportDialogActivity>(dialog);
				ASSERT_NE(teleport, nullptr);
				const auto selected = std::ranges::find_if(teleport->td.exits, [&](const auto & item) { return item.first == exit->id; });
				ASSERT_NE(selected, teleport->td.exits.end());
				const auto answer = static_cast<ui32>(std::distance(teleport->td.exits.begin(), selected));
				ASSERT_EQ(handler.activities->submitReply(dialog->getActiveQuestionID(), player, answer), ReplyOutcome::Accepted);
				EXPECT_EQ(hero->visitablePos(), exit->visitablePos());
			}
		std::cout << (approachVisits ? "Approach and portal visits" : "Direct server portal visits")
			<< " checked in both directions; forced guard victories=" << battles
			<< ", level-up choices=" << levelUps << std::endl;
		if(std::getenv("VCMI_INSPECT_REQUIRE_ALL_GUARDS"))
		{
			ASSERT_TRUE(approachVisits);
			EXPECT_EQ(battles, 2 * portals.size());
		}
	}
	std::cout << "Initialized external toroidal map: " << map.width << "x" << map.height
		<< ", checked isolated portal pairs=" << channels.size() << std::endl;
}

TEST(MapFormat, ConvertsExternalLegacyMapWithWrappedFootprints)
{
	const char * path = std::getenv("VCMI_TOROID_LEGACY_H3M");
	if(!path || !*path)
		GTEST_SKIP() << "Set VCMI_TOROID_LEGACY_H3M to a legacy map";
	std::ifstream file(path, std::ios::binary);
	ASSERT_TRUE(file.is_open());
	const std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	EditorCallback callback(nullptr);
	CMapService service;
	auto map = service.loadMap(bytes.data(), static_cast<int>(bytes.size()), "ExternalLegacyMap", "core", "ASCII", &callback);
	ASSERT_NE(map, nullptr);
	callback.setMap(map.get());
	ASSERT_FALSE(map->toroidal);
	map->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);
	size_t crossing = 0;
	for(const auto & object : map->objects)
		if(object && map->hasStaticFootprintAcrossToroidalSeam(*object))
			++crossing;
	ASSERT_GT(crossing, 0u);
	CMemoryBuffer archive;
	{
		CMapSaverJson saver(&archive);
		ASSERT_NO_THROW(saver.saveMap(map));
	}
	archive.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&archive);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	ASSERT_EQ(reopened->width, map->width);
	ASSERT_EQ(reopened->height, map->height);
	ASSERT_EQ(reopened->levels(), map->levels());
	ASSERT_EQ(reopened->instanceNames.size(), map->instanceNames.size());
	for(const auto & [name, object] : map->instanceNames)
	{
		const auto found = reopened->instanceNames.find(name);
		ASSERT_NE(found, reopened->instanceNames.end()) << name;
		const auto * copy = found->second.get();
		ASSERT_EQ(copy->ID, object->ID) << name;
		ASSERT_EQ(copy->anchorPos(), object->anchorPos()) << name;
		ASSERT_EQ(copy->getWidth(), object->getWidth()) << name;
		ASSERT_EQ(copy->getHeight(), object->getHeight()) << name;
		for(int x = 0; x < object->getWidth(); ++x)
			for(int y = 0; y < object->getHeight(); ++y)
			{
				const auto pos = object->anchorPos() - int3(x, y, 0);
				EXPECT_EQ(copy->coveringAt(pos), object->coveringAt(pos)) << name << " " << pos.toString();
				EXPECT_EQ(copy->blockingAt(pos), object->blockingAt(pos)) << name << " " << pos.toString();
				EXPECT_EQ(copy->visitableAt(pos), object->visitableAt(pos)) << name << " " << pos.toString();
			}
	}
	const auto names = [](const CMap & source, const std::vector<ObjectInstanceID> & ids)
	{
		std::multiset<std::string> result;
		for(const auto & id : ids)
			result.insert(source.getObject(id)->instanceName);
		return result;
	};
	const auto orderedNames = [](const CMap & source, const std::vector<ObjectInstanceID> & ids)
	{
		std::vector<std::string> result;
		for(const auto & id : ids)
			result.push_back(source.getObject(id)->instanceName);
		return result;
	};
	for(int z = 0; z < map->levels(); ++z)
		for(int x = 0; x < map->width; ++x)
			for(int y = 0; y < map->height; ++y)
			{
				const int3 pos(x, y, z);
				const auto & before = map->getTile(pos);
				const auto & after = reopened->getTile(pos);
				EXPECT_EQ(names(*map, before.blockingObjects), names(*reopened, after.blockingObjects)) << pos.toString();
				EXPECT_EQ(orderedNames(*map, before.visitableObjects), orderedNames(*reopened, after.visitableObjects)) << pos.toString();
			}
	ASSERT_FALSE(::testing::Test::HasFailure());
	if(const char * outputPath = std::getenv("VCMI_TOROID_LEGACY_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream output(outputPath, std::ios::binary);
		ASSERT_TRUE(output);
		output.write(reinterpret_cast<const char *>(archive.getBuffer().data()), archive.getSize());
		output.close();
		ASSERT_TRUE(output);
	}
}

TEST(MapFormat, ToroidalWrappedFootprintsSurviveVmapRoundTrip)
{
	const PlayerColor player(0);
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, true)
		.playerActive(player)
		.hero(int3(10, 10, 0), HeroTypeID(0), player)
		.resource(int3(15, 15, 0), GameResID::GOLD, 10)
		.terrainOverlay(int3(20, 20, 0), Obj::CURSED_GROUND1)
		.terrainOverlay(int3(20, 20, 1), Obj::CURSED_GROUND1)
		.terrainOverlay(int3(21, 20, 1), Obj::CURSED_GROUND1)
		.build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("WrappedFootprintRoundTrip", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	CGObjectInstance * resource = nullptr;
	std::vector<CGObjectInstance *> obstacles;
	for(const auto & object : map->objects)
	{
		if(object && object->ID == Obj::RESOURCE)
			resource = object.get();
		if(object && object->ID == Obj::CURSED_GROUND1)
			obstacles.push_back(object.get());
	}
	ASSERT_NE(resource, nullptr);
	ASSERT_EQ(obstacles.size(), 3u);
	auto * obstacle = obstacles[0];
	auto * corner = obstacles[1];
	auto * outside = obstacles[2];
	const auto makeAppearance = [](const std::string & mask)
	{
		const std::string text = R"({ "animation": "test.def", "visitableFrom": ["+++", "+++", "+++"], "mask": )" + mask + " }";
		JsonNode definition(text.data(), text.size(), "WrappedFootprintRoundTrip");
		auto result = std::make_shared<ObjectTemplate>();
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		result->readJson(definition, resolver, false);
		return result;
	};
	map->hideObject(resource);
	resource->pos = int3(0, 10, 0);
	resource->appearance = makeAppearance(R"([ "A0" ])");
	map->showObject(resource);
	map->hideObject(obstacle);
	obstacle->pos = int3(35, 10, 0);
	obstacle->appearance = makeAppearance(R"([ "B" ])");
	map->showObject(obstacle);
	map->hideObject(corner);
	corner->pos = int3(0, 0, 1);
	corner->appearance = makeAppearance(R"([ "BB", "BB" ])");
	map->showObject(corner);
	map->hideObject(outside);
	outside->pos = int3(36, 5, 1);
	outside->appearance = makeAppearance(R"([ "B" ])");
	map->showObject(outside);
	const auto resourceName = resource->instanceName;
	const auto obstacleName = obstacle->instanceName;
	const auto cornerName = corner->instanceName;
	const auto outsideName = outside->instanceName;
	map->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);
	const int3 wrapped(35, 10, 0);
	ASSERT_TRUE(vstd::contains(map->getTile(wrapped).visitableObjects, resource->id));
	ASSERT_TRUE(vstd::contains(map->getTile(wrapped).blockingObjects, obstacle->id));

	CMemoryBuffer archive;
	{
		CMapSaverJson saver(&archive);
		ASSERT_NO_THROW(saver.saveMap(map));
	}
	archive.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&archive);
	std::unique_ptr<CMap> reopened;
	ASSERT_NO_THROW(reopened = reader.loadMap(&reopenedCallback));
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	ASSERT_EQ(reopened->levels(), 2);
	CGObjectInstance * reopenedResource = nullptr;
	CGObjectInstance * reopenedObstacle = nullptr;
	CGObjectInstance * reopenedCorner = nullptr;
	CGObjectInstance * reopenedOutside = nullptr;
	for(const auto & object : reopened->objects)
	{
		if(object && object->instanceName == resourceName)
			reopenedResource = object.get();
		if(object && object->instanceName == obstacleName)
			reopenedObstacle = object.get();
		if(object && object->instanceName == cornerName)
			reopenedCorner = object.get();
		if(object && object->instanceName == outsideName)
			reopenedOutside = object.get();
	}
	ASSERT_NE(reopenedResource, nullptr);
	ASSERT_NE(reopenedObstacle, nullptr);
	ASSERT_NE(reopenedCorner, nullptr);
	ASSERT_NE(reopenedOutside, nullptr);
	EXPECT_EQ(reopenedResource->anchorPos(), int3(0, 10, 0));
	EXPECT_EQ(reopenedResource->visitablePos(), wrapped);
	EXPECT_TRUE(vstd::contains(reopened->getTile(wrapped).visitableObjects, reopenedResource->id));
	EXPECT_TRUE(vstd::contains(reopened->getTile(wrapped).blockingObjects, reopenedResource->id));
	EXPECT_TRUE(vstd::contains(reopened->getTile(wrapped).blockingObjects, reopenedObstacle->id));
	for(const auto & tile : {int3(0, 0, 1), int3(35, 0, 1), int3(0, 35, 1), int3(35, 35, 1)})
		EXPECT_TRUE(vstd::contains(reopened->getTile(tile).blockingObjects, reopenedCorner->id)) << tile.toString();
	EXPECT_EQ(reopenedOutside->anchorPos(), int3(36, 5, 1));
	EXPECT_TRUE(vstd::contains(reopened->getTile(int3(0, 5, 1)).blockingObjects, reopenedOutside->id));
	EXPECT_FALSE(vstd::contains(reopened->getTile(int3(0, 5, 0)).blockingObjects, reopenedOutside->id));

	// More than one image slice would occupy a canonical tile; the renderer
	// cannot yet represent this case and the format must reject it explicitly.
	obstacle->appearance = makeAppearance("[ \"" + std::string(37, 'B') + "\" ]");
	CMemoryBuffer unsupported;
	CMapSaverJson unsupportedSaver(&unsupported);
	EXPECT_THROW(unsupportedSaver.saveMap(map), std::runtime_error);

	// The same oversized template remains legal on a bounded map, but a VMAP
	// claiming toroidal topology must also be rejected when read.
	map->setToroidalAndRebuild(false, CMap::TopologyRebuildObjects::AllMapObjects);
	CMemoryBuffer boundedArchive;
	JsonNode header, objects, surface, underground;
	loadActual(&boundedArchive, map, header, objects, surface, underground);
	EXPECT_NO_THROW(loadOriginal(header, objects, surface, underground));
	header["versionMajor"].Integer() = CMapFormatJson::VERSION_MAJOR_TOROIDAL_TAGGED;
	header["topology"].String() = "toroidal";
	EXPECT_THROW(loadOriginal(header, objects, surface, underground), std::runtime_error);
}

TEST(MapFormat, ToroidalInteractiveOverlapRejectsUnstableVisitOrder)
{
	auto bytes = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false).playerActive(PlayerColor(0))
		.resource(int3(15, 15, 0), GameResID::GOLD, 10)
		.resource(int3(16, 15, 0), GameResID::WOOD, 5).build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("WrappedInteractiveOrder", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	std::vector<CGObjectInstance *> resources;
	for(const auto & object : map->objects)
		if(object && object->ID == Obj::RESOURCE)
			resources.push_back(object.get());
	ASSERT_EQ(resources.size(), 2u);
	const auto makeAppearance = [](const CGObjectInstance & original, const std::string & mask)
	{
		JsonNode definition;
		original.appearance->writeJson(definition, false);
		definition["mask"] = JsonNode(mask.data(), mask.size(), "WrappedInteractiveOrder");
		auto result = std::make_shared<ObjectTemplate>();
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		result->readJson(definition, resolver, false);
		return result;
	};
	map->hideObject(resources[0]);
	resources[0]->pos = int3(35, 10, 0);
	resources[0]->appearance = makeAppearance(*resources[0], R"([ "A" ])");
	map->showObject(resources[0]);
	map->hideObject(resources[1]);
	resources[1]->pos = int3(35, 10, 0);
	resources[1]->appearance = makeAppearance(*resources[1], R"([ "A" ])");
	map->showObject(resources[1]);
	map->hideObject(resources[0]);
	map->showObject(resources[0]);
	CMemoryBuffer boundedArchive;
	{
		CMapSaverJson saver(&boundedArchive);
		EXPECT_NO_THROW(saver.saveMap(map));
	}
	map->hideObject(resources[1]);
	resources[1]->pos = int3(0, 10, 0);
	resources[1]->appearance = makeAppearance(*resources[1], R"([ "A0" ])");
	map->showObject(resources[1]);
	map->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);
	const int3 overlap(35, 10, 0);
	ASSERT_EQ(map->getTile(overlap).visitableObjects.size(), 2u);
	ASSERT_EQ(map->getTile(overlap).topVisitableObj(), resources[1]->id);
	const auto topName = resources[1]->instanceName;
	CMemoryBuffer archive;
	{
		CMapSaverJson saver(&archive);
		saver.saveMap(map);
	}
	if(const char * outputPath = std::getenv("VCMI_TOROID_OVERLAP_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream output(outputPath, std::ios::binary);
		ASSERT_TRUE(output);
		output.write(reinterpret_cast<const char *>(archive.getBuffer().data()), archive.getSize());
		output.close();
		ASSERT_TRUE(output);
	}
	archive.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&archive);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	const auto & tile = reopened->getTile(overlap);
	ASSERT_EQ(tile.visitableObjects.size(), 2u);
	const auto * selected = reopened->getObject(tile.topVisitableObj());
	ASSERT_NE(selected, nullptr);
	EXPECT_EQ(selected->instanceName, topName);

	map->hideObject(resources[0]);
	map->showObject(resources[0]);
	ASSERT_EQ(map->getTile(overlap).topVisitableObj(), resources[0]->id);
	CMemoryBuffer unstableArchive;
	CMapSaverJson unstableSaver(&unstableArchive);
	EXPECT_THROW(unstableSaver.saveMap(map), std::runtime_error);
}

TEST(MapFormat, AdaptedToroidalDemonstrationFixture)
{
	auto bytes = makeToroidalDemoMap().build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("ToroidSeamLaboratory", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	removeTestModEntitiesFromDemo(*map);
	ASSERT_FALSE(map->toroidal);
	map->setToroidalAndRebuild(true);
	map->name = MetaString::createFromRawString("Toroid Seam Demo");
	for(const auto & object : map->objects)
		if(object)
			ASSERT_FALSE(map->hasStaticFootprintAcrossToroidalSeam(*object)) << object->instanceName;
	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	expectDemoHasNoTestModReferences(output);
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&output);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	EXPECT_EQ(reopened->getHeroesOnMap().size(), 2u);
	EXPECT_EQ(reopened->getAllTowns().size(), 2u);
	for(int x = 0; x < reopened->width; ++x)
		for(int y = 0; y < reopened->height; ++y)
			EXPECT_LE(reopened->getTile(int3(x, y, 0)).visitableObjects.size(), 1u) << x << "," << y;
	EXPECT_EQ(reopened->getTile(int3(35, 35, 0)).visitableObjects.size(), 1u);
	EXPECT_EQ(reopened->getTile(int3(35, 1, 0)).visitableObjects.size(), 1u);
	EXPECT_EQ(reopened->getTile(int3(1, 35, 0)).visitableObjects.size(), 1u);
	if(const char * outputPath = std::getenv("VCMI_TOROID_DEMO_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream file(outputPath, std::ios::binary);
		ASSERT_TRUE(file);
		file.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
		file.close();
		ASSERT_TRUE(file);
	}
}

TEST(MapFormat, ToroidalCrossroadsScenarioRoundTrips)
{
	auto bytes = makeToroidalPlaygroundMap().build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("ToroidCrossroads", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	removeTestModEntitiesFromDemo(*map);
	map->setToroidalAndRebuild(true);

	CRandomGenerator random(20261115);
	auto * editor = map->getEditManager();
	std::vector<int3> terrain, road;
	for(int dx : {0, 18})
		for(int x : {33, 34, 35, 0, 1, 2})
			for(int y = 10; y <= 14; ++y)
				terrain.emplace_back((x + dx) % 36, (y + dx) % 36, 0);
	editor->getTerrainSelection().setSelection(terrain);
	editor->drawTerrain(ETerrainId::DIRT, 0, &random);
	for(int n = 0; n < 36; ++n)
		for(int axis : {0, 18})
		{
			road.emplace_back(n, axis, 0);
			road.emplace_back(axis, n, 0);
		}
	editor->getTerrainSelection().setSelection(road);
	editor->drawRoad(RoadId::DIRT_ROAD, &random);

	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	expectDemoHasNoTestModReferences(output);
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&output);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	ASSERT_EQ(reopened->getHeroesOnMap().size(), 2u);
	ASSERT_EQ(reopened->getAllTowns().size(), 2u);
	int mines = 0, monsters = 0, resources = 0;
	for(const auto & object : reopened->objects)
		if(object)
		{
			mines += object->ID == Obj::MINE;
			monsters += object->ID == Obj::MONSTER;
			resources += object->ID == Obj::RESOURCE;
			EXPECT_FALSE(reopened->hasStaticFootprintAcrossToroidalSeam(*object)) << object->instanceName;
		}
	EXPECT_EQ(mines, 4);
	EXPECT_EQ(monsters, 2);
	EXPECT_EQ(resources, 6);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			EXPECT_LE(reopened->getTile(int3(x, y, 0)).visitableObjects.size(), 1u) << x << "," << y;
	for(const auto & pos : road)
		EXPECT_EQ(reopened->getTile(pos).getRoadID(), RoadId::DIRT_ROAD) << pos.toString();
	for(const auto & pos : {int3(35, 12, 0), int3(0, 12, 0), int3(17, 30, 0), int3(18, 30, 0)})
		EXPECT_EQ(reopened->getTile(pos).getTerrainID(), ETerrainId::DIRT);
	ASSERT_FALSE(testing::Test::HasFailure());
	if(const char * outputPath = std::getenv("VCMI_TOROID_CROSSROADS_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream file(outputPath, std::ios::binary);
		ASSERT_TRUE(file);
		file.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
		file.close();
		ASSERT_TRUE(file);
	}
}

TEST(MapFormat, ToroidalBoatDemonstrationFixtureRoundTrips)
{
	const PlayerColor red(0), blue(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).name("Toroid Boat Laboratory")
		.description("Experimental seam-crossing boat and shore fixture.")
		.playerActive(red).playerActive(blue)
		.playerMainTown(red, int3(8, 8, 0), false)
		.playerMainTown(blue, int3(27, 27, 0), false)
		.town(int3(8, 8, 0), FactionID(0), red)
		.town(int3(27, 27, 0), FactionID(0), blue)
		.hero(int3(1, 0, 0), HeroTypeID(0), red)
		.hero(int3(19, 18, 0), HeroTypeID(1), blue);
	const auto bytes = builder.build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("ToroidBoatLaboratory", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	removeTestModEntitiesFromDemo(*map);
	map->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);

	CRandomGenerator random(12345);
	std::vector<int3> water;
	for(int x : {32, 33, 34, 35, 0, 1, 2, 3, 4})
		for(int y : {32, 33, 34, 35, 0, 1, 2, 3, 4})
			if(x >= 32 || y >= 32)
				water.emplace_back(x, y, 0);
	auto * editor = map->getEditManager();
	editor->getTerrainSelection().setSelection(water);
	editor->drawTerrain(ETerrainId::WATER, 0, &random);

	const int3 boatPosition(35, 0, 0);
	auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::BOAT, BoatId::CASTLE);
	const auto templates = factory->getTemplates(ETerrainId::WATER);
	ASSERT_FALSE(templates.empty());
	auto boat = factory->create(&callback, templates.front());
	GameRandomizer objectRandomizer(callback);
	objectRandomizer.setSeed(12345);
	factory->configureObject(boat.get(), objectRandomizer);
	boat->setAnchorPos(boatPosition + boat->getVisitableOffset());
	map->generateUniqueInstanceName(boat.get());
	map->addNewObject(boat);
	ASSERT_EQ(boat->visitablePos(), boatPosition);

	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	expectDemoHasNoTestModReferences(output);
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&output);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	EXPECT_FALSE(reopened->getTile(int3(0, 0, 0)).isWater());
	for(const auto & position : water)
		EXPECT_TRUE(reopened->getTile(position).isWater()) << position.toString();
	const auto & objects = reopened->getTile(boatPosition).visitableObjects;
	ASSERT_FALSE(objects.empty());
	EXPECT_EQ(reopened->getObject(objects.back())->ID, Obj::BOAT);
	ASSERT_FALSE(testing::Test::HasFailure());
	if(const char * outputPath = std::getenv("VCMI_TOROID_BOAT_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream file(outputPath, std::ios::binary);
		ASSERT_TRUE(file);
		file.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
		file.close();
		ASSERT_TRUE(file);
	}
}

TEST(MapFormat, ToroidalOpenWaterCornerFixtureRoundTrips)
{
	const PlayerColor red(0), blue(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).name("Toroid Open Water Corner")
		.description("Experimental open-water diagonal corner fixture.")
		.playerActive(red).playerActive(blue)
		.playerMainTown(red, int3(28, 28, 0), false)
		.playerMainTown(blue, int3(12, 12, 0), false)
		.town(int3(28, 28, 0), FactionID(0), red)
		.town(int3(12, 12, 0), FactionID(0), blue)
		.hero(int3(31, 30, 0), HeroTypeID(0), red)
		.hero(int3(11, 14, 0), HeroTypeID(1), blue);
	const auto bytes = builder.build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("ToroidOpenWaterCorner", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	removeTestModEntitiesFromDemo(*map);
	map->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);

	CRandomGenerator random(12345);
	std::vector<int3> water;
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
		{
			const bool redIsland = x >= 25 && x <= 30 && y >= 25 && y <= 30;
			const bool blueIsland = x >= 7 && x <= 16 && y >= 7 && y <= 16;
			if(!redIsland && !blueIsland)
				water.emplace_back(x, y, 0);
		}
	auto * editor = map->getEditManager();
	editor->getTerrainSelection().setSelection(water);
	editor->drawTerrain(ETerrainId::WATER, 0, &random);

	const int3 boatPosition(31, 30, 0);
	auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::BOAT, BoatId::CASTLE);
	const auto templates = factory->getTemplates(ETerrainId::WATER);
	ASSERT_FALSE(templates.empty());
	auto boat = factory->create(&callback, templates.front());
	GameRandomizer objectRandomizer(callback);
	objectRandomizer.setSeed(12345);
	factory->configureObject(boat.get(), objectRandomizer);
	boat->setAnchorPos(boatPosition + boat->getVisitableOffset());
	map->generateUniqueInstanceName(boat.get());
	map->addNewObject(boat);
	ASSERT_EQ(boat->visitablePos(), boatPosition);

	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	expectDemoHasNoTestModReferences(output);
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&output);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	EXPECT_FALSE(reopened->getTile(int3(30, 30, 0)).isWater());
	for(const auto & position : water)
		EXPECT_TRUE(reopened->getTile(position).isWater()) << position.toString();
	const auto & objects = reopened->getTile(boatPosition).visitableObjects;
	ASSERT_FALSE(objects.empty());
	EXPECT_EQ(reopened->getObject(objects.back())->ID, Obj::BOAT);
	ASSERT_FALSE(testing::Test::HasFailure());
	if(const char * outputPath = std::getenv("VCMI_TOROID_OPEN_WATER_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream file(outputPath, std::ios::binary);
		ASSERT_TRUE(file);
		file.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
		file.close();
		ASSERT_TRUE(file);
	}
}

TEST(MapFormat, ToroidalVisualSeamFixtureRoundTrips)
{
	auto bytes = makeToroidalDemoMap().build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("ToroidVisualSeams", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	removeTestModEntitiesFromDemo(*map);
	map->setToroidalAndRebuild(true);
	map->name = MetaString::createFromRawString("Toroid Roads Demo");

	CRandomGenerator random(12345);
	auto * editor = map->getEditManager();
	std::vector<int3> terrain;
	for(int x : {33, 34, 35, 0, 1, 2})
		for(int y = 10; y <= 15; ++y)
			terrain.emplace_back(x, y, 0);
	editor->getTerrainSelection().setSelection(terrain);
	editor->drawTerrain(ETerrainId::DIRT, 0, &random);

	std::vector<int3> road, river;
	for(int n : {33, 34, 35, 0, 1, 2, 3})
	{
		road.emplace_back(n, 3, 0);
		road.emplace_back(10, n, 0);
		river.emplace_back(5, n, 0);
	}
	editor->getTerrainSelection().setSelection(road);
	editor->drawRoad(RoadId::DIRT_ROAD, &random);
	editor->getTerrainSelection().setSelection(river);
	editor->drawRiver(RiverId::WATER_RIVER, &random);

	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	expectDemoHasNoTestModReferences(output);
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&output);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	for(const auto & pos : road)
		EXPECT_EQ(reopened->getTile(pos).getRoadID(), RoadId::DIRT_ROAD) << pos.toString();
	for(const auto & pos : river)
		EXPECT_EQ(reopened->getTile(pos).getRiverID(), RiverId::WATER_RIVER) << pos.toString();
	for(const int x : {35, 0})
		EXPECT_EQ(reopened->getTile(int3(x, 12, 0)).getTerrainID(), ETerrainId::DIRT);
	for(const auto & pos : {int3(35, 3, 0), int3(0, 3, 0), int3(10, 35, 0), int3(10, 0, 0), int3(5, 35, 0), int3(5, 0, 0)})
	{
		const auto & before = map->getTile(pos);
		const auto & after = reopened->getTile(pos);
		EXPECT_EQ(after.roadDir, before.roadDir);
		EXPECT_EQ(after.riverDir, before.riverDir);
		EXPECT_EQ(after.extTileFlags, before.extTileFlags);
	}
	if(const char * outputPath = std::getenv("VCMI_TOROID_VISUAL_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream file(outputPath, std::ios::binary);
		ASSERT_TRUE(file);
		file.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
		file.close();
		ASSERT_TRUE(file);
	}
}

TEST(MapFormat, PreparesExternalToroidalSeamRoad)
{
	const char * inputPath = std::getenv("VCMI_INSPECT_START_VMAP");
	const char * outputPath = std::getenv("VCMI_EARTH_ROAD_OUTPUT");
	if(!inputPath || !*inputPath || !outputPath || !*outputPath)
		GTEST_SKIP() << "Set VCMI_INSPECT_START_VMAP and VCMI_EARTH_ROAD_OUTPUT";
	ASSERT_FALSE(boost::filesystem::exists(outputPath));
	std::ifstream file(inputPath, std::ios::binary);
	ASSERT_TRUE(file);
	std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderJson reader(&input);
	auto map = reader.loadMap(&callback);
	ASSERT_NE(map, nullptr);
	callback.setMap(map.get());
	ASSERT_TRUE(map->toroidal);
	ASSERT_GE(map->width, 6);
	const bool connectRoad = std::getenv("VCMI_EARTH_CONNECT_ROAD") != nullptr;
	const auto findConnection = [&](const int3 & start) -> std::vector<int3>
	{
		const int width = map->width;
		const int height = map->height;
		const int startIndex = start.y * width + start.x;
		std::vector<int> parent(width * height, -1);
		std::vector<int> queue{startIndex};
		parent[startIndex] = startIndex;
		int target = -1;
		for(size_t next = 0; next < queue.size() && target < 0; ++next)
		{
			const int current = queue[next];
			const int x = current % width;
			const int y = current / width;
			for(const auto & [dx, dy] : std::array<std::pair<int, int>, 8>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1},
				{-1, -1}, {-1, 1}, {1, -1}, {1, 1}}})
			{
				const int nx = x + dx, ny = y + dy;
				if(nx < 0 || nx >= width || ny < 0 || ny >= height)
					continue;
				const int index = ny * width + nx;
				if(parent[index] != -1)
					continue;
				const auto & tile = map->getTile(int3(nx, ny, 0));
				if(!tile.isLand() || tile.blocked() || tile.visitable() || tile.getRiverID() != RiverId::NO_RIVER
				   || !map->canMoveBetween(int3(x, y, 0), int3(nx, ny, 0)))
					continue;
				parent[index] = current;
				if(tile.getRoadID() != RoadId::NO_ROAD)
				{
					target = index;
					break;
				}
				if(tile.getRoadID() == RoadId::NO_ROAD)
					queue.push_back(index);
			}
		}
		std::vector<int3> result;
		if(target < 0)
			return result;
		for(int current = parent[target]; current != startIndex; current = parent[current])
			result.emplace_back(current % width, current / width, 0);
		result.emplace_back(target % width, target / width, 0);
		return result;
	};
	std::vector<int3> rightEdgeConnection, leftEdgeConnection;
	std::vector<int3> road;
	int candidates = 0, rightEdgeReachable = 0, leftEdgeReachable = 0;
	for(int sideLength = 3; sideLength >= 1 && road.empty(); --sideLength)
	{
		for(int y = 0; y < map->height && road.empty(); ++y)
		{
			std::vector<int3> candidate;
			for(int offset = -sideLength; offset < sideLength; ++offset)
			{
				const int x = offset < 0 ? map->width + offset : offset;
				const int3 pos(x, y, 0);
				const auto & tile = map->getTile(pos);
				if(!tile.isLand() || tile.blocked() || tile.visitable()
				   || tile.getRoadID() != RoadId::NO_ROAD || tile.getRiverID() != RiverId::NO_RIVER)
					break;
				candidate.push_back(pos);
			}
			if(candidate.size() != static_cast<size_t>(2 * sideLength))
				continue;
			++candidates;
			if(connectRoad)
			{
				rightEdgeConnection = findConnection(candidate.front());
				leftEdgeConnection = findConnection(candidate.back());
				rightEdgeReachable += !rightEdgeConnection.empty();
				leftEdgeReachable += !leftEdgeConnection.empty();
				if(rightEdgeConnection.empty() || leftEdgeConnection.empty())
					continue;
			}
			road = std::move(candidate);
		}
	}
	if(std::getenv("VCMI_EARTH_EXPECT_NO_CONNECTION"))
	{
		ASSERT_TRUE(connectRoad);
		EXPECT_GT(candidates, 0);
		EXPECT_TRUE(road.empty());
		std::cout << "Road site candidates=" << candidates << ", right edge connected=" << rightEdgeReachable
		          << ", left edge connected=" << leftEdgeReachable << std::endl;
		return;
	}
	ASSERT_GE(road.size(), 2u) << "No eligible seam road; candidates=" << candidates
		<< ", right edge connected=" << rightEdgeReachable << ", left edge connected=" << leftEdgeReachable;
	if(connectRoad)
	{
		std::cout << "Road connection lengths=" << rightEdgeConnection.size() << "," << leftEdgeConnection.size() << std::endl;
		for(const auto * connection : {&rightEdgeConnection, &leftEdgeConnection})
			for(const auto & pos : *connection)
				if(map->getTile(pos).getRoadID() == RoadId::NO_ROAD && !vstd::contains(road, pos))
					road.push_back(pos);
	}
	CRandomGenerator random(12345);
	auto * editor = map->getEditManager();
	editor->getTerrainSelection().setSelection(road);
	editor->drawRoad(RoadId::DIRT_ROAD, &random);
	for(const auto & pos : road)
		ASSERT_EQ(map->getTile(pos).getRoadID(), RoadId::DIRT_ROAD);
	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reopenedReader(&output);
	auto reopened = reopenedReader.loadMap(&reopenedCallback);
	ASSERT_NE(reopened, nullptr);
	ASSERT_TRUE(reopened->toroidal);
	for(const auto & pos : road)
	{
		const auto & before = map->getTile(pos);
		const auto & after = reopened->getTile(pos);
		EXPECT_EQ(after.getRoadID(), RoadId::DIRT_ROAD);
		EXPECT_EQ(after.roadDir, before.roadDir);
		EXPECT_EQ(after.extTileFlags, before.extTileFlags);
	}
	std::ofstream out(outputPath, std::ios::binary);
	ASSERT_TRUE(out);
	out.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
	out.close();
	ASSERT_TRUE(out);
	std::cout << "Seam road row=" << road.front().y << " tiles=" << road.size() << std::endl;
}

TEST(MapFormat, PreparesEarthConnectedSeamRoad)
{
	const char * inputPath = std::getenv("VCMI_INSPECT_START_VMAP");
	const char * outputPath = std::getenv("VCMI_EARTH_SEAM_ROAD_OUTPUT");
	if(!inputPath || !*inputPath || !outputPath || !*outputPath)
		GTEST_SKIP() << "Set VCMI_INSPECT_START_VMAP and VCMI_EARTH_SEAM_ROAD_OUTPUT";
	ASSERT_FALSE(boost::filesystem::exists(outputPath));
	std::ifstream file(inputPath, std::ios::binary);
	ASSERT_TRUE(file);
	std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderJson reader(&input);
	auto map = reader.loadMap(&callback);
	ASSERT_NE(map, nullptr);
	callback.setMap(map.get());
	ASSERT_TRUE(map->toroidal);
	ASSERT_EQ(map->width, 252);
	ASSERT_EQ(map->height, 576);
	const auto cactus = map->instanceNames.find("cactus_7575");
	ASSERT_NE(cactus, map->instanceNames.end());
	ASSERT_FALSE(cactus->second->isVisitable());
	ASSERT_TRUE(vstd::contains(map->getTile(int3(250, 425, 0)).blockingObjects, cactus->second->id));
	map->removeObject(cactus->second->id);
	for(const auto & [name, anchor] : std::array{
		std::pair{"yuccaTrees_7568", int3(251, 430, 0)}, std::pair{"mountain_7569", int3(1, 431, 0)},
		std::pair{"deadVegetation_9592", int3(2, 285, 0)}})
	{
		const auto decoration = map->instanceNames.find(name);
		ASSERT_NE(decoration, map->instanceNames.end()) << name;
		ASSERT_FALSE(decoration->second->isVisitable());
		ASSERT_EQ(decoration->second->anchorPos(), anchor);
		map->removeObject(decoration->second->id);
	}
	const std::vector<int3> road = {{250, 425, 0}, {251, 425, 0}, {0, 425, 0}, {1, 425, 0}};
	for(const auto & pos : road)
	{
		const auto & tile = map->getTile(pos);
		ASSERT_TRUE(tile.isLand()) << pos.toString();
		ASSERT_FALSE(tile.blocked()) << pos.toString();
		ASSERT_FALSE(tile.visitable()) << pos.toString();
		ASSERT_EQ(tile.getRiverID(), RiverId::NO_RIVER) << pos.toString();
	}
	ASSERT_NE(map->getTile(int3(2, 425, 0)).getRoadID(), RoadId::NO_ROAD);
	ASSERT_TRUE(map->canMoveBetween(road.back(), int3(2, 425, 0)));
	ASSERT_TRUE(map->canMoveBetween(int3(2, 425, 0), road.back()));
	for(size_t i = 1; i < road.size(); ++i)
	{
		ASSERT_TRUE(map->canMoveBetween(road[i - 1], road[i]));
		ASSERT_TRUE(map->canMoveBetween(road[i], road[i - 1]));
	}
	CRandomGenerator random(12345);
	auto * editor = map->getEditManager();
	editor->getTerrainSelection().setSelection(road);
	editor->drawRoad(RoadId::DIRT_ROAD, &random);
	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reopenedReader(&output);
	auto reopened = reopenedReader.loadMap(&reopenedCallback);
	ASSERT_NE(reopened, nullptr);
	ASSERT_TRUE(reopened->toroidal);
	for(const auto & pos : road)
		ASSERT_EQ(reopened->getTile(pos).getRoadID(), RoadId::DIRT_ROAD) << pos.toString();
	ASSERT_EQ(reopened->instanceNames.count("cactus_7575"), 0u);
	ASSERT_EQ(reopened->instanceNames.count("yuccaTrees_7568"), 0u);
	ASSERT_EQ(reopened->instanceNames.count("mountain_7569"), 0u);
	ASSERT_EQ(reopened->instanceNames.count("deadVegetation_9592"), 0u);
	ASSERT_EQ(reopened->instanceNames.count("monolithTwoWay_11148"), 1u);
	ASSERT_EQ(reopened->instanceNames.count("randomResource_11819"), 1u);
	ASSERT_EQ(reopened->instanceNames.count("randomMonsterLevel3_6689"), 1u);
	std::ofstream out(outputPath, std::ios::binary);
	ASSERT_TRUE(out);
	out.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
	out.close();
	ASSERT_TRUE(out);
}

TEST(MapFormat, PreparesEarthPortalRoadFixture)
{
	const char * inputPath = std::getenv("VCMI_INSPECT_START_VMAP");
	const char * outputPath = std::getenv("VCMI_EARTH_PORTAL_ROAD_OUTPUT");
	if(!inputPath || !*inputPath || !outputPath || !*outputPath)
		GTEST_SKIP() << "Set VCMI_INSPECT_START_VMAP and VCMI_EARTH_PORTAL_ROAD_OUTPUT";
	ASSERT_FALSE(boost::filesystem::exists(outputPath));
	std::ifstream file(inputPath, std::ios::binary);
	ASSERT_TRUE(file);
	std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderJson reader(&input);
	auto map = reader.loadMap(&callback);
	ASSERT_NE(map, nullptr);
	callback.setMap(map.get());
	ASSERT_TRUE(map->toroidal);
	ASSERT_EQ(map->width, 252);
	ASSERT_EQ(map->height, 576);
	for(const auto & [name, anchor] : std::array{
		std::pair{"yuccaTrees_7568", int3(251, 430, 0)}, std::pair{"mountain_7569", int3(1, 431, 0)}})
	{
		const auto decoration = map->instanceNames.find(name);
		ASSERT_NE(decoration, map->instanceNames.end());
		ASSERT_FALSE(decoration->second->isVisitable());
		ASSERT_EQ(decoration->second->anchorPos(), anchor);
		map->removeObject(decoration->second->id);
	}
	const auto portal = map->instanceNames.find("monolithTwoWay_5267");
	ASSERT_NE(portal, map->instanceNames.end());
	ASSERT_EQ(portal->second->visitablePos(), int3(250, 429, 0));
	ASSERT_TRUE(map->canMoveBetween(int3(250, 430, 0), portal->second->visitablePos()));
	const std::vector<int3> road = {
		int3(251, 425, 0), int3(0, 425, 0), int3(1, 425, 0),
		int3(1, 426, 0), int3(1, 427, 0), int3(0, 428, 0),
		int3(0, 429, 0), int3(0, 430, 0), int3(251, 430, 0), int3(250, 430, 0)};
	for(const auto & pos : road)
	{
		const auto & tile = map->getTile(pos);
		if(tile.blocked())
			for(const auto & id : tile.blockingObjects)
				std::cout << "Blocked road cell " << pos.toString() << ": " << map->getObject(id)->instanceName << std::endl;
		ASSERT_TRUE(tile.isLand()) << pos.toString();
		ASSERT_FALSE(tile.blocked()) << pos.toString();
		ASSERT_FALSE(tile.visitable()) << pos.toString();
		ASSERT_EQ(tile.getRiverID(), RiverId::NO_RIVER) << pos.toString();
	}
	ASSERT_NE(map->getTile(int3(2, 425, 0)).getRoadID(), RoadId::NO_ROAD);
	CRandomGenerator random(12345);
	auto * editor = map->getEditManager();
	editor->getTerrainSelection().setSelection(road);
	editor->drawRoad(RoadId::DIRT_ROAD, &random);
	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reopenedReader(&output);
	auto reopened = reopenedReader.loadMap(&reopenedCallback);
	ASSERT_NE(reopened, nullptr);
	ASSERT_TRUE(reopened->toroidal);
	for(const auto & pos : road)
		EXPECT_EQ(reopened->getTile(pos).getRoadID(), RoadId::DIRT_ROAD) << pos.toString();
	EXPECT_EQ(reopened->instanceNames.count("yuccaTrees_7568"), 0u);
	EXPECT_EQ(reopened->instanceNames.count("mountain_7569"), 0u);
	EXPECT_EQ(reopened->instanceNames.count("monolithTwoWay_5267"), 1u);
	std::ofstream out(outputPath, std::ios::binary);
	ASSERT_TRUE(out);
	out.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
	out.close();
	ASSERT_TRUE(out);
}

TEST(MapFormat, PreparesEarthWaterPortalRepair)
{
	const char * inputPath = std::getenv("VCMI_INSPECT_START_VMAP");
	const char * outputPath = std::getenv("VCMI_EARTH_WATER_PORTAL_OUTPUT");
	if(!inputPath || !*inputPath || !outputPath || !*outputPath)
		GTEST_SKIP() << "Set VCMI_INSPECT_START_VMAP and VCMI_EARTH_WATER_PORTAL_OUTPUT";
	ASSERT_FALSE(boost::filesystem::exists(outputPath));
	std::ifstream file(inputPath, std::ios::binary);
	ASSERT_TRUE(file);
	std::vector<ui8> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	ASSERT_FALSE(bytes.empty());
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderJson reader(&input);
	auto map = reader.loadMap(&callback);
	ASSERT_NE(map, nullptr);
	callback.setMap(map.get());
	ASSERT_TRUE(map->toroidal);
	ASSERT_EQ(map->width, 252);
	ASSERT_EQ(map->height, 576);
	const auto portal = map->instanceNames.find("monolithTwoWay_5267");
	ASSERT_NE(portal, map->instanceNames.end());
	ASSERT_EQ(portal->second->anchorPos(), int3(251, 429, 0));
	ASSERT_TRUE(map->getTile(portal->second->visitablePos()).isLand());
	for(int x : {238, 239, 240})
	{
		const auto & tile = map->getTile(int3(x, 429, 0));
		ASSERT_TRUE(tile.isWater());
		ASSERT_FALSE(tile.blocked());
		ASSERT_FALSE(tile.visitable());
		ASSERT_EQ(tile.getRiverID(), RiverId::NO_RIVER);
	}
	const int3 approach(239, 430, 0);
	ASSERT_TRUE(map->getTile(approach).isWater());
	ASSERT_FALSE(map->getTile(approach).blocked());
	ASSERT_FALSE(map->getTile(approach).visitable());
	map->moveObject(portal->second->id, int3(240, 429, 0));
	ASSERT_EQ(portal->second->visitablePos(), int3(239, 429, 0));
	ASSERT_TRUE(map->canMoveBetween(approach, portal->second->visitablePos()));
	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reopenedReader(&output);
	auto reopened = reopenedReader.loadMap(&reopenedCallback);
	ASSERT_NE(reopened, nullptr);
	const auto restored = reopened->instanceNames.find("monolithTwoWay_5267");
	ASSERT_NE(restored, reopened->instanceNames.end());
	EXPECT_EQ(restored->second->visitablePos(), int3(239, 429, 0));
	EXPECT_TRUE(reopened->getTile(restored->second->visitablePos()).isWater());
	EXPECT_TRUE(reopened->canMoveBetween(approach, restored->second->visitablePos()));
	std::ofstream out(outputPath, std::ios::binary);
	ASSERT_TRUE(out);
	out.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
	out.close();
	ASSERT_TRUE(out);
}

TEST(MapFormat, ToroidalMixedRoadTurnsRoundTrip)
{
	auto bytes = makeToroidalDemoMap().build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("ToroidMixedRoads", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	removeTestModEntitiesFromDemo(*map);
	map->setToroidalAndRebuild(true);
	map->name = MetaString::createFromRawString("Toroid Mixed Road Turns");

	// Each seam shape has an identical interior control, including road types.
	const std::vector<int3> dirt = {
		int3(34, 4, 0), int3(35, 4, 0), int3(14, 4, 0), int3(15, 4, 0)};
	const std::vector<int3> gravel = {
		int3(0, 4, 0), int3(1, 4, 0), int3(16, 4, 0), int3(17, 4, 0),
		int3(10, 34, 0), int3(10, 35, 0), int3(25, 14, 0), int3(25, 15, 0)};
	const std::vector<int3> cobblestone = {
		int3(10, 0, 0), int3(10, 1, 0), int3(25, 16, 0), int3(25, 17, 0),
		int3(0, 35, 0), int3(0, 0, 0), int3(1, 0, 0),
		int3(20, 19, 0), int3(20, 20, 0), int3(21, 20, 0)};
	CRandomGenerator random(12345);
	auto * editor = map->getEditManager();
	for(const auto & [positions, type] : {
		std::pair{&dirt, RoadId::DIRT_ROAD},
		std::pair{&gravel, RoadId::GRAVEL_ROAD},
		std::pair{&cobblestone, RoadId::COBBLESTONE_ROAD}})
	{
		editor->getTerrainSelection().setSelection(*positions);
		editor->drawRoad(type, &random);
	}

	const auto compareShape = [&](const CMap & checked, int3 seam, int3 interior, int minDir, int maxDir)
	{
		const auto & actual = checked.getTile(seam);
		const auto & expected = checked.getTile(interior);
		EXPECT_EQ(actual.getRoadID(), expected.getRoadID());
		EXPECT_GE(actual.roadDir, minDir);
		EXPECT_LE(actual.roadDir, maxDir);
		EXPECT_GE(expected.roadDir, minDir);
		EXPECT_LE(expected.roadDir, maxDir);
		EXPECT_EQ(actual.extTileFlags & 0b00110000, expected.extTileFlags & 0b00110000);
	};
	compareShape(*map, int3(35, 4, 0), int3(15, 4, 0), 12, 13);
	compareShape(*map, int3(10, 35, 0), int3(25, 15, 0), 10, 11);
	compareShape(*map, int3(0, 0, 0), int3(20, 20, 0), 0, 1);

	CMemoryBuffer output;
	{
		CMapSaverJson saver(&output);
		saver.saveMap(map);
	}
	expectDemoHasNoTestModReferences(output);
	output.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&output);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	ASSERT_TRUE(reopened->toroidal);
	for(const auto * positions : {&dirt, &gravel, &cobblestone})
		for(const auto & pos : *positions)
		{
			const auto & before = map->getTile(pos);
			const auto & after = reopened->getTile(pos);
			EXPECT_EQ(after.getRoadID(), before.getRoadID()) << pos.toString();
			EXPECT_EQ(after.roadDir, before.roadDir) << pos.toString();
			EXPECT_EQ(after.extTileFlags, before.extTileFlags) << pos.toString();
		}
	compareShape(*reopened, int3(35, 4, 0), int3(15, 4, 0), 12, 13);
	compareShape(*reopened, int3(10, 35, 0), int3(25, 15, 0), 10, 11);
	compareShape(*reopened, int3(0, 0, 0), int3(20, 20, 0), 0, 1);

	if(const char * outputPath = std::getenv("VCMI_TOROID_MIXED_ROAD_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream file(outputPath, std::ios::binary);
		ASSERT_TRUE(file);
		file.write(reinterpret_cast<const char *>(output.getBuffer().data()), output.getSize());
		file.close();
		ASSERT_TRUE(file);
	}
}

TEST(MapFormat, ToroidalVisualWrappedTownFixtureRoundTrips)
{
	auto bytes = makeToroidalDemoMap().build();
	CMemoryBuffer input;
	input.write(bytes.data(), static_cast<si64>(bytes.size()));
	input.seek(0);
	EditorCallback callback(nullptr);
	CMapLoaderH3M loader("ToroidVisualWrappedTown", "core", "ASCII", &input);
	auto map = loader.loadMap(&callback);
	callback.setMap(map.get());
	removeTestModEntitiesFromDemo(*map);
	ASSERT_EQ(map->getAllTowns().size(), 2u);
	auto * town = map->getObject(map->getAllTowns().back());
	ASSERT_NE(town, nullptr);
	map->hideObject(town);
	town->pos = int3(0, 4, 0);
	map->showObject(town);
	map->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);
	map->name = MetaString::createFromRawString("Toroid Wrapped Town");
	ASSERT_TRUE(map->hasStaticFootprintAcrossToroidalSeam(*town));
	const auto name = town->instanceName;
	CMemoryBuffer archive;
	{
		CMapSaverJson saver(&archive);
		saver.saveMap(map);
	}
	expectDemoHasNoTestModReferences(archive);
	archive.seek(0);
	EditorCallback reopenedCallback(nullptr);
	CMapLoaderJson reader(&archive);
	auto reopened = reader.loadMap(&reopenedCallback);
	reopenedCallback.setMap(reopened.get());
	const auto found = reopened->instanceNames.find(name);
	ASSERT_NE(found, reopened->instanceNames.end());
	EXPECT_TRUE(reopened->hasStaticFootprintAcrossToroidalSeam(*found->second));
	if(const char * outputPath = std::getenv("VCMI_TOROID_VISUAL_TOWN_OUTPUT"))
	{
		ASSERT_FALSE(boost::filesystem::exists(outputPath));
		std::ofstream file(outputPath, std::ios::binary);
		ASSERT_TRUE(file);
		file.write(reinterpret_cast<const char *>(archive.getBuffer().data()), archive.getSize());
		file.close();
		ASSERT_TRUE(file);
	}
}
