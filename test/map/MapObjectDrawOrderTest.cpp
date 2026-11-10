/*
 * MapObjectDrawOrderTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/MapObjectDrawOrder.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/modding/IdentifierResolver.h"
#include "../../lib/GameLibrary.h"

namespace
{
const int3 tile(5, 5, 0);

/// mask lines are written top to bottom, like in object config
std::shared_ptr<ObjectTemplate> makeTemplate(const std::string & mask, int zIndex)
{
	const std::string text = R"({ "animation" : "test.def", "visitableFrom" : [], "mask" : )" + mask + R"(, "zIndex" : )" + std::to_string(zIndex) + " }";

	JsonNode node(text.data(), text.size(), "MapObjectDrawOrderTest");
	auto result = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	result->readJson(node, resolver, false);
	return result;
}

class MapObjectDrawOrderTest : public ::testing::Test
{
protected:
	CMap map{nullptr};
	std::vector<std::unique_ptr<CGObjectInstance>> objects;

	/// The name is the letter of the object in the result of draw(): capital for the body, small for the shadow
	CGObjectInstance * addObject(char name, const std::string & mask, const int3 & anchor, int zIndex = 0, MapObjectID type = Obj::NO_OBJ)
	{
		auto object = std::make_unique<CGObjectInstance>(nullptr);
		object->id = ObjectInstanceID(name);
		object->ID = type;
		object->appearance = makeTemplate(mask, zIndex);
		object->pos = anchor;
		objects.push_back(std::move(object));
		return objects.back().get();
	}

	/// Objects are put on the tile one after another, like H3 does when it loads the map
	std::string draw(const std::vector<const CGObjectInstance *> & placed, int slotOffsetY = 0, const int3 & drawTile = tile)
	{
		std::vector<const CGObjectInstance *> onTile;

		for(const auto * object : placed)
			onTile.insert(MapObjectDrawOrder::findInsertPosition(onTile, map, object, drawTile, [](const CGObjectInstance * entry) { return entry; }), object);

		std::string result;

		for(const auto & step : MapObjectDrawOrder::getDrawSteps(onTile, map, drawTile, [](const CGObjectInstance * entry) { return entry; }, [&](const CGObjectInstance *) { return Point(0, slotOffsetY); }))
		{
			const char name = static_cast<char>(step.object->id.getNum());
			result += step.isShadow ? static_cast<char>(std::tolower(name)) : name;
		}
		return result;
	}
};
}

// Tree-like object: passable top row, blocked bottom row. The crown of the tree is on the tile above the trunk
TEST_F(MapObjectDrawOrderTest, TreeCrownCoversObjectBehindIt)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * rock = addObject('R', R"([ "B" ])", int3(5, 5, 0));

	// whichever of them is loaded first
	EXPECT_EQ(draw({tree, rock}), "rtRT");
	EXPECT_EQ(draw({rock, tree}), "rtRT");
}

TEST_F(MapObjectDrawOrderTest, EqualObjectsKeepLoadOrderWhenAddedAgain)
{
	const auto * first = addObject('A', R"([ "B" ])", int3(5, 5, 0));
	const auto * second = addObject('B', R"([ "B" ])", int3(5, 5, 0));

	EXPECT_EQ(draw({first, second}), "abAB");
	EXPECT_EQ(draw({second, first}), "abAB");
}

TEST_F(MapObjectDrawOrderTest, BackgroundObjectIsBelowEverythingAndItsShadowToo)
{
	const auto * ground = addObject('G', R"([ "B" ])", int3(5, 5, 0), 100);
	const auto * rock = addObject('R', R"([ "B" ])", int3(5, 5, 0));

	// shadows go after the bottom layer, so that they never darken the objects lying on the ground
	EXPECT_EQ(draw({ground, rock}), "GgrR");
	EXPECT_EQ(draw({rock, ground}), "GgrR");
}

TEST_F(MapObjectDrawOrderTest, ForegroundObjectIsAboveEverything)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * overlay = addObject('O', R"([ "B" ])", int3(5, 5, 0), -100);

	EXPECT_EQ(draw({overlay, tree}), "toTO");
	EXPECT_EQ(draw({tree, overlay}), "toTO");
}

TEST_F(MapObjectDrawOrderTest, HeroWalksBehindTreeCrownButInFrontOfObjectsBehindHim)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * rock = addObject('R', R"([ "B" ])", int3(5, 5, 0));
	const auto * hero = addObject('H', R"([ "B" ])", int3(5, 5, 0), 0, Obj::HERO);

	// hero has no shadow of its own here and stands between the layers of the tile
	EXPECT_EQ(draw({rock, tree, hero}), "rtRHT");
}

TEST_F(MapObjectDrawOrderTest, TopOfHeroIsBehindTreeCrown)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * hero = addObject('H', R"([ "B" ])", int3(5, 5, 0), 0, Obj::HERO);

	// the tile is above the row on which the hero stands, so only his head reaches it
	EXPECT_EQ(draw({tree, hero}, MapObjectDrawOrder::fixedSlotBodyRowLimit), "tTH");
}

TEST_F(MapObjectDrawOrderTest, DrawOrderUsesObjectLayersAcrossToroidalSeam)
{
	map.width = 10;
	map.height = 10;
	map.toroidal = true;
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(0, 0, 0));
	const auto * rock = addObject('R', R"([ "B" ])", int3(0, 9, 0));
	EXPECT_EQ(MapObjectDrawOrder::tileForObject(map, tree, int3(0, 9, 0)), int3(0, -1, 0));
	EXPECT_EQ(MapObjectDrawOrder::tileForObject(map, rock, int3(0, 9, 0)), int3(0, 9, 0));
	EXPECT_EQ(draw({tree, rock}, 0, int3(0, 9, 0)), "rtRT");
	EXPECT_EQ(draw({rock, tree}, 0, int3(0, 9, 0)), "rtRT");
}

TEST_F(MapObjectDrawOrderTest, RejectsStaticOccupiedFootprintsAcrossToroidalSeams)
{
	map.width = 10;
	map.height = 10;
	map.toroidal = true;

	const auto * inside = addObject('I', R"([ "BB", "BB" ])", int3(1, 1, 0));
	const auto * west = addObject('W', R"([ "BB" ])", int3(0, 5, 0));
	const auto * north = addObject('N', R"([ "V", "B" ])", int3(5, 0, 0));
	const auto * corner = addObject('C', R"([ "VB", "BB" ])", int3(0, 0, 0));
	const auto * blank = addObject('E', R"([ "0B" ])", int3(0, 5, 0));
	const auto * hero = addObject('H', R"([ "BB" ])", int3(0, 5, 0), 0, Obj::HERO);

	EXPECT_FALSE(map.hasUnsupportedToroidalFootprint(*inside));
	EXPECT_TRUE(map.hasUnsupportedToroidalFootprint(*west));
	EXPECT_TRUE(map.hasUnsupportedToroidalFootprint(*north));
	EXPECT_TRUE(map.hasUnsupportedToroidalFootprint(*corner));
	EXPECT_FALSE(map.hasUnsupportedToroidalFootprint(*blank));
	EXPECT_FALSE(map.hasUnsupportedToroidalFootprint(*hero));
	map.toroidal = false;
	EXPECT_FALSE(map.hasUnsupportedToroidalFootprint(*corner));
}

TEST_F(MapObjectDrawOrderTest, ToroidalObjectFootprintOccupiesAndClearsWrappedTiles)
{
	map.width = 10;
	map.height = 10;
	map.toroidal = true;
	map.initTerrain();
	auto * object = addObject('O', R"([ "AB", "BB" ])", int3(0, 0, 0));
	map.showObject(object);
	for(const auto & tile : {int3(0, 0, 0), int3(9, 0, 0), int3(0, 9, 0), int3(9, 9, 0)})
		EXPECT_TRUE(vstd::contains(map.getTile(tile).blockingObjects, object->id)) << tile.toString();
	EXPECT_TRUE(vstd::contains(map.getTile(int3(9, 9, 0)).visitableObjects, object->id));
	map.hideObject(object);
	for(const auto & tile : {int3(0, 0, 0), int3(9, 0, 0), int3(0, 9, 0), int3(9, 9, 0)})
	{
		EXPECT_FALSE(vstd::contains(map.getTile(tile).blockingObjects, object->id));
		EXPECT_FALSE(vstd::contains(map.getTile(tile).visitableObjects, object->id));
	}

	map.toroidal = false;
	map.showObject(object);
	EXPECT_TRUE(vstd::contains(map.getTile(int3(0, 0, 0)).blockingObjects, object->id));
	EXPECT_FALSE(vstd::contains(map.getTile(int3(9, 9, 0)).blockingObjects, object->id));
	EXPECT_FALSE(vstd::contains(map.getTile(int3(9, 9, 0)).visitableObjects, object->id));
}

TEST_F(MapObjectDrawOrderTest, OversizedToroidalFootprintOccupiesEachTileOnce)
{
	map.width = 3;
	map.height = 3;
	map.toroidal = true;
	map.initTerrain();
	auto * object = addObject('O', R"([ "BBBB" ])", int3(0, 1, 0));
	map.showObject(object);
	for(int x = 0; x < map.width; ++x)
		EXPECT_EQ(std::ranges::count(map.getTile(int3(x, 1, 0)).blockingObjects, object->id), 1);
	map.hideObject(object);
	for(int x = 0; x < map.width; ++x)
		EXPECT_FALSE(vstd::contains(map.getTile(int3(x, 1, 0)).blockingObjects, object->id));
}

TEST_F(MapObjectDrawOrderTest, ChangingTopologyRebuildsShownButNotHiddenObjects)
{
	map.width = 10;
	map.height = 10;
	map.initTerrain();
	for(int z = 0; z < map.levels(); ++z)
		for(int x = 0; x < map.width; ++x)
			for(int y = 0; y < map.height; ++y)
				map.getTile(int3(x, y, z)).terrainType = ETerrainId::GRASS;

	auto shown = std::make_shared<CGObjectInstance>(nullptr);
	shown->id = ObjectInstanceID(0);
	shown->pos = int3(0, 0, 0);
	shown->appearance = makeTemplate(R"([ "BB", "BB" ])", 0);
	auto hidden = std::make_shared<CGObjectInstance>(nullptr);
	hidden->id = ObjectInstanceID(1);
	hidden->pos = int3(5, 5, 0);
	hidden->appearance = makeTemplate(R"([ "B" ])", 0);
	auto overlapping = std::make_shared<CGObjectInstance>(nullptr);
	overlapping->id = ObjectInstanceID(2);
	overlapping->pos = int3(0, 0, 0);
	overlapping->appearance = makeTemplate(R"([ "B" ])", 0);
	map.objects.push_back(shown);
	map.objects.push_back(hidden);
	map.objects.push_back(overlapping);
	map.showObject(overlapping.get());
	map.showObject(shown.get());

	map.setToroidalAndRebuild(true);
	EXPECT_TRUE(vstd::contains(map.getTile(int3(9, 9, 0)).blockingObjects, shown->id));
	EXPECT_FALSE(vstd::contains(map.getTile(int3(5, 5, 0)).blockingObjects, hidden->id));
	EXPECT_EQ(map.getTile(int3(0, 0, 0)).blockingObjects, (std::vector<ObjectInstanceID>{overlapping->id, shown->id}));
	map.setToroidalAndRebuild(false);
	EXPECT_TRUE(vstd::contains(map.getTile(int3(0, 0, 0)).blockingObjects, shown->id));
	EXPECT_FALSE(vstd::contains(map.getTile(int3(9, 9, 0)).blockingObjects, shown->id));
	EXPECT_FALSE(vstd::contains(map.getTile(int3(5, 5, 0)).blockingObjects, hidden->id));
	EXPECT_EQ(map.getTile(int3(0, 0, 0)).blockingObjects, (std::vector<ObjectInstanceID>{overlapping->id, shown->id}));
}

// Tracked compatibility gap: occupancy alone cannot distinguish a fully
// clipped shown object from a deliberately hidden object.
TEST_F(MapObjectDrawOrderTest, DISABLED_ChangingTopologyRestoresClippedShownObjectsButNotHiddenObjects)
{
	map.width = 10;
	map.height = 10;
	map.initTerrain();
	for(int z = 0; z < map.levels(); ++z)
		for(int x = 0; x < map.width; ++x)
			for(int y = 0; y < map.height; ++y)
				map.getTile(int3(x, y, z)).terrainType = ETerrainId::GRASS;

	auto shown = std::make_shared<CGObjectInstance>(nullptr);
	shown->id = ObjectInstanceID(0);
	shown->pos = int3(0, 5, 0);
	shown->appearance = makeTemplate(R"([ "A0" ])", 0);
	auto hidden = std::make_shared<CGObjectInstance>(nullptr);
	hidden->id = ObjectInstanceID(1);
	hidden->pos = int3(0, 6, 0);
	hidden->appearance = makeTemplate(R"([ "A0" ])", 0);
	map.objects = {shown, hidden};
	map.showObject(shown.get());

	map.setToroidalAndRebuild(true);
	EXPECT_TRUE(vstd::contains(map.getTile(int3(9, 5, 0)).visitableObjects, shown->id));
	EXPECT_TRUE(vstd::contains(map.getTile(int3(9, 5, 0)).blockingObjects, shown->id));
	EXPECT_FALSE(vstd::contains(map.getTile(int3(9, 6, 0)).visitableObjects, hidden->id));
}

TEST_F(MapObjectDrawOrderTest, AuthoringTopologyRebuildIncludesClippedObjects)
{
	map.width = 10;
	map.height = 10;
	map.initTerrain();
	for(int z = 0; z < map.levels(); ++z)
		for(int x = 0; x < map.width; ++x)
			for(int y = 0; y < map.height; ++y)
				map.getTile(int3(x, y, z)).terrainType = ETerrainId::GRASS;

	auto object = std::make_shared<CGObjectInstance>(nullptr);
	object->id = ObjectInstanceID(0);
	object->pos = int3(0, 5, 0);
	object->appearance = makeTemplate(R"([ "A0" ])", 0);
	map.objects = {object, nullptr};
	map.showObject(object.get());

	for(int iteration = 0; iteration < 2; ++iteration)
	{
		map.setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);
		EXPECT_TRUE(vstd::contains(map.getTile(int3(9, 5, 0)).visitableObjects, object->id));
		EXPECT_TRUE(vstd::contains(map.getTile(int3(9, 5, 0)).blockingObjects, object->id));
		map.setToroidalAndRebuild(false, CMap::TopologyRebuildObjects::AllMapObjects);
		EXPECT_FALSE(vstd::contains(map.getTile(int3(9, 5, 0)).visitableObjects, object->id));
		EXPECT_FALSE(vstd::contains(map.getTile(int3(9, 5, 0)).blockingObjects, object->id));
	}
}

TEST_F(MapObjectDrawOrderTest, WrappedInteractiveOverlapCanHaveDifferentVisualAndVisitOrder)
{
	map.width = 10;
	map.height = 10;
	map.initTerrain();
	for(int z = 0; z < map.levels(); ++z)
		for(int x = 0; x < map.width; ++x)
			for(int y = 0; y < map.height; ++y)
				map.getTile(int3(x, y, z)).terrainType = ETerrainId::GRASS;

	auto existing = std::make_shared<CGObjectInstance>(nullptr);
	existing->id = ObjectInstanceID(0);
	existing->pos = int3(9, 5, 0);
	existing->appearance = makeTemplate(R"([ "A" ])", -100);
	auto wrapped = std::make_shared<CGObjectInstance>(nullptr);
	wrapped->id = ObjectInstanceID(1);
	wrapped->pos = int3(0, 5, 0);
	wrapped->appearance = makeTemplate(R"([ "AA" ])", 100);
	map.objects = {existing, wrapped};
	map.showObject(existing.get());
	map.showObject(wrapped.get());
	const int3 overlap(9, 5, 0);
	ASSERT_EQ(map.getTile(overlap).topVisitableObj(), existing->id);

	map.setToroidalAndRebuild(true);
	const auto & tileAtOverlap = map.getTile(overlap);
	ASSERT_EQ(tileAtOverlap.visitableObjects, (std::vector<ObjectInstanceID>{existing->id, wrapped->id}));
	EXPECT_EQ(tileAtOverlap.objectVisitedBy(ObjectInstanceID()), wrapped->id);
	EXPECT_EQ(MapObjectDrawOrder::findTopObject(map, {existing.get(), wrapped.get()}, overlap), existing.get());
}
