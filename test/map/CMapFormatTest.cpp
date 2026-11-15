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
#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormatH3M.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/modding/IdentifierResolver.h"

#include "../../lib/VCMIDirs.h"

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
