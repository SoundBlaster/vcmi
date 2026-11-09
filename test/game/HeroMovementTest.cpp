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
#include "../mock/ToroidalDemoMap.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGResource.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/mapping/MapTopology.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormatH3M.h"
#include "../../lib/mapping/MapFormatJson.h"
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

class ToroidalDemoGameTest : public TinyMapGameTest
{
protected:
	const Services * gameServices() override { return LIBRARY; }
	void mapLoaded(CMap * loaded) override
	{
		TinyMapGameTest::mapLoaded(loaded);
		loaded->setToroidalAndRebuild(true);
	}
};

TEST_F(ToroidalDemoGameTest, HeroCrossesCornerAndBothEdges)
{
	const PlayerColor player(0);
	startWithMap(makeToroidalDemoMap());
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	hero->setMovementPoints(3000);
	const auto goldBefore = gameState()->players.at(player).resources[GameResID::GOLD];
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 35, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_GT(gameState()->players.at(player).resources[GameResID::GOLD], goldBefore);
	// The blocking resource is collected from the adjacent tile; the hero enters on the next step.
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 35, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), int3(35, 35, 0));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 35, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), int3(0, 0, 0));
}

TEST(ToroidalDemoVmapTest, SavedMapStartsAndSupportsCornerVisit)
{
	auto source = makeToroidalDemoMap().build();
	CMemoryBuffer legacyBytes;
	legacyBytes.write(source.data(), static_cast<si64>(source.size()));
	legacyBytes.seek(0);
	EditorCallback editor(nullptr);
	CMapLoaderH3M legacyLoader("ToroidSeamLaboratory", "core", "ASCII", &legacyBytes);
	auto legacy = legacyLoader.loadMap(&editor);
	editor.setMap(legacy.get());
	legacy->setToroidalAndRebuild(true);
	CMemoryBuffer archive;
	{
		CMapSaverJson saver(&archive);
		saver.saveMap(legacy);
	}
	class DemoMapService : public CMapService
	{
	public:
		explicit DemoMapService(const std::vector<ui8> & data) : data(data) {}
		std::unique_ptr<CMap> loadMap(const ResourcePath &, IGameInfoCallback * callback) const override
		{
			return CMapService::loadMap(data.data(), static_cast<int>(data.size()), "ToroidSeamLaboratory", "core", "ASCII", callback);
		}
		std::unique_ptr<CMapHeader> loadMapHeader(const ResourcePath &, bool = false) const override
		{
			return CMapService::loadMapHeader(data.data(), static_cast<int>(data.size()), "ToroidSeamLaboratory", "core", "ASCII");
		}
	private:
		const std::vector<ui8> & data;
	};
	DemoMapService service(archive.getBuffer());
	auto header = service.loadMapHeader(ResourcePath("memory"));
	ASSERT_TRUE(header->toroidal);
	StartInfo info;
	info.mapname = "memory";
	info.difficulty = static_cast<ui8>(EMapDifficulty::EASY);
	info.mode = EStartMode::NEW_GAME;
	for(int index = 0; index < static_cast<int>(header->players.size()); ++index)
	{
		const auto & playerInfo = header->players[index];
		if(!(playerInfo.canHumanPlay || playerInfo.canComputerPlay))
			continue;
		auto & settings = info.playerInfos[PlayerColor(index)];
		settings.color = PlayerColor(index);
		settings.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(index));
		settings.name = "Player";
		settings.castle = playerInfo.defaultCastle();
		settings.hero = playerInfo.defaultHero();
		settings.bonus = PlayerStartingBonus::GOLD;
	}
	auto state = std::make_shared<CGameState>();
	state->preInit(LIBRARY);
	GameRandomizer randomizer(*state);
	Load::ProgressAccumulator progress;
	state->init(&service, &info, randomizer, progress, false);
	ASSERT_TRUE(state->getMap().toroidal);
	CGHeroInstance * hero = nullptr;
	for(const auto & object : state->getMap().objects)
		if(auto * candidate = dynamic_cast<CGHeroInstance *>(object.get()); candidate && candidate->getOwner() == PlayerColor(0))
			hero = candidate;
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	hero->setMovementPoints(3000);
	CPathsInfo paths(state->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *state, hero);
	CPathfinder pathfinder(*state, config);
	pathfinder.calculatePaths();
	const auto * corner = paths.getNode(int3(35, 35, 0), EPathfindingLayer::LAND);
	ASSERT_TRUE(corner->reachable());
	ASSERT_NE(corner->theNodeBefore, nullptr);
	EXPECT_EQ(corner->theNodeBefore->coord, hero->visitablePos());
	const auto goldBefore = state->players.at(PlayerColor(0)).resources[GameResID::GOLD];
	GameHandlerTestServer server(state, PlayerColor(0));
	CGameHandler handler(server, state);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 35, 0)), EMovementMode::STANDARD, false, PlayerColor(0), EPathfindingLayer::LAND));
	EXPECT_GT(state->players.at(PlayerColor(0)).resources[GameResID::GOLD], goldBefore);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 35, 0)), EMovementMode::STANDARD, false, PlayerColor(0), EPathfindingLayer::LAND));
	ASSERT_EQ(hero->visitablePos(), int3(35, 35, 0));
	const auto heroID = hero->id;
	const auto goldAfter = state->players.at(PlayerColor(0)).resources[GameResID::GOLD];
	CMemorySerializer stateBytes, headerBytes;
	state->saveGame(stateBytes.oser);
	state->saveHeader(headerBytes.oser);
	const auto path = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-demo-seam-%%%%-%%%%.vsgm1");
	struct TemporarySave
	{
		boost::filesystem::path path;
		~TemporarySave() { boost::filesystem::remove(path); }
	} temporarySave{path};
	CSaveFile save;
	save.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*state).toBytes());
	save.addChunk(SaveFileFormat::CHUNK_HEADER, headerBytes.extractBuffer());
	save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, stateBytes.extractBuffer());
	save.write(path);
	CLoadFile load(path);
	auto restored = std::make_shared<CGameState>();
	restored->preInit(LIBRARY);
	auto chunk = load.openChunk(SaveFileFormat::CHUNK_GAMESTATE, restored.get());
	restored->loadGame(chunk->iser);
	ASSERT_TRUE(chunk->isFullyRead());
	ASSERT_TRUE(restored->getMap().toroidal);
	auto * restoredHero = dynamic_cast<CGHeroInstance *>(restored->getMap().getObject(heroID));
	ASSERT_NE(restoredHero, nullptr);
	ASSERT_EQ(restoredHero->visitablePos(), int3(35, 35, 0));
	EXPECT_EQ(restored->players.at(PlayerColor(0)).resources[GameResID::GOLD], goldAfter);
	GameHandlerTestServer restoredServer(restored, PlayerColor(0));
	CGameHandler restoredHandler(restoredServer, restored);
	ASSERT_TRUE(restoredHandler.moveHero(heroID, restoredHero->convertFromVisitablePos(int3(0, 35, 0)), EMovementMode::STANDARD, false, PlayerColor(0), EPathfindingLayer::LAND));
	EXPECT_EQ(restoredHero->visitablePos(), int3(0, 35, 0));
}

