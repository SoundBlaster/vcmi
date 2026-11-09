/*
 * HeroMovementTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/GameHandlerTestServer.h"
#include "../mock/TinyMapGameTest.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/modding/IdentifierResolver.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/spells/adventure/AdventureSpellMechanics.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/serializer/CMemorySerializer.h"
#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/CSaveFile.h"
#include "../../server/CGameHandler.h"

class HeroMovementTest : public TinyMapGameTest
{
protected:
	const Services * gameServices() override { return LIBRARY; }
	void checkToroidalSeamMovement(EMapFormat format);
	void checkToroidalSightRange(EMapFormat format);
	void checkBlockedToroidalStep(EMapFormat format);
};

void HeroMovementTest::checkToroidalSeamMovement(EMapFormat format)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(format);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 1, 0));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());

	const auto moveTo = [&](const int3 & tile)
	{
		if(hero->visitablePos() != tile)
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(tile), EMovementMode::MONOLITH, false, PlayerColor::NEUTRAL, EPathfindingLayer::LAND));
		ASSERT_EQ(hero->visitablePos(), tile);
		hero->setMovementPoints(3000);
	};
	const auto costFor = [&](const int3 & from, const int3 & to)
	{
		moveTo(from);
		const int before = hero->movementPointsRemaining();
		EXPECT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(to), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
		EXPECT_EQ(hero->visitablePos(), to);
		return before - hero->movementPointsRemaining();
	};
	const int straightCost = costFor(int3(10, 10, 0), int3(11, 10, 0));
	const int diagonalCost = costFor(int3(10, 10, 0), int3(11, 11, 0));
	ASSERT_GT(straightCost, 0);
	ASSERT_GT(diagonalCost, straightCost);

	const std::vector<std::pair<int3, int3>> steps = {
		{{0, 5, 0}, {35, 5, 0}}, {{35, 5, 0}, {0, 5, 0}},
		{{5, 0, 0}, {5, 35, 0}}, {{5, 35, 0}, {5, 0, 0}},
		{{0, 0, 0}, {35, 35, 0}}, {{35, 35, 0}, {0, 0, 0}},
		{{35, 0, 0}, {0, 35, 0}}, {{0, 35, 0}, {35, 0, 0}}
	};
	for(const auto & [from, to] : steps)
	{
		SCOPED_TRACE(from.toString() + " -> " + to.toString());
		moveTo(from);
		CPathsInfo paths(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
		CPathfinder pathfinder(*gameState(), config);
		pathfinder.calculatePaths();
		const auto * node = paths.getNode(to, EPathfindingLayer::LAND);
		ASSERT_TRUE(node->reachable());
		ASSERT_NE(node->theNodeBefore, nullptr);
		EXPECT_EQ(node->theNodeBefore->coord, from);
		EXPECT_EQ(costFor(from, to), from.x != to.x && from.y != to.y ? diagonalCost : straightCost);
	}
}

void HeroMovementTest::checkBlockedToroidalStep(EMapFormat format)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(format);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player).terrain(int3(35, 1, 0), TerrainId::ROCK);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 1, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), int3(0, 1, 0));
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
}

void HeroMovementTest::checkToroidalSightRange(EMapFormat format)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(format);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	setMapVisibility(player, false);

	FowTilesType tiles;
	gameState()->getTilesInRange(tiles, int3(0, 0, 0), 1, ETileVisibility::HIDDEN, player);
	EXPECT_TRUE(tiles.contains(int3(35, 0, 0)));
	EXPECT_TRUE(tiles.contains(int3(0, 35, 0)));
	EXPECT_TRUE(tiles.contains(int3(35, 35, 0)));
	EXPECT_FALSE(tiles.contains(int3(33, 0, 0)));
}

TEST_F(HeroMovementTest, pathfinderAndServerCrossToroidalSeams)
{
	checkToroidalSeamMovement(EMapFormat::SOD);
}

TEST_F(HeroMovementTest, pathfinderAndServerCrossToroidalSeamsRoE)
{
	checkToroidalSeamMovement(EMapFormat::ROE);
}

TEST_F(HeroMovementTest, sightRangeWrapsWithoutRevealingDistantTiles)
{
	checkToroidalSightRange(EMapFormat::SOD);
}

TEST_F(HeroMovementTest, sightRangeWrapsWithoutRevealingDistantTilesRoE)
{
	checkToroidalSightRange(EMapFormat::ROE);
}

TEST_F(HeroMovementTest, blockedToroidalStepIsRejected)
{
	checkBlockedToroidalStep(EMapFormat::SOD);
}

TEST_F(HeroMovementTest, blockedToroidalStepIsRejectedRoE)
{
	checkBlockedToroidalStep(EMapFormat::ROE);
}

TEST_F(HeroMovementTest, monstersGuardAcrossToroidalEdgesAndCorners)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player)
		.monster(int3(35, 5, 0), CreatureID(0), 10)
		.monster(int3(35, 35, 0), CreatureID(0), 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->calculateGuardingGreaturePositions();

	const auto guards = [&](const int3 & tile, const int3 & position)
	{
		const auto monsters = gameState()->guardingCreatures(tile);
		return std::ranges::any_of(monsters, [&](const auto * monster) { return monster->visitablePos() == position; });
	};
	EXPECT_TRUE(guards(int3(0, 5, 0), int3(35, 5, 0)));
	EXPECT_TRUE(guards(int3(0, 0, 0), int3(35, 35, 0)));
	EXPECT_FALSE(guards(int3(2, 5, 0), int3(35, 5, 0)));

	map()->toroidal = false;
	map()->calculateGuardingGreaturePositions();
	EXPECT_FALSE(guards(int3(0, 5, 0), int3(35, 5, 0)));
	EXPECT_FALSE(guards(int3(0, 0, 0), int3(35, 35, 0)));
}

TEST_F(HeroMovementTest, ObjectVisibilityUsesWrappedFootprintTiles)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	setMapVisibility(player, false);
	gameState()->getPlayerTeam(player)->fogOfWarMap[int3(35, 35, 0)] = 1;

	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "VV", "VV" ] })";
	JsonNode definition(text.data(), text.size(), "ObjectVisibilityTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	CGObjectInstance object(gameState().get());
	object.appearance = appearance;
	object.pos = int3(0, 0, 0);

	map()->toroidal = false;
	EXPECT_FALSE(object.isVisibleFor(player));
	map()->toroidal = true;
	EXPECT_TRUE(object.isVisibleFor(player));
	gameState()->getPlayerTeam(player)->fogOfWarMap[int3(35, 35, 0)] = 0;
	EXPECT_FALSE(object.isVisibleFor(player));
}

TEST_F(HeroMovementTest, crossedSeamAndFogSurviveGameStateReload)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	setMapVisibility(player, false);

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	const int3 destination(35, 1, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_EQ(hero->visitablePos(), destination);
	const auto heroID = hero->id;
	const int3 hiddenTile(15, 15, 0);
	ASSERT_TRUE(gameState()->isVisibleFor(destination, player));
	ASSERT_FALSE(gameState()->isVisibleFor(hiddenTile, player));

	CMemorySerializer serialized;
	gameState()->saveGame(serialized.oser);
	CMemorySerializer header;
	gameState()->saveHeader(header.oser);
	const auto savePath = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-seam-save-%%%%-%%%%-%%%%.vsgm1");
	struct TemporarySave
	{
		boost::filesystem::path path;
		~TemporarySave() { boost::filesystem::remove(path); }
	} temporarySave{savePath};
	CSaveFile save;
	save.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*gameState()).toBytes());
	save.addChunk(SaveFileFormat::CHUNK_HEADER, header.extractBuffer());
	save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, serialized.extractBuffer());
	save.write(savePath);
	CLoadFile loader(savePath);
	CMapHeader loadedHeader;
	StartInfo loadedStartInfo;
	auto headerChunk = loader.openChunk(SaveFileFormat::CHUNK_HEADER, nullptr);
	headerChunk->iser & loadedHeader;
	headerChunk->iser & loadedStartInfo;
	EXPECT_TRUE(headerChunk->isFullyRead());
	EXPECT_TRUE(loadedHeader.toroidal);
	auto restored = std::make_shared<CGameState>();
	restored->preInit(LIBRARY);
	auto gameChunk = loader.openChunk(SaveFileFormat::CHUNK_GAMESTATE, restored.get());
	restored->loadGame(gameChunk->iser);
	EXPECT_TRUE(gameChunk->isFullyRead());
	EXPECT_TRUE(restored->getMap().toroidal);
	ASSERT_NE(restored->getMap().getObject(heroID), nullptr);
	EXPECT_EQ(restored->getMap().getObject(heroID)->visitablePos(), destination);
	EXPECT_TRUE(restored->isVisibleFor(destination, player));
	EXPECT_FALSE(restored->isVisibleFor(hiddenTile, player));
}

/// Two towns that share an entrance tile, as found on some SoD maps. The pathfinder only considers the top town.
TEST_F(HeroMovementTest, sharedEntranceInteractsOnlyWithTopTown)
{
	const PlayerColor player(0);
	const int3 townPos(10, 10, 0);
	const int3 entrance(8, 10, 0);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(player)
		.town(townPos, FactionID::CASTLE, PlayerColor::NEUTRAL)
		.town(townPos, FactionID::CASTLE, player)
		.hero(int3(9, 11, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));

	auto towns = findAll<CGTownInstance>();
	ASSERT_EQ(towns.size(), 2);
	auto * neutralTown = towns[0]->getOwner() == player ? towns[1] : towns[0];
	auto * ownTown = towns[0]->getOwner() == player ? towns[0] : towns[1];
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);

	ASSERT_EQ(neutralTown->visitablePos(), entrance);
	ASSERT_EQ(ownTown->visitablePos(), entrance);
	ASSERT_EQ(map()->getTile(entrance).topVisitableObj(), ownTown->id);
	ASSERT_EQ(hero->visitablePos(), entrance + int3(0, 1, 0));

	if(!neutralTown->stacksCount())
	{
		ASSERT_TRUE(neutralTown->setCreature(SlotID(0), CreatureID(0), 10));
	}
	ASSERT_FALSE(neutralTown->passableFor(player));

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	ASSERT_TRUE(gameHandler.moveHero(hero->id, hero->convertFromVisitablePos(entrance), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));

	EXPECT_EQ(hero->visitablePos(), entrance);
	EXPECT_EQ(ownTown->getVisitingHero(), hero);
	EXPECT_EQ(neutralTown->getVisitingHero(), nullptr);
	EXPECT_EQ(neutralTown->getOwner(), PlayerColor::NEUTRAL);
	EXPECT_EQ(gameState()->getBattle(player), nullptr);
}

/// A hero starting on an entrance shared by two towns of its owner visits only the top town
TEST_F(HeroMovementTest, heroStartingOnSharedEntranceVisitsOnlyTopTown)
{
	const PlayerColor player(0);
	const int3 townPos(10, 10, 0);
	const int3 entrance(8, 10, 0);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(player)
		.town(townPos, FactionID::CASTLE, player)
		.town(townPos, FactionID::CASTLE, player)
		.hero(int3(9, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), entrance);

	const auto & tile = map()->getTile(entrance);
	const auto * topTown = gameState()->getTown(tile.objectVisitedBy(hero->id));
	ASSERT_NE(topTown, nullptr);

	EXPECT_EQ(hero->getVisitedTown(), topTown);
	for(const auto * town : findAll<CGTownInstance>())
		EXPECT_EQ(town->getVisitingHero(), town == topTown ? hero : nullptr);
}

/// A hero stays where he stood when visiting another hero. Water Walk lasts only one day, so a visit
/// made from water would leave the hero stranded there.
TEST_F(HeroMovementTest, noVisitFromWaterToHeroStandingOnObject)
{
	const PlayerColor player(0);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(player)
		.hero(int3(10, 10, 0), HeroTypeID(0), player)
		.hero(int3(14, 10, 0), HeroTypeID(1), player)
		.keymaster(int3(13, 10, 0), 0);
	startWithMap(std::move(builder));
	revealMap(player);

	CGHeroInstance * walker = nullptr;
	CGHeroInstance * islander = nullptr;
	for(auto * hero : findAll<CGHeroInstance>())
		(hero->getHeroTypeID() == HeroTypeID(0) ? walker : islander) = hero;
	ASSERT_NE(walker, nullptr);
	ASSERT_NE(islander, nullptr);
	// a hero standing on a passable object, as on a Garden of Revelation, leaves two visitable objects on the tile
	ASSERT_EQ(map()->getTile(islander->visitablePos()).visitableObjects.size(), 2);

	// islander stands on a single land tile surrounded by water
	const int3 island = islander->visitablePos();
	for(int x = walker->visitablePos().x + 1; x <= island.x + 1; ++x)
		for(int y = island.y - 1; y <= island.y + 1; ++y)
			if(int3(x, y, 0) != island)
				map()->getTile(int3(x, y, 0)).terrainType = ETerrainId::WATER;

	walker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	walker->setMovementPoints(2000);

	CPathsInfo paths(gameState()->getMapSize(), walker);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), walker);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();

	ASSERT_TRUE(paths.getNode(island + int3(0, 1, 0), EPathfindingLayer::WATER)->reachable());
	EXPECT_FALSE(paths.getNode(island, EPathfindingLayer::LAND)->reachable());
}

TEST_F(HeroMovementTest, cursedGroundBlocksHighLevelAdventureSpells)
{
	const PlayerColor player(0);
	const int3 cursedTile(10, 10, 0);
	const SpellID dimensionDoor(SpellID::decode("core:dimensionDoor"));
	const SpellID viewAir(SpellID::decode("core:viewAir"));

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(player)
		.terrainOverlay(cursedTile, Obj::CURSED_GROUND1)
		.hero(cursedTile + int3(1, 0, 0), HeroTypeID(0), player)
		.heroSpells({dimensionDoor, viewAir})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}});
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), cursedTile);
	hero->mana = 100;

	auto canCast = [&](SpellID spell)
	{
		spells::detail::ProblemImpl problem;
		return spell.toSpell()->getAdventureMechanics().canBeCast(problem, gameState().get(), hero);
	};
	auto moveHero = [&](const int3 & destination)
	{
		TryMoveHero move;
		move.id = hero->id;
		move.result = TryMoveHero::SUCCESS;
		move.start = hero->pos;
		move.end = hero->convertFromVisitablePos(destination);
		move.movePoints = hero->movementPointsRemaining();
		gameState()->apply(move);
	};

	EXPECT_FALSE(canCast(dimensionDoor));
	EXPECT_TRUE(canCast(viewAir));

	moveHero(int3(20, 20, 0));
	EXPECT_TRUE(canCast(dimensionDoor));

	moveHero(cursedTile);
	EXPECT_FALSE(canCast(dimensionDoor));
}
