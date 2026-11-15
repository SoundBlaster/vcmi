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
#include "AI/Nullkiller2/Analyzers/DangerHitMapAnalyzer.h"
#include "AI/Nullkiller2/Behaviors/EscapeBehavior.h"
#include "AI/Nullkiller2/Engine/AIMemory.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "AI/Nullkiller2/Pathfinding/Actions/AdventureSpellCastMovementActions.h"

#include "mock/GameHandlerTestClient.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/gameState/CGameState.h"
#include "lib/CPlayerState.h"
#include "lib/bonuses/Bonus.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGResource.h"
#include "lib/mapObjects/MiscObjects.h"
#include "lib/mapping/MapTopology.h"
#include "lib/networkPacks/PacksForServer.h"
#include "lib/serializer/CMemorySerializer.h"
#include "lib/spells/CSpell.h"

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
		if(const auto * move = dynamic_cast<const MoveHero *>(&request))
		{
			++movementRequests;
			for(const auto & destination : move->path)
				moves.emplace_back(destination, move->layer);
		}

		return GameHandlerTestClient::sendRequest(request, player, waitTillRealize);
	}

	int movementRequests = 0;
	std::vector<std::pair<int3, EPathfindingLayer>> moves;
};

class AcknowledgingServer : public GameHandlerTestServer
{
public:
	AcknowledgingServer(const std::shared_ptr<CGameState> & state, PlayerColor player)
		: GameHandlerTestServer(state, player), state(state) {}
	void applyPack(CPackForClient & pack) override
	{
		if(gateway)
			if(const auto * removal = dynamic_cast<const RemoveObject *>(&pack))
				if(const auto * object = state->getObjInstance(removal->objectID))
					gateway->objectRemoved(object, removal->initiator);
		GameHandlerTestServer::applyPack(pack);
		if(gateway)
			if(const auto * move = dynamic_cast<const TryMoveHero *>(&pack))
			{
				gateway->invalidatePaths();
				gateway->heroMoved(*move, false);
			}
	}
	void sendPack(CPackForClient & pack, GameConnectionID) override
	{
		if(const auto * applied = dynamic_cast<const PackageApplied *>(&pack))
			response = *applied;
	}
	std::optional<PackageApplied> response;
	NK2AI::AIGateway * gateway = nullptr;
private:
	std::shared_ptr<CGameState> state;
};

class SpellAndMoveClient : public IClient
{
public:
	SpellAndMoveClient(const std::shared_ptr<CGameState> & state, PlayerColor player)
		: server(state, player), handler(server, state)
	{
		state->actingPlayers.insert(player);
	}
	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override { return std::nullopt; }
	int sendRequest(const CPackForServer & request, PlayerColor player, bool) override
	{
		request.player = player;
		request.requestID = ++requestID;
		if(dynamic_cast<const CastAdvSpell *>(&request))
		{
			++casts;
			if(beforeCast)
				beforeCast();
		}
		const auto * movement = dynamic_cast<const MoveHero *>(&request);
		const auto * hero = movement ? handler.gameState().getHero(movement->hid) : nullptr;
		const auto before = hero ? hero->visitablePos() : int3(-1, -1, -1);
		if(movement)
			++moves;
		server.response.reset();
		auto copy = CMemorySerializer::deepCopy(request);
		handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, *copy);
		if(!server.response || server.response->requestID != requestID)
			throw std::runtime_error("Server did not acknowledge request");
		if(hero && server.response->result)
			appliedMoves.emplace_back(before, hero->visitablePos(), movement->layer);
		gateway->requestRealized(&*server.response);
		return requestID;
	}
	NK2AI::AIGateway * gateway = nullptr;
	int casts = 0;
	int moves = 0;
	std::vector<std::tuple<int3, int3, EPathfindingLayer>> appliedMoves;
	std::function<void()> beforeCast;
	void enableNotifications() { server.gateway = gateway; }