class GrailTopologyTest : public TinyMapGameTest
{
protected:
	bool toroidal = true;
	bool unspecified = false;

	const Services * gameServices() override { return LIBRARY; }

	void mapLoaded(CMap * loaded) override
	{
		TinyMapGameTest::mapLoaded(loaded);
		loaded->toroidal = toroidal;
		loaded->grailPos = unspecified ? int3(-1, -1, -1) : int3(0, 10, 0);
		loaded->grailRadius = unspecified ? 0 : 1;
		for(int x = 0; x < loaded->width; ++x)
			for(int y = 0; y < loaded->height; ++y)
				loaded->getTile(int3(x, y, 0)).terrainType = TerrainId::ROCK;
		loaded->getTile(int3(35, 10, 0)).terrainType = TerrainId::GRASS;
	}

	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false).playerActive(PlayerColor(0)).hero(int3(6, 5, 0), HeroTypeID(0), PlayerColor(0));
		startWithMap(std::move(builder));
	}
};

TEST_F(GrailTopologyTest, boundedMapKeepsBorderExclusion)
{
	toroidal = false;
	startGame();
	EXPECT_EQ(map()->grailPos, int3(0, 10, 0));
}

TEST_F(GrailTopologyTest, radiusIncludesEligibleTileAcrossSeam)
{
	startGame();
	EXPECT_EQ(map()->grailPos, int3(35, 10, 0));
}

