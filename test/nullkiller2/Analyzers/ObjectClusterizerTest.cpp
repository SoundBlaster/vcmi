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
#include "lib/mapping/MapTopology.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGResource.h"
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
}