private:
	AcknowledgingServer server;
	CGameHandler handler;
	int requestID = 0;
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

class ToroidalHeroChainTest : public ExecuteHeroChainMovementTest,
	public ::testing::WithParamInterface<int>
{
protected:
	std::pair<int3, int3> seamPositions() const
	{
		static const std::array<std::pair<int, int>, 8> directions = {{{-1, -1}, {-1, 0}, {-1, 1},
			{0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}}};
		const auto [dx, dy] = directions.at(GetParam() % directions.size());
		const int z = GetParam() / directions.size();
		return {int3(dx < 0 ? 0 : dx > 0 ? 35 : 17, dy < 0 ? 0 : dy > 0 ? 35 : 17, z),
			int3(dx < 0 ? 35 : dx > 0 ? 0 : 17, dy < 0 ? 35 : dy > 0 ? 0 : 17, z)};
	}

	void executeTransitToShore(BonusType ability, EPathfindingLayer transitLayer)
	{
		const auto [start, water] = seamPositions();
		const MapTopology topology(36, 36, 2, true);
		const auto direction = topology.displacement(start, water);
		const auto shore = *topology.canonicalize(water + direction);
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, true).playerActive(PLAYER).playerActive(ENEMY)
			.hero(int3(19, 18, 0), HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID(0), 1}})
			.hero(int3(20, 20, 0), HeroTypeID(1), ENEMY);
		for(int x = 0; x < 36; ++x)
			for(int y = 0; y < 36; ++y)
			{
				const int3 position(x, y, start.z);
				if(position != start && position != shore && position != int3(18, 18, 0) && position != int3(19, 20, 0))
					builder.terrain(position, TerrainId::WATER);
			}
		startWithMap(std::move(builder));
		map()->setToroidalAndRebuild(true);
		revealMap(PLAYER);
		auto * hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);
		map()->moveObject(hero->id, hero->convertFromVisitablePos(start));
		ASSERT_EQ(hero->visitablePos(), start);
		hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, ability, BonusSource::OTHER, 0, BonusSourceID()));
		hero->setMovementPoints(2000);
		MoveCountingClient client(gameState(), PLAYER);
		auto gateway = makeGateway(PLAYER, &client);
		const auto paths = withGameStateReadLock([&]
		{
			preparePlanning(*gateway);
			return gateway->nullkiller->pathfinder->getPathInfo(shore);
		});
		const auto path = std::ranges::find_if(paths, [&](const auto & candidate)
		{
			return candidate.targetHero == hero && !candidate.nodes.empty() && candidate.turn() == 0;
		});
		ASSERT_NE(path, paths.end());
		EXPECT_NO_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(*path).accept(gateway.get()); }));
		EXPECT_EQ(hero->visitablePos(), shore);
		EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
		EXPECT_GE(client.movementRequests, 2);
		EXPECT_NE(std::ranges::find(client.moves, std::pair(hero->convertFromVisitablePos(water), transitLayer)), client.moves.end());
		EXPECT_LT(hero->movementPointsRemaining(), 2000);
	}

	void planAndCastTransitSpell(bool flight, bool rejectCast = false)
	{
		const auto [start, water] = seamPositions();
		const MapTopology topology(36, 36, 2, true);
		const auto shore = *topology.canonicalize(water + topology.displacement(start, water));
		const SpellID spell(SpellID::decode(flight ? "core:fly" : "core:waterWalk"));
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, true).playerActive(PLAYER).playerActive(ENEMY)
			.hero(int3(19, 18, 0), HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID(0), 1}})
			.heroPrimary(1, 1, 10, 10)
			.heroSecondarySkills({{SecondarySkill::WISDOM, 3}, {SecondarySkill::AIR_MAGIC, 3}, {SecondarySkill::WATER_MAGIC, 3}})
			.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
			.heroSpells({spell})
			.hero(int3(20, 20, 0), HeroTypeID(1), ENEMY);
		for(int x = 0; x < 36; ++x)
			for(int y = 0; y < 36; ++y)
			{
				const int3 position(x, y, start.z);
				if(position != start && position != shore && position != int3(18, 18, 0) && position != int3(19, 20, 0))
					builder.terrain(position, TerrainId::WATER);
			}
		startWithMap(std::move(builder));
		map()->setToroidalAndRebuild(true);
		revealMap(PLAYER);
		auto * hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);
		map()->moveObject(hero->id, hero->convertFromVisitablePos(start));
		hero->mana = 100;
		hero->setMovementPoints(2000);
		ASSERT_TRUE(hero->canCastThisSpell(spell.toSpell()));
		SpellAndMoveClient client(gameState(), PLAYER);
		auto gateway = makeGateway(PLAYER, &client);
		client.gateway = gateway.get();
		const auto paths = withGameStateReadLock([&]
		{
			preparePlanning(*gateway);
			return gateway->nullkiller->pathfinder->getPathInfo(shore);
		});
		const auto path = std::ranges::find_if(paths, [&](const auto & candidate)
		{
			return candidate.targetHero == hero && !candidate.nodes.empty() && candidate.turn() == 0
				&& std::ranges::any_of(candidate.nodes, [](const auto & node)
				{
					return dynamic_cast<const NK2AI::AIPathfinding::AdventureCastAction *>(node.specialAction.get()) != nullptr;
				});
		});
		ASSERT_NE(path, paths.end());
		if(rejectCast)
		{
			client.beforeCast = [&] { hero->mana = 0; };
			EXPECT_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(*path).accept(gateway.get()); }), NK2AI::cannotFulfillGoalException);
			EXPECT_EQ(client.casts, 1);
			EXPECT_FALSE(gateway->lastAdventureCastSucceeded.load());
			EXPECT_EQ(client.moves, 0);
			EXPECT_EQ(hero->visitablePos(), start);
			EXPECT_EQ(hero->movementPointsRemaining(), 2000);
			EXPECT_FALSE(hero->hasBonusOfType(flight ? BonusType::FLYING_MOVEMENT : BonusType::WATER_WALKING));
			return;
		}
		EXPECT_NO_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(*path).accept(gateway.get()); }));
		EXPECT_EQ(client.casts, 1);
		EXPECT_TRUE(gateway->lastAdventureCastSucceeded.load());
		EXPECT_TRUE(hero->hasBonusOfType(flight ? BonusType::FLYING_MOVEMENT : BonusType::WATER_WALKING));
		EXPECT_LT(hero->mana, 100);
		EXPECT_GE(client.moves, 2);
		EXPECT_TRUE(std::ranges::any_of(client.appliedMoves, [flight](const auto & move)
		{
			const auto & [from, to, layer] = move;
			return (std::abs(from.x - to.x) > 1 || std::abs(from.y - to.y) > 1)
				&& layer == (flight ? EPathfindingLayer::AIR : EPathfindingLayer::WATER);
		}));
		EXPECT_EQ(hero->visitablePos(), shore);
		EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
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