TEST_F(GrailTopologyTest, unspecifiedGrailCanBePlacedAtEdge)
{
	unspecified = true;
	startGame();
	EXPECT_EQ(map()->grailPos, int3(35, 10, 0));
}

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

	struct Step { int3 from; int3 to; int direction; };
	const std::vector<Step> steps = {
		{{0, 5, 0}, {35, 5, 0}, 8}, {{35, 5, 0}, {0, 5, 0}, 4},
		{{5, 0, 0}, {5, 35, 0}, 2}, {{5, 35, 0}, {5, 0, 0}, 6},
		{{0, 0, 0}, {35, 35, 0}, 1}, {{35, 35, 0}, {0, 0, 0}, 5},
		{{35, 0, 0}, {0, 35, 0}, 3}, {{0, 35, 0}, {35, 0, 0}, 7}
	};
	for(const auto & [from, to, direction] : steps)
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
		EXPECT_EQ(hero->moveDir, direction);
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

TEST_F(HeroMovementTest, removingCornerGuardRefreshesWrappedGuardCache)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player)
		.monster(int3(35, 35, 0), CreatureID(0), 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->calculateGuardingGreaturePositions();

	const int3 monsterPosition(35, 35, 0);
	const auto guardID = map()->getTile(monsterPosition).topVisitableObj();
	const auto * guard = map()->getObject(guardID);
	ASSERT_NE(guard, nullptr);
	for(const auto & tile : {int3(0, 0, 0), int3(0, 35, 0), int3(35, 0, 0)})
		EXPECT_EQ(gameState()->guardingCreaturePosition(tile), monsterPosition);

	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.removeObject(guard, player));
	for(const auto & tile : {int3(0, 0, 0), int3(0, 35, 0), int3(35, 0, 0)})
		EXPECT_EQ(gameState()->guardingCreaturePosition(tile), int3(-1, -1, -1));
}

TEST_F(HeroMovementTest, visionsRangeUsesToroidalDistance)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(1, 0, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	const BonusSubtypeID subtype(BonusCustomSubtype(0));
	CGObjectInstance target(gameState().get());
	map()->toroidal = true;
	target.pos = int3(35, 0, 0);
	EXPECT_FALSE(hero->hasVisions(&target, subtype));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::VISIONS, BonusSource::OTHER, 1, BonusSourceID(), subtype));

	map()->toroidal = false;
	EXPECT_FALSE(hero->hasVisions(&target, subtype));
	target.pos = int3(1, 0, 0);
	EXPECT_TRUE(hero->hasVisions(&target, subtype));
	map()->toroidal = true;
	target.pos = int3(35, 0, 0);
	EXPECT_TRUE(hero->hasVisions(&target, subtype));
	target.pos = int3(35, 35, 0);
	EXPECT_TRUE(hero->hasVisions(&target, subtype));
	target.pos = int3(30, 30, 0);
	EXPECT_FALSE(hero->hasVisions(&target, subtype));
	target.pos = int3(35, 35, 1);
	EXPECT_FALSE(hero->hasVisions(&target, subtype));
}

TEST_F(HeroMovementTest, subterraneanGatePairsWithNearestAcrossToroidalSeam)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "A" ] })";
	JsonNode definition(text.data(), text.size(), "GatePairingTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	const auto addGate = [&](const int3 & pos, const std::string & name)
	{
		auto gate = std::make_shared<CGSubterraneanGate>(gameState().get());
		gate->ID = Obj::SUBTERRANEAN_GATE;
		gate->pos = pos;
		gate->instanceName = name;
		gate->appearance = appearance;
		map()->addNewObject(gate);
		return gate;
	};
	auto surface = addGate(int3(0, 5, 0), "surface_gate");
	auto interior = addGate(int3(3, 5, 1), "interior_gate");
	auto across = addGate(int3(35, 5, 1), "across_gate");

	map()->toroidal = false;
	CGSubterraneanGate::postInit(gameState().get());
	EXPECT_EQ(surface->channel, interior->channel);
	EXPECT_NE(surface->channel, across->channel);
	map()->teleportChannels.clear();
	for(const auto & gate : {surface, interior, across})
		gate->channel = TeleportChannelID();
	map()->toroidal = true;
	CGSubterraneanGate::postInit(gameState().get());
	EXPECT_EQ(surface->channel, across->channel);
	EXPECT_NE(surface->channel, interior->channel);
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

TEST_F(HeroMovementTest, SpecialTerrainCoversOppositeCornerOnToroidalMap)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	class SyntheticTerrainPatch : public CGTerrainPatch
	{
	public:
		using CGTerrainPatch::CGTerrainPatch;
		BattleField getBattlefield() const override { return BattleField(0); }
	};
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "VV", "VV" ] })";
	JsonNode definition(text.data(), text.size(), "SeamTerrainPatchTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto patch = std::make_shared<SyntheticTerrainPatch>(gameState().get());
	patch->appearance = appearance;
	patch->pos = int3(0, 0, 0);
	patch->instanceName = "seam_terrain_patch";
	map()->addNewObject(patch);
	EXPECT_EQ(gameState()->getSpecialTerrainAt(int3(0, 0, 0)), BattleField(0));
	EXPECT_EQ(gameState()->getSpecialTerrainAt(int3(35, 35, 0)), BattleField::NONE);
	map()->toroidal = true;
	EXPECT_EQ(gameState()->getSpecialTerrainAt(int3(35, 35, 0)), BattleField(0));
	EXPECT_EQ(gameState()->getSpecialTerrainAt(int3(35, 35, 1)), BattleField::NONE);
}

