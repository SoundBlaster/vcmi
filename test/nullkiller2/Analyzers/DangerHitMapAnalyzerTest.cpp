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
#include "lib/pathfinder/CPathfinder.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

namespace
{
using Seam = std::pair<int3, int3>;
class Nullkiller2_DangerAcrossSeam : public NullkillerTest,
	public ::testing::WithParamInterface<std::tuple<Seam, int>> {};
class Nullkiller2_PathfinderKnowledge : public NullkillerTest {};
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
