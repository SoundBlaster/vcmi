/*
 * ExecuteHeroChainTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"

#include "mock/GameHandlerTestClient.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/gameState/CGameState.h"
#include "lib/CPlayerState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGResource.h"
#include "lib/mapObjects/MiscObjects.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER(0);
const PlayerColor ENEMY(1);
const int3 HERO_ANCHOR_POS(6, 5, 0);
const int3 HERO_POS(5, 5, 0);
const int3 SCROLL_POS(6, 5, 0);
const int3 BEYOND_SCROLL_POS(7, 5, 0);

class MoveCountingClient : public GameHandlerTestClient
{
public:
	using GameHandlerTestClient::GameHandlerTestClient;

	int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) override
	{
		if(dynamic_cast<const MoveHero *>(&request))
			++movementRequests;

		return GameHandlerTestClient::sendRequest(request, player, waitTillRealize);
	}

	int movementRequests = 0;
};

class ExecuteHeroChainMovementTest : public NullkillerTest
{
protected:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER)
			.hero(HERO_ANCHOR_POS, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID(0), 1}})
			.scroll(SCROLL_POS, SpellID(0))
			.playerActive(ENEMY)
			.hero({30, 30, 0}, HeroTypeID(1), ENEMY);

		startWithMap(std::move(builder));
	}
};
}

TEST_F(ExecuteHeroChainMovementTest, blockingVisitOnRouteStopsChainForReplanning)
{
	startGame();
	revealMap(PLAYER);

	auto * hero = findHeroByOwner(PLAYER);
	auto * scroll = findFirst<CGArtifact>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(scroll, nullptr);
	ASSERT_EQ(hero->visitablePos(), HERO_POS);
	ASSERT_EQ(scroll->visitablePos(), SCROLL_POS);
	const auto scrollID = scroll->id;
	hero->setMovementPoints(2000);

	MoveCountingClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);

	NK2AI::AIPath path;
	path.targetHero = hero;
	path.heroArmy = hero;
	path.chainMask = 1;
	path.nodes.push_back(pathNode(*hero, BEYOND_SCROLL_POS));
	path.nodes.push_back(pathNode(*hero, SCROLL_POS));
	path.nodes.push_back(pathNode(*hero, HERO_POS));

	EXPECT_NO_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()); }));
	EXPECT_EQ(gameState()->getObjInstance(scrollID), nullptr) << "the hero must pick up the scroll";
	EXPECT_EQ(hero->visitablePos(), HERO_POS) << "picking up a scroll does not move the hero";
	EXPECT_EQ(client.movementRequests, 1) << "the chain must stop after the pickup instead of following the stale route";
}

TEST_F(ExecuteHeroChainMovementTest, plannedCornerResourceVisitUsesRealServer)
{
	const int3 start(0, 0, 0), destination(35, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).playerActive(ENEMY)
		.hero(int3(1, 0, 0), HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(0), 1}})
		.hero(int3(20, 20, 0), HeroTypeID(1), ENEMY)
		.resource(destination, GameResID::GOLD, 10);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(PLAYER);
	auto * hero = findHeroByOwner(PLAYER);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	ASSERT_EQ(hero->visitablePos(), start);
	ASSERT_EQ(resource->visitablePos(), destination);
	const auto resourceID = resource->id;
	const auto goldBefore = gameState()->players.at(PLAYER).resources[GameResID::GOLD];
	hero->setMovementPoints(2000);

	MoveCountingClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);
	const auto paths = withGameStateReadLock([&]
	{
		preparePlanning(*gateway);
		return gateway->nullkiller->pathfinder->getPathInfo(destination);
	});
	const auto path = std::ranges::find_if(paths, [&](const auto & candidate)
	{
		return candidate.targetHero == hero && !candidate.nodes.empty() && candidate.turn() == 0
			&& candidate.firstTileToGet() == destination;
	});
	ASSERT_NE(path, paths.end());
	EXPECT_NO_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(*path, resource).accept(gateway.get()); }));
	EXPECT_EQ(client.movementRequests, 1);
	EXPECT_EQ(gameState()->getMap().getObject(resourceID), nullptr);
	EXPECT_EQ(gameState()->players.at(PLAYER).resources[GameResID::GOLD], goldBefore + 1000);
	EXPECT_EQ(hero->visitablePos(), start);
}

TEST_F(ExecuteHeroChainMovementTest, repeatedIdleChainFailsForReplanning)
{
	startGame();

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(2000);

	MoveCountingClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);

	NK2AI::AIPath path;
	path.targetHero = hero;
	path.heroArmy = hero;
	path.chainMask = 1;
	path.nodes.push_back(pathNode(*hero, HERO_POS));

	EXPECT_NO_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()); }));
	EXPECT_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()); }), NK2AI::cannotFulfillGoalException)
		<< "an idle chain counted as success would be selected again on every pass";
	EXPECT_EQ(client.movementRequests, 0);
}

TEST_F(ExecuteHeroChainMovementTest, multiTurnNodeLocksHeroWithoutMovementPoints)
{
	startGame();
	revealMap(PLAYER);

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(10); // not enough for a single step

	MoveCountingClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);

	auto node = pathNode(*hero, int3(20, 5, 0));
	node.turns = 1;

	NK2AI::AIPath path;
	path.targetHero = hero;
	path.heroArmy = hero;
	path.chainMask = 1;
	path.nodes.push_back(node);

	EXPECT_NO_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()); }));
	EXPECT_TRUE(gateway->nullkiller->isHeroLocked(hero)) << "the hero must wait for the next turn instead of failing the route";
	EXPECT_EQ(client.movementRequests, 0);
}