TEST_F(HeroMovementTest, ObjectTileQueriesFindFootprintAcrossToroidalCorner)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "AA", "AA" ] })";
	JsonNode definition(text.data(), text.size(), "ObjectTileQueryTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	CGObjectInstance object(gameState().get());
	object.appearance = appearance;
	object.pos = int3(0, 0, 0);
	const int3 local(-1, -1, 0);
	const int3 across(35, 35, 0);
	ASSERT_TRUE(object.visitableAt(local));
	ASSERT_TRUE(object.blockingAt(local));
	ASSERT_TRUE(object.coveringAt(local));
	EXPECT_FALSE(object.visitableAt(across));
	EXPECT_FALSE(object.blockingAt(across));
	EXPECT_FALSE(object.coveringAt(across));
	EXPECT_EQ(object.getBlockedPosOnMap(map()->getTopology()), (std::set<int3>{int3(0, 0, 0)}));
	map()->toroidal = true;
	EXPECT_EQ(object.getBlockedPosOnMap(map()->getTopology()), (std::set<int3>{int3(0, 0, 0), int3(35, 0, 0), int3(0, 35, 0), int3(35, 35, 0)}));
	EXPECT_TRUE(object.visitableAt(across));
	EXPECT_TRUE(object.blockingAt(across));
	EXPECT_TRUE(object.coveringAt(across));
	EXPECT_EQ(object.drawLayerAt(across), object.drawLayerAt(local));
	EXPECT_FALSE(object.visitableAt(int3(34, 35, 0)));
	EXPECT_FALSE(object.coveringAt(int3(35, 35, 1)));
	EXPECT_FALSE(object.visitableAt(int3(36, 35, 0)));
	EXPECT_FALSE(object.blockingAt(int3(-1, -1, -1)));
	object.cb = nullptr;
	EXPECT_FALSE(object.coveringAt(across));
	EXPECT_TRUE(object.coveringAt(local));
	EditorCallback unboundEditor(nullptr);
	object.cb = &unboundEditor;
	EXPECT_NO_THROW(EXPECT_FALSE(object.coveringAt(across)));
	unboundEditor.setMap(map());
	EXPECT_TRUE(object.coveringAt(across));
}

TEST_F(HeroMovementTest, SeamSpanningBlockerPreventsMovementOnBothSides)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(35, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(34, 10, 0));
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "BB" ] })";
	JsonNode definition(text.data(), text.size(), "SeamBlockerTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto obstacle = std::make_shared<CGObjectInstance>(gameState().get());
	obstacle->pos = int3(0, 10, 0);
	obstacle->instanceName = "synthetic_seam_blocker";
	obstacle->appearance = appearance;
	map()->addNewObject(obstacle);
	ASSERT_TRUE(map()->getTile(int3(0, 10, 0)).blocked());
	ASSERT_TRUE(map()->getTile(int3(35, 10, 0)).blocked());
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(int3(35, 10, 0), EPathfindingLayer::LAND)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 10, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), int3(34, 10, 0));
	TryMoveHero placement;
	placement.id = hero->id;
	placement.start = hero->pos;
	placement.end = hero->convertFromVisitablePos(int3(1, 10, 0));
	placement.movePoints = 3000;
	placement.result = TryMoveHero::TELEPORTATION;
	gameState()->apply(placement);
	ASSERT_EQ(hero->visitablePos(), int3(1, 10, 0));
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 10, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), int3(1, 10, 0));
}

