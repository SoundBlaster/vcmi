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
#include "../../lib/mapObjects/CGResource.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/gameState/CGameState.h"
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

#include "../../lib/VCMIDirs.h"
#include "../../server/CGameHandler.h"
#include "../../server/battles/BattleProcessor.h"
#include "../../server/activities/Activity.h"
#include "../../server/activities/MapActivities.h"
#include "../../server/activities/ActivityProcessor.h"
#include "../mock/GameHandlerTestServer.h"

#include "../JsonComparer.h"
#include "../mock/TinyH3MBuilder.h"
#include "../mock/ToroidalDemoMap.h"

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
	for(int index = 0; index < static_cast<int>(header->players.size()); ++index)
	{
		const auto & info = header->players[index];
		if(!info.canHumanPlay && !info.canComputerPlay)
			continue;
		auto & player = start.playerInfos[PlayerColor(index)];
		player.color = PlayerColor(index);
		player.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(index));
		player.name = "Inspection";
		player.castle = info.defaultCastle();
		player.hero = info.defaultHero();
		player.bonus = PlayerStartingBonus::GOLD;
	}
	auto state = std::make_shared<CGameState>();
	state->preInit(LIBRARY);
	GameRandomizer randomizer(*state);
	Load::ProgressAccumulator progress;
	state->init(&service, &start, randomizer, progress, false);
	auto & map = state->getMap();
	ASSERT_TRUE(map.toroidal);
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
			GameHandlerTestServer server(state, player);
			CGameHandler handler(server, state);
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(crossing->second),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_EQ(hero->visitablePos(), crossing->second);
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(crossing->first),
			                             EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			ASSERT_EQ(hero->visitablePos(), crossing->first);
			std::cout << "Land seam crossing in both directions: " << crossing->first.toString()
			          << " <-> " << crossing->second.toString() << std::endl;
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
