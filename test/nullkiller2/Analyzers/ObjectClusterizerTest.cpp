/*
 * ObjectClusterizerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Analyzers/ObjectClusterizer.h"
#include "AI/Nullkiller2/Pathfinding/Actors.h"
#include "AI/Nullkiller2/Pathfinding/ObjectGraph.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"
#include "AI/Nullkiller2/Engine/AIMemory.h"
#include "lib/CPlayerState.h"
#include "lib/mapping/MapTopology.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGResource.h"
#include "lib/networkPacks/PacksForClient.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

namespace
{
const MapTopology bounded(36, 36, 2, false);
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorReachScalesRangeByAvailableCasts)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 3, 2, 2}
	};

	EXPECT_TRUE(NK2AI::canReachWithDimensionDoor(reach, int3(16, 14, 0), bounded));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(17, 14, 0), bounded));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(16, 15, 0), bounded));
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorExpansionDoesNotReachOutsideCastEnvelope)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 4, 4, 1}
	};

	EXPECT_TRUE(NK2AI::canReachWithDimensionDoor(reach, int3(14, 14, 0), bounded));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(15, 14, 0), bounded));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(14, 15, 0), bounded));
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorExpansionUsesAnyReachableHero)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 1, 2, 2},
		{int3(20, 20, 0), 2, 3, 3}
	};

	EXPECT_TRUE(NK2AI::canReachWithDimensionDoor(reach, int3(26, 26, 0), bounded));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(27, 26, 0), bounded));
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorReachDoesNotCrossLevels)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 3, 2, 2}
	};

	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(10, 10, 1), bounded));
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorReachWrapsEdgesAndCorners)
{
	const MapTopology toroidal(36, 36, 2, true);
	const std::vector<NK2AI::DimensionDoorExpansionReach> reach = {{int3(0, 0, 0), 3, 2, 1}};
	EXPECT_TRUE(NK2AI::canReachWithDimensionDoor(reach, int3(35, 35, 0), toroidal));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(35, 35, 0), bounded));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(32, 35, 0), toroidal));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(35, 35, 1), toroidal));
}

class Nullkiller2_ObjectClusterCenter : public NullkillerTest {};

TEST_F(Nullkiller2_ObjectClusterCenter, heroActorUsesConfirmedToroidalMovementLayer)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PlayerColor(0)).hero({6, 5, 0}, HeroTypeID(0), PlayerColor(0));
	startWithMap(std::move(builder));
	auto * hero = findHeroByOwner(PlayerColor(0));
	ASSERT_NE(hero, nullptr);
	class ActorProbe final : public NK2AI::ChainActor
	{
	public:
		explicit ActorProbe(const CGHeroInstance * hero) : ChainActor(hero, NK2AI::HeroRole::MAIN, 1) {}
	};
	map()->toroidal = true;
	for(const auto layer : {EPathfindingLayer::AIR, EPathfindingLayer::WATER})
	{
		hero->setConfirmedMovementLayer(layer);
		EXPECT_EQ(ActorProbe(hero).layer, layer);
	}
	map()->toroidal = false;
	EXPECT_EQ(ActorProbe(hero).layer, EPathfindingLayer::LAND);
	map()->toroidal = true;
	hero->setConfirmedMovementLayer(EPathfindingLayer::AUTO);
	EXPECT_EQ(ActorProbe(hero).layer, EPathfindingLayer::LAND);
}

TEST_F(Nullkiller2_ObjectClusterCenter, clusterCenterTreatsOppositeEdgesAsNearby)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero({6, 5, 0}, HeroTypeID(0), player)
		.resource({0, 10, 0}, GameResID::GOLD, 1000)
		.resource({35, 10, 0}, GameResID::GOLD, 1000)
		.resource({18, 10, 0}, GameResID::GOLD, 1000);
	startWithMap(std::move(builder));
	revealMap(player);
	auto callback = makeCallback(player);
	NK2AI::ObjectCluster cluster;
	const auto resources = findAll<CGResource>();
	for(const auto * resource : resources)
	{
		NK2AI::ClusterObjectInfo info{};
		info.priority = 1;
		cluster.objects.insert({resource->id, info});
	}
	ASSERT_EQ(cluster.objects.size(), 3);
	map()->toroidal = false;
	const auto * ordinary = cluster.calculateCenter(callback.get());
	ASSERT_NE(ordinary, nullptr);
	EXPECT_EQ(ordinary->visitablePos().x, 18);
	map()->toroidal = true;
	const auto * wrapped = cluster.calculateCenter(callback.get());
	ASSERT_NE(wrapped, nullptr);
	EXPECT_TRUE(wrapped->visitablePos().x == 0 || wrapped->visitablePos().x == 35);
	const auto middle = std::ranges::find_if(resources, [](const auto * object) { return object->visitablePos().x == 18; });
	ASSERT_NE(middle, resources.end());
	{
		NK2AI::ClusterObjects::accessor info;
		ASSERT_TRUE(cluster.objects.find(info, (*middle)->id));
		info->second.priority = 5;
	}
	EXPECT_EQ(cluster.calculateCenter(callback.get()), *middle) << "higher-priority targets keep their weight";

	NK2AI::ObjectCluster opposite;
	const CGResource * first = nullptr;
	for(const auto * resource : resources)
		if(resource->visitablePos().x != 35)
		{
			NK2AI::ClusterObjectInfo info{};
			info.priority = 1;
			opposite.objects.insert({resource->id, info});
			if(!first || resource->id.getNum() < first->id.getNum())
				first = resource;
		}
	ASSERT_EQ(opposite.objects.size(), 2);
	EXPECT_EQ(opposite.calculateCenter(callback.get()), first) << "an ambiguous circular mean has a deterministic fallback";

	// Removing the last target can leave a locked cluster temporarily empty.
	opposite.reset();
	for(const bool toroidal : {false, true})
	{
		map()->toroidal = toroidal;
		EXPECT_EQ(opposite.calculateCenter(callback.get()), nullptr);
	}
}

TEST_F(Nullkiller2_ObjectClusterCenter, ignoresTargetsNoLongerVisibleToPlayer)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.hero({6, 5, 0}, HeroTypeID(0), player)
		.resource({0, 10, 0}, GameResID::GOLD, 1000)
		.resource({18, 10, 0}, GameResID::GOLD, 1000);
	startWithMap(std::move(builder));
	revealMap(player);
	auto callback = makeCallback(player);
	const auto resources = findAll<CGResource>();
	ASSERT_EQ(resources.size(), 2u);
	const auto edge = std::ranges::find_if(resources, [](const auto * object) { return object->visitablePos().x == 0; });
	const auto interior = std::ranges::find_if(resources, [](const auto * object) { return object->visitablePos().x == 18; });
	ASSERT_NE(edge, resources.end());
	ASSERT_NE(interior, resources.end());

	NK2AI::ObjectCluster cluster;
	NK2AI::ClusterObjectInfo info{};
	info.priority = 1;
	cluster.objects.insert({(*edge)->id, info});
	cluster.objects.insert({(*interior)->id, info});
	auto * team = gameState()->getPlayerTeam(player);
	for(int x = 0; x < map()->width; ++x)
		for(int y = 0; y < map()->height; ++y)
			team->fogOfWarMap[int3(x, y, 0)] = 0;
	team->fogOfWarMap[(*interior)->visitablePos()] = 1;
	ASSERT_EQ(callback->tryGetObj((*edge)->id), nullptr);
	ASSERT_EQ(callback->tryGetObj((*interior)->id), *interior);
	for(const bool toroidal : {false, true})
	{
		map()->toroidal = toroidal;
		EXPECT_EQ(cluster.calculateCenter(callback.get()), *interior);
		EXPECT_EQ(cluster.getObjects(callback.get()), (std::vector<const CGObjectInstance *>{*interior}));
	}
	team->fogOfWarMap[(*interior)->visitablePos()] = 0;
	ASSERT_EQ(callback->tryGetObj((*interior)->id), nullptr);
	for(const bool toroidal : {false, true})
	{
		map()->toroidal = toroidal;
		EXPECT_EQ(cluster.calculateCenter(callback.get()), nullptr);
		EXPECT_TRUE(cluster.getObjects(callback.get()).empty());
	}
	team->fogOfWarMap[(*edge)->visitablePos()] = 1;
	for(const bool toroidal : {false, true})
	{
		map()->toroidal = toroidal;
		EXPECT_EQ(cluster.getObjects(callback.get()), (std::vector<const CGObjectInstance *>{*edge}));
	}
}

class Nullkiller2_ObjectGraphSeams : public NullkillerTest, public ::testing::WithParamInterface<std::tuple<std::pair<int3, int3>, int>> {};

TEST_P(Nullkiller2_ObjectGraphSeams, connectsWrappedObjectsAndPreservesOrdinaryHeroValidation)
{
	const PlayerColor player(0);
	auto [coordinates, level] = GetParam();
	auto [first, second] = coordinates;
	first.z = second.z = level;
	const int3 adjacent(
		first.x == 0 ? 1 : (first.x == 35 ? 34 : first.x),
		first.y == 0 ? 1 : (first.y == 35 ? 34 : first.y), level);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true)
		.playerActive(player)
		.hero({19, 18, 0}, HeroTypeID(0), player)
		.resource(first, GameResID::GOLD, 1000)
		.resource(adjacent, GameResID::GOLD, 1000)
		.resource(second, GameResID::GOLD, 1000);
	startWithMap(std::move(builder));
	revealMap(player);
	const auto resources = findAll<CGResource>();
	ASSERT_EQ(resources.size(), 3);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	const auto heroID = hero->id;
	// Keep the removed hero alive until the pathfinder holding it is destroyed.
	auto heroLifetime = map()->objects.at(heroID.getNum());
	auto gateway = makeGateway(player);
	for(const auto * resource : resources)
		gateway->nullkiller->memory->addVisitableObject(resource);
	for(const bool toroidal : {false, true})
	{
		map()->toroidal = toroidal;
		NK2AI::ObjectGraph graph;
		graph.updateGraph(gateway->nullkiller.get());
		ASSERT_TRUE(graph.hasNodeAt(first));
		ASSERT_TRUE(graph.hasNodeAt(second));
		ASSERT_TRUE(graph.hasNodeAt(adjacent));
		const auto & links = graph.getNode(first).connections;
		EXPECT_NE(links.find(adjacent), links.end()) << "ordinary adjacent control";
		EXPECT_EQ(links.find(second) != links.end(), toroidal);
		// The graph calculator destroys its temporary heroes at the end of updateGraph.
		EXPECT_EQ(gateway->nullkiller->pathfinder->getStorage(), nullptr);
	}

	// A subsequent real-hero search must not inherit synthetic-actor permission.
	map()->toroidal = true;
	NK2AI::HeroMap<NK2AI::HeroRole> heroes;
	heroes[hero] = NK2AI::HeroRole::MAIN;
	gateway->nullkiller->pathfinder->updatePaths(heroes, NK2AI::PathfinderSettings());
	const int3 destination(19, 18, 0);
	ASSERT_FALSE(gateway->nullkiller->pathfinder->getPathInfo(destination).empty());
	RemoveObject removeHero(heroID, player);
	gameState()->apply(removeHero);
	EXPECT_TRUE(gateway->nullkiller->pathfinder->getPathInfo(destination).empty());
}

INSTANTIATE_TEST_SUITE_P(EdgesAndCorners, Nullkiller2_ObjectGraphSeams, ::testing::Combine(::testing::Values(
	std::pair{int3(0, 10, 0), int3(35, 10, 0)},
	std::pair{int3(35, 10, 0), int3(0, 10, 0)},
	std::pair{int3(10, 0, 0), int3(10, 35, 0)},
	std::pair{int3(10, 35, 0), int3(10, 0, 0)},
	std::pair{int3(0, 0, 0), int3(35, 35, 0)},
	std::pair{int3(35, 35, 0), int3(0, 0, 0)},
	std::pair{int3(0, 35, 0), int3(35, 0, 0)},
	std::pair{int3(35, 0, 0), int3(0, 35, 0)}), ::testing::Values(0, 1)));