TEST_F(HeroMovementTest, FlyingAbilityDoesNotPermitLandMovementIntoBlockedTile)
{
	const PlayerColor player(0);
	const int3 origin(0, 10, 0);
	const int3 destination(35, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "B" ] })";
	JsonNode definition(text.data(), text.size(), "SeamBlockerTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto obstacle = std::make_shared<CGObjectInstance>(gameState().get());
	obstacle->pos = destination;
	obstacle->instanceName = "synthetic_blocker";
	obstacle->appearance = appearance;
	map()->addNewObject(obstacle);
	ASSERT_TRUE(map()->getTile(destination).blocked());
	ASSERT_FALSE(map()->getTile(destination).visitable());
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(destination, EPathfindingLayer::LAND)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	EXPECT_EQ(hero->visitablePos(), destination);
}

TEST_F(HeroMovementTest, CollectsResourceWithVisitableTileAcrossSeam)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(35, 10, 0), HeroTypeID(0), player)
		.resource(int3(15, 15, 0), GameResID::GOLD, 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(34, 10, 0));
	const std::string text = R"({ "animation": "test.def", "visitableFrom": ["+++", "+++", "+++"], "mask": [ "A0" ] })";
	JsonNode definition(text.data(), text.size(), "SeamResourceTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	map()->hideObject(resource);
	resource->pos = int3(0, 10, 0);
	resource->appearance = appearance;
	map()->showObject(resource);
	const int3 destination(35, 10, 0);
	ASSERT_EQ(resource->visitablePos(), int3(-1, 10, 0));
	ASSERT_EQ(map()->getTile(destination).topVisitableObj(), resource->id);
	const auto id = resource->id;
	const auto gold = gameState()->players.at(player).resources[GameResID::GOLD];
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold + 1000);
	EXPECT_EQ(map()->getObject(id), nullptr);
}

TEST_F(HeroMovementTest, HeroCanEmbarkSailAndDisembarkAcrossToroidalSeam)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player)
		.terrain(int3(35, 1, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 1, 0));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	const int3 west(35, 1, 0);
	const int3 east(0, 1, 0);
	handler.createBoat(west, BoatId::CASTLE, player);
	const auto routeRemaining = [&](const int3 & destination, EPathfindingLayer layer)
	{
		CPathsInfo paths(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
		CPathfinder pathfinder(*gameState(), config);
		pathfinder.calculatePaths();
		const auto * node = paths.getNode(destination, layer);
		if(!node->reachable() || node->turns != 0)
			return -1;
		return static_cast<int>(node->moveRemains);
	};
	const int embarkRemaining = routeRemaining(west, EPathfindingLayer::SAIL);
	ASSERT_GE(embarkRemaining, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(west), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	ASSERT_EQ(hero->visitablePos(), west);
	EXPECT_EQ(hero->movementPointsRemaining(), embarkRemaining);

	map()->getTile(east).terrainType = TerrainId::WATER;
	hero->setMovementPoints(3000);
	const int sailingRemaining = routeRemaining(east, EPathfindingLayer::SAIL);
	ASSERT_GE(sailingRemaining, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(east), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	ASSERT_EQ(hero->visitablePos(), east);
	EXPECT_EQ(hero->movementPointsRemaining(), sailingRemaining);

	map()->getTile(west).terrainType = TerrainId::GRASS;
	hero->setMovementPoints(3000);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(west), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), east);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	const int disembarkRemaining = routeRemaining(west, EPathfindingLayer::LAND);
	ASSERT_GE(disembarkRemaining, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(west), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_FALSE(hero->inBoat());
	EXPECT_EQ(hero->visitablePos(), west);
	EXPECT_EQ(hero->movementPointsRemaining(), disembarkRemaining);
}

TEST_F(HeroMovementTest, ServerRejectsForbiddenObjectEntranceAcrossToroidalSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player)
		.resource(destination, GameResID::GOLD, 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "A" ] })";
	JsonNode definition(text.data(), text.size(), "ForbiddenSeamEntrance");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	map()->hideObject(resource);
	resource->appearance = appearance;
	map()->showObject(resource);
	const auto resourceId = resource->id;
	const auto gold = gameState()->players.at(player).resources[GameResID::GOLD];
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(destination, EPathfindingLayer::LAND)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
	EXPECT_NE(map()->getObject(resourceId), nullptr);
	for(const auto layer : {EPathfindingLayer::AIR, EPathfindingLayer::WATER, EPathfindingLayer::AVIATE})
	{
		EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, layer));
		EXPECT_EQ(hero->visitablePos(), origin);
		EXPECT_EQ(hero->movementPointsRemaining(), 3000);
		EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
		EXPECT_NE(map()->getObject(resourceId), nullptr);
	}
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::AIR));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
	EXPECT_NE(map()->getObject(resourceId), nullptr);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
	EXPECT_NE(map()->getObject(resourceId), nullptr);
}

