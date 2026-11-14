/*
 * AIUtilityTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/AIUtility.h"

#include "nullkiller2/NullkillerTest.h"

#include "lib/CPlayerState.h"
#include "lib/callback/IClient.h"
#include "lib/gameState/CGameState.h"
#include "lib/gameState/QuestInfo.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/Quest.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"
#include "lib/serializer/CTypeList.h"

#include <future>

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const int3 HERO_POS(5, 5, 0);
const int3 SEER_POS(7, 5, 0);

TinyH3M::TinyH3MBuilder makeQuestlessSeerMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder
		.hotaVersion(3)
		.size(36, false)
		.name("NK2QuestlessSeer")
		.playerActive(PLAYER)
		.randomTown({2, 2, 0}, PLAYER)
		.hero(HERO_POS, HeroTypeID(0), PLAYER)
		.seerHutMulti(SEER_POS, {}, {});

	return builder;
}

TinyH3M::TinyH3MBuilder makeQuestGuardMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder
		.hotaVersion(3)
		.size(36, false)
		.name("NK2KnownQuestGuard")
		.playerActive(PLAYER)
		.randomTown({2, 2, 0}, PLAYER)
		.hero(HERO_POS, HeroTypeID(0), PLAYER)
		.questGuard(SEER_POS, TinyH3M::TinyH3MBuilder::missionLevel(30));

	return builder;
}

class Nullkiller2_AIUtility : public NullkillerTest {};
}

TEST(Nullkiller2_AIStatus, RetryWaitDoesNotReportARejectedTurnAsFinished)
{
	NK2AI::AIStatus status(nullptr);
	status.startedTurn();
	EXPECT_FALSE(status.waitForTurnEnd(std::chrono::milliseconds(20)));
	EXPECT_TRUE(status.haveTurn());
}

TEST(Nullkiller2_AIStatus, RetryWaitWakesWhenTurnEnds)
{
	NK2AI::AIStatus status(nullptr);
	status.startedTurn();
	std::promise<void> started;
	auto startedFuture = started.get_future();
	auto waiting = std::async(std::launch::async, [&] {
		started.set_value();
		return status.waitForTurnEnd(std::chrono::seconds(2));
	});
	startedFuture.wait();
	EXPECT_EQ(waiting.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
	status.madeTurn();
	EXPECT_EQ(waiting.wait_for(std::chrono::seconds(1)), std::future_status::ready);
	EXPECT_TRUE(waiting.get());
}

TEST_F(Nullkiller2_AIUtility, RejectedEndTurnRetriesWithoutHoldingGameStateLock)
{
	ASSERT_NO_FATAL_FAILURE(startWithMap(makeQuestlessSeerMap()));
	gameState()->players.at(PLAYER).status = EPlayerStatus::INGAME;
	class RetryClient : public IClient
	{
	public:
		NK2AI::AIGateway * gateway = nullptr;
		std::promise<void> firstRequest;
		std::vector<std::chrono::steady_clock::time_point> attempts;
		std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override { return std::nullopt; }
		int sendRequest(const CPackForServer & request, PlayerColor, bool) override
		{
			if(dynamic_cast<const EndTurn *>(&request))
			{
				attempts.push_back(std::chrono::steady_clock::now());
				PackageApplied reply;
				reply.packType = CTypeList::getInstance().getTypeID<EndTurn>(nullptr);
				reply.result = attempts.size() > 1;
				gateway->requestRealized(&reply);
				if(attempts.size() == 1)
					firstRequest.set_value();
			}
			return static_cast<int>(attempts.size());
		}
	} client;
	auto first = client.firstRequest.get_future();
	auto gateway = makeGateway(PLAYER, &client);
	client.gateway = gateway.get();
	gateway->status.startedTurn();
	auto turn = std::async(std::launch::async, [&] {
		withGameStateReadLock([&] { gateway->endTurn(); });
	});
	const auto firstResult = first.wait_for(std::chrono::seconds(2));
	if(firstResult != std::future_status::ready)
		gateway->status.madeTurn();
	EXPECT_EQ(firstResult, std::future_status::ready);
	if(firstResult != std::future_status::ready)
	{
		turn.get();
		return;
	}
	auto writer = std::async(std::launch::async, [] {
		std::unique_lock lock(CGameState::mutex);
		return true;
	});
	EXPECT_EQ(writer.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);
	EXPECT_TRUE(writer.get());
	const auto finished = turn.wait_for(std::chrono::seconds(3));
	if(finished != std::future_status::ready)
		gateway->status.madeTurn();
	EXPECT_EQ(finished, std::future_status::ready);
	turn.get();
	ASSERT_EQ(client.attempts.size(), 2);
	EXPECT_GE(client.attempts[1] - client.attempts[0], std::chrono::milliseconds(800));
	EXPECT_FALSE(gateway->status.haveTurn());
}

TEST_F(Nullkiller2_AIUtility, neighbourEnumerationWrapsAtCorners)
{
	ASSERT_NO_FATAL_FAILURE(startWithMap(makeQuestlessSeerMap()));
	const auto callback = makeCallback(PLAYER);
	const int3 corner(0, 0, 0);
	std::set<int3> neighbours;

	NK2AI::foreach_neighbour(*callback, corner, [&](const int3 & tile) { neighbours.insert(tile); });
	EXPECT_EQ(neighbours.size(), 3);
	EXPECT_FALSE(neighbours.contains(int3(35, 35, 0)));

	map()->toroidal = true;
	neighbours.clear();
	NK2AI::foreach_neighbour(*callback, corner, [&](const int3 & tile) { neighbours.insert(tile); });
	EXPECT_EQ(neighbours.size(), 8);
	EXPECT_TRUE(neighbours.contains(int3(35, 0, 0)));
	EXPECT_TRUE(neighbours.contains(int3(0, 35, 0)));
	EXPECT_TRUE(neighbours.contains(int3(35, 35, 0)));
}

TEST_F(Nullkiller2_AIUtility, trackedSeerWithoutActiveQuestIsNotVisitable)
{
	ASSERT_NO_FATAL_FAILURE(startWithMap(makeQuestlessSeerMap()));

	const auto * hero = findHeroAt(HERO_POS);
	const auto * seer = expectAt<SeerHut>(SEER_POS);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(seer->getActiveQuest(), nullptr);

	gameState()->players.at(PLAYER).quests.push_back(seer->getQuestIdentity());

	const auto callback = makeCallback(PLAYER);
	auto gateway = std::make_unique<NK2AI::AIGateway>();
	gateway->initGameInterface(std::shared_ptr<Environment>(), callback);

	EXPECT_FALSE(NK2AI::shouldVisit(gateway->nullkiller.get(), hero, seer));
}

// quest objects are never remembered as visited, so a known but unmet guard would be revisited every pass
TEST_F(Nullkiller2_AIUtility, knownUnmetQuestGuardIsNotVisitable)
{
	ASSERT_NO_FATAL_FAILURE(startWithMap(makeQuestGuardMap()));

	const auto * hero = findHeroAt(HERO_POS);
	const auto * guard = expectAt<QuestGuard>(SEER_POS);
	ASSERT_NE(hero, nullptr);

	const auto callback = makeCallback(PLAYER);
	auto gateway = std::make_unique<NK2AI::AIGateway>();
	gateway->initGameInterface(std::shared_ptr<Environment>(), callback);

	EXPECT_TRUE(NK2AI::shouldVisit(gateway->nullkiller.get(), hero, guard));

	gameState()->players.at(PLAYER).quests.push_back(guard->getQuestIdentity());

	EXPECT_FALSE(NK2AI::shouldVisit(gateway->nullkiller.get(), hero, guard));
}