TEST_P(ToroidalHeroChainTest, plannedResourceVisitUsesRealServer)
{
	const auto [start, destination] = seamPositions();
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(PLAYER).playerActive(ENEMY)
		.hero(int3(19, 18, 0), HeroTypeID(0), PLAYER)
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
	map()->moveObject(hero->id, hero->convertFromVisitablePos(start));
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

TEST_P(ToroidalHeroChainTest, plannedMovementUsesRealServer)
{
	const auto [start, destination] = seamPositions();
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(PLAYER).playerActive(ENEMY)
		.hero(int3(19, 18, 0), HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(0), 1}})
		.hero(int3(20, 20, 0), HeroTypeID(1), ENEMY);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(PLAYER);
	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	map()->moveObject(hero->id, hero->convertFromVisitablePos(start));
	ASSERT_EQ(hero->visitablePos(), start);
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
	EXPECT_NO_THROW(withGameStateReadLock([&] { NK2AI::Goals::ExecuteHeroChain(*path).accept(gateway.get()); }));
	EXPECT_EQ(client.movementRequests, 1);
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_LT(hero->movementPointsRemaining(), 2000);
}

TEST_P(ToroidalHeroChainTest, plannedWaterWalkCrossesSeamAndLands)
{
	executeTransitToShore(BonusType::WATER_WALKING, EPathfindingLayer::WATER);
}