TEST_F(HeroMovementTest, RoadMovementCostAppliesAcrossToroidalSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->getTile(origin).roadType = RoadId::COBBLESTONE_ROAD;
	map()->getTile(destination).roadType = RoadId::COBBLESTONE_ROAD;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * node = paths.getNode(destination, EPathfindingLayer::LAND);
	ASSERT_TRUE(node->reachable());
	ASSERT_EQ(node->turns, 0);
	ASSERT_NE(node->theNodeBefore, nullptr);
	EXPECT_EQ(node->theNodeBefore->coord, origin);
	EXPECT_EQ(node->moveRemains, 2950);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_EQ(hero->movementPointsRemaining(), 2950);
}

TEST_F(HeroMovementTest, ObjectEntranceDirectionIsRespectedAcrossToroidalSeams)
{
	const PlayerColor player(0);
	const int3 origin(0, 35, 0);
	const int3 destination(0, 0, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 35, 0), HeroTypeID(0), player)
		.resource(destination, GameResID::GOLD, 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	const std::string text = R"({ "animation": "test.def", "visitableFrom": ["---", "---", "---"], "mask": [ "A" ] })";
	JsonNode definition(text.data(), text.size(), "DirectionalSeamEntrance");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	ASSERT_FALSE(appearance->isVisitableFrom(0, -1));
	ASSERT_TRUE(appearance->isVisitableFrom(-1, 0));
	map()->hideObject(resource);
	resource->appearance = appearance;
	map()->showObject(resource);
	const auto resourceId = resource->id;
	const auto gold = gameState()->players.at(player).resources[GameResID::GOLD];
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * node = paths.getNode(destination, EPathfindingLayer::LAND);
	ASSERT_TRUE(node->reachable());
	ASSERT_NE(node->theNodeBefore, nullptr);
	EXPECT_NE(node->theNodeBefore->coord, origin);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
	EXPECT_NE(map()->getObject(resourceId), nullptr);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 35, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold + 1000);
	EXPECT_EQ(map()->getObject(resourceId), nullptr);
}

TEST_F(HeroMovementTest, PathfinderHonorsSourceObjectExitDirectionAcrossSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "A" ] })";
	JsonNode definition(text.data(), text.size(), "ForbiddenSeamExit");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto object = std::make_shared<CGObjectInstance>(gameState().get());
	object->pos = origin;
	object->instanceName = "forbidden_seam_exit";
	object->appearance = appearance;
	map()->hideObject(hero);
	map()->addNewObject(object);
	map()->showObject(hero);
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(destination, EPathfindingLayer::LAND)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
}

TEST_F(HeroMovementTest, HeroOnFootCannotClaimSailingLayerAcrossSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), destination);
}

TEST_F(HeroMovementTest, HeroWithMovementBonusesUsesAirAndWaterLayersAcrossSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player)
		.terrain(destination, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_EQ(hero->visitablePos(), destination);
	map()->getTile(origin).terrainType = TerrainId::WATER;
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(origin), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), origin);
	map()->getTile(destination).terrainType = TerrainId::GRASS;
	hero->setMovementPoints(3000);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
}

TEST_F(HeroMovementTest, WaterWalkingCannotVisitBlockingObjectAcrossSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player)
		.terrain(destination, TerrainId::WATER)
		.resource(destination, GameResID::GOLD, 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	ASSERT_TRUE(map()->getTile(destination).blocked());
	const auto resourceId = resource->id;
	const auto gold = gameState()->players.at(player).resources[GameResID::GOLD];
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(destination, EPathfindingLayer::WATER)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
	EXPECT_NE(map()->getObject(resourceId), nullptr);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
	EXPECT_NE(map()->getObject(resourceId), nullptr);
}

