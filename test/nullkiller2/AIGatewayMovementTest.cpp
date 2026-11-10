/*
 * AIGatewayMovementTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"
#include "nullkiller2/ScriptedMovementClient.h"

#include "lib/callback/CCallback.h"
#include "AI/Nullkiller2/Pathfinding/Actions/TownPortalAction.h"
#include "AI/Nullkiller2/Pathfinding/Actions/DimensionDoorAction.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/mapObjects/CGResource.h"
#include "lib/mapping/TerrainTile.h"

namespace NK2AI
{
const CGHeroInstance * getNearestHero(const Nullkiller * aiNk, const CGTownInstance * town);
}

namespace
{
const PlayerColor PLAYER(0);

class Nullkiller2_AIGateway : public NullkillerTest
{
protected:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PLAYER)
			.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID::ARCHER, 1}})
			.terrain(UNREACHABLE_TILE, TerrainId::ROCK);

		startWithMap(std::move(builder));
		revealMap(PLAYER);

		hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);
		hero->setMovementPoints(2000);

		client = std::make_unique<ScriptedMovementClient>(gameState());
		callback = makeCallback(PLAYER, client.get());
		gateway = makeGateway(callback);
		client->notify(*gateway);
	}

	void startBlockedObjectGame(const int3 & origin, const int3 & destination, bool toroidal)
	{
		const PlayerColor enemy(1);
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false).playerActive(PLAYER).playerActive(enemy)
			.hero(origin + int3(1, 0, 0), HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID::ARCHER, 1}})
			.resource(destination, GameResID::GOLD, 10);
		startWithMap(std::move(builder));
		map()->toroidal = toroidal;
		revealMap(PLAYER);
		hero = findHeroByOwner(PLAYER);
		auto * resource = findFirst<CGResource>();
		ASSERT_NE(hero, nullptr);
		ASSERT_NE(resource, nullptr);
		ASSERT_EQ(hero->visitablePos(), origin);
		ASSERT_EQ(resource->visitablePos(), destination);
		resource->tempOwner = enemy;
		gameState()->blockedContacts.insert({PLAYER, enemy});
		hero->setMovementPoints(2000);
		client = std::make_unique<ScriptedMovementClient>(gameState());
		callback = makeCallback(PLAYER, client.get());
		gateway = makeGateway(callback);
		client->notify(*gateway);
	}

	NK2AI::HeroMovementResult moveHeroTo(const int3 & destination)
	{
		// AI turns normally enter movement with the game-state read lock held;
		// waitTillFree temporarily releases and reacquires that lock.
		std::shared_lock lock(CGameState::mutex);
		return gateway->moveHeroToTile(destination, NK2AI::HeroPtr(hero, callback.get()));
	}

	int3 distantDestination() const
	{
		return hero->visitablePos() + int3(4, 0, 0);
	}

	const int3 UNREACHABLE_TILE{20, 20, 0};

	std::unique_ptr<ScriptedMovementClient> client;
	std::shared_ptr<CCallback> callback;
	std::unique_ptr<NK2AI::AIGateway> gateway;
	CGHeroInstance * hero = nullptr;
};
}

TEST_F(Nullkiller2_AIGateway, stopsRouteWhenMoveConsumesLastMovementPoint)
{
	startGame();
	client->drainMovement(0);

	const auto destination = distantDestination();

	EXPECT_EQ(moveHeroTo(destination), NK2AI::HeroMovementResult::PROGRESSED);
	EXPECT_EQ(client->movementRequests, 1) << "a hero without movement points must not request further steps";
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_NE(hero->visitablePos(), destination);
}

TEST_F(Nullkiller2_AIGateway, stopsRouteWhenMoveLeavesFewerMovementPointsThanPlanned)
{
	startGame();
	client->drainMovement(1);

	EXPECT_EQ(moveHeroTo(distantDestination()), NK2AI::HeroMovementResult::PROGRESSED);
	EXPECT_EQ(client->movementRequests, 1) << "the server would reject a step the hero can no longer afford";
	EXPECT_EQ(hero->movementPointsRemaining(), 1);
}

TEST_F(Nullkiller2_AIGateway, stopsRouteWhenStepBecomesMoreExpensiveThanPlanned)
{
	const HeroTypeID fortressHero(112);
	const CreatureID gnoll(98);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PLAYER)
		.hero({5, 5, 0}, fortressHero, PLAYER)
		.heroGarrison({{gnoll, 1}});
	for(int x = 4; x < 10; ++x)
		builder.terrain({x, 5, 0}, TerrainId::SWAMP);

	startWithMap(std::move(builder));
	revealMap(PLAYER);
	hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);

	// native swamp costs 100 per step, so the first step leaves the planned 160 points
	hero->setMovementPoints(260);
	client = std::make_unique<ScriptedMovementClient>(gameState());
	client->drainMovement(160);
	// non-native unit given by a map event makes swamp cost 175
	client->afterMovement = [this]() { hero->addToSlot(SlotID(1), CreatureID::ARCHER, 1); };
	callback = makeCallback(PLAYER, client.get());
	gateway = makeGateway(callback);
	client->notify(*gateway);

	EXPECT_EQ(moveHeroTo(distantDestination()), NK2AI::HeroMovementResult::PROGRESSED);
	EXPECT_EQ(client->movementRequests, 1) << "the server would reject a step that costs more than the hero has";
}

TEST_F(Nullkiller2_AIGateway, throwsAndStopsRouteAfterFailedMovePacket)
{
	startGame();
	client->failMovement();

	const auto startPosition = hero->visitablePos();

	EXPECT_THROW(moveHeroTo(distantDestination()), NK2AI::cannotFulfillGoalException);
	EXPECT_EQ(client->movementRequests, 1) << "a rejected step must end the route";
	EXPECT_EQ(hero->visitablePos(), startPosition);
}

TEST_F(Nullkiller2_AIGateway, reportsUnreachableTileAsBlocked)
{
	startGame();

	EXPECT_EQ(moveHeroTo(UNREACHABLE_TILE), NK2AI::HeroMovementResult::BLOCKED);
	EXPECT_EQ(client->movementRequests, 0);
}

TEST_F(Nullkiller2_AIGateway, choosesDirectRouteAcrossToroidalSeam)
{
	startGame();
	map()->toroidal = true;
	const std::vector<std::pair<int3, int3>> steps = {
		{{0, 5, 0}, {35, 5, 0}}, {{35, 5, 0}, {0, 5, 0}},
		{{5, 0, 0}, {5, 35, 0}}, {{5, 35, 0}, {5, 0, 0}},
		{{0, 0, 0}, {35, 35, 0}}, {{35, 35, 0}, {0, 0, 0}},
		{{35, 0, 0}, {0, 35, 0}}, {{0, 35, 0}, {35, 0, 0}}
	};
	for(const auto & [start, destination] : steps)
	{
		SCOPED_TRACE(start.toString() + " -> " + destination.toString());
		TryMoveHero placement;
		placement.id = hero->id;
		placement.start = hero->pos;
		placement.end = hero->convertFromVisitablePos(start);
		placement.movePoints = 2000;
		placement.result = TryMoveHero::TELEPORTATION;
		gameState()->apply(placement);
		gateway->invalidatePaths();
		gateway->heroMoved(placement, false);
		ASSERT_EQ(hero->visitablePos(), start);
		const auto paths = withGameStateReadLock([&]
		{
			preparePlanning(*gateway);
			return gateway->nullkiller->pathfinder->getPathInfo(destination);
		});
		ASSERT_TRUE(std::ranges::any_of(paths, [&](const auto & path)
		{
			return path.targetHero == hero && !path.nodes.empty() && path.turn() == 0 && path.firstTileToGet() == destination;
		}));
		const int requestsBefore = client->movementRequests;
		EXPECT_EQ(moveHeroTo(destination), NK2AI::HeroMovementResult::COMPLETE);
		EXPECT_EQ(client->movementRequests, requestsBefore + 1);
		EXPECT_TRUE(client->requested(destination));
		EXPECT_EQ(hero->visitablePos(), destination);
	}
}

TEST_F(Nullkiller2_AIGateway, DoesNotRequestBlockedEnemyInteractionAcrossSeam)
{
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	startBlockedObjectGame(origin, destination, true);
	ASSERT_NE(hero, nullptr);
	EXPECT_THROW(moveHeroTo(destination), NK2AI::cannotFulfillGoalException);
	EXPECT_EQ(client->movementRequests, 0);
	EXPECT_EQ(hero->visitablePos(), origin);
}

TEST_F(Nullkiller2_AIGateway, DoesNotRequestBlockedEnemyInteractionOnBoundedMap)
{
	const int3 origin(4, 5, 0);
	const int3 destination(5, 5, 0);
	startBlockedObjectGame(origin, destination, false);
	ASSERT_NE(hero, nullptr);
	EXPECT_THROW(moveHeroTo(destination), NK2AI::cannotFulfillGoalException);
	EXPECT_EQ(client->movementRequests, 0);
	EXPECT_EQ(hero->visitablePos(), origin);
}

TEST_F(Nullkiller2_AIGateway, RequestsEnemyInteractionWhenContactIsAllowed)
{
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	startBlockedObjectGame(origin, destination, true);
	ASSERT_NE(hero, nullptr);
	gameState()->blockedContacts.clear();
	EXPECT_EQ(moveHeroTo(destination), NK2AI::HeroMovementResult::COMPLETE);
	EXPECT_EQ(client->movementRequests, 1);
	EXPECT_TRUE(client->requested(destination));
}

TEST_F(Nullkiller2_AIGateway, plansUnskilledTownPortalToNearestTownAcrossSeam)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PLAYER)
		.hero({1, 10, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.heroPrimary(0, 0, 20, 20)
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
		.heroSpells({SpellID(SpellID::decode("core:townPortal"))})
		.town({9, 10, 0}, FactionID(0), PLAYER)
		.town({35, 10, 0}, FactionID(0), PLAYER);
	startWithMap(std::move(builder));
	revealMap(PLAYER);
	hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->mana = 200;
	hero->setMovementPoints(300);
	const auto towns = findAll<CGTownInstance>();
	auto across = std::ranges::find_if(towns, [](const auto * town) { return town->anchorPos().x == 35; });
	auto interior = std::ranges::find_if(towns, [](const auto * town) { return town->anchorPos().x == 9; });
	ASSERT_NE(across, towns.end());
	ASSERT_NE(interior, towns.end());
	for(const bool toroidal : {false, true})
	{
		SCOPED_TRACE(toroidal ? "toroidal" : "bounded");
		map()->toroidal = toroidal;
		client = std::make_unique<ScriptedMovementClient>(gameState());
		callback = makeCallback(PLAYER, client.get());
		gateway = makeGateway(callback);
		client->notify(*gateway);
		const auto * expected = toroidal ? *across : *interior;
		const auto paths = withGameStateReadLock([&]
		{
			preparePlanning(*gateway);
			return gateway->nullkiller->pathfinder->getPathInfo(expected->visitablePos());
		});
		EXPECT_TRUE(std::ranges::any_of(paths, [](const auto & path)
		{
			return path.turn() == 0 && std::ranges::any_of(path.nodes, [](const auto & node)
			{
				return dynamic_cast<const NK2AI::AIPathfinding::TownPortalAction *>(node.specialAction.get()) != nullptr;
			});
		})) << "AI should consider the same nearest town as the game spell";
	}
}

TEST_F(Nullkiller2_AIGateway, plansDimensionDoorLandingAcrossToroidalSeam)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PLAYER)
		.hero({1, 10, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.heroPrimary(10, 10, 10, 50)
		.heroSecondarySkills({{SecondarySkill::AIR_MAGIC, 3}})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
		.heroSpells({SpellID(8)});
	startWithMap(std::move(builder));
	revealMap(PLAYER);
	hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->mana = 200;
	hero->setMovementPoints(300);
	for(int x : {35, 0, 1})
		for(int y = 9; y <= 11; ++y)
			if(x != 0 || y != 10)
				map()->getTile(int3(x, y, 0)).terrainType = TerrainId::ROCK;
	for(const bool toroidal : {false, true})
	{
		SCOPED_TRACE(toroidal ? "toroidal" : "bounded");
		map()->toroidal = toroidal;
		client = std::make_unique<ScriptedMovementClient>(gameState());
		callback = makeCallback(PLAYER, client.get());
		gateway = makeGateway(callback);
		client->notify(*gateway);
		const auto paths = withGameStateReadLock([&]
		{
			NK2AI::HeroMap<NK2AI::HeroRole> heroes;
			heroes[hero] = NK2AI::HeroRole::MAIN;
			NK2AI::PathfinderSettings settings;
			settings.useHeroChain = false;
			settings.useDimensionDoor = true;
			gateway->nullkiller->pathfinder->updatePaths(heroes, settings);
			return gateway->nullkiller->pathfinder->getPathInfo(int3(30, 10, 0));
		});
		const bool found = std::ranges::any_of(paths, [](const auto & path)
		{
			return path.turn() == 0 && std::ranges::any_of(path.nodes, [](const auto & node)
			{
				return dynamic_cast<const NK2AI::AIPathfinding::DimensionDoorAction *>(node.specialAction.get()) != nullptr;
			});
		});
		EXPECT_EQ(found, toroidal) << "AI should enumerate the same landing the spell permits";
	}
}

TEST_F(Nullkiller2_AIGateway, startupRecognizesHeroNearTownAcrossSeam)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PLAYER)
		.hero({6, 12, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.town({2, 11, 0}, FactionID(0), PLAYER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(PLAYER);
	hero = findHeroByOwner(PLAYER);
	const auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(town, nullptr);
	TryMoveHero placement;
	placement.id = hero->id;
	placement.start = hero->pos;
	placement.end = hero->convertFromVisitablePos(int3(35, 12, 0));
	placement.movePoints = 2000;
	placement.result = TryMoveHero::TELEPORTATION;
	gameState()->apply(placement);
	for(const bool toroidal : {false, true})
	{
		SCOPED_TRACE(toroidal ? "toroidal" : "bounded");
		map()->setToroidalAndRebuild(toroidal);
		client = std::make_unique<ScriptedMovementClient>(gameState());
		callback = makeCallback(PLAYER, client.get());
		gateway = makeGateway(callback);
		client->notify(*gateway);
		const auto * nearest = withGameStateReadLock([&]
		{
			preparePlanning(*gateway);
			return NK2AI::getNearestHero(gateway->nullkiller.get(), town);
		});
		EXPECT_EQ(nearest, toroidal ? hero : nullptr);
	}
}