TEST_P(ToroidalHeroChainTest, plannedFlightCrossesSeamAndLands)
{
	executeTransitToShore(BonusType::FLYING_MOVEMENT, EPathfindingLayer::AIR);
}

TEST_P(ToroidalHeroChainTest, plansAndCastsWaterWalkAcrossSeam)
{
	planAndCastTransitSpell(false);
}

TEST_P(ToroidalHeroChainTest, plansAndCastsFlightAcrossSeam)
{
	planAndCastTransitSpell(true);
}

TEST_P(ToroidalHeroChainTest, rejectedWaterWalkDoesNotMoveAcrossSeam)
{
	planAndCastTransitSpell(false, true);
}

TEST_P(ToroidalHeroChainTest, rejectedFlightDoesNotMoveAcrossSeam)
{
	planAndCastTransitSpell(true, true);
}

INSTANTIATE_TEST_SUITE_P(AllSeamsAndLevels, ToroidalHeroChainTest, ::testing::Range(0, 16));

class ToroidalAITransientSafetyTest : public NullkillerTest, public ::testing::WithParamInterface<int>
{
};

TEST_P(ToroidalAITransientSafetyTest, ReturnsToIslandAfterServerBackedTransit)
{
	const int z = GetParam() / 2;
	const bool flight = GetParam() % 2;
	const int3 island(0, 18, z), safeWater(30, 18, z), unsafeWater(29, 18, z);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(PLAYER).playerActive(ENEMY)
		.hero(int3(19, 18, 0), HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(0), 1}})
		.hero(int3(20, 20, 0), HeroTypeID(1), ENEMY);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
		{
			const int3 position(x, y, z);
			if(position != island && position != int3(18, 18, 0) && position != int3(19, 20, 0))
				builder.terrain(position, TerrainId::WATER);
		}
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(PLAYER);
	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	map()->moveObject(hero->id, hero->convertFromVisitablePos(island));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT,
		flight ? BonusType::FLYING_MOVEMENT : BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(1200);
	MoveCountingClient client(gameState(), PLAYER);
	const auto transitLayer = flight ? EPathfindingLayer::AIR : EPathfindingLayer::WATER;
	for(int x = 35; x >= 30; --x)
	{
		const int3 tile(x, 18, z);
		client.sendRequest(MoveHero({hero->convertFromVisitablePos(tile)}, transitLayer, hero->id, true), PLAYER, false);
		ASSERT_EQ(hero->visitablePos(), tile);
	}
	const auto pointsBefore = hero->movementPointsRemaining();
	client.sendRequest(MoveHero({hero->convertFromVisitablePos(unsafeWater)}, transitLayer, hero->id, true), PLAYER, false);
	ASSERT_EQ(hero->visitablePos(), safeWater);
	ASSERT_EQ(hero->movementPointsRemaining(), pointsBefore);
	auto callback = makeCallback(PLAYER, &client);
	auto gateway = makeGateway(callback);
	const auto move = [&](const int3 & destination)
	{
		return withGameStateReadLock([&] { return gateway->moveHeroToTile(destination, NK2AI::HeroPtr(hero, callback.get())); });
	};
	const auto requestsBefore = client.movementRequests;
	EXPECT_EQ(move(unsafeWater), NK2AI::HeroMovementResult::BLOCKED);
	EXPECT_EQ(client.movementRequests, requestsBefore);
	EXPECT_EQ(hero->visitablePos(), safeWater);
	EXPECT_EQ(hero->movementPointsRemaining(), pointsBefore);
	EXPECT_EQ(move(island), NK2AI::HeroMovementResult::COMPLETE);
	EXPECT_EQ(hero->visitablePos(), island);
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_GT(client.movementRequests, requestsBefore);
}