TEST_F(HeroMovementTest, HeroCrossesMinimumToroidalMapCorner)
{
	const PlayerColor player(0);
	const int3 origin(0, 0, 0);
	const int3 destination(2, 2, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(3, false).playerActive(player).hero(int3(1, 0, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * node = paths.getNode(destination, EPathfindingLayer::LAND);
	ASSERT_TRUE(node->reachable());
	ASSERT_EQ(node->turns, 0);
	ASSERT_NE(node->theNodeBefore, nullptr);
	EXPECT_EQ(node->theNodeBefore->coord, origin);
	const auto remaining = node->moveRemains;
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_EQ(hero->movementPointsRemaining(), remaining);
}

TEST_F(HeroMovementTest, HeroCanEmbarkAndDisembarkAcrossToroidalCorner)
{
	const PlayerColor player(0);
	const int3 shore(0, 0, 0);
	const int3 water(35, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 0, 0), HeroTypeID(0), player)
		.terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), shore);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(water, BoatId::CASTLE, player);

	for(const auto & [destination, layer] : {std::pair{water, EPathfindingLayer::SAIL}, std::pair{shore, EPathfindingLayer::LAND}})
	{
		hero->setMovementPoints(3000);
		CPathsInfo paths(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
		CPathfinder pathfinder(*gameState(), config);
		pathfinder.calculatePaths();
		const auto * node = paths.getNode(destination, layer);
		ASSERT_TRUE(node->reachable());
		ASSERT_EQ(node->turns, 0);
		ASSERT_NE(node->theNodeBefore, nullptr);
		EXPECT_EQ(node->theNodeBefore->coord, hero->visitablePos());
		const auto remaining = node->moveRemains;
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, layer));
		EXPECT_EQ(hero->inBoat(), layer == EPathfindingLayer::SAIL);
		EXPECT_EQ(hero->visitablePos(), destination);
		EXPECT_EQ(hero->movementPointsRemaining(), remaining);
	}
}

TEST_F(HeroMovementTest, BoatCannotCutAcrossLandAtToroidalCorner)
{
	const PlayerColor player(0);
	const int3 origin(0, 0, 0);
	const int3 destination(35, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 0, 0), HeroTypeID(0), player)
		.terrain(origin, TerrainId::WATER)
		.terrain(destination, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(origin, BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(origin), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(destination, EPathfindingLayer::SAIL)->reachable());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_TRUE(hero->inBoat());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_TRUE(hero->inBoat());
}

TEST_F(HeroMovementTest, HeroCanEmbarkAviationVehicleAcrossToroidalSeam)
{
	const PlayerColor player(0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player)
		.terrain(destination, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(destination, BoatId::CASTLE, player);
	auto * vehicle = dynamic_cast<CGBoat *>(map()->getObject(map()->getTile(destination).topVisitableObj()));
	ASSERT_NE(vehicle, nullptr);
	vehicle->layer = EPathfindingLayer::AVIATE;
	hero->setMovementPoints(3000);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AVIATE));
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::AIR));
	EXPECT_FALSE(hero->inBoat());
	EXPECT_EQ(hero->visitablePos(), int3(0, 5, 0));
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::AVIATE));
	EXPECT_TRUE(hero->inBoat());
	EXPECT_EQ(hero->visitablePos(), destination);
}

TEST_F(HeroMovementTest, AviationVehicleCannotEnterVisitableObjectAcrossSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 5, 0), HeroTypeID(0), player)
		.terrain(origin, TerrainId::WATER)
		.terrain(destination, TerrainId::WATER)
		.resource(destination, GameResID::GOLD, 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	auto * resource = findFirst<CGResource>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	const auto resourceId = resource->id;
	const auto gold = gameState()->players.at(player).resources[GameResID::GOLD];
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(origin, BoatId::CASTLE, player);
	auto * vehicle = dynamic_cast<CGBoat *>(map()->getObject(map()->getTile(origin).topVisitableObj()));
	ASSERT_NE(vehicle, nullptr);
	vehicle->layer = EPathfindingLayer::AVIATE;
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(origin), EMovementMode::STANDARD, false, player, EPathfindingLayer::AVIATE));
	ASSERT_TRUE(hero->inBoat());
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(destination, EPathfindingLayer::AVIATE)->reachable());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::AVIATE));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], gold);
	EXPECT_NE(map()->getObject(resourceId), nullptr);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 6, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::AVIATE));
	EXPECT_TRUE(hero->inBoat());
	EXPECT_EQ(hero->visitablePos(), int3(0, 6, 0));
}

