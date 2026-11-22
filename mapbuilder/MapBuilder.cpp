/*
 * MapBuilder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../lib/CConsoleHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/GameLibrary.h"
#include "../lib/TerrainHandler.h"
#include "../lib/VCMIDirs.h"
#include "../lib/callback/EditorCallback.h"
#include "../lib/filesystem/CMemoryBuffer.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/json/JsonNode.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/MapFormat.h"
#include "../lib/mapping/MapFormatJson.h"
#include "../lib/mapping/ObstacleProxy.h"
#include "../lib/texts/MetaString.h"

namespace
{
constexpr int maxSide = 512;

struct Level
{
	std::vector<std::string> terrain;
	std::vector<std::string> obstacles;
};

/// Terrain grid sent by the map painter; every field is checked because the request comes from a public web page
struct Request
{
	std::string name;
	std::string description;
	bool toroidal = false;
	int seed = 0;
	std::map<char, TerrainId> legend;
	std::vector<Level> levels;
	int width = 0;
	int height = 0;
};

std::vector<std::string> readRows(const JsonNode & node, const std::string & field)
{
	if(!node.isVector())
		throw std::runtime_error(field + " must be an array of strings");
	std::vector<std::string> rows;
	for(const auto & row : node.Vector())
	{
		if(!row.isString())
			throw std::runtime_error(field + " must be an array of strings");
		rows.push_back(row.String());
	}
	return rows;
}

/// Returns the field if it is absent or has the expected type
const JsonNode & optionalField(const JsonNode & node, const std::string & name, JsonNode::JsonType type)
{
	const auto & field = node[name];
	if(!field.isNull() && field.getType() != type)
		throw std::runtime_error("Field '" + name + "' has a wrong type");
	return field;
}

TerrainId findCoreTerrain(const std::string & identifier)
{
	for(const auto & terrain : LIBRARY->terrainTypeHandler->getObjects())
		if(terrain && terrain->getJsonKey() == "core:" + identifier)
			return terrain->getId();
	throw std::runtime_error("Unknown terrain '" + identifier + "'");
}

Request parseRequest(const std::string & path)
{
	std::ifstream input(path, std::ios::binary);
	if(!input.good())
		throw std::runtime_error("Cannot read " + path);
	const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	JsonParsingSettings settings;
	settings.mode = JsonParsingSettings::JsonFormatMode::JSON;
	settings.strict = true;
	const JsonNode json(text.data(), text.size(), settings, path);

	if(!json.isStruct())
		throw std::runtime_error("The request must be a JSON object");

	Request request;
	request.name = optionalField(json, "name", JsonNode::JsonType::DATA_STRING).String();
	request.description = optionalField(json, "description", JsonNode::JsonType::DATA_STRING).String();
	request.toroidal = optionalField(json, "toroidal", JsonNode::JsonType::DATA_BOOL).Bool();
	request.seed = static_cast<int>(optionalField(json, "seed", JsonNode::JsonType::DATA_INTEGER).Integer());

	for(const auto & [key, value] : optionalField(json, "legend", JsonNode::JsonType::DATA_STRUCT).Struct())
	{
		if(key.size() != 1 || !value.isString())
			throw std::runtime_error("legend must map single characters to terrain identifiers");
		request.legend[key.front()] = findCoreTerrain(value.String());
	}

	if(!json["levels"].isVector() || json["levels"].Vector().empty() || json["levels"].Vector().size() > 2)
		throw std::runtime_error("levels must hold one or two levels");
	for(const auto & levelNode : json["levels"].Vector())
	{
		if(!levelNode.isStruct())
			throw std::runtime_error("Each level must be a JSON object");
		Level level;
		level.terrain = readRows(levelNode["terrain"], "terrain");
		if(!levelNode["obstacles"].isNull())
			level.obstacles = readRows(levelNode["obstacles"], "obstacles");
		request.levels.push_back(std::move(level));
	}

	request.height = static_cast<int>(request.levels.front().terrain.size());
	request.width = request.height > 0 ? static_cast<int>(request.levels.front().terrain.front().size()) : 0;
	if(request.width < 1 || request.height < 1 || request.width > maxSide || request.height > maxSide)
		throw std::runtime_error("Map sides must be between 1 and " + std::to_string(maxSide));

	for(const auto & level : request.levels)
	{
		if(static_cast<int>(level.terrain.size()) != request.height)
			throw std::runtime_error("All levels must have the same height");
		for(const auto & row : level.terrain)
		{
			if(static_cast<int>(row.size()) != request.width)
				throw std::runtime_error("All terrain rows must have the same width");
			for(const char cell : row)
				if(!request.legend.contains(cell))
					throw std::runtime_error(std::string("Terrain character '") + cell + "' is missing from the legend");
		}
		if(level.obstacles.empty())
			continue;
		if(static_cast<int>(level.obstacles.size()) != request.height)
			throw std::runtime_error("Obstacle mask must match the terrain size");
		for(const auto & row : level.obstacles)
			if(static_cast<int>(row.size()) != request.width || row.find_first_not_of(".x") != std::string::npos)
				throw std::runtime_error("Obstacle rows must match the terrain width and contain only '.' and 'x'");
	}
	return request;
}

/// Base land goes first, so that later terrains draw their transitions over it; water and rock come last and restore their borders
std::vector<TerrainId> drawingOrder(const Request & request)
{
	std::set<TerrainId> used;
	for(const auto & [letter, terrain] : request.legend)
		used.insert(terrain);

	const auto rank = [](const TerrainId & id)
	{
		const auto * terrain = id.toEntity(LIBRARY);
		if(id == ETerrainId::DIRT)
			return 0;
		if(id == ETerrainId::SAND)
			return 1;
		if(terrain->isWater())
			return 3;
		if(terrain->isRock())
			return 4;
		return 2;
	};
	std::vector<TerrainId> order(used.begin(), used.end());
	std::stable_sort(order.begin(), order.end(), [&](const TerrainId & a, const TerrainId & b) { return rank(a) < rank(b); });
	return order;
}

void build(const std::string & requestPath, const std::string & outputPath, const std::string & reportPath)
{
	const Request request = parseRequest(requestPath);

	EditorCallback callback(nullptr);
	auto map = std::make_unique<CMap>(&callback);
	callback.setMap(map.get());
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = request.width;
	map->height = request.height;
	map->mapLayers = {MapLayerId::SURFACE};
	if(request.levels.size() > 1)
		map->mapLayers.push_back(MapLayerId::UNDERGROUND);
	map->initTerrain();
	if(!request.name.empty())
		map->name = MetaString::createFromTextID(mapRegisterLocalizedString("map", *map, TextIdentifier("header", "name"), request.name, "english"));
	if(!request.description.empty())
		map->description = MetaString::createFromTextID(mapRegisterLocalizedString("map", *map, TextIdentifier("header", "description"), request.description, "english"));

	CRandomGenerator rand(request.seed);
	auto * editManager = map->getEditManager();
	editManager->clearTerrain(&rand);
	if(request.toroidal)
		map->setToroidalAndRebuild(true);

	for(const auto & terrain : drawingOrder(request))
	{
		std::vector<int3> tiles;
		for(size_t z = 0; z < request.levels.size(); ++z)
			for(int y = 0; y < request.height; ++y)
				for(int x = 0; x < request.width; ++x)
					if(request.legend.at(request.levels[z].terrain[y][x]) == terrain)
						tiles.emplace_back(x, y, static_cast<int>(z));
		if(tiles.empty())
			continue;
		editManager->getTerrainSelection().setSelection(tiles);
		editManager->drawTerrain(terrain, 10, &rand);
	}
	editManager->getTerrainSelection().clearSelection();

	std::map<std::pair<int, TerrainId>, std::unique_ptr<EditorObstaclePlacer>> painters;
	for(size_t z = 0; z < request.levels.size(); ++z)
	{
		const auto & mask = request.levels[z].obstacles;
		if(mask.empty())
			continue;
		for(int y = 0; y < request.height; ++y)
		{
			for(int x = 0; x < request.width; ++x)
			{
				const int3 pos(x, y, static_cast<int>(z));
				const auto & tile = map->getTile(pos);
				if(mask[y][x] != 'x' || tile.isWater() || !tile.getTerrain()->isPassable() || tile.blocked())
					continue;
				auto & painter = painters[{static_cast<int>(z), tile.getTerrainID()}];
				if(!painter)
				{
					painter = std::make_unique<EditorObstaclePlacer>(map.get());
					painter->collectPossibleObstacles(tile.getTerrainID());
				}
				painter->addBlockedTile(pos);
			}
		}
	}
	size_t obstacles = 0;
	for(auto & painter : painters)
		obstacles += painter.second->placeObstacles(rand).size();

	// Tiles where the transition rules of the engine replaced the requested terrain
	std::map<TerrainId, char> letters;
	for(const auto & [letter, terrain] : request.legend)
		letters.emplace(terrain, letter);
	JsonNode changed;
	changed.setType(JsonNode::JsonType::DATA_VECTOR);
	for(size_t z = 0; z < request.levels.size(); ++z)
	{
		for(int y = 0; y < request.height; ++y)
		{
			for(int x = 0; x < request.width; ++x)
			{
				const auto actual = map->getTile(int3(x, y, static_cast<int>(z))).getTerrainID();
				if(actual == request.legend.at(request.levels[z].terrain[y][x]))
					continue;
				JsonNode entry;
				entry.Vector().emplace_back(x);
				entry.Vector().emplace_back(y);
				entry.Vector().emplace_back(static_cast<int>(z));
				entry.Vector().emplace_back(letters.contains(actual) ? std::string(1, letters.at(actual)) : actual.toEntity(LIBRARY)->getJsonKey());
				changed.Vector().push_back(entry);
			}
		}
	}

	CMemoryBuffer buffer;
	{
		CMapSaverJson saver(&buffer);
		saver.saveMap(map);
	}
	EditorCallback reloadedCallback(nullptr);
	const auto & bytes = buffer.getBuffer();
	if(!CMapService().loadMap(reinterpret_cast<const uint8_t *>(bytes.data()), static_cast<int>(buffer.getSize()), "built.vmap", "", "", &reloadedCallback))
		throw std::runtime_error("The built map cannot be loaded back");

	std::ofstream out(outputPath, std::ios::binary);
	out.write(reinterpret_cast<const char *>(buffer.getBuffer().data()), buffer.getSize());
	out.close();
	if(!out.good())
		throw std::runtime_error("Cannot write " + outputPath);

	JsonNode report;
	report["width"].Integer() = request.width;
	report["height"].Integer() = request.height;
	report["levels"].Integer() = static_cast<int64_t>(request.levels.size());
	report["obstacles"].Integer() = static_cast<int64_t>(obstacles);
	report["changed"] = changed;
	std::ofstream reportOut(reportPath, std::ios::binary);
	reportOut << report.toCompactString();
	reportOut.close();
	if(!reportOut.good())
		throw std::runtime_error("Cannot write " + reportPath);
}
}

int main(int argc, const char * argv[])
{
	if(argc != 4)
	{
		std::cerr << "Usage: vcmimapbuilder REQUEST.json OUTPUT.vmap REPORT.json" << std::endl;
		return 1;
	}

	std::array<std::string, 3> paths;
	for(size_t i = 0; i < paths.size(); ++i)
		paths[i] = boost::filesystem::absolute(argv[i + 1]).string();
	// Same as vcmiserver: a development build finds its config next to the executable
	boost::filesystem::current_path(boost::filesystem::system_complete(argv[0]).parent_path());

	CConsoleHandler console;
	CBasicLogConfigurator logConfigurator(VCMIDirs::get().userLogsPath() / "VCMI_MapBuilder_log.txt", &console);
	logConfigurator.configureDefault();
	initializeBaseFilesystem(false);
	auto library = std::make_unique<GameLibrary>();
	library->initializeFilesystem();
	logConfigurator.configure();
	library->initializeLibrary();
	setGameLibrary(std::move(library));

	int result = 0;
	try
	{
		build(paths[0], paths[1], paths[2]);
	}
	catch(const std::exception & e)
	{
		// Invalid requests are reported to the caller instead of crashing, the web wrapper relays the message
		std::cerr << "Error: " << e.what() << std::endl;
		result = 2;
	}

	logConfigurator.deconfigure();
	setGameLibrary(nullptr);
	return result;
}
