/*
 * WorldMapProbe.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CCreatureHandler.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGResource.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/mapping/MapTopology.h"
#include "../../lib/mapping/ObstacleProxy.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/texts/MetaString.h"

namespace
{
std::vector<std::string> readRows(const char * path)
{
	std::ifstream input(path);
	if(!input.good())
		throw std::runtime_error(std::string("Cannot read ") + path);
	std::vector<std::string> rows;
	for(std::string line; std::getline(input, line);)
		if(!line.empty() && line[0] != '#')
			rows.push_back(line);
	return rows;
}

/// Places objects of a hand-picked world layout on a toroidal map, one by one, in the nearest free spot
class WorldPlacer
{
public:
	WorldPlacer(CMap & map, const std::vector<std::string> & obstacleMask, CRandomGenerator & rand)
		: map(map), obstacleMask(obstacleMask), rand(rand)
	{
	}

	std::optional<int3> wrap(const int3 & pos) const
	{
		return map.getTopology().canonicalize(pos);
	}

	bool isFree(const int3 & pos, bool water) const
	{
		const auto tile = wrap(pos);
		if(!tile || reserved.contains(*tile))
			return false;
		const auto & t = map.getTile(*tile);
		return t.isWater() == water && !t.blocked() && !t.visitable();
	}

	bool isObstacleArea(const int3 & pos) const
	{
		const auto tile = wrap(pos);
		return tile && obstacleMask.at(tile->y).at(tile->x) == 'x';
	}

	std::shared_ptr<CGObjectInstance> create(MapObjectID id, MapObjectSubID subID, const int3 & near, const std::string & preferred = {})
	{
		const auto & handler = LIBRARY->objtypeh->getHandlerFor(id, subID);
		const auto terrain = map.getTile(*wrap(near)).getTerrainID();
		auto templates = handler.getTemplates(terrain);
		if(templates.empty())
			templates = handler.getTemplates();
		if(templates.empty())
			throw std::runtime_error("No templates for object " + std::to_string(id.getNum()) + ":" + std::to_string(subID.getNum()));
		std::shared_ptr<const ObjectTemplate> chosen = templates.front();
		for(const auto & tmpl : templates)
			if(tmpl->stringID == preferred)
				chosen = tmpl;
		return handler.create(map.cb, chosen);
	}

	bool fits(const CGObjectInstance & object, const int3 & anchor, bool water, bool avoidObstacles) const
	{
		const auto & appearance = *object.appearance;
		for(int dy = 0; dy < static_cast<int>(appearance.getHeight()); ++dy)
		{
			for(int dx = 0; dx < static_cast<int>(appearance.getWidth()); ++dx)
			{
				const int3 tile = anchor - int3(dx, dy, 0);
				if(!appearance.isBlockedAt(dx, dy) && !appearance.isVisitableAt(dx, dy))
				{
					// a covered tile must stay on the map, but may overlap anything
					if(!wrap(tile))
						return false;
					continue;
				}
				if(!isFree(tile, water) || (avoidObstacles && isObstacleArea(tile)))
					return false;
			}
		}
		if(!appearance.isVisitable())
			return true;

		const int3 visitable = anchor - appearance.getVisitableOffset();
		if(water)
		{
			for(const auto & dir : int3::getDirs())
				if(isFree(visitable + dir, true))
					return true;
			return false;
		}
		// land objects are entered from below
		return isFree(visitable + int3(0, 1, 0), false);
	}

	/// Inserts the object at the free spot nearest to `near`, at least minDistance and at most maxDistance away
	bool place(const std::shared_ptr<CGObjectInstance> & object, const int3 & near, int minDistance, int maxDistance, bool water = false, bool avoidObstacles = false)
	{
		std::vector<int3> candidates;
		for(int dy = -maxDistance; dy <= maxDistance; ++dy)
			for(int dx = -maxDistance; dx <= maxDistance; ++dx)
				if(dx * dx + dy * dy >= minDistance * minDistance && dx * dx + dy * dy <= maxDistance * maxDistance)
					candidates.push_back(near + int3(dx, dy, 0));
		RandomGeneratorUtil::randomShuffle(candidates, rand);
		std::stable_sort(candidates.begin(), candidates.end(), [&](const int3 & a, const int3 & b)
		{
			return near.dist2dSQ(a) < near.dist2dSQ(b);
		});

		for(const auto & candidate : candidates)
		{
			const auto anchor = wrap(candidate);
			if(!anchor || !fits(*object, *anchor, water, avoidObstacles))
				continue;
			object->setAnchorPos(*anchor);
			map.getEditManager()->insertObject(object);
			reserveAround(*object);
			return true;
		}
		return false;
	}

	/// Keeps obstacles off the object and the tiles from which it is entered
	void reserveAround(const CGObjectInstance & object)
	{
		const int margin = object.ID == Obj::TOWN ? 3 : 1;
		for(const auto & tile : object.getBlockedPos())
			reserveSquare(tile, 0);
		if(object.isVisitable())
			reserveSquare(object.visitablePos() + int3(0, 1, 0), margin);
	}

	void reserveSquare(const int3 & center, int radius)
	{
		for(int dy = -radius; dy <= radius; ++dy)
			for(int dx = -radius; dx <= radius; ++dx)
				if(const auto tile = wrap(center + int3(dx, dy, 0)))
					reserved.insert(*tile);
	}

	const std::set<int3> & getReserved() const
	{
		return reserved;
	}

private:
	CMap & map;
	const std::vector<std::string> & obstacleMask;
	CRandomGenerator & rand;
	std::set<int3> reserved;
};

std::vector<CreatureID> creaturesOfLevel(int level)
{
	std::vector<CreatureID> result;
	for(const auto & creature : LIBRARY->creh->getObjects())
		if(creature && !creature->special && creature->getAIValue() > 0 && creature->getLevel() == level && creature->getModScope() == "core")
			result.push_back(creature->getId());
	return result;
}

std::shared_ptr<CGCreature> createMonster(CMap & map, WorldPlacer & placer, const int3 & near, int level, CRandomGenerator & rand)
{
	static const std::array<std::pair<int, int>, 8> amounts = {{{0, 0}, {20, 40}, {12, 25}, {8, 16}, {5, 10}, {3, 7}, {2, 4}, {1, 3}}};
	const auto candidates = creaturesOfLevel(level);
	const auto creature = *RandomGeneratorUtil::nextItem(candidates, rand);
	auto monster = std::dynamic_pointer_cast<CGCreature>(placer.create(Obj::MONSTER, creature.getNum(), near));
	monster->initialCharacter = CGCreature::Character::AGGRESSIVE;
	const auto [low, high] = amounts.at(level);
	monster->putStack(SlotID(0), std::make_unique<CStackInstance>(map.cb, creature, rand.nextInt(low, high)));
	return monster;
}
}

// Explicitly enabled local tool: paints a toroidal map from a text grid of terrain letters, then populates it
TEST(MapManager, PaintWorldMapProbe)
{
	const char * gridPath = std::getenv("VCMI_WORLD_GRID");
	const char * output = std::getenv("VCMI_WORLD_OUTPUT");
	if(!gridPath || !output)
		GTEST_SKIP() << "Set VCMI_WORLD_GRID and VCMI_WORLD_OUTPUT to paint a local map";

	const auto rows = readRows(gridPath);
	ASSERT_FALSE(rows.empty());

	EditorCallback callback(nullptr);
	auto map = std::make_unique<CMap>(&callback);
	callback.setMap(map.get());
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = static_cast<int>(rows.front().size());
	map->height = static_cast<int>(rows.size());
	map->mapLayers = {MapLayerId::SURFACE};
	map->initTerrain();
	if(const char * name = std::getenv("VCMI_WORLD_NAME"))
		map->name = MetaString::createFromTextID(mapRegisterLocalizedString("map", *map, TextIdentifier("header", "name"), name, "english"));
	if(const char * description = std::getenv("VCMI_WORLD_DESCRIPTION"))
		map->description = MetaString::createFromTextID(mapRegisterLocalizedString("map", *map, TextIdentifier("header", "description"), description, "english"));

	CRandomGenerator rand(12345);
	auto * editManager = map->getEditManager();
	editManager->clearTerrain(&rand);
	map->setToroidalAndRebuild(true);

	const std::map<char, TerrainId> letters = {
		{'w', ETerrainId::WATER}, {'g', ETerrainId::GRASS}, {'s', ETerrainId::SAND}, {'d', ETerrainId::DIRT},
		{'n', ETerrainId::SNOW}, {'r', ETerrainId::ROUGH}, {'m', ETerrainId::SWAMP}};
	// Base land first, so that later terrains draw their transitions over it; water last restores coastlines
	for(const char letter : std::string("drsgmnw"))
	{
		std::vector<int3> tiles;
		for(int y = 0; y < map->height; ++y)
		{
			ASSERT_EQ(rows[y].size(), static_cast<size_t>(map->width)) << "row " << y;
			for(int x = 0; x < map->width; ++x)
				if(rows[y][x] == letter)
					tiles.emplace_back(x, y, 0);
		}
		if(tiles.empty())
			continue;
		editManager->getTerrainSelection().setSelection(tiles);
		editManager->drawTerrain(letters.at(letter), 10, &rand);
	}

	const char * obstaclePath = std::getenv("VCMI_WORLD_OBSTACLES");
	const char * sitesPath = std::getenv("VCMI_WORLD_SITES");
	if(obstaclePath && sitesPath)
	{
		const auto obstacleMask = readRows(obstaclePath);
		ASSERT_EQ(obstacleMask.size(), rows.size());
		WorldPlacer placer(*map, obstacleMask, rand);

		std::vector<std::shared_ptr<CGTownInstance>> playerTowns;
		std::vector<std::shared_ptr<CGTownInstance>> towns;
		for(const auto & line : readRows(sitesPath))
		{
			std::istringstream fields(line);
			int x;
			int y;
			int faction;
			int owner;
			fields >> x >> y >> faction >> owner;
			const int3 site(x, y, 0);
			const bool playable = owner >= 0;
			auto town = std::dynamic_pointer_cast<CGTownInstance>(placer.create(Obj::TOWN, FactionID(faction), site, playable ? "fort" : "village"));
			ASSERT_NE(town, nullptr);
			town->tempOwner = playable ? PlayerColor(owner) : PlayerColor::NEUTRAL;
			town->addBuilding(BuildingID::DEFAULT);
			town->addBuilding(BuildingID::TAVERN);
			if(playable)
				town->addBuilding(BuildingID::FORT);
			for(const auto & spell : LIBRARY->spellh->getDefaultAllowed())
				town->possibleSpells.push_back(spell);
			if(!placer.place(town, site, 0, 14))
			{
				std::cout << "No room for town at " << site.toString() << std::endl;
				continue;
			}
			towns.push_back(town);
			if(!playable)
				continue;
			playerTowns.push_back(town);
			auto & player = map->players.at(owner);
			player.canHumanPlay = true;
			player.canComputerPlay = true;
			player.allowedFactions = {FactionID(faction)};
			player.isFactionRandom = false;
			player.hasMainTown = true;
			player.generateHeroAtMainTown = true;
			player.posOfMainTown = town->anchorPos();
			player.team = TeamID(owner);
		}

		const auto placeMine = [&](GameResID resource, const int3 & near, int minDistance, int maxDistance, int guardLevel)
		{
			auto mine = std::dynamic_pointer_cast<CGMine>(placer.create(Obj::MINE, resource.getNum(), near));
			mine->tempOwner = PlayerColor::NEUTRAL;
			mine->producedResource = resource;
			mine->producedQuantity = mine->defaultResProduction();
			if(!placer.place(mine, near, minDistance, maxDistance, false, true))
				return;
			if(guardLevel > 0)
				{
				const int3 guardSpot = mine->visitablePos() + int3(0, 2, 0);
				placer.place(createMonster(*map, placer, guardSpot, guardLevel, rand), guardSpot, 0, 2);
			}
		};

		// Each starting town gets the usual nearby wood and ore, then guarded rare mines further out
		const std::array<GameResID, 4> rare = {GameResID::MERCURY, GameResID::SULFUR, GameResID::CRYSTAL, GameResID::GEMS};
		for(const auto & town : playerTowns)
		{
			const int3 center = town->visitablePos();
			placeMine(GameResID::WOOD, center, 4, 9, 0);
			placeMine(GameResID::ORE, center, 4, 9, 0);
			placeMine(*RandomGeneratorUtil::nextItem(rare, rand), center, 10, 16, 2);
			placeMine(*RandomGeneratorUtil::nextItem(rare, rand), center, 10, 18, 3);
			placeMine(GameResID::GOLD, center, 14, 22, 4);
			for(const auto resource : std::array<GameResID, 4>{GameResID::WOOD, GameResID::ORE, GameResID::GOLD, *RandomGeneratorUtil::nextItem(rare, rand)})
				placer.place(placer.create(Obj::RESOURCE, resource.getNum(), center), center, 3, 8, false, true);

			// a shipyard on the nearest coast, with a free boat in front of it
			auto shipyard = std::dynamic_pointer_cast<CGShipyard>(placer.create(Obj::SHIPYARD, 0, center));
			shipyard->tempOwner = PlayerColor::NEUTRAL;
			std::vector<int3> offsets;
			static_cast<const IBoatGenerator &>(*shipyard).getOutOffsets(offsets);
			for(int distance = 3; distance <= 24; distance += 3)
			{
				bool placed = false;
				for(int dy = -distance; dy <= distance && !placed; ++dy)
				{
					for(int dx = -distance; dx <= distance && !placed; ++dx)
					{
						const auto anchor = placer.wrap(center + int3(dx, dy, 0));
						if(!anchor || !placer.fits(*shipyard, *anchor, false, false))
							continue;
						const int3 visitable = *anchor - shipyard->appearance->getVisitableOffset();
						for(const auto & offset : offsets)
						{
							if(!placer.isFree(visitable + offset, true))
								continue;
							placer.place(shipyard, *anchor, 0, 0);
							auto boat = placer.create(Obj::BOAT, *LIBRARY->objtypeh->knownSubObjects(Obj::BOAT).begin(), visitable + offset);
							boat->tempOwner = PlayerColor::NEUTRAL;
							placer.place(boat, visitable + offset, 0, 3, true);
							placed = true;
							break;
						}
					}
				}
				if(placed)
					break;
			}
		}
		for(const auto & town : towns)
			if(!town->getOwner().isValidPlayer())
				placeMine(GameResID::GOLD, town->visitablePos(), 5, 12, 5);

		std::vector<int3> land;
		for(int y = 0; y < map->height; ++y)
			for(int x = 0; x < map->width; ++x)
				if(rows[y][x] != 'w' && obstacleMask[y][x] != 'x')
					land.emplace_back(x, y, 0);
		std::vector<int3> ocean;
		for(int y = 0; y < map->height; ++y)
			for(int x = 0; x < map->width; ++x)
				if(rows[y][x] == 'w')
					ocean.emplace_back(x, y, 0);

		const auto distanceToStart = [&](const int3 & pos)
		{
			double best = std::numeric_limits<double>::max();
			for(const auto & town : playerTowns)
				best = std::min(best, std::sqrt(static_cast<double>(*map->getTopology().horizontalDistanceSquaredIgnoringLevel(pos, town->visitablePos()))));
			return best;
		};

		// Landmarks from the picture
		const std::vector<std::tuple<MapObjectID, int, int3>> landmarks = {
			{Obj::PYRAMID, 0, int3(132, 52, 0)},
			{Obj::WINDMILL, 0, int3(68, 72, 0)},
			{Obj::WINDMILL, 0, int3(140, 30, 0)},
			{Obj::WATER_WHEEL, 0, int3(128, 46, 0)},
			{Obj::REDWOOD_OBSERVATORY, 0, int3(48, 30, 0)},
			{Obj::REDWOOD_OBSERVATORY, 0, int3(200, 28, 0)},
			{Obj::OBELISK, 0, int3(126, 54, 0)},
			{Obj::LIBRARY_OF_ENLIGHTENMENT, 0, int3(178, 40, 0)},
		};
		for(const auto & [id, subID, pos] : landmarks)
			placer.place(placer.create(id, subID, pos), pos, 0, 8, false, true);

		const std::vector<int3> whirlpools = {int3(172, 76, 0), int3(20, 70, 0), int3(98, 60, 0), int3(240, 60, 0)};
		for(const auto & pos : whirlpools)
			placer.place(placer.create(Obj::WHIRLPOOL, 0, pos), pos, 0, 6, true);
		for(int i = 0; i < 6; ++i)
		{
			const auto pos = *RandomGeneratorUtil::nextItem(ocean, rand);
			placer.place(placer.create(Obj::FLOTSAM, 0, pos), pos, 0, 3, true);
			placer.place(placer.create(Obj::SEA_CHEST, 0, pos), pos, 2, 6, true);
		}

		int monsters = 0;
		int treasures = 0;
		RandomGeneratorUtil::randomShuffle(land, rand);
		for(size_t i = 0; i < land.size() && (monsters < 220 || treasures < 420); ++i)
		{
			const int3 pos = land[i];
			const double distance = distanceToStart(pos);
			if(distance < 6)
				continue;
			if(treasures < 420 && i % 3 != 0)
			{
				std::shared_ptr<CGObjectInstance> object;
				const int roll = rand.nextInt(0, 9);
				if(roll < 6)
					object = placer.create(Obj::RESOURCE, rand.nextInt(0, 6), pos);
				else if(roll < 8)
					object = placer.create(Obj::TREASURE_CHEST, 0, pos);
				else
					object = placer.create(Obj::CAMPFIRE, 0, pos);
				if(placer.place(object, pos, 0, 0, false, true))
					++treasures;
			}
			else if(monsters < 220)
			{
				const int level = std::clamp(static_cast<int>(distance / 9) + rand.nextInt(0, 1), 1, 7);
				if(placer.place(createMonster(*map, placer, pos, level, rand), pos, 0, 0, false, true))
					++monsters;
			}
		}
		std::cout << "Placed " << towns.size() << " towns, " << monsters << " monsters, " << treasures << " treasures" << std::endl;

		// Obstacles fill whole areas, so keep a path free from every object to the nearest town on its landmass
		{
			const auto index = [&](const int3 & pos) { return pos.y * map->width + pos.x; };
			std::vector<int> cost;
			std::vector<int3> previous;
			using Entry = std::pair<int, int3>;
			auto later = [](const Entry & a, const Entry & b) { return a.first > b.first; };
			std::priority_queue<Entry, std::vector<Entry>, decltype(later)> queue(later);
			const auto approach = [&](const CGObjectInstance & object) { return placer.wrap(object.visitablePos() + int3(0, 1, 0)); };
			const auto reset = [&]()
			{
				cost.assign(map->width * map->height, std::numeric_limits<int>::max());
				previous.assign(map->width * map->height, int3(-1, -1, -1));
			};
			const auto addSource = [&](const int3 & start)
			{
				cost[index(start)] = 0;
				queue.emplace(0, start);
			};
			const auto trace = [&](std::optional<int3> step)
			{
				if(!step || cost[index(*step)] == std::numeric_limits<int>::max())
					return false;
				while(step->isValid())
				{
					placer.reserveSquare(*step, 0);
					step = previous[index(*step)];
				}
				return true;
			};
			const auto search = [&]()
			{
			while(!queue.empty())
			{
				const auto [currentCost, current] = queue.top();
				queue.pop();
				if(currentCost > cost[index(current)])
					continue;
				for(const auto & dir : int3::getDirs())
				{
					const auto next = placer.wrap(current + dir);
					if(!next)
						continue;
					const auto & tile = map->getTile(*next);
					if(tile.isWater() || tile.blocked() || tile.visitable())
						continue;
					const int nextCost = currentCost + (placer.isObstacleArea(*next) ? 4 : 1);
					if(nextCost >= cost[index(*next)])
						continue;
					cost[index(*next)] = nextCost;
					previous[index(*next)] = current;
					queue.emplace(nextCost, *next);
				}
			}
			};

			// towns of one landmass are joined to each other
			for(const auto & town : towns)
			{
				reset();
				addSource(*approach(*town));
				search();
				for(const auto & other : towns)
					if(other != town)
						trace(approach(*other));
			}

			// every other object is joined to a town or to a shore where a boat can land
			reset();
			for(const auto & town : towns)
				addSource(*approach(*town));
			for(int y = 0; y < map->height; ++y)
			{
				for(int x = 0; x < map->width; ++x)
				{
					const int3 pos(x, y, 0);
					const auto & tile = map->getTile(pos);
					if(tile.isWater() || tile.blocked() || tile.visitable())
						continue;
					for(const auto & dir : int3::getDirs())
					{
						const auto next = placer.wrap(pos + dir);
						if(next && map->getTile(*next).isWater())
						{
							addSource(pos);
							break;
						}
					}
				}
			}
			search();
			int unreachable = 0;
			for(const auto & object : map->objects)
			{
				if(!object || !object->isVisitable() || object->ID == Obj::BOAT || object->ID == Obj::WHIRLPOOL || object->ID == Obj::FLOTSAM || object->ID == Obj::SEA_CHEST)
					continue;
				if(!trace(approach(*object)))
					++unreachable;
			}
			std::cout << "Objects without a land path to a town or shore: " << unreachable << std::endl;
		}

		std::map<TerrainId, std::unique_ptr<EditorObstaclePlacer>> painters;
		for(int y = 0; y < map->height; ++y)
		{
			for(int x = 0; x < map->width; ++x)
			{
				const int3 pos(x, y, 0);
				const auto & tile = map->getTile(pos);
				if(obstacleMask[y][x] != 'x' || placer.getReserved().contains(pos) || tile.blocked() || tile.visitable() || tile.isWater())
					continue;
				auto & painter = painters[tile.getTerrainID()];
				if(!painter)
				{
					painter = std::make_unique<EditorObstaclePlacer>(map.get());
					painter->collectPossibleObstacles(tile.getTerrainID());
				}
				painter->addBlockedTile(pos);
			}
		}
		size_t obstacles = 0;
		for(auto & painter : painters)
			obstacles += painter.second->placeObstacles(rand).size();
		std::cout << "Placed " << obstacles << " obstacles" << std::endl;

		// Reachable land from each starting town, ignoring monsters
		for(const auto & town : playerTowns)
		{
			std::set<int3> seen;
			std::deque<int3> queue;
			const int3 start = *placer.wrap(town->visitablePos() + int3(0, 1, 0));
			queue.push_back(start);
			seen.insert(start);
			int reachedTowns = 0;
			while(!queue.empty())
			{
				const int3 current = queue.front();
				queue.pop_front();
				for(const auto & dir : int3::getDirs())
				{
					const auto next = placer.wrap(current + dir);
					if(!next || seen.contains(*next))
						continue;
					const auto & tile = map->getTile(*next);
					if(tile.isWater())
						continue;
					for(const auto & object : tile.visitableObjects)
						if(map->getObject(object)->ID == Obj::TOWN && map->getObject(object)->getOwner().isValidPlayer())
							++reachedTowns;
					if(tile.blocked())
						continue;
					seen.insert(*next);
					queue.push_back(*next);
				}
			}
			std::cout << "Town " << town->getOwner().toString() << " at " << town->anchorPos().toString() << " reaches " << seen.size()
			          << " land tiles and " << reachedTowns << " starting town entrances" << std::endl;
		}
	}

	CMemoryBuffer buffer;
	{
		CMapSaverJson saver(&buffer);
		saver.saveMap(map);
	}
	buffer.seek(0);
	EditorCallback reloadedCallback(nullptr);
	CMapLoaderJson loader(&buffer);
	const auto reloaded = loader.loadMap(&reloadedCallback);
	ASSERT_NE(reloaded, nullptr);
	EXPECT_TRUE(reloaded->toroidal);
	EXPECT_EQ(reloaded->objects.size(), map->objects.size());

	std::ofstream out(output, std::ios::binary);
	ASSERT_TRUE(out.good());
	out.write(reinterpret_cast<const char *>(buffer.getBuffer().data()), buffer.getSize());
	out.close();
	ASSERT_TRUE(out.good());
}
