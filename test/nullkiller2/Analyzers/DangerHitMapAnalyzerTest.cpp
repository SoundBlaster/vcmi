/*
 * DangerHitMapAnalyzerTest.cpp, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder.
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Analyzers/DangerHitMapAnalyzer.h"
#include "AI/Nullkiller2/Engine/AIMemory.h"
#include "lib/CPlayerState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/MiscObjects.h"
#include "lib/pathfinder/CPathfinder.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

namespace
{
using Seam = std::pair<int3, int3>;
class Nullkiller2_DangerAcrossSeam : public NullkillerTest,
	public ::testing::WithParamInterface<std::tuple<Seam, int>> {};
class Nullkiller2_PathfinderKnowledge : public NullkillerTest {};
class Nullkiller2_DangerThroughPortal : public NullkillerTest, public ::testing::WithParamInterface<bool> {};
}

TEST_P(Nullkiller2_DangerThroughPortal, UsesObserversKnowledgeOfEnemyTeleportRoute)
{
	const PlayerColor observer(0), enemy(1);
	const int3 entrance(1, 10, 0), exit(30, 25, 0), enemyStart(3, 10, 0), target(29, 25, 0);
	const std::set<int3> land = {entrance, exit, enemyStart, int3(2, 10, 0), target, int3(18, 18, 0)};
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(observer).playerActive(enemy)
		.hero({19, 18, 0}, HeroTypeID(0), observer)
		.hero({4, 10, 0}, HeroTypeID(1), enemy)
		.heroGarrison({{CreatureID::ARCHER, 100}})
		.twoWayPortal(entrance, 0).twoWayPortal(exit, 0);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(!land.contains(int3(x, y, 0)))
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(GetParam());
	revealMap(observer);
	setMapVisibility(enemy, false);
	auto & enemyFog = gameState()->getPlayerTeam(enemy)->fogOfWarMap;
	enemyFog[enemyStart] = 1;
	enemyFog[entrance] = 1;
	auto * enemyHero = findHeroByOwner(enemy);
	ASSERT_NE(enemyHero, nullptr);
	ASSERT_EQ(enemyHero->visitablePos(), enemyStart);
	auto gateway = makeGateway(observer);
	auto & ai = *gateway->nullkiller;
	ai.memory->addVisitableObject(enemyHero);
	ai.dangerHitMap->updateHitMap();
	ASSERT_GT(ai.dangerHitMap->getTileThreat(int3(2, 10, 0)).fastestDanger.danger, 0);
	const auto threat = ai.dangerHitMap->getTileThreat(target).fastestDanger;
	EXPECT_EQ(threat.turn, 0);
	EXPECT_GT(threat.danger, 0);
	EXPECT_EQ(threat.heroPtr.idOrNone(), enemyHero->id);

	// Enemy vision cannot reveal a teleport exit that the AI has not seen.
	gameState()->getPlayerTeam(observer)->fogOfWarMap[exit] = 0;
	enemyFog[exit] = 1;
	ai.dangerHitMap->resetHitmap();
	ai.dangerHitMap->updateHitMap();
	EXPECT_EQ(ai.dangerHitMap->getTileThreat(target).fastestDanger.danger, 0);
}

INSTANTIATE_TEST_SUITE_P(MapTopologies, Nullkiller2_DangerThroughPortal, ::testing::Bool());

TEST_F(Nullkiller2_PathfinderKnowledge, ObserverControlsVisibleTeleportChannel)
{
	const PlayerColor observer(0), owner(1);
	const int3 entrancePos(0, 10, 0), exitPos(35, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(observer).playerActive(owner)
		.hero({19, 18, 0}, HeroTypeID(0), observer)
		.hero({11, 10, 0}, HeroTypeID(1), owner)
		.hero({14, 10, 0}, HeroTypeID(2), owner)
		.twoWayPortal(entrancePos, 0).twoWayPortal(exitPos, 0);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(observer);
	setMapVisibility(owner, false);
	gameState()->getPlayerTeam(owner)->fogOfWarMap[entrancePos] = 1;
	auto * hero = findHeroByOwner(owner);
	auto * entrance = dynamic_cast<const CGTeleport *>(findObjectAt(entrancePos));
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(entrance, nullptr);

	PathfinderOptions observerOptions(*gameState());
	observerOptions.knowledgePlayer = observer;
	observerOptions.useTeleportTwoWay = true;
	PathfinderOptions ownerOptions(*gameState());
	ownerOptions.useTeleportTwoWay = true;
	CPathfinderHelper observed(*gameState(), hero, observerOptions);
	CPathfinderHelper ordinary(*gameState(), hero, ownerOptions);

	EXPECT_TRUE(observed.isAllowedTeleportEntrance(entrance));
	EXPECT_FALSE(ordinary.isAllowedTeleportEntrance(entrance));
	EXPECT_TRUE(vstd::contains(observed.getAllowedTeleportChannelExits(entrance->channel), exitPos));
	EXPECT_FALSE(vstd::contains(ordinary.getAllowedTeleportChannelExits(entrance->channel), exitPos));
	// Exit access is still judged relative to the moving hero's owner.
	auto * observerHero = findHeroByOwner(observer);
	auto * friendlyHero = findHeroAt(int3(14, 10, 0));
	ASSERT_NE(observerHero, nullptr);
	ASSERT_NE(friendlyHero, nullptr);
	map()->moveObject(observerHero->id, observerHero->convertFromVisitablePos(exitPos));
	EXPECT_TRUE(vstd::contains(observed.getAllowedTeleportChannelExits(entrance->channel), exitPos));
	map()->moveObject(observerHero->id, int3(19, 18, 0));
	map()->moveObject(friendlyHero->id, friendlyHero->convertFromVisitablePos(exitPos));
	EXPECT_FALSE(vstd::contains(observed.getAllowedTeleportChannelExits(entrance->channel), exitPos));
	map()->moveObject(friendlyHero->id, int3(14, 10, 0));

	gameState()->getPlayerTeam(observer)->fogOfWarMap[exitPos] = 0;
	gameState()->getPlayerTeam(owner)->fogOfWarMap[exitPos] = 1;
	EXPECT_FALSE(observed.isAllowedTeleportEntrance(entrance));
	EXPECT_TRUE(ordinary.isAllowedTeleportEntrance(entrance));
	EXPECT_FALSE(vstd::contains(observed.getAllowedTeleportChannelExits(entrance->channel), exitPos));
	EXPECT_TRUE(vstd::contains(ordinary.getAllowedTeleportChannelExits(entrance->channel), exitPos));
}

TEST_F(Nullkiller2_PathfinderKnowledge, ObserverControlsOneWayExitCount)
{
	const PlayerColor observer(0), owner(1);
	const int3 entrancePos(5, 10, 0), firstExit(15, 10, 0), secondExit(25, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(observer).playerActive(owner)
		.hero({19, 18, 0}, HeroTypeID(0), observer)
		.hero({11, 10, 0}, HeroTypeID(1), owner)
		.oneWayPortalEntrance(entrancePos, 0)
		.oneWayPortalExit(firstExit, 0).oneWayPortalExit(secondExit, 0);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(observer);
	revealMap(owner);
	auto * hero = findHeroByOwner(owner);
	auto * entrance = dynamic_cast<const CGTeleport *>(findObjectAt(entrancePos));
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(entrance, nullptr);
	PathfinderOptions observerOptions(*gameState());
	observerOptions.knowledgePlayer = observer;
	observerOptions.useTeleportOneWay = true;
	observerOptions.useTeleportOneWayRandom = true;
	PathfinderOptions ownerOptions(*gameState());
	ownerOptions.useTeleportOneWay = true;
	ownerOptions.useTeleportOneWayRandom = true;
	CPathfinderHelper observed(*gameState(), hero, observerOptions);
	CPathfinderHelper ordinary(*gameState(), hero, ownerOptions);

	gameState()->getPlayerTeam(observer)->fogOfWarMap[secondExit] = 0;
	EXPECT_TRUE(observed.addTeleportOneWay(entrance));
	EXPECT_FALSE(observed.addTeleportOneWayRandom(entrance));
	EXPECT_FALSE(ordinary.addTeleportOneWay(entrance));
	EXPECT_TRUE(ordinary.addTeleportOneWayRandom(entrance));
	gameState()->getPlayerTeam(observer)->fogOfWarMap[secondExit] = 1;
	gameState()->getPlayerTeam(owner)->fogOfWarMap[secondExit] = 0;
	EXPECT_FALSE(observed.addTeleportOneWay(entrance));
	EXPECT_TRUE(observed.addTeleportOneWayRandom(entrance));
	EXPECT_TRUE(ordinary.addTeleportOneWay(entrance));
	EXPECT_FALSE(ordinary.addTeleportOneWayRandom(entrance));
}

TEST_F(Nullkiller2_PathfinderKnowledge, ObserverControlsKnownNeighborsAndGuards)
{
	const PlayerColor observer(0), owner(1);
	const int3 guard(35, 10, 0), target(0, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(observer).playerActive(owner)
		.hero({19, 18, 0}, HeroTypeID(0), observer)
		.hero({11, 10, 0}, HeroTypeID(1), owner)
		.monster(guard, CreatureID::ARCHER, 10);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(observer);
	revealMap(owner);
	auto * hero = findHeroByOwner(owner);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(gameState()->guardingCreaturePosition(target), guard);

	PathfinderOptions observerOptions(*gameState());
	observerOptions.knowledgePlayer = observer;
	CPathfinderHelper observed(*gameState(), hero, observerOptions);
	CPathfinderHelper ordinary(*gameState(), hero, PathfinderOptions(*gameState()));
	auto & observerFog = gameState()->getPlayerTeam(observer)->fogOfWarMap;
	auto & ownerFog = gameState()->getPlayerTeam(owner)->fogOfWarMap;
	const auto hasGuardNeighbor = [&](const CPathfinderHelper & helper)
	{
		NeighbourTilesVector neighbors;
		helper.getNeighbours(map()->getTile(target), target, neighbors, false, true);
		return std::ranges::find(neighbors, guard) != neighbors.end();
	};

	observerFog[guard] = 0;
	EXPECT_EQ(observed.getGuardiansCount(target), 0);
	EXPECT_FALSE(observed.getKnownGuardingCreaturePosition(target).isValid());
	EXPECT_FALSE(hasGuardNeighbor(observed));
	EXPECT_EQ(ordinary.getGuardiansCount(target), 1);
	EXPECT_EQ(ordinary.getKnownGuardingCreaturePosition(target), guard);
	EXPECT_TRUE(hasGuardNeighbor(ordinary));

	observerFog[guard] = 1;
	ownerFog[guard] = 0;
	EXPECT_EQ(observed.getGuardiansCount(target), 1);
	EXPECT_EQ(observed.getKnownGuardingCreaturePosition(target), guard);
	EXPECT_TRUE(hasGuardNeighbor(observed));
	EXPECT_EQ(ordinary.getGuardiansCount(target), 0);
	EXPECT_FALSE(ordinary.getKnownGuardingCreaturePosition(target).isValid());
	EXPECT_FALSE(hasGuardNeighbor(ordinary));
}

TEST_P(Nullkiller2_DangerAcrossSeam, RecognizesVisibleEnemyThreatInCurrentTurn)
{
	const auto & [seam, level] = GetParam();
	const PlayerColor player(0), enemy(1);
	const int3 source(seam.first.x, seam.first.y, level);
	const int3 target(seam.second.x, seam.second.y, level);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).playerActive(enemy)
		.hero({19, 18, 0}, HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.hero(source + int3(1, 0, 0), HeroTypeID(1), enemy)
		.heroGarrison({{CreatureID::ARCHER, 100}});
	startWithMap(std::move(builder));
	revealMap(player);
	// A player's threat estimate must not depend on the other team's fog.
	// The deliberately narrow enemy vision isolates that information boundary.
	setMapVisibility(enemy, false);
	gameState()->getPlayerTeam(enemy)->fogOfWarMap[source] = 1;
	auto * enemyHero = findHeroByOwner(enemy);
	ASSERT_NE(enemyHero, nullptr);
	ASSERT_EQ(enemyHero->visitablePos(), source);
	enemyHero->setMovementPoints(300);

	auto gateway = makeGateway(player);
	auto & ai = *gateway->nullkiller;
	ai.memory->addVisitableObject(enemyHero);
	ASSERT_EQ(ai.cc->getPlayerRelations(player, enemy), PlayerRelations::ENEMIES);
	ASSERT_EQ(ai.cc->getObjInstance(enemyHero->id), enemyHero);
	ASSERT_TRUE(ai.memory->visitableObjs.contains(enemyHero->id));

	map()->setToroidalAndRebuild(false);
	ai.dangerHitMap->updateHitMap();
	const auto interior = source + int3(source.x == 34 ? -1 : 1, 0, 0);
	EXPECT_GT(ai.dangerHitMap->getTileThreat(interior).fastestDanger.danger, 0)
		<< "the bounded control must detect an ordinary nearby enemy";
	const auto boundedThreat = ai.dangerHitMap->getTileThreat(target).fastestDanger;
	EXPECT_NE(boundedThreat.turn, 0) << "the bounded control should not cross the map in one turn";

	map()->setToroidalAndRebuild(true);
	ai.dangerHitMap->resetHitmap();
	ai.dangerHitMap->updateHitMap();
	const auto wrappedThreat = ai.dangerHitMap->getTileThreat(target).fastestDanger;
	EXPECT_EQ(wrappedThreat.turn, 0);
	EXPECT_GT(wrappedThreat.danger, 0);
	EXPECT_EQ(wrappedThreat.heroPtr.idOrNone(), enemyHero->id);

	// A fresh AI should not infer a hidden target from the opposing team's fog.
	gameState()->getPlayerTeam(player)->fogOfWarMap[target] = 0;
	gameState()->getPlayerTeam(enemy)->fogOfWarMap[target] = 1;
	auto hiddenGateway = makeGateway(player);
	hiddenGateway->nullkiller->memory->addVisitableObject(enemyHero);
	hiddenGateway->nullkiller->dangerHitMap->updateHitMap();
	EXPECT_EQ(hiddenGateway->nullkiller->dangerHitMap->getTileThreat(target).fastestDanger.danger, 0);
}

INSTANTIATE_TEST_SUITE_P(EdgesAndCorners, Nullkiller2_DangerAcrossSeam,
	::testing::Combine(::testing::Values(
		Seam{{0, 10, 0}, {35, 10, 0}}, Seam{{34, 10, 0}, {0, 10, 0}},
		Seam{{10, 0, 0}, {10, 35, 0}}, Seam{{10, 34, 0}, {10, 0, 0}},
		Seam{{0, 0, 0}, {35, 35, 0}}, Seam{{34, 34, 0}, {0, 0, 0}},
		Seam{{0, 34, 0}, {35, 0, 0}}, Seam{{34, 0, 0}, {0, 35, 0}}),
		::testing::Values(0, 1)));