TEST_F(HeroMovementTest, BoatCanSailAcrossToroidalCornerAndBothAxes)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 0, 0), HeroTypeID(0), player)
		.terrain(int3(0, 0, 0), TerrainId::WATER)
		.terrain(int3(35, 0, 0), TerrainId::WATER)
		.terrain(int3(0, 35, 0), TerrainId::WATER)
		.terrain(int3(35, 35, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(1, 0, 0));
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(int3(0, 0, 0), BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));

	for(const int3 destination : {
		int3(35, 35, 0), int3(0, 0, 0), // northwest and southeast
		int3(35, 0, 0), int3(0, 35, 0), int3(35, 0, 0), int3(0, 0, 0), // west, northeast, southwest, east
		int3(0, 35, 0), int3(0, 0, 0) // north and south
	})
	{
		hero->setMovementPoints(3000);
		CPathsInfo paths(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
		CPathfinder pathfinder(*gameState(), config);
		pathfinder.calculatePaths();
		const auto * node = paths.getNode(destination, EPathfindingLayer::SAIL);
		ASSERT_TRUE(node->reachable());
		ASSERT_EQ(node->turns, 0);
		ASSERT_NE(node->theNodeBefore, nullptr);
		EXPECT_EQ(node->theNodeBefore->coord, hero->visitablePos());
		const auto remaining = node->moveRemains;
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
		ASSERT_TRUE(hero->inBoat());
		EXPECT_EQ(hero->visitablePos(), destination);
		EXPECT_EQ(hero->movementPointsRemaining(), remaining);
	}
}

TEST_F(HeroMovementTest, UndergroundSeamPreservesLevelAndFogIsolation)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(1, 1, 1), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	setMapVisibility(player, false);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 1, 1));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	const int3 destination(35, 1, 1);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_TRUE(gameState()->isVisibleFor(destination, player));
	EXPECT_FALSE(gameState()->isVisibleFor(int3(35, 1, 0), player));
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 1, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), destination);

	revealMap(player);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * wrappedNeighbor = paths.getNode(int3(0, 1, 1), EPathfindingLayer::LAND);
	ASSERT_TRUE(wrappedNeighbor->reachable());
	ASSERT_NE(wrappedNeighbor->theNodeBefore, nullptr);
	EXPECT_EQ(wrappedNeighbor->theNodeBefore->coord, destination);
	EXPECT_FALSE(paths.getNode(int3(35, 1, 0), EPathfindingLayer::LAND)->reachable());
}

TEST_F(HeroMovementTest, HeroCollectsResourceAcrossToroidalCorner)
{
	const PlayerColor player(0);
	const int3 destination(35, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 0, 0), HeroTypeID(0), player)
		.resource(destination, GameResID::GOLD, 10);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	hero->setMovementPoints(3000);
	const auto resourceID = map()->getTile(destination).topVisitableObj();
	ASSERT_TRUE(resourceID.hasValue());
	const auto goldBefore = gameState()->players.at(player).resources[GameResID::GOLD];
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), int3(0, 0, 0));
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], goldBefore + 1000);
	EXPECT_EQ(map()->getObject(resourceID), nullptr);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_EQ(gameState()->players.at(player).resources[GameResID::GOLD], goldBefore + 1000);
}

TEST_F(HeroMovementTest, MonsterGuardAcrossSeamStartsBattle)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 7, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.monster(int3(35, 5, 0), CreatureID(27), 100);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->calculateGuardingGreaturePositions();
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 7, 0));
	const auto guardID = map()->getTile(int3(35, 5, 0)).topVisitableObj();
	const auto * guard = dynamic_cast<const CArmedInstance *>(map()->getObject(guardID));
	ASSERT_NE(guard, nullptr);
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 6, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	const auto * battle = gameState()->getBattle(player);
	ASSERT_NE(battle, nullptr);
	EXPECT_EQ(battle->getSideArmy(BattleSide::DEFENDER), guard);
}

TEST_F(HeroMovementTest, MonsterGuardAcrossCornerStartsBattle)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.monster(int3(35, 35, 0), CreatureID(27), 100);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->calculateGuardingGreaturePositions();
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 1, 0));
	const auto guardID = map()->getTile(int3(35, 35, 0)).topVisitableObj();
	const auto * guard = dynamic_cast<const CArmedInstance *>(map()->getObject(guardID));
	ASSERT_NE(guard, nullptr);
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	const auto * battle = gameState()->getBattle(player);
	ASSERT_NE(battle, nullptr);
	EXPECT_EQ(battle->getSideArmy(BattleSide::DEFENDER), guard);
}

TEST_F(HeroMovementTest, FlyingAbilityDoesNotBypassGuardOnLandTransit)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.monster(int3(35, 35, 0), CreatureID(27), 100);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->calculateGuardingGreaturePositions();
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::LAND));
	EXPECT_NE(gameState()->getBattle(player), nullptr);
}

TEST_F(HeroMovementTest, FlyingTransitStillBypassesGuardAcrossCorner)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.monster(int3(35, 35, 0), CreatureID(27), 100);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->calculateGuardingGreaturePositions();
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	EXPECT_EQ(gameState()->getBattle(player), nullptr);
	EXPECT_EQ(hero->visitablePos(), int3(0, 0, 0));
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