TEST_P(ToroidalAITransientSafetyTest, FullTurnCastsToCollectVisibleGoldAcrossSeam)
{
	const int z = GetParam() / 2;
	const bool flight = GetParam() % 2;
	const SpellID spell(SpellID::decode(flight ? "core:fly" : "core:waterWalk"));
	const int3 island(0, 18, z), shore(34, 18, z), gold(33, 18, z);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(PLAYER).playerActive(ENEMY)
		.hero(int3(19, 18, 0), HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(0), 1}})
		.heroPrimary(1, 1, 10, 10)
		.heroSecondarySkills({{SecondarySkill::WISDOM, 3}, {SecondarySkill::AIR_MAGIC, 3}, {SecondarySkill::WATER_MAGIC, 3}})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
		.heroSpells({spell})
		.hero(int3(20, 20, 0), HeroTypeID(1), ENEMY)
		.resource(gold, GameResID::GOLD, 10);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
		{
			const int3 position(x, y, z);
			if(position != island && position != shore && position != gold && position != int3(18, 18, 0) && position != int3(19, 20, 0))
				builder.terrain(position, TerrainId::WATER);
		}
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(PLAYER);
	auto * hero = findHeroByOwner(PLAYER);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	const auto resourceId = resource->id;
	map()->moveObject(hero->id, hero->convertFromVisitablePos(island));
	hero->mana = 100;
	hero->setMovementPoints(2000);
	const auto goldBefore = gameState()->players.at(PLAYER).resources[GameResID::GOLD];
	SpellAndMoveClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);
	client.gateway = gateway.get();
	client.enableNotifications();
	withGameStateReadLock([&] { gateway->nullkiller->makeTurn(); });
	EXPECT_EQ(client.casts, 1);
	EXPECT_TRUE(gateway->lastAdventureCastSucceeded.load());
	EXPECT_EQ(gameState()->getObjInstance(resourceId), nullptr);
	EXPECT_EQ(gameState()->players.at(PLAYER).resources[GameResID::GOLD], goldBefore + 1000);
	EXPECT_TRUE(std::ranges::any_of(client.appliedMoves, [flight](const auto & move)
	{
		const auto & [from, to, layer] = move;
		return (std::abs(from.x - to.x) > 1 || std::abs(from.y - to.y) > 1)
			&& layer == (flight ? EPathfindingLayer::AIR : EPathfindingLayer::WATER);
	}));
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
}

INSTANTIATE_TEST_SUITE_P(AbilitiesAndLevels, ToroidalAITransientSafetyTest, ::testing::Range(0, 4));

using EscapeSeam = std::pair<int3, int3>;
class ToroidalAIEscapeTest : public NullkillerTest,
	public ::testing::WithParamInterface<std::tuple<EscapeSeam, int>> {};

TEST_P(ToroidalAIEscapeTest, FullTurnEscapesEnemyAcrossSeam)
{
	const auto & [seam, z] = GetParam();
	const int3 start(seam.first.x, seam.first.y, z), direction = seam.second;
	const MapTopology topology(36, 36, 2, true);
	const auto position = [&](int distance) { return *topology.canonicalize(start + direction * distance); };
	const auto enemyPosition = position(-1);
	std::set<int3> corridor;
	for(int distance = -1; distance <= 20; ++distance)
		corridor.insert(position(distance));
	if(z == 0)
	{
		// The H3M loader needs valid starting positions before heroes are moved.
		corridor.insert(int3(5, 5, 0));
		corridor.insert(int3(7, 5, 0));
	}
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(PLAYER).playerActive(ENEMY)
		.hero(int3(6, 5, 0), HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.hero(int3(8, 5, 0), HeroTypeID(1), ENEMY)
		.heroGarrison({{CreatureID::ARCHER, 100}});
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(!corridor.contains(int3(x, y, z)))
				builder.terrain(int3(x, y, z), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(PLAYER);
	revealMap(ENEMY);
	auto * hero = findHeroByOwner(PLAYER);
	auto * enemy = findHeroByOwner(ENEMY);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(enemy, nullptr);
	map()->moveObject(hero->id, hero->convertFromVisitablePos(start));
	map()->moveObject(enemy->id, enemy->convertFromVisitablePos(enemyPosition));
	ASSERT_EQ(hero->visitablePos(), start);
	ASSERT_EQ(enemy->visitablePos(), enemyPosition);
	hero->setMovementPoints(3000);
	enemy->setMovementPoints(100);
	SpellAndMoveClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);
	client.gateway = gateway.get();
	client.enableNotifications();
	NK2AI::AIGateway::memorizeVisitableObjs(gateway->nullkiller->memory,
		gateway->nullkiller->dangerHitMap, PLAYER, gateway->cc);
	ASSERT_TRUE(gateway->nullkiller->memory->visitableObjs.contains(enemy->id));
	withGameStateReadLock([&] { NK2AI::NullkillerTestAccess::prepareState(*gateway->nullkiller); });
	const auto initialThreat = gateway->nullkiller->dangerHitMap->getTileThreat(start).fastestDanger;
	const auto refugeThreat = gateway->nullkiller->dangerHitMap->getTileThreat(position(20)).fastestDanger;
	ASSERT_EQ(initialThreat.turn, 0);
	ASSERT_GT(initialThreat.danger, hero->estimateHeroCombatValue());
	ASSERT_NE(refugeThreat.turn, 0);
	const auto escapeGoals = NK2AI::Goals::EscapeBehavior().decompose(gateway->nullkiller.get());
	ASSERT_EQ(escapeGoals.size(), 1u);
	const auto * escapeChain = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(escapeGoals.front().get());
	ASSERT_NE(escapeChain, nullptr);
	ASSERT_EQ(escapeChain->getHero(), hero);
	const auto escapeDestination = escapeChain->getPath().targetTile();
	ASSERT_NE(gateway->nullkiller->dangerHitMap->getTileThreat(escapeDestination).fastestDanger.turn, 0);
	withGameStateReadLock([&] { gateway->nullkiller->makeTurn(); });
	EXPECT_TRUE(std::ranges::any_of(client.appliedMoves, [&](const auto & move)
	{
		const auto & [from, to, layer] = move;
		return from == start && to == position(1) && layer == EPathfindingLayer::LAND;
	}));
	EXPECT_TRUE(std::ranges::any_of(client.appliedMoves, [&](const auto & move)
	{
		return std::get<1>(move) == escapeDestination;
	})) << "the server should apply the destination chosen by escape planning";
	EXPECT_NE(hero->visitablePos(), start);
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_EQ(enemy->visitablePos(), enemyPosition);
}

INSTANTIATE_TEST_SUITE_P(EdgesCornersAndLevels, ToroidalAIEscapeTest,
	::testing::Combine(::testing::Values(
		EscapeSeam{{0, 18, 0}, {-1, 0, 0}}, EscapeSeam{{35, 18, 0}, {1, 0, 0}},
		EscapeSeam{{18, 0, 0}, {0, -1, 0}}, EscapeSeam{{18, 35, 0}, {0, 1, 0}},
		EscapeSeam{{0, 0, 0}, {-1, -1, 0}}, EscapeSeam{{35, 35, 0}, {1, 1, 0}},
		EscapeSeam{{0, 35, 0}, {-1, 1, 0}}, EscapeSeam{{35, 0, 0}, {1, -1, 0}}),
		::testing::Values(0, 1)));

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
