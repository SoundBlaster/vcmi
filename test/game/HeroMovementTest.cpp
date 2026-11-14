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

#include "../../client/mapView/MapViewBattle.h"
#include "../../client/netlag/PackRollbackGeneratorVisitor.h"

#include "../mock/GameHandlerTestServer.h"
#include "../mock/TinyMapGameTest.h"
#include "../mock/ToroidalDemoMap.h"
#include "../../client/adventureMap/MapAudioSources.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/filesystem/CFilesystemLoader.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/AdapterLoaders.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGResource.h"
#include "../../lib/mapObjects/CGPandoraBox.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/mapping/MapTopology.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/MapFormatH3M.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/modding/IdentifierResolver.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../lib/networkPacks/LobbyTopologyCapability.h"
#include "../../lib/network/NetworkInterface.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/spells/adventure/AdventureSpellMechanics.h"
#include "../../lib/spells/adventure/DimensionDoorEffect.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/serializer/CMemorySerializer.h"
#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/CSaveFile.h"
#include "../../lib/serializer/ESerializationVersion.h"
#include "../../lib/serializer/GameConnection.h"
#include "../../server/CGameHandler.h"
#include "../../server/CVCMIServer.h"
#include "../../server/TurnTimerHandler.h"
#include "../../server/activities/ActivityProcessor.h"
#include "../../server/battles/BattleProcessor.h"
#include "../../server/processors/HeroPoolProcessor.h"
#include "../../server/processors/PlayerMessageProcessor.h"
#include "../../server/processors/TurnOrderProcessor.h"

class HeroMovementTest : public TinyMapGameTest
{
protected:
	const Services * gameServices() override { return LIBRARY; }
	void checkToroidalSeamMovement(EMapFormat format);
	void checkToroidalSightRange(EMapFormat format);
	void checkBlockedToroidalStep(EMapFormat format);
	void checkDimensionDoorAcrossToroidalSeam(const int3 & start, const int3 & destination);
	void checkSingleIslandTransitStopsBeforeStranding(SpellID spell, BonusType bonus, EPathfindingLayer layer);
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

class ToroidalMemoryMapService : public CMapService
{
public:
	explicit ToroidalMemoryMapService(const std::vector<ui8> & data) : data(data) {}
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

TEST(ToroidalDemoVmapTest, StartingHeroUsesMainTownOutsideMap)
{
	for(const int3 rawAnchor : {int3(-1, 10, 0), int3(36, 10, 0), int3(-1, -1, 0), int3(36, 36, 0)})
	for(const bool existingHero : {false, true})
	{
		SCOPED_TRACE(rawAnchor.toString());
		SCOPED_TRACE(existingHero);
		const PlayerColor player(0);
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false).playerActive(player)
			.town(int3(10, 10, 0), FactionID::CASTLE, player)
			.playerMainTown(player, int3(10, 10, 0), true);
		if(existingHero)
			builder.hero(int3(20, 20, 0), HeroTypeID(0), player);
		const auto source = builder.build();
		CMemoryBuffer legacyBytes;
		legacyBytes.write(source.data(), static_cast<si64>(source.size()));
		legacyBytes.seek(0);
		EditorCallback editor(nullptr);
		CMapLoaderH3M legacyLoader("WrappedMainTown", "core", "ASCII", &legacyBytes);
		auto map = legacyLoader.loadMap(&editor);
		editor.setMap(map.get());
		CGTownInstance * town = nullptr;
		for(const auto & object : map->objects)
			if(auto * candidate = dynamic_cast<CGTownInstance *>(object.get()))
				town = candidate;
		ASSERT_NE(town, nullptr);
		ASSERT_TRUE(map->players[0].hasMainTown);
		ASSERT_TRUE(map->players[0].generateHeroAtMainTown);
		ASSERT_EQ(map->players[0].posOfMainTown, town->anchorPos());
		const auto originalAnchor = town->anchorPos();
		EventCondition objective(EventCondition::CONTROL_CURRENT);
		objective.objectType = MapObjectID(Obj::TOWN);
		objective.position = originalAnchor;
		TriggeredEvent objectiveEvent;
		objectiveEvent.identifier = "moved_town_control";
		objectiveEvent.effect.type = EventEffect::VICTORY;
		objectiveEvent.trigger = EventExpression(EventExpression::Variant(objective));
		map->triggeredEvents.push_back(objectiveEvent);
		map->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);
		map->getEditManager()->moveObject(town, rawAnchor);
		ASSERT_EQ(map->players[0].posOfMainTown, rawAnchor);
		map->getEditManager()->getUndoManager().undo();
		ASSERT_EQ(town->anchorPos(), originalAnchor);
		ASSERT_EQ(map->players[0].posOfMainTown, originalAnchor);
		map->getEditManager()->getUndoManager().redo();
		ASSERT_EQ(town->anchorPos(), rawAnchor);
		ASSERT_EQ(map->players[0].posOfMainTown, rawAnchor);
		const int3 entrance = town->visitablePos();
		ASSERT_TRUE(map->isInTheMap(entrance));
		if(existingHero)
		{
			CGHeroInstance * placedHero = nullptr;
			for(const auto & object : map->objects)
				if(auto * candidate = dynamic_cast<CGHeroInstance *>(object.get()); candidate && candidate->getOwner() == player)
					placedHero = candidate;
			ASSERT_NE(placedHero, nullptr);
			map->moveObject(placedHero->id, placedHero->convertFromVisitablePos(entrance));
		}
		CMemoryBuffer archive;
		{
			CMapSaverJson saver(&archive);
			saver.saveMap(map);
		}
		ToroidalMemoryMapService service(archive.getBuffer());
		auto header = service.loadMapHeader(ResourcePath("memory"));
		ASSERT_TRUE(header->players[0].hasMainTown);
		ASSERT_TRUE(header->players[0].generateHeroAtMainTown);
		ASSERT_EQ(header->players[0].posOfMainTown, rawAnchor);
		StartInfo info;
		info.mapname = "memory";
		info.difficulty = static_cast<ui8>(EMapDifficulty::EASY);
		info.mode = EStartMode::NEW_GAME;
		auto & settings = info.playerInfos[player];
		settings.color = player;
		settings.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(0));
		settings.name = "Player";
		settings.castle = header->players[0].defaultCastle();
		settings.hero = header->players[0].defaultHero();
		settings.bonus = PlayerStartingBonus::GOLD;
		auto state = std::make_shared<CGameState>();
		state->preInit(LIBRARY);
		GameRandomizer randomizer(*state);
		Load::ProgressAccumulator progress;
		state->init(&service, &info, randomizer, progress, false);
		const CGHeroInstance * hero = nullptr;
		const CGTownInstance * startedTown = nullptr;
		int heroCount = 0;
		for(const auto & object : state->getMap().objects)
		{
			if(auto * candidate = dynamic_cast<const CGHeroInstance *>(object.get()); candidate && candidate->getOwner() == player)
			{
				hero = candidate;
				++heroCount;
			}
			if(auto * candidate = dynamic_cast<const CGTownInstance *>(object.get()); candidate && candidate->getOwner() == player)
				startedTown = candidate;
		}
		ASSERT_NE(hero, nullptr);
		EXPECT_EQ(heroCount, 1);
		ASSERT_NE(startedTown, nullptr);
		EXPECT_EQ(hero->visitablePos(), entrance);
		EXPECT_EQ(startedTown->getVisitingHero(), hero);
		const auto event = std::ranges::find_if(state->getMap().triggeredEvents, [](const TriggeredEvent & candidate)
		{
			return candidate.identifier == "moved_town_control";
		});
		ASSERT_NE(event, state->getMap().triggeredEvents.end());
		EventCondition resolved;
		event->trigger.test([&](const EventCondition & condition)
		{
			resolved = condition;
			return true;
		});
		EXPECT_EQ(resolved.position, rawAnchor);
		EXPECT_EQ(resolved.objectID, startedTown->id);
		EXPECT_TRUE(state->checkForVictory(player, resolved));
	}
}

TEST(ToroidalDemoVmapTest, ServerStartRejectsAnIncompatiblePeer)
{
	const auto directory = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-start-%%%%-%%%%");
	boost::filesystem::create_directory(directory);
	struct TemporaryMap
	{
		boost::filesystem::path directory;
		bool registered = false;
		~TemporaryMap()
		{
			if(registered)
				CResourceHandler::removeFilesystem("local", "mapEditor");
			boost::filesystem::remove_all(directory);
		}
	} temporary{directory};

	auto source = makeToroidalDemoMap().build();
	CMemoryBuffer legacyBytes;
	legacyBytes.write(source.data(), static_cast<si64>(source.size()));
	legacyBytes.seek(0);
	EditorCallback editor(nullptr);
	CMapLoaderH3M legacyLoader("ToroidSeamLaboratory", "core", "ASCII", &legacyBytes);
	auto map = legacyLoader.loadMap(&editor);
	editor.setMap(map.get());
	map->setToroidalAndRebuild(true);
	CMapService service;
	service.saveMap(map, directory / "server-start-test.vmap");
	CResourceHandler::addFilesystem("local", "mapEditor", std::make_unique<CFilesystemLoader>("MAPEDITOR/", directory, 0));
	const ResourcePath mapPath("MAPEDITOR/server-start-test", EResType::MAP);
	temporary.registered = CResourceHandler::get("mapEditor")->existsResource(mapPath);
	ASSERT_TRUE(temporary.registered);
	auto header = service.loadMapHeader(mapPath);
	ASSERT_TRUE(header->toroidal);

	class StartCapture final : public INetworkConnection
	{
	public:
		void sendPacket(const std::vector<std::byte> &) override {}
		void setAsyncWritesEnabled(bool) override {}
		void close() override {}
	};
	std::vector<std::shared_ptr<INetworkConnection>> transports;
	const auto configure = [&](CVCMIServer & server, bool supported)
	{
		server.si = std::make_shared<StartInfo>();
		server.si->mapname = "MAPEDITOR/server-start-test";
		server.si->difficulty = static_cast<ui8>(EMapDifficulty::EASY);
		server.si->mode = EStartMode::NEW_GAME;
		server.mi = std::make_shared<CMapInfo>();
		server.mi->fileURI = server.si->mapname;
		for(int index = 0; index < static_cast<int>(header->players.size()); ++index)
		{
			const auto & player = header->players[index];
			if(!(player.canHumanPlay || player.canComputerPlay))
				continue;
			auto & settings = server.si->playerInfos[PlayerColor(index)];
			settings.color = PlayerColor(index);
			settings.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(index));
			settings.name = "Player";
			settings.castle = player.defaultCastle();
			settings.hero = player.defaultHero();
			settings.bonus = PlayerStartingBonus::GOLD;
		}
		auto transport = std::make_shared<StartCapture>();
		transports.push_back(transport);
		auto connection = std::make_shared<GameConnection>(transport);
		connection->setSerializationVersion(ESerializationVersion::CURRENT);
		connection->setToroidalTopologySupported(supported);
		server.activeConnections.push_back(connection);
	};
	CVCMIServer compatible(0, true);
	configure(compatible, true);
	ASSERT_TRUE(compatible.prepareToStartGame());
	ASSERT_NE(compatible.gh, nullptr);
	ASSERT_TRUE(compatible.gh->gameInfo().getMapHeader()->toroidal);
	CVCMIServer incompatible(0, true);
	configure(incompatible, false);
	EXPECT_FALSE(incompatible.prepareToStartGame());
	EXPECT_EQ(incompatible.gh, nullptr);
}

TEST(ToroidalDemoVmapTest, TcpLobbyStartsToroidalGameForTwoPeers)
{
	const auto directory = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-lobby-%%%%-%%%%");
	boost::filesystem::create_directory(directory);
	struct TemporaryMap
	{
		boost::filesystem::path directory;
		bool registered = false;
		~TemporaryMap()
		{
			if(registered)
				CResourceHandler::removeFilesystem("local", "mapEditor");
			boost::filesystem::remove_all(directory);
		}
	} temporary{directory};

	const PlayerColor red(0), blue(1);
	auto mapBuilder = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false).name("Toroid TCP Lobby")
		.playerActive(red).playerActive(blue)
		.playerMainTown(red, int3(8, 8, 0), false)
		.playerMainTown(blue, int3(27, 27, 0), false)
		.town(int3(8, 8, 0), FactionID(0), red)
		.town(int3(27, 27, 0), FactionID(0), blue)
		.hero(int3(1, 0, 0), HeroTypeID(0), red)
		.hero(int3(1, 5, 0), HeroTypeID(1), blue)
		.terrain(int3(35, 4, 0), TerrainId::ROCK)
		.resource(int3(35, 35, 0), GameResID::GOLD, 10);
	for(int x = 0; x <= 2; ++x)
		for(int y = 33; y <= 35; ++y)
			mapBuilder.terrain(int3(x, y, 0), TerrainId::WATER);
	auto source = mapBuilder.build();
	CMemoryBuffer bytes;
	bytes.write(source.data(), static_cast<si64>(source.size()));
	bytes.seek(0);
	EditorCallback editor(nullptr);
	CMapLoaderH3M loader("ToroidSeamLaboratory", "core", "ASCII", &bytes);
	auto map = loader.loadMap(&editor);
	editor.setMap(map.get());
	map->setToroidalAndRebuild(true);
	CMapService mapService;
	mapService.saveMap(map, directory / "tcp-lobby.vmap");
	CResourceHandler::addFilesystem("local", "mapEditor", std::make_unique<CFilesystemLoader>("MAPEDITOR/", directory, 0));
	const ResourcePath mapPath("MAPEDITOR/tcp-lobby", EResType::MAP);
	temporary.registered = CResourceHandler::get("mapEditor")->existsResource(mapPath);
	ASSERT_TRUE(temporary.registered);
	auto header = mapService.loadMapHeader(mapPath);
	ASSERT_TRUE(header->toroidal);

	class LobbyPeer final : public INetworkClientListener
	{
	public:
		explicit LobbyPeer(std::string name, bool offerTopology = true) : name(std::move(name)), offerTopology(offerTopology) {}
		void onConnectionFailed(const std::string & error) override { failure = error; }
		void onDisconnected(const std::shared_ptr<INetworkConnection> &, const std::string & error) override { failure = error; }
		void onConnectionEstablished(const std::shared_ptr<INetworkConnection> & connection) override
		{
			transport = connection;
			codec = std::make_unique<GameConnection>(transport);
			LobbyClientConnected greeting;
			greeting.uuid = name;
			greeting.names = {name};
			greeting.mode = EStartMode::NEW_GAME;
			greeting.version = offerTopology ? LobbyTopologyCapability::encode(ESerializationVersion::CURRENT) : ESerializationVersion::CURRENT;
			codec->sendPack(greeting);
		}
		void onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & bytes) override
		{
			auto packet = codec->retrievePack(bytes);
			if(const auto * greeting = dynamic_cast<const LobbyClientConnected *>(packet.get()); greeting && greeting->uuid == name)
			{
				const auto accepted = LobbyTopologyCapability::decode(greeting->version);
				negotiated = true;
				acceptedTopology = accepted.toroidal;
				acceptedVersion = accepted.version;
				clientId = greeting->clientId;
				codec->setSerializationVersion(accepted.version);
			}
			if(const auto * message = dynamic_cast<const LobbyShowMessage *>(packet.get()))
				messages.push_back(message->message.toString(LIBRARY->staticTexts()));
			if(const auto * message = dynamic_cast<const SystemMessage *>(packet.get()))
				systemMessages.push_back(message->text.toString(LIBRARY->staticTexts()));
			if(auto * start = dynamic_cast<LobbyStartGame *>(packet.get()); start && start->initializedGameState)
			{
				state = start->initializedGameState;
				state->preInit(LIBRARY);
				codec->setCallback(*state);
			}
			else if(auto * gamePack = dynamic_cast<CPackForClient *>(packet.get()); gamePack && state)
			{
				state->apply(*gamePack);
				if(const auto * applied = dynamic_cast<const PackageApplied *>(gamePack))
					results[applied->requestID] = applied->result;
				if(const auto * movement = dynamic_cast<const TryMoveHero *>(gamePack); movement && movement->result == TryMoveHero::FAILED)
					failedMoves.insert(movement->id);
			}
		}
		std::string name;
		std::string failure;
		bool offerTopology;
		bool negotiated = false;
		bool acceptedTopology = false;
		ESerializationVersion acceptedVersion = ESerializationVersion::NONE;
		GameConnectionID clientId = GameConnectionID::INVALID;
		std::vector<std::string> messages;
		std::vector<std::string> systemMessages;
		std::map<uint32_t, bool> results;
		std::set<ObjectInstanceID> failedMoves;
		std::shared_ptr<INetworkConnection> transport;
		std::unique_ptr<GameConnection> codec;
		std::shared_ptr<CGameState> state;
	};

	CVCMIServer server(0, true);
	const auto port = server.prepare(false, true);
	ASSERT_NE(port, 0);
	auto network = INetworkHandler::createHandler();
	LobbyPeer host("lobby-host"), guest("lobby-guest"), legacy("lobby-legacy", false);
	const auto pumpUntil = [&](const auto & ready)
	{
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		while(!ready() && host.failure.empty() && guest.failure.empty() && std::chrono::steady_clock::now() < deadline)
		{
			network->getContext().restart();
			network->getContext().poll();
			server.getNetworkHandler().getContext().restart();
			server.getNetworkHandler().getContext().poll();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	};
	network->connectToRemote(host, "127.0.0.1", port);
	pumpUntil([&] { return host.negotiated; });
	ASSERT_TRUE(host.negotiated) << host.failure;
	ASSERT_TRUE(host.acceptedTopology);
	ASSERT_EQ(host.acceptedVersion, ESerializationVersion::CURRENT);
	network->connectToRemote(guest, "127.0.0.1", port);
	pumpUntil([&] { return guest.negotiated; });
	ASSERT_TRUE(guest.negotiated) << guest.failure;
	ASSERT_TRUE(guest.acceptedTopology);
	ASSERT_EQ(guest.acceptedVersion, ESerializationVersion::CURRENT);
	ASSERT_EQ(server.activeConnections.size(), 2);

	server.si->mapname = "MAPEDITOR/tcp-lobby";
	server.si->difficulty = static_cast<ui8>(EMapDifficulty::EASY);
	server.si->mode = EStartMode::NEW_GAME;
	server.mi = std::make_shared<CMapInfo>();
	server.mi->fileURI = server.si->mapname;
	server.mi->mapHeader = std::move(header);
	for(int index = 0; index < 2; ++index)
	{
		const auto color = PlayerColor(index);
		const auto & sourcePlayer = server.mi->mapHeader->players[index];
		auto & playerSettings = server.si->playerInfos[color];
		playerSettings.color = color;
		playerSettings.name = index == 0 ? host.name : guest.name;
		playerSettings.castle = sourcePlayer.defaultCastle();
		playerSettings.hero = sourcePlayer.defaultHero();
		playerSettings.bonus = PlayerStartingBonus::GOLD;
		for(const auto & [id, player] : server.playerNames)
			if(player.name == playerSettings.name)
				playerSettings.connectedPlayerIDs.insert(id);
	}
	ASSERT_TRUE(server.hasPlayerAt(PlayerColor(0), server.activeConnections[0]->connectionID));
	ASSERT_TRUE(server.hasPlayerAt(PlayerColor(1), server.activeConnections[1]->connectionID));
	// The untagged peer offers the same ordinal but must be downgraded and
	// prevent a toroidal game from starting through the actual lobby path.
	network->connectToRemote(legacy, "127.0.0.1", port);
	pumpUntil([&] { return legacy.negotiated || !legacy.failure.empty(); });
	ASSERT_TRUE(legacy.negotiated) << legacy.failure;
	EXPECT_FALSE(legacy.acceptedTopology);
	EXPECT_EQ(legacy.acceptedVersion, ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER);
	ASSERT_EQ(server.activeConnections.size(), 3);
	const auto messagesBefore = host.messages.size();
	LobbyStartGame start;
	host.codec->sendPack(start);
	pumpUntil([&] { return host.messages.size() > messagesBefore; });
	ASSERT_GT(host.messages.size(), messagesBefore) << host.failure;
	EXPECT_EQ(host.messages.back(), MetaString::createFromTextID("vcmi.server.errors.incompatibleVersion").toString(LIBRARY->staticTexts()));
	EXPECT_EQ(server.getState(), EServerState::LOBBY);
	EXPECT_EQ(server.gh, nullptr);
	EXPECT_FALSE(host.state);
	EXPECT_FALSE(guest.state);
	EXPECT_FALSE(legacy.state);
	LobbyClientDisconnected disconnect;
	disconnect.clientId = legacy.clientId;
	legacy.codec->sendPack(disconnect);
	pumpUntil([&] { return server.activeConnections.size() == 2; });
	ASSERT_EQ(server.activeConnections.size(), 2);
	ASSERT_EQ(server.getState(), EServerState::LOBBY);
	host.codec->sendPack(start);
	pumpUntil([&] { return host.state && guest.state; });
	ASSERT_TRUE(host.state) << host.failure;
	ASSERT_TRUE(guest.state) << guest.failure;
	ASSERT_NE(server.gh, nullptr);
	ASSERT_EQ(server.getState(), EServerState::GAMEPLAY);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_TRUE(peer->state->getMap().toroidal);
		EXPECT_EQ(peer->state->getMapSize(), server.gh->gameState().getMapSize());
		const auto heroes = peer->state->getHeroes(PlayerColor(0));
		ASSERT_EQ(heroes.size(), 1);
		EXPECT_EQ(heroes.front()->visitablePos(), int3(0, 0, 0));
	}
	pumpUntil([&] { return host.state->actingPlayers.contains(red) && guest.state->actingPlayers.contains(red); });
	ASSERT_TRUE(server.gh->gameState().actingPlayers.contains(red));
	ASSERT_TRUE(host.state->actingPlayers.contains(red));
	ASSERT_TRUE(guest.state->actingPlayers.contains(red));
	const auto heroes = server.gh->gameState().getHeroes(red);
	ASSERT_EQ(heroes.size(), 1);
	const auto * hero = heroes.front();
	const auto goldBefore = server.gh->gameState().players.at(red).resources[GameResID::GOLD];
	const int3 destination(35, 35, 0);
	MoveHero movement({hero->convertFromVisitablePos(destination)}, EPathfindingLayer::LAND, hero->id, false);
	movement.player = red;
	movement.requestID = 10;
	host.codec->sendPack(movement);
	pumpUntil([&] {
		return host.results.contains(10)
			&& host.state->players.at(red).resources[GameResID::GOLD] == goldBefore + 1000
			&& guest.state->players.at(red).resources[GameResID::GOLD] == goldBefore + 1000;
	});
	ASSERT_TRUE(host.results.contains(10)) << host.failure;
	EXPECT_TRUE(host.results.at(10));
	EXPECT_EQ(server.gh->gameState().getHero(hero->id)->visitablePos(), int3(0, 0, 0));
	EXPECT_EQ(server.gh->gameState().players.at(red).resources[GameResID::GOLD], goldBefore + 1000);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_EQ(peer->state->getHero(hero->id)->visitablePos(), int3(0, 0, 0)) << peer->failure;
		EXPECT_EQ(peer->state->players.at(red).resources[GameResID::GOLD], goldBefore + 1000);
	}
	movement.requestID = 11;
	host.codec->sendPack(movement);
	pumpUntil([&] {
		return host.results.contains(11)
			&& host.state->getHero(hero->id)->visitablePos() == destination
			&& guest.state->getHero(hero->id)->visitablePos() == destination;
	});
	ASSERT_TRUE(host.results.contains(11)) << host.failure;
	EXPECT_TRUE(host.results.at(11));
	EXPECT_EQ(server.gh->gameState().getHero(hero->id)->visitablePos(), destination);
	for(const LobbyPeer * peer : {&host, &guest})
		EXPECT_EQ(peer->state->getHero(hero->id)->visitablePos(), destination) << peer->failure;

	GiveBonus flightBonus;
	flightBonus.id = hero->id;
	flightBonus.bonus = Bonus(BonusDuration::ONE_DAY, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID());
	server.gh->giveHeroBonus(&flightBonus);
	pumpUntil([&] {
		return host.state->getHero(hero->id)->hasBonusOfType(BonusType::FLYING_MOVEMENT)
			&& guest.state->getHero(hero->id)->hasBonusOfType(BonusType::FLYING_MOVEMENT);
	});
	ASSERT_TRUE(host.state->getHero(hero->id)->hasBonusOfType(BonusType::FLYING_MOVEMENT));
	ASSERT_TRUE(guest.state->getHero(hero->id)->hasBonusOfType(BonusType::FLYING_MOVEMENT));
	FowTilesType knownWater;
	for(int x = 0; x <= 2; ++x)
		for(int y = 33; y <= 35; ++y)
			knownWater.insert(int3(x, y, 0));
	server.gh->changeFogOfWar(knownWater, red, ETileVisibility::REVEALED);
	server.gh->setMovePoints(hero->id, 350);
	pumpUntil([&] {
		return host.state->getHero(hero->id)->movementPointsRemaining() == 350
			&& guest.state->getHero(hero->id)->movementPointsRemaining() == 350
			&& std::all_of(knownWater.begin(), knownWater.end(), [&](const int3 & tile) {
				return host.state->isVisibleFor(tile, red) && guest.state->isVisibleFor(tile, red);
			});
	});
	ASSERT_EQ(host.state->getHero(hero->id)->movementPointsRemaining(), 350);
	ASSERT_EQ(guest.state->getHero(hero->id)->movementPointsRemaining(), 350);
	ASSERT_TRUE(std::all_of(knownWater.begin(), knownWater.end(), [&](const int3 & tile) {
		return host.state->isVisibleFor(tile, red) && guest.state->isVisibleFor(tile, red);
	}));
	const int3 airborne(0, 35, 0);
	MoveHero flight({hero->convertFromVisitablePos(airborne)}, EPathfindingLayer::AIR, hero->id, true);
	flight.player = red;
	flight.requestID = 20;
	host.codec->sendPack(flight);
	pumpUntil([&] {
		return host.results.contains(20)
			&& host.state->getHero(hero->id)->visitablePos() == airborne
			&& guest.state->getHero(hero->id)->visitablePos() == airborne;
	});
	ASSERT_TRUE(host.results.contains(20)) << host.failure;
	ASSERT_TRUE(host.results.at(20));
	for(const LobbyPeer * peer : {&host, &guest})
		ASSERT_EQ(peer->state->getHero(hero->id)->getPathfindingStartLayer(), EPathfindingLayer::AIR);
	const auto pointsBeforeRejectedFlight = server.gh->gameState().getHero(hero->id)->movementPointsRemaining();
	ASSERT_EQ(pointsBeforeRejectedFlight, 250);
	CPathfinderHelper movementCost(server.gh->gameState(), hero, PathfinderOptions(server.gh->gameInfo()));
	const auto unsafeCost = movementCost.getMovementCost(airborne, int3(1, 34, 0), EPathfindingLayer::AIR, pointsBeforeRejectedFlight);
	ASSERT_GT(unsafeCost, 0);
	ASSERT_LT(unsafeCost, pointsBeforeRejectedFlight);
	const auto redVisibility = [&](const CGameState & state)
	{
		std::vector<bool> result;
		for(int x = 0; x < 36; ++x)
			for(int y = 0; y < 36; ++y)
				result.push_back(state.isVisibleFor(int3(x, y, 0), red));
		return result;
	};
	const auto redFogBeforeRejection = redVisibility(server.gh->gameState());
	ASSERT_EQ(redVisibility(*host.state), redFogBeforeRejection);
	ASSERT_EQ(redVisibility(*guest.state), redFogBeforeRejection);
	MoveHero unsafeFlight({hero->convertFromVisitablePos(int3(1, 34, 0))}, EPathfindingLayer::AIR, hero->id, true);
	unsafeFlight.player = red;
	unsafeFlight.requestID = 24;
	host.codec->sendPack(unsafeFlight);
	pumpUntil([&] { return host.results.contains(24) && host.failedMoves.contains(hero->id) && guest.failedMoves.contains(hero->id); });
	ASSERT_TRUE(host.results.contains(24)) << host.failure;
	ASSERT_TRUE(host.failedMoves.contains(hero->id));
	ASSERT_TRUE(guest.failedMoves.contains(hero->id));
	EXPECT_FALSE(host.results.at(24));
#ifndef ENABLE_GOLDMASTER
	EXPECT_TRUE(std::any_of(host.systemMessages.begin(), host.systemMessages.end(), [](const std::string & message) {
		return message.find("Cannot leave the known landing area") != std::string::npos;
	}));
#endif
	EXPECT_EQ(server.gh->gameState().getHero(hero->id)->visitablePos(), airborne);
	EXPECT_EQ(server.gh->gameState().getHero(hero->id)->movementPointsRemaining(), pointsBeforeRejectedFlight);
	EXPECT_EQ(redVisibility(server.gh->gameState()), redFogBeforeRejection);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_EQ(peer->state->getHero(hero->id)->visitablePos(), airborne);
		EXPECT_EQ(peer->state->getHero(hero->id)->movementPointsRemaining(), pointsBeforeRejectedFlight);
		EXPECT_EQ(redVisibility(*peer->state), redFogBeforeRejection);
	}

	// Synthesize the same fog frontier on the server and both replicas before
	// retrying. Normal FoW packets cannot create this artificial state because
	// owned observers immediately reveal nearby tiles again.
	const int3 hiddenFrontier(2, 34, 0);
	const_cast<CGameState &>(server.gh->gameState()).getPlayerTeam(red)->fogOfWarMap[hiddenFrontier] = 0;
	host.state->getPlayerTeam(red)->fogOfWarMap[hiddenFrontier] = 0;
	guest.state->getPlayerTeam(red)->fogOfWarMap[hiddenFrontier] = 0;
	ASSERT_FALSE(server.gh->gameState().isVisibleFor(hiddenFrontier, red));
	ASSERT_FALSE(host.state->isVisibleFor(hiddenFrontier, red));
	ASSERT_FALSE(guest.state->isVisibleFor(hiddenFrontier, red));
	const auto fogBeforeUnknownRejection = redVisibility(server.gh->gameState());
	ASSERT_EQ(redVisibility(*host.state), fogBeforeUnknownRejection);
	ASSERT_EQ(redVisibility(*guest.state), fogBeforeUnknownRejection);
	host.failedMoves.clear();
	guest.failedMoves.clear();
	unsafeFlight.requestID = 33;
	host.codec->sendPack(unsafeFlight);
	pumpUntil([&] { return host.results.contains(33) && host.failedMoves.contains(hero->id) && guest.failedMoves.contains(hero->id); });
	ASSERT_TRUE(host.results.contains(33)) << host.failure;
	EXPECT_FALSE(host.results.at(33));
	ASSERT_TRUE(host.failedMoves.contains(hero->id));
	ASSERT_TRUE(guest.failedMoves.contains(hero->id));
	EXPECT_EQ(server.gh->gameState().getHero(hero->id)->visitablePos(), airborne);
	EXPECT_EQ(server.gh->gameState().getHero(hero->id)->movementPointsRemaining(), pointsBeforeRejectedFlight);
	EXPECT_EQ(redVisibility(server.gh->gameState()), fogBeforeUnknownRejection);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_EQ(peer->state->getHero(hero->id)->visitablePos(), airborne);
		EXPECT_EQ(peer->state->getHero(hero->id)->movementPointsRemaining(), pointsBeforeRejectedFlight);
		EXPECT_EQ(redVisibility(*peer->state), fogBeforeUnknownRejection);
	}
	FoWChange restoreVisibility;
	restoreVisibility.player = red;
	restoreVisibility.mode = ETileVisibility::REVEALED;
	restoreVisibility.tiles.insert(hiddenFrontier);
	server.gh->sendAndApply(restoreVisibility);
	pumpUntil([&] {
		return host.state->isVisibleFor(hiddenFrontier, red)
			&& guest.state->isVisibleFor(hiddenFrontier, red);
	});
	ASSERT_TRUE(host.state->isVisibleFor(hiddenFrontier, red));
	ASSERT_TRUE(guest.state->isVisibleFor(hiddenFrontier, red));
	EndTurn prematureEnd;
	prematureEnd.player = red;
	prematureEnd.requestID = 21;
	host.codec->sendPack(prematureEnd);
	pumpUntil([&] { return host.results.contains(21); });
	ASSERT_TRUE(host.results.contains(21)) << host.failure;
	ASSERT_FALSE(host.results.at(21));
	ASSERT_TRUE(server.gh->gameState().actingPlayers.contains(red));
	ASSERT_TRUE(host.state->actingPlayers.contains(red));
	ASSERT_TRUE(guest.state->actingPlayers.contains(red));
	GiveBonus waterWalkBonus;
	waterWalkBonus.id = hero->id;
	waterWalkBonus.bonus = Bonus(BonusDuration::ONE_DAY, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID());
	server.gh->giveHeroBonus(&waterWalkBonus);
	pumpUntil([&] {
		return host.state->getHero(hero->id)->hasBonusOfType(BonusType::WATER_WALKING)
			&& guest.state->getHero(hero->id)->hasBonusOfType(BonusType::WATER_WALKING);
	});
	const int3 walking(0, 34, 0);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		const auto * replicaHero = peer->state->getHero(hero->id);
		ASSERT_TRUE(replicaHero->hasBonusOfType(BonusType::WATER_WALKING));
		CPathsInfo replicaPaths(peer->state->getMapSize(), replicaHero);
		auto replicaConfig = std::make_shared<SingleHeroPathfinderConfig>(replicaPaths, *peer->state, replicaHero);
		CPathfinder replicaSearch(*peer->state, replicaConfig);
		replicaSearch.calculatePaths();
		const auto * waterNode = replicaPaths.getNode(walking, EPathfindingLayer::WATER);
		ASSERT_TRUE(waterNode->reachable());
		ASSERT_NE(waterNode->theNodeBefore, nullptr);
		EXPECT_EQ(waterNode->theNodeBefore->coord, airborne);
		EXPECT_EQ(waterNode->theNodeBefore->layer, EPathfindingLayer::AIR);
	}
	MoveHero waterStep({hero->convertFromVisitablePos(walking)}, EPathfindingLayer::WATER, hero->id, true);
	waterStep.player = red;
	waterStep.requestID = 25;
	host.codec->sendPack(waterStep);
	pumpUntil([&] {
		return host.results.contains(25)
			&& host.state->getHero(hero->id)->visitablePos() == walking
			&& guest.state->getHero(hero->id)->visitablePos() == walking;
	});
	ASSERT_TRUE(host.results.contains(25)) << host.failure;
	ASSERT_TRUE(host.results.at(25));
	for(const LobbyPeer * peer : {&host, &guest})
		EXPECT_EQ(peer->state->getHero(hero->id)->getPathfindingStartLayer(), EPathfindingLayer::WATER);
	MoveHero land({hero->convertFromVisitablePos(destination)}, EPathfindingLayer::LAND, hero->id, false);
	land.player = red;
	land.requestID = 22;
	host.codec->sendPack(land);
	pumpUntil([&] {
		return host.results.contains(22)
			&& host.state->getHero(hero->id)->visitablePos() == destination
			&& guest.state->getHero(hero->id)->visitablePos() == destination;
	});
	ASSERT_TRUE(host.results.contains(22)) << host.failure;
	ASSERT_TRUE(host.results.at(22));
	for(const LobbyPeer * peer : {&host, &guest})
		ASSERT_EQ(peer->state->getHero(hero->id)->getPathfindingStartLayer(), EPathfindingLayer::LAND);

	EndTurn endTurn;
	endTurn.player = red;
	endTurn.requestID = 12;
	host.codec->sendPack(endTurn);
	pumpUntil([&] { return host.results.contains(12) && host.state->actingPlayers.contains(blue) && guest.state->actingPlayers.contains(blue); });
	ASSERT_TRUE(host.results.contains(12)) << host.failure;
	ASSERT_TRUE(host.results.at(12));
	ASSERT_TRUE(server.gh->gameState().actingPlayers.contains(blue));
	ASSERT_TRUE(host.state->actingPlayers.contains(blue));
	ASSERT_TRUE(guest.state->actingPlayers.contains(blue));
	const auto blueHeroes = server.gh->gameState().getHeroes(blue);
	ASSERT_EQ(blueHeroes.size(), 1);
	const auto * blueHero = blueHeroes.front();
	ASSERT_EQ(blueHero->visitablePos(), int3(0, 5, 0));
	const auto blueMovementBefore = blueHero->movementPointsRemaining();
	const auto visibility = [&](const CGameState & state)
	{
		std::vector<bool> result;
		for(const PlayerColor player : {red, blue})
			for(int x = 0; x < 36; ++x)
				for(int y = 0; y < 36; ++y)
					result.push_back(state.isVisibleFor(int3(x, y, 0), player));
		return result;
	};
	const auto serverFog = visibility(server.gh->gameState());
	const auto hostFog = visibility(*host.state);
	const auto guestFog = visibility(*guest.state);
	ASSERT_EQ(hostFog, serverFog);
	ASSERT_EQ(guestFog, serverFog);
	const int3 blockedDestination(35, 4, 0);
	ASSERT_FALSE(server.gh->gameState().getMap().getTile(blockedDestination).getTerrain()->isPassable());
	for(const LobbyPeer * peer : {&host, &guest})
	{
		ASSERT_EQ(peer->state->getHero(blueHero->id)->visitablePos(), int3(0, 5, 0));
		ASSERT_EQ(peer->state->getHero(blueHero->id)->movementPointsRemaining(), blueMovementBefore);
	}
	MoveHero blockedMove({blueHero->convertFromVisitablePos(blockedDestination)}, EPathfindingLayer::LAND, blueHero->id, false);
	blockedMove.player = blue;
	blockedMove.requestID = 14;
	guest.codec->sendPack(blockedMove);
	pumpUntil([&] { return guest.results.contains(14) && host.failedMoves.contains(blueHero->id) && guest.failedMoves.contains(blueHero->id); });
	ASSERT_TRUE(guest.results.contains(14)) << guest.failure;
	ASSERT_TRUE(host.failedMoves.contains(blueHero->id)) << host.failure;
	ASSERT_TRUE(guest.failedMoves.contains(blueHero->id)) << guest.failure;
	EXPECT_FALSE(guest.results.at(14));
	EXPECT_EQ(visibility(server.gh->gameState()), serverFog);
	EXPECT_EQ(visibility(*host.state), hostFog);
	EXPECT_EQ(visibility(*guest.state), guestFog);
	EXPECT_EQ(server.gh->gameState().getHero(blueHero->id)->visitablePos(), int3(0, 5, 0));
	EXPECT_EQ(server.gh->gameState().getHero(blueHero->id)->movementPointsRemaining(), blueMovementBefore);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_EQ(peer->state->getHero(blueHero->id)->visitablePos(), int3(0, 5, 0)) << peer->failure;
		EXPECT_EQ(peer->state->getHero(blueHero->id)->movementPointsRemaining(), blueMovementBefore);
	}
	const int3 blueDestination(35, 5, 0);
	MoveHero guestMove({blueHero->convertFromVisitablePos(blueDestination)}, EPathfindingLayer::LAND, blueHero->id, false);
	guestMove.player = blue;
	guestMove.requestID = 13;
	guest.codec->sendPack(guestMove);
	pumpUntil([&] {
		return guest.results.contains(13)
			&& host.state->getHero(blueHero->id)->visitablePos() == blueDestination
			&& guest.state->getHero(blueHero->id)->visitablePos() == blueDestination;
	});
	ASSERT_TRUE(guest.results.contains(13)) << guest.failure;
	EXPECT_TRUE(guest.results.at(13));
	EXPECT_EQ(server.gh->gameState().getHero(blueHero->id)->visitablePos(), blueDestination);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_EQ(peer->state->getHero(blueHero->id)->visitablePos(), blueDestination) << peer->failure;
		EXPECT_EQ(visibility(*peer->state), visibility(server.gh->gameState()));
	}
	EndTurn finishBlue;
	finishBlue.player = blue;
	finishBlue.requestID = 23;
	guest.codec->sendPack(finishBlue);
	pumpUntil([&] { return guest.results.contains(23) && host.state->actingPlayers.contains(red) && guest.state->actingPlayers.contains(red); });
	ASSERT_TRUE(guest.results.contains(23)) << guest.failure;
	ASSERT_TRUE(guest.results.at(23));
	EXPECT_FALSE(server.gh->gameState().getHero(hero->id)->hasBonusOfType(BonusType::FLYING_MOVEMENT));
	EXPECT_FALSE(server.gh->gameState().getHero(hero->id)->hasBonusOfType(BonusType::WATER_WALKING));
	for(const LobbyPeer * peer : {&host, &guest})
	{
		const auto * replicaHero = peer->state->getHero(hero->id);
		EXPECT_FALSE(replicaHero->hasBonusOfType(BonusType::FLYING_MOVEMENT));
		EXPECT_FALSE(replicaHero->hasBonusOfType(BonusType::WATER_WALKING));
		EXPECT_EQ(replicaHero->getPathfindingStartLayer(), EPathfindingLayer::LAND);
	}

	// Expire a real server timer while the hero is airborne over water.
	GiveBonus timedFlight;
	timedFlight.id = hero->id;
	timedFlight.bonus = Bonus(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID());
	server.gh->giveHeroBonus(&timedFlight);
	server.gh->setMovePoints(hero->id, 300);
	FowTilesType landingArea;
	const auto topology = server.gh->gameState().getMap().getTopology();
	for(int dx = -3; dx <= 3; ++dx)
		for(int dy = -3; dy <= 3; ++dy)
			landingArea.insert(*topology.canonicalize(airborne + int3(dx, dy, 0)));
	server.gh->changeFogOfWar(landingArea, red, ETileVisibility::REVEALED);
	pumpUntil([&] {
		return host.state->getHero(hero->id)->movementPointsRemaining() == 300
			&& guest.state->getHero(hero->id)->movementPointsRemaining() == 300
			&& host.state->getHero(hero->id)->hasBonusOfType(BonusType::FLYING_MOVEMENT)
			&& guest.state->getHero(hero->id)->hasBonusOfType(BonusType::FLYING_MOVEMENT)
			&& std::all_of(landingArea.begin(), landingArea.end(), [&](const int3 & tile) {
				return host.state->isVisibleFor(tile, red) && guest.state->isVisibleFor(tile, red);
			});
	});
	auto * startInfo = server.gh->gameInfo().gameState().getStartInfo();
	// Keep the live timer from expiring while the TCP movement is in flight;
	// the two explicit ticks below exercise the countdown deterministically.
	startInfo->turnTimerInfo.turnTimer = 100000;
	startInfo->turnTimerInfo.baseTimer = 0;
	server.gh->turnTimerHandler->onGameplayStart(red);
	server.gh->turnTimerHandler->onGameplayStart(blue);
	server.gh->turnTimerHandler->onPlayerGetTurn(red);
	flight.requestID = 26;
	host.codec->sendPack(flight);
	pumpUntil([&] {
		return host.results.contains(26)
			&& host.state->getHero(hero->id)->visitablePos() == airborne
			&& guest.state->getHero(hero->id)->visitablePos() == airborne;
	});
	ASSERT_TRUE(host.results.contains(26)) << host.failure;
	ASSERT_TRUE(host.results.at(26));
	ASSERT_EQ(server.gh->gameState().getHero(hero->id)->getPathfindingStartLayer(), EPathfindingLayer::AIR);
	startInfo->turnTimerInfo.turnTimer = 1;
	server.gh->turnTimerHandler->onPlayerGetTurn(red);
	server.gh->turnTimerHandler->update(1);
	ASSERT_EQ(server.gh->gameState().getHero(hero->id)->visitablePos(), airborne);
	server.gh->turnTimerHandler->update(1);
	pumpUntil([&] {
		return host.state->getHero(hero->id)->getPathfindingStartLayer() == EPathfindingLayer::LAND
			&& guest.state->getHero(hero->id)->getPathfindingStartLayer() == EPathfindingLayer::LAND;
	});
	const auto * landed = server.gh->gameState().getHero(hero->id);
	ASSERT_EQ(landed->getPathfindingStartLayer(), EPathfindingLayer::LAND);
	ASSERT_NE(landed->visitablePos(), airborne);
	EXPECT_TRUE(int3(0, 0, 0).areNeighbours(topology.displacement(airborne, landed->visitablePos())));
	EXPECT_EQ(landed->movementPointsRemaining(), 100);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_EQ(peer->state->getHero(hero->id)->visitablePos(), landed->visitablePos());
		EXPECT_EQ(peer->state->getHero(hero->id)->movementPointsRemaining(), landed->movementPointsRemaining());
	}
	// After the landing, the next timer tick must actually transfer the turn.
	// Give the next player enough time to submit its own end-turn request.
	startInfo->turnTimerInfo.turnTimer = 100000;
	server.gh->turnTimerHandler->update(1);
	pumpUntil([&] { return host.state->actingPlayers.contains(blue) && guest.state->actingPlayers.contains(blue); });
	ASSERT_TRUE(server.gh->gameState().actingPlayers.contains(blue));
	ASSERT_TRUE(host.state->actingPlayers.contains(blue)) << host.failure;
	ASSERT_TRUE(guest.state->actingPlayers.contains(blue)) << guest.failure;
	const auto blueTurnPause = server.gh->activities->topActivity(blue);
	ASSERT_NE(blueTurnPause, nullptr);
	QuestionAnswer acceptBlueTurn(blueTurnPause->getActiveQuestionID(), 0);
	acceptBlueTurn.player = blue;
	acceptBlueTurn.requestID = 29;
	guest.codec->sendPack(acceptBlueTurn);
	pumpUntil([&] { return guest.results.contains(29); });
	ASSERT_TRUE(guest.results.contains(29));
	ASSERT_TRUE(guest.results.at(29));
	// The remaining test covers a manual turn without a timer.
	startInfo->turnTimerInfo.turnTimer = 0;
	EndTurn finishBlueAfterTimer;
	finishBlueAfterTimer.player = blue;
	finishBlueAfterTimer.requestID = 28;
	guest.codec->sendPack(finishBlueAfterTimer);
	pumpUntil([&] { return guest.results.contains(28) && host.state->actingPlayers.contains(red) && guest.state->actingPlayers.contains(red); });
	ASSERT_TRUE(guest.results.contains(28)) << guest.failure;
	ASSERT_TRUE(guest.results.at(28)) << testing::PrintToString(guest.systemMessages);
	// Exercise manual end turn and the in-place landing packet over TCP.
	// Establish the AIR state synthetically; landing is server-driven.
	TryMoveHero stageAir;
	stageAir.id = hero->id;
	stageAir.start = landed->anchorPos();
	stageAir.end = stageAir.start;
	stageAir.movePoints = landed->movementPointsRemaining();
	stageAir.result = TryMoveHero::SUCCESS;
	stageAir.movementLayer = EPathfindingLayer::AIR;
	server.gh->sendAndApply(stageAir);
	pumpUntil([&] {
		return host.state->getHero(hero->id)->getConfirmedMovementLayer() == EPathfindingLayer::AIR
			&& guest.state->getHero(hero->id)->getConfirmedMovementLayer() == EPathfindingLayer::AIR;
	});
	ASSERT_EQ(host.state->getHero(hero->id)->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	ASSERT_EQ(guest.state->getHero(hero->id)->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	const auto landingPosition = landed->visitablePos();
	const auto landingPoints = landed->movementPointsRemaining();
	EndTurn finishRedOnLand;
	finishRedOnLand.player = red;
	finishRedOnLand.requestID = 27;
	host.codec->sendPack(finishRedOnLand);
	pumpUntil([&] {
		return host.results.contains(27) && host.state->actingPlayers.contains(blue)
			&& guest.state->actingPlayers.contains(blue)
			&& host.state->getHero(hero->id)->getConfirmedMovementLayer() == EPathfindingLayer::LAND
			&& guest.state->getHero(hero->id)->getConfirmedMovementLayer() == EPathfindingLayer::LAND;
	});
	ASSERT_TRUE(host.results.contains(27)) << host.failure;
	ASSERT_TRUE(host.results.at(27));
	for(const LobbyPeer * peer : {&host, &guest})
	{
		const auto * replica = peer->state->getHero(hero->id);
		EXPECT_EQ(replica->getConfirmedMovementLayer(), EPathfindingLayer::LAND) << peer->failure;
		EXPECT_EQ(replica->visitablePos(), landingPosition);
		EXPECT_EQ(replica->movementPointsRemaining(), landingPoints);
	}
	ASSERT_TRUE(server.gh->gameState().actingPlayers.contains(blue));
	ASSERT_TRUE(host.state->actingPlayers.contains(blue));
	ASSERT_TRUE(guest.state->actingPlayers.contains(blue));

	// Re-enter AIR through the network, then simulate exhausted movement and
	// end the turn. Both peers must apply the emergency return and zero points.
	EndTurn finishBlueBeforeRescue;
	finishBlueBeforeRescue.player = blue;
	finishBlueBeforeRescue.requestID = 30;
	guest.codec->sendPack(finishBlueBeforeRescue);
	pumpUntil([&] { return guest.results.contains(30) && host.state->actingPlayers.contains(red) && guest.state->actingPlayers.contains(red); });
	ASSERT_TRUE(guest.results.contains(30));
	ASSERT_TRUE(guest.results.at(30));
	const auto rescueOrigin = server.gh->gameState().getHero(hero->id)->visitablePos();
	std::optional<int3> rescueWater;
	for(int dx = -1; dx <= 1 && !rescueWater; ++dx)
		for(int dy = -1; dy <= 1; ++dy)
		{
			const auto candidate = topology.canonicalize(rescueOrigin + int3(dx, dy, 0));
			if(candidate && server.gh->gameState().getMap().getTile(*candidate).isWater())
			{
				rescueWater = candidate;
				break;
			}
		}
	ASSERT_TRUE(rescueWater.has_value());
	server.gh->setMovePoints(hero->id, 500);
	pumpUntil([&] { return host.state->getHero(hero->id)->movementPointsRemaining() == 500; });
	MoveHero departForRescue({hero->convertFromVisitablePos(*rescueWater)}, EPathfindingLayer::AIR, hero->id, true);
	departForRescue.player = red;
	departForRescue.requestID = 31;
	host.codec->sendPack(departForRescue);
	pumpUntil([&] { return host.results.contains(31) && guest.state->getHero(hero->id)->visitablePos() == *rescueWater; });
	ASSERT_TRUE(host.results.contains(31));
	ASSERT_TRUE(host.results.at(31));
	server.gh->setMovePoints(hero->id, 1);
	pumpUntil([&] { return host.state->getHero(hero->id)->movementPointsRemaining() == 1 && guest.state->getHero(hero->id)->movementPointsRemaining() == 1; });
	EndTurn finishStranded;
	finishStranded.player = red;
	finishStranded.requestID = 32;
	host.codec->sendPack(finishStranded);
	pumpUntil([&] {
		return host.results.contains(32) && host.state->actingPlayers.contains(blue) && guest.state->actingPlayers.contains(blue)
			&& host.state->getHero(hero->id)->visitablePos() == rescueOrigin && guest.state->getHero(hero->id)->visitablePos() == rescueOrigin;
	});
	ASSERT_TRUE(host.results.contains(32)) << host.failure;
	ASSERT_TRUE(host.results.at(32)) << testing::PrintToString(host.systemMessages);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		EXPECT_EQ(peer->state->getHero(hero->id)->visitablePos(), rescueOrigin);
		EXPECT_EQ(peer->state->getHero(hero->id)->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
		EXPECT_EQ(peer->state->getHero(hero->id)->movementPointsRemaining(), 0);
	}

	// A synthetic replicated movement packet creates a stranded AIR state
	// without a recorded origin, as in an older save. End turn must replicate
	// the fallback landing and preserve the already revealed fog on both peers.
	EndTurn finishBlueBeforeFallback;
	finishBlueBeforeFallback.player = blue;
	finishBlueBeforeFallback.requestID = 34;
	guest.codec->sendPack(finishBlueBeforeFallback);
	pumpUntil([&] { return guest.results.contains(34) && host.state->actingPlayers.contains(red) && guest.state->actingPlayers.contains(red); });
	ASSERT_TRUE(guest.results.contains(34)) << guest.failure;
	ASSERT_TRUE(guest.results.at(34));
	TryMoveHero stageWithoutOrigin;
	stageWithoutOrigin.id = hero->id;
	stageWithoutOrigin.start = server.gh->gameState().getHero(hero->id)->anchorPos();
	stageWithoutOrigin.end = hero->convertFromVisitablePos(*rescueWater);
	stageWithoutOrigin.result = TryMoveHero::TELEPORTATION;
	stageWithoutOrigin.movePoints = 1;
	stageWithoutOrigin.movementLayer = EPathfindingLayer::AIR;
	server.gh->sendAndApply(stageWithoutOrigin);
	pumpUntil([&] {
		return host.state->getHero(hero->id)->visitablePos() == *rescueWater
			&& guest.state->getHero(hero->id)->visitablePos() == *rescueWater;
	});
	ASSERT_EQ(host.state->getHero(hero->id)->visitablePos(), *rescueWater);
	ASSERT_EQ(guest.state->getHero(hero->id)->visitablePos(), *rescueWater);
	const auto fogBeforeFallback = redVisibility(server.gh->gameState());
	ASSERT_EQ(redVisibility(*host.state), fogBeforeFallback);
	ASSERT_EQ(redVisibility(*guest.state), fogBeforeFallback);
	EndTurn finishWithoutOrigin;
	finishWithoutOrigin.player = red;
	finishWithoutOrigin.requestID = 35;
	host.codec->sendPack(finishWithoutOrigin);
	pumpUntil([&] {
		return host.results.contains(35) && host.state->actingPlayers.contains(blue) && guest.state->actingPlayers.contains(blue);
	});
	ASSERT_TRUE(host.results.contains(35)) << host.failure;
	ASSERT_TRUE(host.results.at(35)) << testing::PrintToString(host.systemMessages);
	const auto * fallbackHero = server.gh->gameState().getHero(hero->id);
	ASSERT_NE(fallbackHero, nullptr);
	const auto fallbackPosition = fallbackHero->visitablePos();
	EXPECT_NE(fallbackPosition, *rescueWater);
	EXPECT_TRUE(server.gh->gameState().getMap().getTile(fallbackPosition).isLand());
	EXPECT_EQ(fallbackHero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_EQ(fallbackHero->movementPointsRemaining(), 0);
	EXPECT_EQ(redVisibility(server.gh->gameState()), fogBeforeFallback);
	for(const LobbyPeer * peer : {&host, &guest})
	{
		const auto * replica = peer->state->getHero(hero->id);
		ASSERT_NE(replica, nullptr);
		EXPECT_EQ(replica->visitablePos(), fallbackPosition);
		EXPECT_EQ(replica->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
		EXPECT_EQ(replica->movementPointsRemaining(), 0);
		EXPECT_EQ(redVisibility(*peer->state), fogBeforeFallback);
	}
}

TEST(ToroidalDemoVmapTest, DecodesExternalUntaggedPrototypeSave)
{
	const char * suppliedPath = std::getenv("VCMI_TOROID_PROTOTYPE_SAVE");
	if(!suppliedPath || !*suppliedPath)
		GTEST_SKIP() << "Set VCMI_TOROID_PROTOTYPE_SAVE to a branch-era saved game";
	const boost::filesystem::path path(suppliedPath);
	ASSERT_TRUE(boost::filesystem::is_regular_file(path));
	ASSERT_EQ(CLoadFile::readVersion(path), ESerializationVersion::TOROIDAL_MAP);
	std::ifstream file(path.c_str(), std::ios::binary);
	std::array<std::byte, SaveFileFormat::FILE_HEADER_SIZE> fileHeader{};
	file.read(reinterpret_cast<char *>(fileHeader.data()), fileHeader.size());
	ASSERT_TRUE(file.good());
	ASSERT_EQ(SaveFileFormat::readUInt32(fileHeader.data() + SaveFileFormat::FILE_MAGIC.size()), static_cast<uint32_t>(ESerializationVersion::TOROIDAL_MAP));
	CLoadFile saved(path);
	CMapHeader header;
	StartInfo info;
	auto headerChunk = saved.openChunk(SaveFileFormat::CHUNK_HEADER, nullptr);
	headerChunk->iser & header;
	headerChunk->iser & info;
	ASSERT_TRUE(headerChunk->isFullyRead());
	ASSERT_TRUE(header.toroidal);
	// The global test preset may not match the save's mods. Decode the chunks
	// directly without claiming the normal mod-compatibility gate accepted it.
	auto state = std::make_shared<CGameState>();
	GameHandlerTestServer server(state);
	CGameHandler handler(server, state);
	auto game = saved.openChunk(SaveFileFormat::CHUNK_GAMESTATE, state.get());
	ASSERT_NO_THROW(state->loadGame(game->iser));
	ASSERT_NO_THROW(game->iser & handler);
	EXPECT_TRUE(game->isFullyRead());
	EXPECT_TRUE(state->getMap().toroidal);
	EXPECT_EQ(state->getMap().width, header.width);
	EXPECT_EQ(state->getMap().height, header.height);
}

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

TEST_F(ToroidalDemoGameTest, ServerSaveUsesTheTopologySpecificSerializationVersion)
{
	const PlayerColor player(0);
	startWithMap(makeToroidalDemoMap());
	const auto directory = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-server-save-%%%%-%%%%");
	boost::filesystem::create_directories(directory / "Saves");
	boost::filesystem::create_directories(directory / "config");
	{
		std::ofstream config((directory / "config/serverSettings.json").c_str());
		config << "{}";
	}
	auto * local = dynamic_cast<CFilesystemList *>(CResourceHandler::get("local"));
	ASSERT_NE(local, nullptr);
	auto loader = std::make_shared<CFilesystemLoader>("", directory, 4);
	local->addLoader(loader, true);
	struct TemporaryFilesystem
	{
		CFilesystemList * local;
		ISimpleResourceLoader * loader;
		boost::filesystem::path directory;
		~TemporaryFilesystem()
		{
			local->removeLoader(loader);
			boost::filesystem::remove_all(directory);
		}
	} temporary{local, loader.get(), directory};
	const auto configPath = CResourceHandler::get()->getResourceName(JsonPath::builtin("config/serverSettings.json"));
	ASSERT_TRUE(configPath.has_value());
	ASSERT_TRUE(boost::filesystem::equivalent(*configPath, directory / "config/serverSettings.json"));

	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	for(const int scenario : {0, 1, 2})
	{
		const bool toroidal = scenario == 1;
		map()->setToroidalAndRebuild(toroidal);
		ObjectInstanceID shownID, hiddenID;
		ObjectInstanceID airborneHeroID;
		if(scenario == 1)
		{
			auto * hero = findHeroByOwner(player);
			ASSERT_NE(hero, nullptr);
			auto resources = findAll<CGResource>();
			ASSERT_FALSE(resources.empty());
			auto * resource = resources.front();
			map()->moveObject(hero->id, hero->convertFromVisitablePos(int3(1, 0, 0)));
			map()->hideObject(resource);
			const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "A" ] })";
			JsonNode definition(text.data(), text.size(), "SavedAirborneSource");
			auto appearance = std::make_shared<ObjectTemplate>();
			appearance->id = resource->ID;
			appearance->subid = resource->subID;
			InstantIdentifierResolver resolver(*LIBRARY->identifiers());
			appearance->readJson(definition, resolver, false);
			resource->appearance = appearance;
			resource->pos = int3(0, 0, 0);
			map()->showObject(resource);
			hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
			hero->setMovementPoints(3000);
			ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
			airborneHeroID = hero->id;
		}
		if(scenario == 2)
		{
			std::vector<CGObjectInstance *> resources;
			for(const auto & object : map()->objects)
				if(object && object->ID == Obj::RESOURCE)
					resources.push_back(object.get());
			ASSERT_GE(resources.size(), 2u);
			shownID = resources[0]->id;
			hiddenID = resources[1]->id;
			map()->moveObject(shownID, int3(-2, 10, 0));
			map()->moveObject(hiddenID, int3(-2, 12, 0));
			map()->hideObject(resources[1]);
			ASSERT_TRUE(map()->hasUnindexedShownObjects());
		}
		const std::string name = "Saves/" + directory.filename().string() + "-" + std::to_string(scenario);
		const auto path = directory / (name + ".vsgm1");
		ASSERT_TRUE(loader->createResource(name + ".vsgm1"));
		ASSERT_TRUE(local->createResource(name + ".vsgm1"));
		const auto localPath = local->getResourceName(ResourcePath(name, EResType::SAVEGAME));
		ASSERT_TRUE(localPath.has_value());
		ASSERT_TRUE(boost::filesystem::equivalent(*localPath, path));
		const auto selectedPath = CResourceHandler::get()->getResourceName(ResourcePath(name, EResType::SAVEGAME));
		ASSERT_TRUE(selectedPath.has_value());
		ASSERT_TRUE(boost::filesystem::equivalent(*selectedPath, path));
		handler.save(name, PlayerColor::NEUTRAL, 0);
		ASSERT_TRUE(boost::filesystem::exists(path));
		if(scenario == 1)
		{
			// The immediately preceding branch save stored the layer only in
			// server state. Its normal load must populate the replicated hero.
			const auto priorVersion = ESerializationVersion::TOROIDAL_MOVEMENT_SOURCE_LAYER;
			const std::string priorName = name + "-prior";
			const auto priorPath = directory / (priorName + ".vsgm1");
			ASSERT_TRUE(loader->createResource(priorName + ".vsgm1"));
			CMemorySerializer priorHeader, priorGame;
			priorHeader.oser.version = priorVersion;
			priorGame.oser.version = priorVersion;
			gameState()->saveHeader(priorHeader.oser);
			gameState()->saveGame(priorGame.oser);
			priorGame.oser & handler;
			CSaveFile priorSave(priorVersion);
			priorSave.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*gameState()).toBytes());
			priorSave.addChunk(SaveFileFormat::CHUNK_HEADER, priorHeader.extractBuffer());
			priorSave.addChunk(SaveFileFormat::CHUNK_GAMESTATE, priorGame.extractBuffer());
			priorSave.write(priorPath);
			StartInfo priorInfo = *gameState()->getStartInfo();
			priorInfo.mapname = priorName;
			CGameHandler priorRestored(server, gameState());
			ASSERT_NO_THROW(priorRestored.load(priorInfo));
			const auto * priorHero = priorRestored.gs->getHero(airborneHeroID);
			ASSERT_NE(priorHero, nullptr);
			EXPECT_EQ(priorHero->getPathfindingStartLayer(), EPathfindingLayer::AIR);

			// Write and reopen the last server-state version before transit origins
			// existed. Then construct a stranded hero with an unsuitable origin.
			const auto noOriginVersion = ESerializationVersion::TOROIDAL_REPLICATED_MOVEMENT_LAYER;
			const auto noOriginPath = directory / (name + "-no-origin.vsgm1");
			CMemorySerializer noOriginHeader, noOriginGame;
			noOriginHeader.oser.version = noOriginVersion;
			noOriginGame.oser.version = noOriginVersion;
			gameState()->saveHeader(noOriginHeader.oser);
			gameState()->saveGame(noOriginGame.oser);
			noOriginGame.oser & handler;
			CSaveFile noOriginSave(noOriginVersion);
			noOriginSave.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*gameState()).toBytes());
			noOriginSave.addChunk(SaveFileFormat::CHUNK_HEADER, noOriginHeader.extractBuffer());
			noOriginSave.addChunk(SaveFileFormat::CHUNK_GAMESTATE, noOriginGame.extractBuffer());
			noOriginSave.write(noOriginPath);
			ASSERT_EQ(CLoadFile::readVersion(noOriginPath), noOriginVersion);
			CLoadFile noOriginLoaded(noOriginPath);
			auto noOriginState = std::make_shared<CGameState>();
			noOriginState->preInit(LIBRARY);
			auto noOriginChunk = noOriginLoaded.openChunk(SaveFileFormat::CHUNK_GAMESTATE, noOriginState.get());
			noOriginState->loadGame(noOriginChunk->iser);
			GameHandlerTestServer noOriginServer(noOriginState, player);
			CGameHandler noOriginRecovery(noOriginServer, noOriginState);
			noOriginChunk->iser & noOriginRecovery;
			ASSERT_TRUE(noOriginChunk->isFullyRead());
			auto * noOriginHero = noOriginState->getHero(airborneHeroID);
			ASSERT_NE(noOriginHero, nullptr);
			ASSERT_EQ(noOriginHero->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
			noOriginHero->setMovementPoints(1);
			noOriginState->getMap().getTile(int3(1, 0, 0)).terrainType = TerrainId::WATER;
			auto * noOriginTeam = noOriginState->getPlayerTeam(player);
			ASSERT_NE(noOriginTeam, nullptr);
			for(int z = 0; z < noOriginState->getMap().levels(); ++z)
				for(int x = 0; x < noOriginState->getMap().width; ++x)
					for(int y = 0; y < noOriginState->getMap().height; ++y)
						noOriginTeam->fogOfWarMap[int3(x, y, z)] = 1;
			ASSERT_TRUE(noOriginRecovery.tryRecoverTransientHeroes(player));
			EXPECT_NE(noOriginHero->visitablePos(), int3(0, 0, 0));
			EXPECT_NE(noOriginHero->visitablePos(), int3(1, 0, 0));
			EXPECT_TRUE(noOriginState->getMap().getTile(noOriginHero->visitablePos()).isLand());
			EXPECT_EQ(noOriginHero->visitablePos().z, 0);
			EXPECT_EQ(noOriginHero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
			EXPECT_EQ(noOriginHero->movementPointsRemaining(), 0);
		}
		const auto expectedVersion = scenario == 0 ? ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER : ESerializationVersion::CURRENT;
		EXPECT_EQ(CLoadFile::readVersion(path), expectedVersion);
		std::ifstream file(path.c_str(), std::ios::binary);
		std::array<std::byte, SaveFileFormat::FILE_HEADER_SIZE> fileHeader{};
		file.read(reinterpret_cast<char *>(fileHeader.data()), fileHeader.size());
		ASSERT_TRUE(file.good());
		EXPECT_EQ(SaveFileFormat::readUInt32(fileHeader.data() + SaveFileFormat::FILE_MAGIC.size()), SaveFileFormat::encodeVersion(expectedVersion));
		CLoadFile loaded(path);
		CMapHeader loadedHeader;
		StartInfo loadedStart;
		auto header = loaded.openChunk(SaveFileFormat::CHUNK_HEADER, nullptr);
		header->iser & loadedHeader;
		header->iser & loadedStart;
		EXPECT_TRUE(header->isFullyRead());
		EXPECT_EQ(loadedHeader.toroidal, toroidal);
		auto loadedState = std::make_shared<CGameState>();
		loadedState->preInit(LIBRARY);
		auto game = loaded.openChunk(SaveFileFormat::CHUNK_GAMESTATE, loadedState.get());
		loadedState->loadGame(game->iser);
		EXPECT_EQ(loadedState->getMap().toroidal, toroidal);
		if(scenario == 1)
		{
			// Reopen the actual file in a separate state to exercise the persisted
			// departure point without changing the ordinary landing check below.
			auto recoveryState = std::make_shared<CGameState>();
			recoveryState->preInit(LIBRARY);
			auto recoveryChunk = loaded.openChunk(SaveFileFormat::CHUNK_GAMESTATE, recoveryState.get());
			recoveryState->loadGame(recoveryChunk->iser);
			GameHandlerTestServer recoveryServer(recoveryState, player);
			CGameHandler recovery(recoveryServer, recoveryState);
			recoveryChunk->iser & recovery;
			ASSERT_TRUE(recoveryChunk->isFullyRead());
			auto * recoveryHero = recoveryState->getHero(airborneHeroID);
			ASSERT_NE(recoveryHero, nullptr);
			ASSERT_EQ(recoveryHero->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
			recoveryHero->setMovementPoints(1); // simulate a stranded loaded hero
			auto * team = recoveryState->getPlayerTeam(player);
			ASSERT_NE(team, nullptr);
			for(int z = 0; z < recoveryState->getMap().levels(); ++z)
				for(int x = 0; x < recoveryState->getMap().width; ++x)
					for(int y = 0; y < recoveryState->getMap().height; ++y)
						team->fogOfWarMap[int3(x, y, z)] = 1;
			ASSERT_TRUE(recovery.tryRecoverTransientHeroes(player));
			EXPECT_EQ(recoveryHero->visitablePos(), int3(1, 0, 0));
			EXPECT_EQ(recoveryHero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
			EXPECT_EQ(recoveryHero->movementPointsRemaining(), 0);

			GameHandlerTestServer restoredServer(loadedState, player);
			CGameHandler restored(restoredServer, loadedState);
			game->iser & restored;
			ASSERT_TRUE(game->isFullyRead());
			auto * hero = loadedState->getHero(airborneHeroID);
			ASSERT_NE(hero, nullptr);
			ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
			EXPECT_EQ(hero->getPathfindingStartLayer(), EPathfindingLayer::AIR);
			ASSERT_TRUE(restored.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
			EXPECT_EQ(hero->visitablePos(), int3(35, 0, 0));
		}
		if(scenario == 2)
		{
			auto & restoredMap = loadedState->getMap();
			ASSERT_TRUE(restoredMap.hasUnindexedShownObjects());
			const auto shownPos = restoredMap.getObject(shownID)->visitablePos();
			const auto hiddenPos = restoredMap.getObject(hiddenID)->visitablePos();
			restoredMap.setToroidalAndRebuild(true);
			const auto shownTile = restoredMap.getTopology().canonicalize(shownPos);
			const auto hiddenTile = restoredMap.getTopology().canonicalize(hiddenPos);
			ASSERT_TRUE(shownTile);
			ASSERT_TRUE(hiddenTile);
			EXPECT_TRUE(vstd::contains(restoredMap.getTile(*shownTile).visitableObjects, shownID));
			EXPECT_FALSE(vstd::contains(restoredMap.getTile(*hiddenTile).visitableObjects, hiddenID));
		}
		StartInfo loadInfo = *gameState()->getStartInfo();
		loadInfo.mapname = name;
		CGameHandler reloaded(server, gameState());
		ASSERT_NO_THROW(reloaded.load(loadInfo));
		EXPECT_EQ(reloaded.gameInfo().getMapHeader()->toroidal, toroidal);
	}
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
	ToroidalMemoryMapService service(archive.getBuffer());
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

TEST(ToroidalDemoVmapTest, SavedWrappedResourceCanBeCollected)
{
	const PlayerColor player(0);
	auto source = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false).playerActive(player)
		.hero(int3(35, 10, 0), HeroTypeID(0), player)
		.resource(int3(15, 15, 0), GameResID::GOLD, 10).build();
	CMemoryBuffer legacyBytes;
	legacyBytes.write(source.data(), static_cast<si64>(source.size()));
	legacyBytes.seek(0);
	EditorCallback editor(nullptr);
	CMapLoaderH3M legacyLoader("WrappedResource", "core", "ASCII", &legacyBytes);
	auto legacy = legacyLoader.loadMap(&editor);
	editor.setMap(legacy.get());
	CGResource * resource = nullptr;
	for(const auto & object : legacy->objects)
		if(auto * candidate = dynamic_cast<CGResource *>(object.get()))
			resource = candidate;
	ASSERT_NE(resource, nullptr);
	const std::string text = R"({ "animation": "test.def", "visitableFrom": ["+++", "+++", "+++"], "mask": [ "A0" ] })";
	JsonNode definition(text.data(), text.size(), "WrappedResource");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	legacy->hideObject(resource);
	resource->pos = int3(0, 10, 0);
	resource->appearance = appearance;
	legacy->showObject(resource);
	legacy->setToroidalAndRebuild(true, CMap::TopologyRebuildObjects::AllMapObjects);
	CMemoryBuffer archive;
	{
		CMapSaverJson saver(&archive);
		ASSERT_NO_THROW(saver.saveMap(legacy));
	}
	ToroidalMemoryMapService service(archive.getBuffer());
	auto header = service.loadMapHeader(ResourcePath("memory"));
	ASSERT_TRUE(header->toroidal);
	StartInfo info;
	info.mapname = "memory";
	info.difficulty = static_cast<ui8>(EMapDifficulty::EASY);
	info.mode = EStartMode::NEW_GAME;
	const auto & playerInfo = header->players[player.getNum()];
	auto & settings = info.playerInfos[player];
	settings.color = player;
	settings.connectedPlayerIDs.insert(static_cast<PlayerConnectionID>(0));
	settings.name = "Player";
	settings.castle = playerInfo.defaultCastle();
	settings.hero = playerInfo.defaultHero();
	settings.bonus = PlayerStartingBonus::GOLD;
	auto state = std::make_shared<CGameState>();
	state->preInit(LIBRARY);
	GameRandomizer randomizer(*state);
	Load::ProgressAccumulator progress;
	ASSERT_NO_THROW(state->init(&service, &info, randomizer, progress, false));
	CGHeroInstance * hero = nullptr;
	resource = nullptr;
	for(const auto & object : state->getMap().objects)
	{
		if(auto * candidate = dynamic_cast<CGHeroInstance *>(object.get()); candidate && candidate->getOwner() == player)
			hero = candidate;
		if(auto * candidate = dynamic_cast<CGResource *>(object.get()))
			resource = candidate;
	}
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(resource, nullptr);
	const int3 destination(35, 10, 0);
	ASSERT_EQ(hero->visitablePos(), int3(34, 10, 0));
	ASSERT_EQ(resource->visitablePos(), destination);
	ASSERT_EQ(state->getMap().getTile(destination).topVisitableObj(), resource->id);
	const auto resourceID = resource->id;
	const auto gold = state->players.at(player).resources[GameResID::GOLD];
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(state, player);
	CGameHandler handler(server, state);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(state->players.at(player).resources[GameResID::GOLD], gold + 1000);
	EXPECT_EQ(state->getMap().getObject(resourceID), nullptr);
}

class GrailTopologyTest : public TinyMapGameTest
{
protected:
	bool toroidal = true;
	bool unspecified = false;
	bool wrappedHole = false;

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
		if(wrappedHole)
		{
			const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "V" ] })";
			JsonNode definition(text.data(), text.size(), "WrappedGrailHoleTest");
			auto appearance = std::make_shared<ObjectTemplate>();
			InstantIdentifierResolver resolver(*LIBRARY->identifiers());
			appearance->readJson(definition, resolver, false);
			appearance->id = Obj::HOLE;
			appearance->subid = MapObjectSubID(0);
			auto hole = std::make_shared<CGObjectInstance>(gameState().get());
			hole->ID = Obj::HOLE;
			hole->appearance = appearance;
			hole->pos = int3(-1, 10, 0);
			hole->instanceName = "wrapped_grail_hole";
			loaded->addNewObject(hole);
		}
	}

	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false).playerActive(PlayerColor(0)).hero(int3(6, 5, 0), HeroTypeID(0), PlayerColor(0));
		startWithMap(std::move(builder));
	}
};

class FavorableWindsTopologyTest : public TinyMapGameTest
{
protected:
	bool toroidal = true;
	bool sailing = false;

	const Services * gameServices() override { return LIBRARY; }
	void mapLoaded(CMap * loaded) override
	{
		TinyMapGameTest::mapLoaded(loaded);
		loaded->setToroidalAndRebuild(toroidal);
	}

	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false).playerActive(PlayerColor(0)).hero(sailing ? int3(2, 35, 0) : int3(11, 10, 0), HeroTypeID(0), PlayerColor(0))
			.terrainOverlay(int3(0, 0, 0), Obj::FAVORABLE_WINDS);
		if(sailing)
			builder.terrain(int3(0, 35, 0), TerrainId::WATER).terrain(int3(35, 35, 0), TerrainId::WATER);
		startWithMap(std::move(builder));
	}
};

TEST_F(FavorableWindsTopologyTest, InitializesWindAreaAcrossCorner)
{
	startGame();
	const auto objects = map()->getObjects();
	const auto winds = std::ranges::find_if(objects, [](const auto & object)
	{
		return object && object->ID == Obj::FAVORABLE_WINDS;
	});
	ASSERT_NE(winds, objects.end());
	ASSERT_GE((*winds)->getWidth(), 2);
	ASSERT_GE((*winds)->getHeight(), 2);
	for(const auto & tile : {int3(0, 0, 0), int3(35, 0, 0), int3(0, 35, 0), int3(35, 35, 0)})
		EXPECT_NE(map()->getTile(tile).extTileFlags & 128, 0) << tile.toString();
	EXPECT_EQ(map()->getTile(int3(15, 15, 0)).extTileFlags & 128, 0);
}

TEST_F(FavorableWindsTopologyTest, BoundedMapStillClipsWindArea)
{
	toroidal = false;
	startGame();
	EXPECT_NE(map()->getTile(int3(0, 0, 0)).extTileFlags & 128, 0);
	for(const auto & tile : {int3(35, 0, 0), int3(0, 35, 0), int3(35, 35, 0)})
		EXPECT_EQ(map()->getTile(tile).extTileFlags & 128, 0) << tile.toString();
}

TEST_F(FavorableWindsTopologyTest, WindChangesSailingCostAcrossSeam)
{
	sailing = true;
	startGame();
	const PlayerColor player(0);
	const int3 source(0, 35, 0);
	const int3 destination(35, 35, 0);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(source, BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(source), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	hero->setMovementPoints(3000);
	const TerrainTile windy = map()->getTile(source);
	ASSERT_TRUE(windy.hasFavorableWinds());
	TerrainTile calm = windy;
	calm.extTileFlags &= ~128;
	const TerrainTile destinationTile = map()->getTile(destination);
	CPathfinderHelper helper(*gameState(), hero, PathfinderOptions(*gameState()));
	const int windyCost = helper.getMovementCost(source, destination, EPathfindingLayer::SAIL, 3000, false, &windy, &destinationTile);
	const int calmCost = helper.getMovementCost(source, destination, EPathfindingLayer::SAIL, 3000, false, &calm, &destinationTile);
	ASSERT_GT(calmCost, 0);
	EXPECT_EQ(windyCost, static_cast<int>(calmCost * 2.0 / 3));
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * node = paths.getNode(destination, EPathfindingLayer::SAIL);
	ASSERT_TRUE(node->reachable());
	ASSERT_NE(node->theNodeBefore, nullptr);
	ASSERT_EQ(node->theNodeBefore->coord, source);
	EXPECT_EQ(node->moveRemains, 3000 - windyCost);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_EQ(hero->movementPointsRemaining(), node->moveRemains);
}

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

TEST_F(GrailTopologyTest, WrappedHoleIsNotAnEligibleGrailLocation)
{
	wrappedHole = true;
	startGame();
	EXPECT_EQ(map()->grailPos, int3(0, 10, 0));
	EXPECT_TRUE(map()->isHoleAt(int3(35, 10, 0)));
	EXPECT_TRUE(map()->isHoleAt(int3(-1, 10, 0)));
	EXPECT_FALSE(map()->isHoleAt(int3(35, 10, 1)));
	EXPECT_EQ(gameState()->getTileDigStatus(int3(35, 10, 0)), EDiggingStatus::TILE_OCCUPIED);
	const auto bytes = gameState()->saveToMemory();
	auto restored = std::make_shared<CGameState>();
	restored->preInit(LIBRARY);
	restored->loadFromMemory(bytes);
	EXPECT_TRUE(restored->getMap().isHoleAt(int3(35, 10, 0)));
	EXPECT_FALSE(restored->getMap().isHoleAt(int3(35, 10, 1)));
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
		{{0, 5, 0}, {35, 4, 0}, 1}, {{0, 5, 0}, {35, 6, 0}, 7},
		{{35, 5, 0}, {0, 4, 0}, 3}, {{35, 5, 0}, {0, 6, 0}, 5},
		{{5, 0, 0}, {4, 35, 0}, 1}, {{5, 0, 0}, {6, 35, 0}, 3},
		{{5, 35, 0}, {4, 0, 0}, 7}, {{5, 35, 0}, {6, 0, 0}, 5},
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

TEST_F(HeroMovementTest, WrappedPathfindingDoesNotRevealHiddenTerrain)
{
	const PlayerColor player(0);
	const int3 hidden(35, 10, 0), goal(34, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 10, 0));
	hero->setMovementPoints(3000);
	auto callback = makeCallback(player);
	const auto pathsFor = [&]()
	{
		auto paths = std::make_unique<CPathsInfo>(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(*paths, *callback, hero);
		callback->calculatePaths(config);
		return paths;
	};
	map()->getTile(hidden).terrainType = TerrainId::ROCK;
	const auto hiddenRock = pathsFor();
	ASSERT_FALSE(hiddenRock->getNode(hidden, EPathfindingLayer::LAND)->reachable());
	ASSERT_TRUE(hiddenRock->getNode(goal, EPathfindingLayer::LAND)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(hidden), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), int3(0, 10, 0));
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	map()->getTile(hidden).terrainType = TerrainId::GRASS;
	const auto hiddenGrass = pathsFor();
	for(int x = 0; x < map()->width; ++x)
		for(int y = 0; y < map()->height; ++y)
		{
			const int3 tile(x, y, 0);
			const auto * rockNode = hiddenRock->getNode(tile, EPathfindingLayer::LAND);
			const auto * grassNode = hiddenGrass->getNode(tile, EPathfindingLayer::LAND);
			EXPECT_EQ(rockNode->reachable(), grassNode->reachable()) << tile.toString();
			EXPECT_EQ(rockNode->accessible, grassNode->accessible) << tile.toString();
			if(rockNode->reachable() && grassNode->reachable())
			{
				EXPECT_EQ(rockNode->turns, grassNode->turns) << tile.toString();
				EXPECT_EQ(rockNode->moveRemains, grassNode->moveRemains) << tile.toString();
			}
		}
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 1;
	const auto visibleGrass = pathsFor();
	EXPECT_TRUE(visibleGrass->getNode(hidden, EPathfindingLayer::LAND)->reachable());
	EXPECT_GT(visibleGrass->getNode(goal, EPathfindingLayer::LAND)->moveRemains,
		hiddenGrass->getNode(goal, EPathfindingLayer::LAND)->moveRemains);
}

TEST_F(HeroMovementTest, WrappedSailingDoesNotRevealHiddenCoast)
{
	const PlayerColor player(0);
	const int3 source(0, 0, 0), destination(35, 35, 0), hidden(35, 0, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 0, 0), HeroTypeID(0), player)
		.terrain(source, TerrainId::WATER).terrain(destination, TerrainId::WATER)
		.terrain(hidden, TerrainId::WATER).terrain(int3(0, 35, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(source, BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(source), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	hero->setMovementPoints(3000);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	auto callback = makeCallback(player);
	const auto pathsFor = [&]()
	{
		auto paths = std::make_unique<CPathsInfo>(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(*paths, *callback, hero);
		callback->calculatePaths(config);
		return paths;
	};
	const auto hiddenWater = pathsFor();
	const auto * waterNode = hiddenWater->getNode(destination, EPathfindingLayer::SAIL);
	ASSERT_TRUE(waterNode->reachable());
	map()->getTile(hidden).terrainType = TerrainId::GRASS;
	const auto hiddenLand = pathsFor();
	const auto * landNode = hiddenLand->getNode(destination, EPathfindingLayer::SAIL);
	ASSERT_TRUE(landNode->reachable());
	EXPECT_EQ(waterNode->turns, landNode->turns);
	EXPECT_EQ(waterNode->moveRemains, landNode->moveRemains);

	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 1;
	const auto visibleLand = pathsFor();
	map()->getTile(hidden).terrainType = TerrainId::WATER;
	const auto visibleWater = pathsFor();
	ASSERT_TRUE(visibleLand->getNode(destination, EPathfindingLayer::SAIL)->reachable());
	ASSERT_TRUE(visibleWater->getNode(destination, EPathfindingLayer::SAIL)->reachable());
	EXPECT_GT(visibleWater->getNode(destination, EPathfindingLayer::SAIL)->moveRemains,
		visibleLand->getNode(destination, EPathfindingLayer::SAIL)->moveRemains);

	// The server checks actual terrain, even when route planning lacks knowledge.
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	EXPECT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), destination);
}

TEST_F(HeroMovementTest, WrappedPathfindingDoesNotRevealHiddenGuard)
{
	const PlayerColor player(0);
	const int3 hiddenGuard(35, 10, 0), target(0, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(3, 10, 0), HeroTypeID(0), player)
		.monster(hiddenGuard, CreatureID(0), 10);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenGuard] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(2, 10, 0));
	hero->setMovementPoints(3000);
	ASSERT_EQ(gameState()->guardingCreaturePosition(target), hiddenGuard);
	auto callback = makeCallback(player);
	const auto pathsFor = [&]()
	{
		auto paths = std::make_unique<CPathsInfo>(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(*paths, *callback, hero);
		callback->calculatePaths(config);
		return paths;
	};
	const auto hiddenPaths = pathsFor();
	const auto * hiddenNode = hiddenPaths->getNode(target, EPathfindingLayer::LAND);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenGuard] = 1;
	const auto visiblePaths = pathsFor();
	const auto * visibleNode = visiblePaths->getNode(target, EPathfindingLayer::LAND);
	EXPECT_EQ(visibleNode->accessible, EPathAccessibility::GUARDED);
	EXPECT_NE(hiddenNode->action, visibleNode->action);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenGuard] = 0;
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	const auto * guard = map()->getObject(map()->getTile(hiddenGuard).topVisitableObj());
	ASSERT_NE(guard, nullptr);
	ASSERT_TRUE(handler.removeObject(guard, player));
	const auto unguardedPaths = pathsFor();
	const auto * unguardedNode = unguardedPaths->getNode(target, EPathfindingLayer::LAND);
	EXPECT_EQ(hiddenNode->accessible, unguardedNode->accessible);
	EXPECT_EQ(hiddenNode->action, unguardedNode->action);
	EXPECT_EQ(hiddenNode->reachable(), unguardedNode->reachable());
	EXPECT_EQ(hiddenNode->moveRemains, unguardedNode->moveRemains);
	for(int x = 0; x < map()->width; ++x)
		for(int y = 0; y < map()->height; ++y)
		{
			const int3 tile(x, y, 0);
			const auto * withGuard = hiddenPaths->getNode(tile, EPathfindingLayer::LAND);
			const auto * withoutGuard = unguardedPaths->getNode(tile, EPathfindingLayer::LAND);
			EXPECT_EQ(withGuard->accessible, withoutGuard->accessible) << tile.toString();
			EXPECT_EQ(withGuard->reachable(), withoutGuard->reachable()) << tile.toString();
			if(withGuard->reachable() && withoutGuard->reachable())
			{
				EXPECT_EQ(withGuard->action, withoutGuard->action) << tile.toString();
				EXPECT_EQ(withGuard->turns, withoutGuard->turns) << tile.toString();
				EXPECT_EQ(withGuard->moveRemains, withoutGuard->moveRemains) << tile.toString();
			}
		}
}

TEST_F(HeroMovementTest, WrappedPlayerGuardQueriesDoNotRevealHiddenGuards)
{
	const PlayerColor player(0);
	const int3 hiddenGuard(35, 10, 0), visibleGuard(35, 11, 0), target(0, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(3, 10, 0), HeroTypeID(0), player)
		.monster(hiddenGuard, CreatureID(0), 10).monster(visibleGuard, CreatureID(0), 10);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto & fog = gameState()->getPlayerTeam(player)->fogOfWarMap;
	fog[hiddenGuard] = 0;
	fog[visibleGuard] = 0;
	auto callback = makeCallback(player);
	CCallback unrestricted(gameState(), std::nullopt, nullptr);
	ASSERT_EQ(gameState()->guardingCreaturePosition(target), hiddenGuard);
	EXPECT_TRUE(callback->getGuardingCreatures(target).empty());
	EXPECT_EQ(callback->guardingCreaturePosition(target), int3(-1, -1, -1));
	EXPECT_EQ(callback->getGuardingCreaturePosition(target), int3(-1, -1, -1));
	EXPECT_EQ(unrestricted.guardingCreaturePosition(target), hiddenGuard);
	EXPECT_EQ(unrestricted.getGuardingCreaturePosition(target), hiddenGuard);
	EXPECT_FALSE(unrestricted.getGuardingCreatures(target).empty());

	fog[visibleGuard] = 1;
	const auto guards = callback->getGuardingCreatures(target);
	ASSERT_EQ(guards.size(), 1u);
	EXPECT_EQ(guards.front()->visitablePos(), visibleGuard);
	EXPECT_EQ(callback->guardingCreaturePosition(target), visibleGuard);
	EXPECT_EQ(callback->getGuardingCreaturePosition(target), visibleGuard);
	EXPECT_EQ(gameState()->guardingCreaturePosition(target), hiddenGuard);

	fog[target] = 0;
	EXPECT_EQ(callback->getGuardingCreaturePosition(target), int3(-1, -1, -1));

	// The pre-existing bounded-map callback behavior remains unchanged.
	map()->setToroidalAndRebuild(false);
	const int3 boundedTarget(34, 10, 0);
	fog[hiddenGuard] = 0;
	ASSERT_TRUE(fog[boundedTarget]);
	const auto actualGuard = gameState()->guardingCreaturePosition(boundedTarget);
	ASSERT_EQ(actualGuard, hiddenGuard);
	EXPECT_EQ(callback->guardingCreaturePosition(boundedTarget), actualGuard);
	EXPECT_EQ(callback->getGuardingCreaturePosition(boundedTarget), actualGuard);
	EXPECT_FALSE(callback->getGuardingCreatures(boundedTarget).empty());
}

TEST_F(HeroMovementTest, WrappedDimensionDoorCursorDoesNotRevealHiddenGuard)
{
	const PlayerColor player(0);
	const int3 hiddenGuard(35, 10, 0), visibleGuard(35, 11, 0), target(0, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(3, 10, 0), HeroTypeID(0), player)
		.monster(hiddenGuard, CreatureID(0), 10).monster(visibleGuard, CreatureID(0), 10);
	overrideSettingBeforeInit(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS, true);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto & fog = gameState()->getPlayerTeam(player)->fogOfWarMap;
	fog[hiddenGuard] = 0;
	fog[visibleGuard] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	auto callback = makeCallback(player);
	JsonNode config;
	config["cursor"].String() = "ordinary";
	config["cursorGuarded"].String() = "guarded";
	config["exposeFow"].Bool() = true;
	DimensionDoorEffect effect(nullptr, config);
	const IAdventureSpellEffect & view = effect;
	EXPECT_EQ(view.getCursorForTarget(callback.get(), hero, target), "ordinary");
	EXPECT_EQ(view.getCursorForTarget(gameState().get(), hero, target), "ordinary");
	fog[visibleGuard] = 1;
	EXPECT_EQ(view.getCursorForTarget(callback.get(), hero, target), "guarded");
	EXPECT_EQ(view.getCursorForTarget(gameState().get(), hero, target), "guarded");
	fog[target] = 0;
	EXPECT_EQ(view.getCursorForTarget(callback.get(), hero, target), "ordinary");
	fog[target] = 1;
	map()->setToroidalAndRebuild(false);
	const int3 boundedTarget(34, 10, 0);
	fog[visibleGuard] = 0;
	EXPECT_EQ(view.getCursorForTarget(callback.get(), hero, boundedTarget), "ordinary");
	EXPECT_EQ(view.getCursorForTarget(gameState().get(), hero, boundedTarget), "ordinary");
	fog[hiddenGuard] = 1;
	EXPECT_EQ(view.getCursorForTarget(callback.get(), hero, boundedTarget), "guarded");
	EXPECT_EQ(view.getCursorForTarget(gameState().get(), hero, boundedTarget), "guarded");
}

TEST_F(HeroMovementTest, WrappedDimensionDoorPreviewDoesNotRevealHiddenTerrain)
{
	const PlayerColor player(0);
	const int3 source(0, 10, 0), destination(35, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(source, HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto & fog = gameState()->getPlayerTeam(player)->fogOfWarMap;
	fog[destination] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	auto callback = makeCallback(player);
	JsonNode config;
	config["rangeX"].Integer() = 3;
	config["rangeY"].Integer() = 3;
	config["ignoreFow"].Bool() = true;
	config["exposeFow"].Bool() = true;
	DimensionDoorEffect effect(nullptr, config);
	map()->getTile(destination).terrainType = TerrainId::GRASS;
	EXPECT_TRUE(effect.isValidTargetFrom(callback.get(), hero, source, destination));
	EXPECT_TRUE(effect.isValidTargetFrom(gameState().get(), hero, source, destination));
	map()->getTile(destination).terrainType = TerrainId::WATER;
	EXPECT_TRUE(effect.isValidTargetFrom(callback.get(), hero, source, destination));
	EXPECT_FALSE(effect.isValidTargetFrom(gameState().get(), hero, source, destination));
	config["ignoreFow"].Bool() = false;
	DimensionDoorEffect fogRestricted(nullptr, config);
	EXPECT_FALSE(fogRestricted.isValidTargetFrom(callback.get(), hero, source, destination));
	fog[destination] = 1;
	EXPECT_FALSE(effect.isValidTargetFrom(callback.get(), hero, source, destination));
	const int3 hiddenSource(0, 11, 0);
	fog[hiddenSource] = 0;
	map()->getTile(destination).terrainType = TerrainId::GRASS;
	map()->getTile(hiddenSource).terrainType = TerrainId::GRASS;
	EXPECT_TRUE(effect.isValidTargetFrom(callback.get(), hero, hiddenSource, destination));
	EXPECT_TRUE(effect.isValidTargetFrom(gameState().get(), hero, hiddenSource, destination));
	map()->getTile(hiddenSource).terrainType = TerrainId::WATER;
	EXPECT_TRUE(effect.isValidTargetFrom(callback.get(), hero, hiddenSource, destination));
	EXPECT_FALSE(effect.isValidTargetFrom(gameState().get(), hero, hiddenSource, destination));
	fog[destination] = 0;
	map()->setToroidalAndRebuild(false);
	map()->getTile(destination).terrainType = TerrainId::WATER;
	config["rangeX"].Integer() = 36;
	config["ignoreFow"].Bool() = true;
	DimensionDoorEffect bounded(nullptr, config);
	EXPECT_TRUE(bounded.isValidTargetFrom(callback.get(), hero, source, destination));
	EXPECT_FALSE(bounded.isValidTargetFrom(gameState().get(), hero, source, destination));
	map()->getTile(destination).terrainType = TerrainId::GRASS;
	EXPECT_TRUE(bounded.isValidTargetFrom(callback.get(), hero, source, destination));
	fog[destination] = 1;
	map()->getTile(destination).terrainType = TerrainId::WATER;
	EXPECT_FALSE(bounded.isValidTargetFrom(callback.get(), hero, source, destination));
}

TEST_F(HeroMovementTest, WrappedPathfindingRecognizesVisibleGuardBesideHiddenGuard)
{
	const PlayerColor player(0);
	const int3 hiddenGuard(35, 10, 0), visibleGuard(35, 11, 0), target(0, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(3, 10, 0), HeroTypeID(0), player)
		.monster(hiddenGuard, CreatureID(0), 10).monster(visibleGuard, CreatureID(0), 10);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenGuard] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(gameState()->guardingCreaturePosition(target), hiddenGuard);
	auto callback = makeCallback(player);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *callback, hero);
	callback->calculatePaths(config);
	const auto * node = paths.getNode(target, EPathfindingLayer::LAND);
	ASSERT_TRUE(node->reachable());
	EXPECT_EQ(node->accessible, EPathAccessibility::GUARDED);
}

TEST_F(HeroMovementTest, sightRangeWrapsWithoutRevealingDistantTiles)
{
	checkToroidalSightRange(EMapFormat::SOD);
}

TEST_F(HeroMovementTest, sightRangeWrapsWithoutRevealingDistantTilesRoE)
{
	checkToroidalSightRange(EMapFormat::ROE);
}

TEST_F(HeroMovementTest, CornerMovementRevealsOnlyTheMovingTeamAndLevel)
{
	const PlayerColor movingPlayer(0);
	const PlayerColor otherPlayer(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(movingPlayer).playerActive(otherPlayer)
		.hero(int3(1, 0, 0), HeroTypeID(0), movingPlayer)
		.hero(int3(19, 18, 0), HeroTypeID(1), otherPlayer);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	ASSERT_NE(gameState()->getPlayerTeam(movingPlayer), gameState()->getPlayerTeam(otherPlayer));
	setMapVisibility(movingPlayer, false);
	setMapVisibility(otherPlayer, false);
	auto * hero = findHeroByOwner(movingPlayer);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	hero->setMovementPoints(3000);
	const auto snapshot = gameState()->saveToMemory();
	std::vector<std::shared_ptr<CGameState>> replicas;
	for(int i = 0; i < 2; ++i)
	{
		auto replica = std::make_shared<CGameState>();
		replica->preInit(LIBRARY);
		replica->loadFromMemory(snapshot);
		replicas.push_back(std::move(replica));
	}
	class Capture final : public INetworkConnection
	{
	public:
		void sendPacket(const std::vector<std::byte> & packet) override { bytes = packet; }
		void setAsyncWritesEnabled(bool) override {}
		void close() override {}
		std::vector<std::byte> bytes;
	};
	class MirrorServer final : public GameHandlerTestServer
	{
	public:
		MirrorServer(const std::shared_ptr<CGameState> & state, PlayerColor player, const std::vector<std::shared_ptr<CGameState>> & replicas)
			: GameHandlerTestServer(state, player), replicas(replicas), transport(std::make_shared<Capture>()), sender(transport), receiver(transport)
		{
			sender.setSerializationVersion(ESerializationVersion::CURRENT);
			receiver.setSerializationVersion(ESerializationVersion::CURRENT);
		}
		void applyPack(CPackForClient & pack) override
		{
			if(dynamic_cast<TryMoveHero *>(&pack))
			{
				sender.sendPack(pack);
				for(const auto & replica : replicas)
				{
					receiver.setCallback(*replica);
					auto decoded = receiver.retrievePack(transport->bytes);
					if(auto * movement = dynamic_cast<TryMoveHero *>(decoded.get()))
					{
						replica->apply(*movement);
						++delivered;
					}
					else
						ADD_FAILURE() << "Expected TryMoveHero after network decoding";
				}
			}
			GameHandlerTestServer::applyPack(pack);
		}
		int delivered = 0;
	private:
		std::vector<std::shared_ptr<CGameState>> replicas;
		std::shared_ptr<Capture> transport;
		GameConnection sender;
		GameConnection receiver;
	} server(gameState(), movingPlayer, replicas);
	CGameHandler handler(server, gameState());
	const int3 destination(35, 35, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, movingPlayer, EPathfindingLayer::LAND));
	ASSERT_EQ(hero->visitablePos(), destination);
	ASSERT_EQ(server.delivered, 2);
	for(const auto & replica : replicas)
	{
		ASSERT_TRUE(replica->getMap().toroidal);
		ASSERT_EQ(replica->getHero(hero->id)->visitablePos(), destination);
	}
	const int radius = hero->getSightRadius();
	for(int z = 0; z < 2; ++z)
		for(int x = 0; x < 36; ++x)
			for(int y = 0; y < 36; ++y)
			{
				const int dx = std::min(std::abs(x - destination.x), 36 - std::abs(x - destination.x));
				const int dy = std::min(std::abs(y - destination.y), 36 - std::abs(y - destination.y));
				const bool expectedVisible = z == 0 && std::round(std::sqrt(dx * dx + dy * dy)) <= radius;
				const int3 tile(x, y, z);
				EXPECT_EQ(gameState()->isVisibleFor(tile, movingPlayer), expectedVisible) << tile.toString();
				EXPECT_FALSE(gameState()->isVisibleFor(tile, otherPlayer)) << tile.toString();
				for(const auto & replica : replicas)
				{
					EXPECT_EQ(replica->isVisibleFor(tile, movingPlayer), expectedVisible) << tile.toString();
					EXPECT_FALSE(replica->isVisibleFor(tile, otherPlayer)) << tile.toString();
				}
			}
}

TEST_F(HeroMovementTest, TcpPeersReceiveAuthoritativeCornerMovement)
{
	const PlayerColor player(0);
	const PlayerColor otherPlayer(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).playerActive(otherPlayer)
		.hero(int3(1, 0, 0), HeroTypeID(0), player)
		.hero(int3(1, 5, 0), HeroTypeID(1), otherPlayer)
		.terrain(int3(34, 35, 0), TerrainId::ROCK);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	setMapVisibility(player, false);
	setMapVisibility(otherPlayer, false);
	gameState()->actingPlayers.insert(player);
	gameState()->actingPlayers.insert(otherPlayer);
	auto * hero = findHeroByOwner(player);
	auto * otherHero = findHeroByOwner(otherPlayer);
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(otherHero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	ASSERT_EQ(otherHero->visitablePos(), int3(0, 5, 0));
	hero->setMovementPoints(3000);
	otherHero->setMovementPoints(3000);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	const auto snapshot = gameState()->saveToMemory();

	class Peer final : public INetworkClientListener
	{
	public:
		explicit Peer(std::string name) : name(std::move(name)) {}
		void onConnectionFailed(const std::string & error) override { failure = error; }
		void onDisconnected(const std::shared_ptr<INetworkConnection> &, const std::string & error) override { failure = error; }
		void onConnectionEstablished(const std::shared_ptr<INetworkConnection> & connection) override
		{
			transport = connection;
			codec = std::make_unique<GameConnection>(transport);
			LobbyClientConnected greeting;
			greeting.uuid = name;
			greeting.names = {name};
			greeting.mode = EStartMode::NEW_GAME;
			greeting.version = LobbyTopologyCapability::encode(ESerializationVersion::CURRENT);
			codec->sendPack(greeting);
		}
		void onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & bytes) override
		{
			auto packet = codec->retrievePack(bytes);
			if(const auto * greeting = dynamic_cast<const LobbyClientConnected *>(packet.get()); greeting && greeting->uuid == name)
			{
				const auto accepted = LobbyTopologyCapability::decode(greeting->version);
				negotiated = accepted.toroidal && accepted.version == ESerializationVersion::CURRENT;
				codec->setSerializationVersion(accepted.version);
			}
			if(auto * movement = dynamic_cast<TryMoveHero *>(packet.get()); movement && state)
			{
				state->apply(*movement);
				++movements;
			}
			if(const auto * applied = dynamic_cast<const PackageApplied *>(packet.get()))
				results[applied->requestID] = applied->result;
		}
		std::string name;
		std::string failure;
		bool negotiated = false;
		int movements = 0;
		std::map<uint32_t, bool> results;
		std::shared_ptr<INetworkConnection> transport;
		std::unique_ptr<GameConnection> codec;
		std::shared_ptr<CGameState> state;
	};

	CVCMIServer server(0, true);
	const auto port = server.prepare(false, true);
	ASSERT_NE(port, 0);
	auto network = INetworkHandler::createHandler();
	Peer first("movement-peer"), second("other-peer");
	const auto pumpUntil = [&](const auto & ready)
	{
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(!ready() && first.failure.empty() && second.failure.empty() && std::chrono::steady_clock::now() < deadline)
		{
			network->getContext().restart();
			network->getContext().poll();
			server.getNetworkHandler().getContext().restart();
			server.getNetworkHandler().getContext().poll();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	};
	network->connectToRemote(first, "127.0.0.1", port);
	pumpUntil([&] { return first.negotiated; });
	ASSERT_TRUE(first.negotiated) << first.failure;
	network->connectToRemote(second, "127.0.0.1", port);
	pumpUntil([&] { return second.negotiated; });
	ASSERT_TRUE(second.negotiated) << second.failure;
	ASSERT_EQ(server.activeConnections.size(), 2);
	ASSERT_TRUE(server.supportsMapTopology(*gameState()->getMapHeader()));

	for(const auto & [id, client] : server.playerNames)
	{
		if(client.name == first.name)
			server.si->playerInfos[player].connectedPlayerIDs.insert(id);
		if(client.name == second.name)
			server.si->playerInfos[otherPlayer].connectedPlayerIDs.insert(id);
	}
	ASSERT_TRUE(server.hasPlayerAt(player, server.activeConnections[0]->connectionID));
	ASSERT_FALSE(server.hasPlayerAt(player, server.activeConnections[1]->connectionID));
	server.gh = std::make_shared<CGameHandler>(server, gameState());
	for(auto & connection : server.activeConnections)
		connection->setCallback(*gameState());
	for(Peer * peer : {&first, &second})
	{
		peer->state = std::make_shared<CGameState>();
		peer->state->preInit(LIBRARY);
		peer->state->loadFromMemory(snapshot);
		peer->codec->setCallback(*peer->state);
	}
	server.setState(EServerState::GAMEPLAY);
	const int3 destination(35, 35, 0);
	MoveHero request({hero->convertFromVisitablePos(destination)}, EPathfindingLayer::LAND, hero->id, false);
	request.player = player;
	request.requestID = 1;
	second.codec->sendPack(request);
	pumpUntil([&] { return second.results.contains(1); });
	ASSERT_TRUE(second.results.contains(1)) << second.failure;
	EXPECT_FALSE(second.results.at(1));
	EXPECT_EQ(hero->visitablePos(), int3(0, 0, 0));
	request.requestID = 2;
	first.codec->sendPack(request);
	pumpUntil([&] { return first.results.contains(2) && first.movements && second.movements; });
	ASSERT_TRUE(first.results.contains(2)) << first.failure;
	EXPECT_TRUE(first.results.at(2));
	ASSERT_EQ(hero->visitablePos(), destination);
	for(const Peer * peer : {&first, &second})
	{
		EXPECT_EQ(peer->movements, 1) << peer->failure;
		EXPECT_TRUE(peer->state->getMap().toroidal);
		EXPECT_EQ(peer->state->getHero(hero->id)->visitablePos(), destination);
		EXPECT_EQ(peer->state->getHero(hero->id)->movementPointsRemaining(), hero->movementPointsRemaining());
		EXPECT_TRUE(peer->state->isVisibleFor(destination, player));
		EXPECT_FALSE(peer->state->isVisibleFor(destination, otherPlayer));
	}
	MoveHero blocked({hero->convertFromVisitablePos(int3(34, 35, 0))}, EPathfindingLayer::LAND, hero->id, false);
	blocked.player = player;
	blocked.requestID = 3;
	first.codec->sendPack(blocked);
	pumpUntil([&] { return first.results.contains(3); });
	ASSERT_TRUE(first.results.contains(3)) << first.failure;
	EXPECT_FALSE(first.results.at(3));
	EXPECT_EQ(hero->visitablePos(), destination);

	const int3 otherDestination(35, 5, 0);
	MoveHero otherRequest({otherHero->convertFromVisitablePos(otherDestination)}, EPathfindingLayer::LAND, otherHero->id, false);
	otherRequest.player = otherPlayer;
	otherRequest.requestID = 4;
	second.codec->sendPack(otherRequest);
	pumpUntil([&] {
		return second.results.contains(4)
			&& first.state->getHero(otherHero->id)->visitablePos() == otherDestination
			&& second.state->getHero(otherHero->id)->visitablePos() == otherDestination;
	});
	ASSERT_TRUE(second.results.contains(4)) << second.failure;
	EXPECT_TRUE(second.results.at(4));
	ASSERT_EQ(otherHero->visitablePos(), otherDestination);
	for(const Peer * peer : {&first, &second})
	{
		EXPECT_GE(peer->movements, 2) << peer->failure;
		EXPECT_EQ(peer->state->getHero(otherHero->id)->visitablePos(), otherDestination);
		EXPECT_EQ(peer->state->getHero(otherHero->id)->movementPointsRemaining(), otherHero->movementPointsRemaining());
		EXPECT_TRUE(peer->state->isVisibleFor(otherDestination, otherPlayer));
	}
	const int3 airborne(0, 35, 0);
	MoveHero flight({hero->convertFromVisitablePos(airborne)}, EPathfindingLayer::AIR, hero->id, true);
	flight.player = player;
	flight.requestID = 5;
	first.codec->sendPack(flight);
	pumpUntil([&] {
		return first.results.contains(5)
			&& first.state->getHero(hero->id)->visitablePos() == airborne
			&& second.state->getHero(hero->id)->visitablePos() == airborne;
	});
	ASSERT_TRUE(first.results.contains(5)) << first.failure;
	ASSERT_TRUE(first.results.at(5));
	ASSERT_EQ(hero->getPathfindingStartLayer(), EPathfindingLayer::AIR);
	for(const Peer * peer : {&first, &second})
	{
		const auto * replicaHero = peer->state->getHero(hero->id);
		ASSERT_EQ(replicaHero->getPathfindingStartLayer(), EPathfindingLayer::AIR);
		CPathsInfo paths(peer->state->getMapSize(), replicaHero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *peer->state, replicaHero);
		CPathfinder pathfinder(*peer->state, config);
		pathfinder.calculatePaths();
		const auto * node = paths.getNode(destination, EPathfindingLayer::LAND);
		ASSERT_TRUE(node->reachable());
		ASSERT_NE(node->theNodeBefore, nullptr);
		EXPECT_EQ(node->theNodeBefore->coord, airborne);
		EXPECT_EQ(node->theNodeBefore->layer, EPathfindingLayer::AIR);
	}
	MoveHero landing({hero->convertFromVisitablePos(destination)}, EPathfindingLayer::LAND, hero->id, false);
	landing.player = player;
	landing.requestID = 6;
	first.codec->sendPack(landing);
	pumpUntil([&] {
		return first.results.contains(6)
			&& first.state->getHero(hero->id)->visitablePos() == destination
			&& second.state->getHero(hero->id)->visitablePos() == destination;
	});
	ASSERT_TRUE(first.results.contains(6)) << first.failure;
	ASSERT_TRUE(first.results.at(6));
	for(const Peer * peer : {&first, &second})
		EXPECT_EQ(peer->state->getHero(hero->id)->getPathfindingStartLayer(), EPathfindingLayer::LAND);
}

TEST_F(HeroMovementTest, blockedToroidalStepIsRejected)
{
	checkBlockedToroidalStep(EMapFormat::SOD);
}

TEST_F(HeroMovementTest, blockedToroidalStepIsRejectedRoE)
{
	checkBlockedToroidalStep(EMapFormat::ROE);
}

TEST_F(HeroMovementTest, MovementAcknowledgementReportsRejectedAndAcceptedRequests)
{
	const PlayerColor player(0), opponent(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).playerActive(opponent)
		.hero(int3(11, 10, 0), HeroTypeID(0), player)
		.hero(int3(21, 20, 0), HeroTypeID(1), opponent)
		.terrain(int3(11, 10, 0), TerrainId::ROCK).terrain(int3(8, 10, 0), TerrainId::ROCK);
	startWithMap(std::move(builder));
	ASSERT_FALSE(map()->toroidal);
	gameState()->actingPlayers.insert(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(10, 10, 0));
	hero->setMovementPoints(3000);

	class Acknowledgements final : public GameHandlerTestServer
	{
	public:
		using GameHandlerTestServer::GameHandlerTestServer;
		void sendPack(CPackForClient & pack, GameConnectionID) override
		{
			if(const auto * applied = dynamic_cast<const PackageApplied *>(&pack))
				results[applied->requestID] = applied->result;
		}
		std::map<uint32_t, bool> results;
	} server(gameState(), player);
	CGameHandler handler(server, gameState());
	MoveHero blocked({hero->convertFromVisitablePos(int3(11, 10, 0))}, EPathfindingLayer::LAND, hero->id, false);
	blocked.player = player;
	blocked.requestID = 1;
	handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, blocked);
	ASSERT_TRUE(server.results.contains(1));
	EXPECT_FALSE(server.results.at(1));
	EXPECT_EQ(hero->visitablePos(), int3(10, 10, 0));

	// A rejected multi-step request is not transactional: an earlier legal
	// step may already have changed the game state before the later failure.
	const auto pointsBeforePartial = hero->movementPointsRemaining();
	MoveHero partial({hero->convertFromVisitablePos(int3(9, 10, 0)), hero->convertFromVisitablePos(int3(8, 10, 0))},
		EPathfindingLayer::LAND, hero->id, false);
	partial.player = player;
	partial.requestID = 3;
	handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, partial);
	ASSERT_TRUE(server.results.contains(3));
	EXPECT_FALSE(server.results.at(3));
	EXPECT_EQ(hero->visitablePos(), int3(9, 10, 0));
	EXPECT_LT(hero->movementPointsRemaining(), pointsBeforePartial);

	MoveHero allowed({hero->convertFromVisitablePos(int3(10, 10, 0))}, EPathfindingLayer::LAND, hero->id, false);
	allowed.player = player;
	allowed.requestID = 2;
	handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, allowed);
	ASSERT_TRUE(server.results.contains(2));
	EXPECT_TRUE(server.results.at(2));
	EXPECT_EQ(hero->visitablePos(), int3(10, 10, 0));
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

TEST_F(HeroMovementTest, CoastalTilesRecognizeWaterAcrossToroidalCorner)
{
	const PlayerColor player(0);
	const int3 water(35, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player)
		.terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	const int3 acrossCorner(0, 0, 0);
	const int3 acrossWestEast(0, 35, 0);
	const int3 acrossNorthSouth(35, 0, 0);
	map()->toroidal = false;
	for(const auto & tile : {acrossCorner, acrossWestEast, acrossNorthSouth})
		EXPECT_FALSE(map()->isCoastalTile(tile));
	map()->toroidal = true;
	for(const auto & tile : {acrossCorner, acrossWestEast, acrossNorthSouth})
		EXPECT_TRUE(map()->isCoastalTile(tile));
	EXPECT_FALSE(map()->isCoastalTile(water));
	EXPECT_FALSE(map()->isCoastalTile(int3(2, 2, 0)));
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

TEST_F(HeroMovementTest, grailCanBeDugAfterCrossingToroidalCorner)
{
	const PlayerColor player(0);
	const int3 destination(35, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 0, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	map()->grailPos = destination;

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	hero->setMovementPoints(hero->movementPointsLimit());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_EQ(hero->visitablePos(), destination);
	hero->setMovementPoints(hero->movementPointsLimit());
	ASSERT_EQ(hero->diggingStatus(), EDiggingStatus::CAN_DIG);
	ASSERT_TRUE(handler.dig(hero));
	EXPECT_TRUE(hero->hasArt(ArtifactID::GRAIL));
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_TRUE(map()->isHoleAt(destination));
	const auto snapshot = gameState()->saveToMemory();
	auto restored = std::make_shared<CGameState>();
	restored->preInit(LIBRARY);
	restored->loadFromMemory(snapshot);
	ASSERT_TRUE(restored->getMap().toroidal);
	EXPECT_TRUE(restored->getMap().isHoleAt(destination));
	const auto * restoredHero = restored->getHero(hero->id);
	ASSERT_NE(restoredHero, nullptr);
	EXPECT_EQ(restoredHero->visitablePos(), destination);
	EXPECT_TRUE(restoredHero->hasArt(ArtifactID::GRAIL));
	EXPECT_EQ(restoredHero->movementPointsRemaining(), 0);
	hero->setMovementPoints(hero->movementPointsLimit());
	EXPECT_EQ(hero->diggingStatus(), EDiggingStatus::TILE_OCCUPIED);
	EXPECT_FALSE(handler.dig(hero));
}

TEST_F(HeroMovementTest, aiPatrolRadiusUsesWrappedManhattanDistance)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 0, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	revealMap(player);
	gameState()->players.at(player).human = false;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(0, 0, 0));
	hero->setMovementPoints(3000);
	hero->patrol.patrolling = true;
	hero->patrol.initialPos = hero->visitablePos();
	hero->patrol.patrolRadius = 2;

	const auto check = [&](bool toroidal)
	{
		map()->setToroidalAndRebuild(toroidal);
		CPathsInfo paths(gameState()->getMapSize(), hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
		CPathfinder pathfinder(*gameState(), config);
		pathfinder.calculatePaths();
		const auto reachable = [&](int x, int y)
		{
			return paths.getNode(int3(x, y, 0), EPathfindingLayer::LAND)->reachable();
		};
		EXPECT_TRUE(reachable(1, 0));
		EXPECT_TRUE(reachable(1, 1));
		EXPECT_FALSE(reachable(2, 1));
		EXPECT_EQ(reachable(35, 0), toroidal);
		EXPECT_EQ(reachable(0, 35), toroidal);
		EXPECT_EQ(reachable(35, 35), toroidal);
		EXPECT_FALSE(reachable(34, 35));
	};
	check(false);
	check(true);
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

TEST_F(HeroMovementTest, AmbientSoundSourcesFollowWrappedObjectFootprints)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	const auto makeAppearance = [](const std::string & mask)
	{
		const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": )" + mask + " }";
		JsonNode definition(text.data(), text.size(), "AmbientSourceTest");
		auto appearance = std::make_shared<ObjectTemplate>();
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		appearance->readJson(definition, resolver, false);
		return appearance;
	};
	CGTerrainPatch overlay(gameState().get());
	overlay.appearance = makeAppearance(R"([ "VV", "VV" ])");
	overlay.pos = int3(0, 0, 1);
	CGObjectInstance blocker(gameState().get());
	blocker.appearance = makeAppearance(R"([ "BB", "BB" ])");
	blocker.pos = overlay.pos;
	CGObjectInstance visitable(gameState().get());
	visitable.appearance = makeAppearance(R"([ "A0", "00" ])");
	visitable.pos = overlay.pos;
	const std::set<int3> bounded{int3(0, 0, 1)};
	const std::set<int3> wrapped{int3(0, 0, 1), int3(35, 0, 1), int3(0, 35, 1), int3(35, 35, 1)};
	EXPECT_EQ(mapAmbientSourceTiles(overlay, map()->getTopology()), bounded);
	EXPECT_EQ(mapAmbientSourceTiles(blocker, map()->getTopology()), bounded);
	EXPECT_TRUE(mapAmbientSourceTiles(visitable, map()->getTopology()).empty());
	map()->toroidal = true;
	EXPECT_EQ(mapAmbientSourceTiles(overlay, map()->getTopology()), wrapped);
	EXPECT_EQ(mapAmbientSourceTiles(blocker, map()->getTopology()), wrapped);
	EXPECT_EQ(mapAmbientSourceTiles(visitable, map()->getTopology()), (std::set<int3>{int3(35, 35, 1)}));
}

TEST_F(HeroMovementTest, UnrestrictedCallbackSeesObjectFullyAcrossToroidalCorner)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "V" ] })";
	JsonNode definition(text.data(), text.size(), "FullyWrappedVisibility");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	CGObjectInstance object(gameState().get());
	object.appearance = appearance;
	object.pos = int3(-1, -1, 1);
	EXPECT_FALSE(gameState()->isVisible(&object));
	map()->toroidal = true;
	EXPECT_TRUE(gameState()->isVisible(&object));
	setMapVisibility(player, false);
	EXPECT_FALSE(gameState()->isVisibleFor(&object, player));
	revealMap(player);
	EXPECT_TRUE(gameState()->isVisibleFor(&object, player));
	object.pos.z = 2;
	EXPECT_FALSE(gameState()->isVisible(&object));
}

TEST_F(HeroMovementTest, VisitablePositionWrapsCornerWithoutChangingLevel)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "A0", "00" ] })";
	JsonNode definition(text.data(), text.size(), "VisitableCornerTest");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	CGObjectInstance object(gameState().get());
	object.appearance = appearance;
	object.pos = int3(0, 0, 1);
	EXPECT_EQ(object.visitablePos(), int3(-1, -1, 1));
	map()->toroidal = true;
	EXPECT_EQ(object.visitablePos(), int3(35, 35, 1));
	EXPECT_EQ(object.anchorPos(), int3(0, 0, 1));
	object.pos.z = -1;
	EXPECT_EQ(object.visitablePos(), int3(-1, -1, -1));
	object.pos.z = 1;
	object.cb = nullptr;
	EXPECT_EQ(object.visitablePos(), int3(-1, -1, 1));
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
	ASSERT_EQ(resource->visitablePos(), destination);
	map()->toroidal = false;
	EXPECT_EQ(resource->visitablePos(), int3(-1, 10, 0));
	map()->toroidal = true;
	EditorCallback unboundEditor(nullptr);
	auto * originalCallback = resource->cb;
	resource->cb = &unboundEditor;
	EXPECT_NO_THROW(EXPECT_EQ(resource->visitablePos(), int3(-1, 10, 0)));
	resource->cb = originalCallback;
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

TEST_F(HeroMovementTest, BoatCannotDisembarkOntoNonblockingEventAcrossSeam)
{
	const PlayerColor player(0);
	const int3 water(0, 5, 0);
	const int3 shore(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 5, 0), HeroTypeID(0), player)
		.terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(water, BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	const auto boatId = hero->getBoat()->id;

	const std::string text = R"({ "animation": "test.def", "mask": [ "V" ] })";
	JsonNode definition(text.data(), text.size(), "NonblockingShoreEvent");
	auto appearance = std::make_shared<ObjectTemplate>();
	appearance->id = Obj::EVENT;
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto event = std::make_shared<CGEvent>(gameState().get());
	event->ID = Obj::EVENT;
	event->subID = MapObjectSubID(0);
	event->pos = shore;
	event->appearance = appearance;
	event->instanceName = "nonblocking_shore_event";
	map()->addNewObject(event);
	ASSERT_TRUE(map()->getTile(shore).visitable());
	ASSERT_FALSE(map()->getTile(shore).blocked());
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * node = paths.getNode(shore, EPathfindingLayer::LAND);
	if(node->reachable() && node->theNodeBefore)
		EXPECT_NE(node->theNodeBefore->coord, water);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(shore), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), water);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	ASSERT_TRUE(hero->inBoat());
	EXPECT_EQ(hero->getBoat()->id, boatId);
	EXPECT_NE(map()->getObject(event->id), nullptr);
	EXPECT_TRUE(map()->getTile(shore).visitable());
	map()->hideObject(event.get());
	ASSERT_FALSE(map()->getTile(shore).visitable());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(shore), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), shore);
	EXPECT_FALSE(hero->inBoat());
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

	// Water walking cannot change the source layer while the hero is on land.
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	CPathsInfo waterPaths(gameState()->getMapSize(), hero);
	auto waterConfig = std::make_shared<SingleHeroPathfinderConfig>(waterPaths, *gameState(), hero);
	CPathfinder waterPathfinder(*gameState(), waterConfig);
	waterPathfinder.calculatePaths();
	EXPECT_FALSE(waterPaths.getNode(destination, EPathfindingLayer::LAND)->reachable());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);

	// A hero that has actually flown over that object can continue in the air.
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	ASSERT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_EQ(hero->visitablePos(), origin);
	map()->moveObject(hero->id, hero->convertFromVisitablePos(int3(1, 5, 0)));
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(origin), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_EQ(hero->visitablePos(), origin);
	CPathsInfo replanned(gameState()->getMapSize(), hero);
	auto replanConfig = std::make_shared<SingleHeroPathfinderConfig>(replanned, *gameState(), hero);
	CPathfinder replan(*gameState(), replanConfig);
	replan.calculatePaths();
	const auto * landing = replanned.getNode(destination, EPathfindingLayer::LAND);
	ASSERT_TRUE(landing->reachable());
	ASSERT_NE(landing->theNodeBefore, nullptr);
	EXPECT_EQ(landing->theNodeBefore->coord, origin);
	EXPECT_EQ(landing->theNodeBefore->layer, EPathfindingLayer::AIR);
	TryMoveHero predicted;
	predicted.id = hero->id;
	predicted.start = hero->anchorPos();
	predicted.end = hero->convertFromVisitablePos(destination);
	predicted.result = TryMoveHero::SUCCESS;
	predicted.movePoints = hero->movementPointsRemaining();
	predicted.movementLayer = EPathfindingLayer::LAND;
	PackRollbackGeneratorVisitor rollback(*gameState());
	predicted.visit(rollback);
	ASSERT_TRUE(rollback.canBeRolledBack());
	auto rollbackPacks = rollback.acquireRollbackPacks();
	ASSERT_FALSE(rollbackPacks.empty());
	gameState()->apply(predicted);
	ASSERT_EQ(hero->getPathfindingStartLayer(), EPathfindingLayer::LAND);
	gameState()->apply(*rollbackPacks.front());
	ASSERT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->getPathfindingStartLayer(), EPathfindingLayer::AIR);
	// Older server save layouts lack this state and remain readable.
	CMemorySerializer oldArchive;
	oldArchive.oser.version = ESerializationVersion::TOROIDAL_UNINDEXED_OBJECTS;
	oldArchive.iser.version = ESerializationVersion::TOROIDAL_UNINDEXED_OBJECTS;
	oldArchive.oser & handler;
	GameHandlerTestServer oldServer(gameState(), player);
	CGameHandler oldRestored(oldServer, gameState());
	oldArchive.iser.cb = gameState().get();
	oldArchive.iser & oldRestored;
	ASSERT_TRUE(oldArchive.isFullyRead());
	// Save the server portion while airborne, then resume with a new handler.
	CMemorySerializer archive;
	archive.oser & handler;
	GameHandlerTestServer restoredServer(gameState(), player);
	CGameHandler restored(restoredServer, gameState());
	archive.iser.cb = gameState().get();
	archive.iser & restored;
	ASSERT_TRUE(archive.isFullyRead());
	const int3 blocked(35, 4, 0);
	map()->getTile(blocked).terrainType = TerrainId::ROCK;
	EXPECT_FALSE(restored.moveHero(hero->id, hero->convertFromVisitablePos(blocked), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_EQ(hero->visitablePos(), origin);
	ASSERT_TRUE(restored.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), destination);
	// Landing returns the hero to LAND; Flight alone must not preserve AIR.
	map()->hideObject(hero);
	map()->moveObject(object->id, destination);
	map()->showObject(hero);
	EXPECT_FALSE(restored.moveHero(hero->id, hero->convertFromVisitablePos(origin), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	EXPECT_EQ(hero->visitablePos(), destination);
}

TEST_F(HeroMovementTest, SailingHeroWithFlightStillHonorsSourceExitAcrossSeam)
{
	const PlayerColor player(0);
	const int3 source(0, 5, 0), destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 5, 0), HeroTypeID(0), player)
		.terrain(source, TerrainId::WATER).terrain(destination, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(source, BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(source), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	const auto boatId = hero->getBoat()->id;

	const std::string text = R"({ "animation": "test.def", "visitableFrom": [], "mask": [ "A" ] })";
	JsonNode definition(text.data(), text.size(), "ForbiddenSailingExit");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto object = std::make_shared<CGObjectInstance>(gameState().get());
	object->pos = source;
	object->appearance = appearance;
	object->instanceName = "forbidden_sailing_exit";
	map()->hideObject(hero);
	map()->addNewObject(object);
	map()->showObject(hero);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * node = paths.getNode(destination, EPathfindingLayer::SAIL);
	if(node->reachable() && node->theNodeBefore)
		EXPECT_NE(node->theNodeBefore->coord, source);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), source);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	ASSERT_TRUE(hero->inBoat());
	EXPECT_EQ(hero->getBoat()->id, boatId);
	map()->hideObject(object.get());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_TRUE(hero->inBoat());
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

TEST_F(HeroMovementTest, ToroidalTransientStepCannotConsumeLastMovementPoints)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), water(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(1, 5, 0), HeroTypeID(0), player).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), land);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	for(const auto layer : {EPathfindingLayer::AIR, EPathfindingLayer::WATER})
	{
		for(const int points : {100, 150})
		{
			hero->setMovementPoints(points);
			EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, layer));
			EXPECT_EQ(hero->visitablePos(), land);
			EXPECT_EQ(hero->movementPointsRemaining(), points);
			EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
		}
	}
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_GT(hero->movementPointsRemaining(), 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(land), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, BoundedTransientStepKeepsExistingLastMovementRule)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), water(1, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(1, 5, 0), HeroTypeID(0), player).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), land);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(100);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), water);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}

TEST_F(HeroMovementTest, ToroidalWaterWalkRejectsStepThatLosesKnownLanding)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), first(35, 5, 0), second(34, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 33; x <= 35; ++x)
		for(int y = 4; y <= 6; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), land);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_EQ(hero->visitablePos(), first);
	ASSERT_EQ(hero->movementPointsRemaining(), 200);
	EXPECT_TRUE(handler.hasHeroInTransientMovement(player));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(land), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), land);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, ToroidalWaterWalkMayUseLastPointsToReachKnownShore)
{
	const PlayerColor player(0);
	const int3 first(35, 5, 0), second(34, 5, 0), shore(33, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 33; x <= 35; ++x)
		for(int y = 4; y <= 6; ++y)
			if(int3(x, y, 0) != shore)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * landing = paths.getNode(shore, EPathfindingLayer::LAND);
	ASSERT_TRUE(landing->reachable());
	EXPECT_EQ(landing->turns, 0);
	EXPECT_EQ(landing->moveRemains, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(shore), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), shore);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, ToroidalFlightRejectsStepThatLosesKnownLanding)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), first(35, 5, 0), second(34, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 33; x <= 35; ++x)
		for(int y = 4; y <= 6; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	hero->setMovementPoints(100);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * landing = paths.getNode(land, EPathfindingLayer::LAND);
	ASSERT_TRUE(landing->reachable());
	EXPECT_EQ(landing->turns, 0);
	EXPECT_EQ(landing->moveRemains, 0);
	hero->setMovementPoints(200);
	ASSERT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_EQ(hero->visitablePos(), first);
	ASSERT_EQ(hero->movementPointsRemaining(), 200);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(land), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, BoundedWaterWalkerCanPlanLandingWithLastMovementPoints)
{
	const PlayerColor player(0);
	const int3 land(2, 5, 0), water(1, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(3, 5, 0), HeroTypeID(0), player).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	hero->setMovementPoints(100);
	// Bounded games do not currently replicate the transit layer: seed a
	// prospective water start to check the common pathfinding rule itself.
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero, PathfinderStart{water, EPathfindingLayer::WATER, 100});
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * landing = paths.getNode(land, EPathfindingLayer::LAND);
	ASSERT_TRUE(landing->reachable());
	EXPECT_EQ(landing->turns, 0);
	EXPECT_EQ(landing->moveRemains, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(land), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}

TEST_F(HeroMovementTest, HeroCanSwitchFromWaterWalkToFlightToReachShore)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(4, 5, 0), HeroTypeID(0), player);
	for(const int x : {35, 0, 1, 2})
		for(int y = 2; y <= 8; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	builder.resource(int3(35, 5, 0), GameResID::GOLD, 1000);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	ASSERT_TRUE(map()->getTile(int3(35, 5, 0)).blocked());
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(500);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	for(const int x : {2, 1, 0})
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(x, 5, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_EQ(hero->movementPointsRemaining(), 200);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * landing = paths.getNode(int3(34, 5, 0), EPathfindingLayer::LAND);
	ASSERT_TRUE(landing->reachable());
	ASSERT_EQ(landing->turns, 0);
	ASSERT_NE(landing->theNodeBefore, nullptr);
	EXPECT_EQ(landing->theNodeBefore->coord, int3(35, 5, 0));
	EXPECT_EQ(landing->theNodeBefore->layer, EPathfindingLayer::AIR);
	ASSERT_NE(landing->theNodeBefore->theNodeBefore, nullptr);
	EXPECT_EQ(landing->theNodeBefore->theNodeBefore->layer, EPathfindingLayer::WATER);
	CPathsInfo noFlightPaths(gameState()->getMapSize(), hero);
	auto noFlightConfig = std::make_shared<SingleHeroPathfinderConfig>(noFlightPaths, *gameState(), hero);
	noFlightConfig->options.useFlying = false;
	CPathfinder noFlightPathfinder(*gameState(), noFlightConfig);
	noFlightPathfinder.calculatePaths();
	EXPECT_NE(noFlightPaths.getNode(int3(34, 5, 0), EPathfindingLayer::LAND)->turns, 0);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 5, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(34, 5, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}

TEST_F(HeroMovementTest, HeroCanSwitchFromFlightToWaterWalkAcrossSeam)
{
	const PlayerColor player(0);
	const int3 first(0, 5, 0), second(35, 5, 0), landing(35, 4, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(2, 5, 0), HeroTypeID(0), player).terrain(first, TerrainId::WATER).terrain(second, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * waterNode = paths.getNode(second, EPathfindingLayer::WATER);
	ASSERT_TRUE(waterNode->reachable());
	ASSERT_EQ(waterNode->turns, 0);
	ASSERT_NE(waterNode->theNodeBefore, nullptr);
	EXPECT_EQ(waterNode->theNodeBefore->coord, first);
	EXPECT_EQ(waterNode->theNodeBefore->layer, EPathfindingLayer::AIR);
	CPathsInfo noWaterPaths(gameState()->getMapSize(), hero);
	auto noWaterConfig = std::make_shared<SingleHeroPathfinderConfig>(noWaterPaths, *gameState(), hero);
	noWaterConfig->options.useWaterWalking = false;
	CPathfinder noWaterPathfinder(*gameState(), noWaterConfig);
	noWaterPathfinder.calculatePaths();
	EXPECT_FALSE(noWaterPaths.getNode(second, EPathfindingLayer::WATER)->reachable());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(landing), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}

TEST_F(HeroMovementTest, CombinedFlightAndWaterWalkPreserveKnownLanding)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), first(35, 5, 0), second(34, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 33; x <= 35; ++x)
		for(int y = 4; y <= 6; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), first);
	EXPECT_EQ(hero->movementPointsRemaining(), 200);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(land), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
}

TEST_F(HeroMovementTest, PathfinderSwitchesTransientLayersAcrossEdgesAndCorners)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	const std::vector<std::pair<int3, int3>> crossings = {
		{{0, 18, 0}, {35, 18, 0}}, {{35, 18, 0}, {0, 18, 0}},
		{{18, 0, 0}, {18, 35, 0}}, {{18, 35, 0}, {18, 0, 0}},
		{{0, 0, 0}, {35, 35, 0}}, {{35, 35, 0}, {0, 0, 0}},
		{{35, 0, 0}, {0, 35, 0}}, {{0, 35, 0}, {35, 0, 0}}
	};
	for(const auto & [source, destination] : crossings)
	{
		map()->getTile(source).terrainType = TerrainId::WATER;
		map()->getTile(destination).terrainType = TerrainId::WATER;
		map()->moveObject(hero->id, hero->convertFromVisitablePos(source));
		for(const auto sourceLayer : {EPathfindingLayer::AIR, EPathfindingLayer::WATER})
		{
			hero->setConfirmedMovementLayer(sourceLayer);
			hero->setMovementPoints(3000);
			CPathsInfo paths(gameState()->getMapSize(), hero);
			auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
			CPathfinder pathfinder(*gameState(), config);
			pathfinder.calculatePaths();
			const EPathfindingLayer destinationLayer = sourceLayer == EPathfindingLayer::AIR ? EPathfindingLayer::WATER : EPathfindingLayer::AIR;
			const auto * node = paths.getNode(destination, destinationLayer);
			SCOPED_TRACE(source.toString() + " to " + destination.toString() + " layer " + std::to_string(sourceLayer));
			ASSERT_TRUE(node->reachable());
			ASSERT_EQ(node->turns, 0);
			ASSERT_NE(node->theNodeBefore, nullptr);
			EXPECT_EQ(node->theNodeBefore->coord, source);
			EXPECT_EQ(node->theNodeBefore->layer, sourceLayer);
		}
	}
}

TEST_F(HeroMovementTest, UnknownShoreDoesNotChangeTransientMoveAcceptance)
{
	const PlayerColor player(0);
	const int3 first(35, 5, 0), second(34, 5, 0), hidden(33, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 33; x <= 35; ++x)
		for(int y = 4; y <= 6; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	ASSERT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->movementPointsRemaining(), 200);
	map()->moveObject(hero->id, hero->convertFromVisitablePos(first));
	hero->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	hero->setMovementPoints(200);
	map()->getTile(hidden).terrainType = TerrainId::GRASS;
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	ASSERT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->movementPointsRemaining(), 200);
}

TEST_F(HeroMovementTest, AlreadyStrandedWaterWalkerCanContinueExploring)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0), first(35, 5, 0), second(34, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 32; x <= 35; ++x)
		for(int y = 2; y <= 8; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	for(int x = 1; x <= 3; ++x)
		for(int y = 2; y <= 8; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	for(int y = 2; y <= 8; ++y)
		if(y != 5)
			builder.terrain(int3(0, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	map()->getTile(origin).terrainType = TerrainId::WATER;
	hero->setMovementPoints(200);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), second);
	EXPECT_EQ(hero->movementPointsRemaining(), 100);
}

TEST_F(HeroMovementTest, StrandedWaterWalkerReturnsToTheLandWhereTransitStarted)
{
	const PlayerColor player(0);
	const int3 island(18, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(19, 18, 0), HeroTypeID(0), player)
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}});
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != island)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	const auto originalArmyStrength = hero->getArmyStrength();
	const auto * originalSpellbook = hero->getArt(ArtifactPosition::SPELLBOOK);
	ASSERT_GT(originalArmyStrength, 0);
	ASSERT_NE(originalSpellbook, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(1200);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	// Establish the recorded origin normally, then synthesize an unsafe state
	// as may occur after loading an older save or an interrupted move.
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(19, 18, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	map()->moveObject(hero->id, hero->convertFromVisitablePos(int3(29, 18, 0)));
	hero->setMovementPoints(100);
	EXPECT_EQ(hero->movementPointsRemaining(), 100);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(30, 18, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	const auto strandedPosition = hero->visitablePos();
	auto * team = gameState()->getPlayerTeam(player);
	ASSERT_TRUE(team->fogOfWarMap[island]);
	team->fogOfWarMap[island] = 0;
	EXPECT_FALSE(handler.tryAutoLandTransientHero(player));
	team->fogOfWarMap[island] = 1;
	map()->getTile(island).terrainType = TerrainId::WATER;
	EXPECT_FALSE(handler.tryAutoLandTransientHero(player));
	map()->getTile(island).terrainType = TerrainId::GRASS;
	const std::string eventDefinition = R"({ "animation": "test.def", "mask": [ "V" ] })";
	JsonNode eventJson(eventDefinition.data(), eventDefinition.size(), "TransientReturnEvent");
	auto eventAppearance = std::make_shared<ObjectTemplate>();
	eventAppearance->id = Obj::EVENT;
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	eventAppearance->readJson(eventJson, resolver, false);
	auto occupied = std::make_shared<CGObjectInstance>(gameState().get());
	occupied->ID = Obj::RESOURCE;
	occupied->pos = island;
	occupied->instanceName = "transient_return_occupied";
	occupied->appearance = eventAppearance;
	map()->addNewObject(occupied);
	ASSERT_FALSE(map()->getTile(island).visitableObjects.empty());
	EXPECT_FALSE(handler.tryAutoLandTransientHero(player));
	map()->removeObject(occupied->id);

	// Invisible events must neither disclose themselves by blocking recovery
	// nor receive a leave callback for a hero that merely flew over them.
	struct LeaveProbe : CGObjectInstance
	{
		using CGObjectInstance::CGObjectInstance;
		mutable bool left = false;
		mutable bool visited = false;
		void onHeroVisit(IGameEventCallback &, const CGHeroInstance *) const override { visited = true; }
		void onHeroLeave(IGameEventCallback &, const CGHeroInstance *) const override { left = true; }
	};
	auto hiddenEvent = std::make_shared<LeaveProbe>(gameState().get());
	hiddenEvent->ID = Obj::EVENT;
	hiddenEvent->pos = island;
	hiddenEvent->instanceName = "transient_return_event";
	hiddenEvent->appearance = eventAppearance;
	map()->addNewObject(hiddenEvent);
	auto sourceEvent = std::make_shared<LeaveProbe>(gameState().get());
	sourceEvent->ID = Obj::EVENT;
	sourceEvent->pos = strandedPosition;
	sourceEvent->instanceName = "transient_source_event";
	sourceEvent->appearance = eventAppearance;
	map()->hideObject(hero);
	map()->addNewObject(sourceEvent);
	map()->showObject(hero);
	EXPECT_EQ(hero->visitablePos(), strandedPosition);
	EXPECT_EQ(hero->movementPointsRemaining(), 100);

	// An old save with no recorded origin must not recover to hidden terrain.
	CMemorySerializer oldArchive;
	oldArchive.oser.version = ESerializationVersion::TOROIDAL_REPLICATED_MOVEMENT_LAYER;
	oldArchive.iser.version = ESerializationVersion::TOROIDAL_REPLICATED_MOVEMENT_LAYER;
	oldArchive.oser & handler;
	GameHandlerTestServer oldServer(gameState(), player);
	CGameHandler oldHandler(oldServer, gameState());
	oldArchive.iser.cb = gameState().get();
	oldArchive.iser & oldHandler;
	ASSERT_TRUE(oldArchive.isFullyRead());
	team->fogOfWarMap[island] = 0;
	EXPECT_FALSE(oldHandler.tryAutoLandTransientHero(player));
	EXPECT_EQ(hero->visitablePos(), strandedPosition);
	team->fogOfWarMap[island] = 1;

	// Current saves preserve the server-owned origin across a restore.
	CMemorySerializer archive;
	archive.oser & handler;
	GameHandlerTestServer restoredServer(gameState(), player);
	CGameHandler restored(restoredServer, gameState());
	archive.iser.cb = gameState().get();
	archive.iser & restored;
	ASSERT_TRUE(archive.isFullyRead());
	gameState()->players.at(player).human = true;
	gameState()->players.at(player).status = EPlayerStatus::INGAME;
	gameState()->actingPlayers.insert(player);
	gameState()->getStartInfo()->turnTimerInfo.turnTimer = 1;
	gameState()->getStartInfo()->turnTimerInfo.baseTimer = 0;
	restored.turnTimerHandler->onGameplayStart(player);
	restored.turnTimerHandler->onPlayerGetTurn(player);
	restored.turnTimerHandler->update(1);
	restored.turnTimerHandler->update(1);
	EXPECT_FALSE(restored.hasHeroInTransientMovement(player));
	EXPECT_EQ(hero->visitablePos(), island);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_EQ(hero->getOwner(), player);
	EXPECT_EQ(hero->getArmyStrength(), originalArmyStrength);
	EXPECT_EQ(hero->getArt(ArtifactPosition::SPELLBOOK), originalSpellbook);
	EXPECT_FALSE(sourceEvent->left);
	EXPECT_FALSE(hiddenEvent->visited);
	EXPECT_NE(gameState()->getObjInstance(hiddenEvent->id), nullptr);
}

TEST_F(HeroMovementTest, ToroidalTransientLandingSearchWorksOn252Map)
{
	const PlayerColor player(0);
	const int3 land(0, 125, 0), water(251, 125, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(252, false).playerActive(player)
		.hero(int3(1, 125, 0), HeroTypeID(0), player).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	ASSERT_EQ(map()->width, 252);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	for(int iteration = 0; iteration < 5; ++iteration)
	{
		hero->setMovementPoints(3000);
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(land), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	}
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, InPlaceLandingChecksAllHeroesBeforeChangingAny)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(1, 5, 0), HeroTypeID(0), player)
		.hero(int3(16, 15, 0), HeroTypeID(1), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * first = findHeroAt(int3(1, 5, 0));
	auto * second = findHeroAt(int3(16, 15, 0));
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	first->setConfirmedMovementLayer(EPathfindingLayer::AIR);
	second->setConfirmedMovementLayer(EPathfindingLayer::AIR);
	const auto firstPosition = first->visitablePos();
	const auto secondPosition = second->visitablePos();
	const int3 hiddenNeighbor(16, 15, 0);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenNeighbor] = 0;
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.tryLandTransientHeroesInPlace(player));
	EXPECT_EQ(first->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	EXPECT_EQ(second->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenNeighbor] = 1;
	ASSERT_TRUE(handler.tryLandTransientHeroesInPlace(player));
	EXPECT_EQ(first->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_EQ(second->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_EQ(first->visitablePos(), firstPosition);
	EXPECT_EQ(second->visitablePos(), secondPosition);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, AutomaticLandingCanStopOnCurrentClearLand)
{
	const PlayerColor player(0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(500);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != destination)
				map()->getTile(int3(x, y, 0)).terrainType = TerrainId::WATER;
	hero->setMovementPoints(0);
	const int3 hiddenNeighbor(0, 5, 0);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenNeighbor] = 0;
	EXPECT_FALSE(handler.tryAutoLandTransientHero(player));
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hiddenNeighbor] = 1;
	const std::string text = R"({ "animation": "test.def", "mask": [ "V" ] })";
	JsonNode definition(text.data(), text.size(), "InPlaceLandingEvent");
	auto appearance = std::make_shared<ObjectTemplate>();
	appearance->id = Obj::EVENT;
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto event = std::make_shared<CGObjectInstance>(gameState().get());
	event->ID = Obj::EVENT;
	event->pos = destination;
	event->instanceName = "in_place_landing_event";
	event->appearance = appearance;
	map()->hideObject(hero);
	map()->addNewObject(event);
	map()->showObject(hero);
	EXPECT_FALSE(handler.tryAutoLandTransientHero(player));
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	map()->hideObject(hero);
	map()->removeObject(event->id);
	map()->showObject(hero);

	TryMoveHero landing;
	landing.id = hero->id;
	landing.start = hero->anchorPos();
	landing.end = landing.start;
	landing.result = TryMoveHero::SUCCESS;
	landing.movePoints = 0;
	landing.movementLayer = EPathfindingLayer::LAND;
	PackRollbackGeneratorVisitor rollback(*gameState());
	landing.visit(rollback);
	ASSERT_TRUE(rollback.canBeRolledBack());
	auto rollbackPacks = rollback.acquireRollbackPacks();
	ASSERT_FALSE(rollbackPacks.empty());
	gameState()->players.at(player).human = true;
	gameState()->players.at(player).status = EPlayerStatus::INGAME;
	gameState()->actingPlayers.insert(player);
	gameState()->getStartInfo()->turnTimerInfo.turnTimer = 1;
	gameState()->getStartInfo()->turnTimerInfo.baseTimer = 0;
	handler.turnTimerHandler->onGameplayStart(player);
	handler.turnTimerHandler->onPlayerGetTurn(player);
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
	gameState()->apply(*rollbackPacks.front());
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	gameState()->apply(landing);
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	CMemorySerializer archive;
	archive.oser & handler;
	GameHandlerTestServer restoredServer(gameState(), player);
	CGameHandler restored(restoredServer, gameState());
	archive.iser.cb = gameState().get();
	archive.iser & restored;
	ASSERT_TRUE(archive.isFullyRead());
	EXPECT_FALSE(restored.hasHeroInTransientMovement(player));
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
}

TEST_F(HeroMovementTest, AutomaticLandingAdvancesOneOrdinaryStepAtATime)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), first(35, 5, 0), second(34, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 33; x <= 35; ++x)
		for(int y = 4; y <= 6; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(500);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(first), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(second), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_EQ(hero->movementPointsRemaining(), 300);
	ASSERT_TRUE(handler.tryAutoLandTransientHero(player));
	EXPECT_EQ(hero->visitablePos(), first);
	EXPECT_EQ(hero->getPathfindingStartLayer(), EPathfindingLayer::WATER);
	EXPECT_EQ(hero->movementPointsRemaining(), 200);
	ASSERT_TRUE(handler.tryAutoLandTransientHero(player));
	EXPECT_EQ(hero->visitablePos(), land);
	EXPECT_EQ(hero->getPathfindingStartLayer(), EPathfindingLayer::LAND);
	EXPECT_EQ(hero->movementPointsRemaining(), 100);
	EXPECT_FALSE(handler.tryAutoLandTransientHero(player));
}

TEST_F(HeroMovementTest, ExpiredTimerWaitsWhenNoSafeLandingExists)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), water(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(1, 5, 0), HeroTypeID(0), player).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			map()->getTile(int3(x, y, 0)).terrainType = TerrainId::WATER;
	const auto remaining = hero->movementPointsRemaining();
	gameState()->players.at(player).human = true;
	gameState()->players.at(player).status = EPlayerStatus::INGAME;
	gameState()->actingPlayers.insert(player);
	gameState()->getStartInfo()->turnTimerInfo.turnTimer = 1;
	gameState()->getStartInfo()->turnTimerInfo.baseTimer = 0;
	handler.turnTimerHandler->onGameplayStart(player);
	handler.turnTimerHandler->onPlayerGetTurn(player);
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	EXPECT_EQ(hero->visitablePos(), water);
	EXPECT_EQ(hero->movementPointsRemaining(), remaining);
	EXPECT_TRUE(handler.hasHeroInTransientMovement(player));
	EXPECT_TRUE(gameState()->actingPlayers.contains(player));
	// Recheck after the retry interval, without searching on every server tick.
	map()->getTile(land).terrainType = TerrainId::GRASS;
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	EXPECT_EQ(hero->visitablePos(), water);
	ASSERT_NO_THROW(handler.turnTimerHandler->update(999));
	EXPECT_EQ(hero->visitablePos(), land);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, MultipleTransientHeroesRecoverOnlyWhenTheWholePlanIsSafe)
{
	const PlayerColor player(0);
	const int3 firstLand(0, 5, 0), secondLand(0, 15, 0);
	const int3 firstWater(35, 5, 0), secondWater(35, 15, 0), thirdLand(10, 25, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(1, 5, 0), HeroTypeID(0), player)
		.hero(int3(1, 15, 0), HeroTypeID(1), player)
		.hero(int3(11, 25, 0), HeroTypeID(2), player)
		.terrain(firstWater, TerrainId::WATER).terrain(secondWater, TerrainId::WATER);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != firstLand && int3(x, y, 0) != secondLand && int3(x, y, 0) != thirdLand)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * first = findHeroAt(int3(1, 5, 0));
	auto * second = findHeroAt(int3(1, 15, 0));
	auto * third = findHeroAt(int3(11, 25, 0));
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	ASSERT_NE(third, nullptr);
	class RecordingServer : public GameHandlerTestServer
	{
	public:
		using GameHandlerTestServer::GameHandlerTestServer;
		std::vector<std::string> events;
		void applyPack(CPackForClient & pack) override
		{
			if(const auto * blocked = dynamic_cast<const PlayerBlocked *>(&pack); blocked && blocked->reason == PlayerBlocked::ONGOING_MOVEMENT)
				events.push_back(blocked->startOrEnd == PlayerBlocked::BLOCKADE_STARTED ? "begin" : "end");
			if(dynamic_cast<const TryMoveHero *>(&pack))
				events.push_back("move");
			GameHandlerTestServer::applyPack(pack);
		}
	};
	RecordingServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	for(auto * hero : {first, second})
	{
		hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
		hero->setMovementPoints(300);
	}
	ASSERT_TRUE(handler.moveHero(first->id, first->convertFromVisitablePos(firstWater), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_TRUE(handler.moveHero(second->id, second->convertFromVisitablePos(secondWater), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	first->setMovementPoints(1);
	second->setMovementPoints(1);
	third->setConfirmedMovementLayer(EPathfindingLayer::AIR);
	ASSERT_EQ(third->visitablePos(), thirdLand);

	const std::string text = R"({ "animation": "test.def", "mask": [ "V" ] })";
	JsonNode definition(text.data(), text.size(), "BlockedMultiHeroReturn");
	auto appearance = std::make_shared<ObjectTemplate>();
	appearance->id = Obj::EVENT;
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto occupied = std::make_shared<CGObjectInstance>(gameState().get());
	occupied->ID = Obj::RESOURCE;
	occupied->pos = secondLand;
	occupied->instanceName = "blocked_multi_hero_return";
	occupied->appearance = appearance;
	map()->addNewObject(occupied);
	server.events.clear();
	EXPECT_FALSE(handler.tryRecoverTransientHeroes(player));
	EXPECT_TRUE(server.events.empty());
	EXPECT_EQ(first->visitablePos(), firstWater);
	EXPECT_EQ(second->visitablePos(), secondWater);
	EXPECT_EQ(third->getConfirmedMovementLayer(), EPathfindingLayer::AIR);
	map()->removeObject(occupied->id);
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(server.events, (std::vector<std::string>{"begin", "move", "move", "move", "end"}));
	EXPECT_EQ(first->visitablePos(), firstLand);
	EXPECT_EQ(second->visitablePos(), secondLand);
	EXPECT_EQ(first->movementPointsRemaining(), 0);
	EXPECT_EQ(second->movementPointsRemaining(), 0);
	EXPECT_EQ(third->visitablePos(), thirdLand);
	EXPECT_EQ(third->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, AutomaticLandingCanSkipAStrandedHero)
{
	const PlayerColor player(0), opponent(1);
	const int3 firstWater(35, 5, 0), secondWater(35, 15, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).playerActive(opponent)
		.hero(int3(1, 5, 0), HeroTypeID(0), player)
		.hero(int3(1, 15, 0), HeroTypeID(1), player)
		.hero(int3(11, 10, 0), HeroTypeID(2), opponent)
		.terrain(firstWater, TerrainId::WATER).terrain(secondWater, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * first = findHeroAt(int3(1, 5, 0));
	auto * second = findHeroAt(int3(1, 15, 0));
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	for(auto * hero : {first, second})
	{
		hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
		hero->setMovementPoints(300);
	}
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(first->id, first->convertFromVisitablePos(firstWater), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_TRUE(handler.moveHero(second->id, second->convertFromVisitablePos(secondWater), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	for(const int x : {33, 34, 35, 0, 1})
		for(int y = 3; y <= 7; ++y)
			map()->getTile(int3(x, y, 0)).terrainType = TerrainId::WATER;
	ASSERT_TRUE(handler.tryAutoLandTransientHero(player));
	EXPECT_EQ(first->visitablePos(), firstWater);
	EXPECT_EQ(first->getPathfindingStartLayer(), EPathfindingLayer::WATER);
	EXPECT_EQ(second->getPathfindingStartLayer(), EPathfindingLayer::LAND);
	EXPECT_TRUE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, ExpiredTimerWaitsUntilAutomaticLandingIsAllowed)
{
	const PlayerColor player(0), opponent(1);
	const int3 water(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).playerActive(opponent)
		.hero(int3(1, 5, 0), HeroTypeID(0), player)
		.hero(int3(11, 10, 0), HeroTypeID(1), opponent).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	gameState()->players.at(player).human = true;
	gameState()->players.at(player).status = EPlayerStatus::INGAME;
	gameState()->actingPlayers.insert(player);
	gameState()->getStartInfo()->turnTimerInfo.turnTimer = 1;
	gameState()->getStartInfo()->turnTimerInfo.baseTimer = 0;
	handler.turnTimerHandler->onGameplayStart(player);
	handler.turnTimerHandler->onPlayerGetTurn(player);
	ASSERT_TRUE(handler.turnTimerHandler->isTimerEnabled(player));
	ASSERT_TRUE(gameState()->isPlayerMakingTurn(player));
	ASSERT_EQ(gameState()->players.at(player).status, EPlayerStatus::INGAME);
	ASSERT_EQ(handler.activities->topActivity(player), nullptr);
	handler.turnTimerHandler->setEndTurnAllowed(player, false);
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	EXPECT_EQ(hero->visitablePos(), water);
	EXPECT_TRUE(handler.hasHeroInTransientMovement(player));
	handler.turnTimerHandler->setEndTurnAllowed(player, true);
	ASSERT_NO_THROW(handler.turnTimerHandler->update(1));
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
	EXPECT_TRUE(gameState()->actingPlayers.contains(player));
}

TEST_F(HeroMovementTest, ExpiredTimerLandsHeroWhoEnteredWaterDuringTheTurn)
{
	const PlayerColor player(0), opponent(1);
	const int3 land(0, 5, 0), water(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).playerActive(opponent)
		.hero(int3(1, 5, 0), HeroTypeID(0), player)
		.hero(int3(11, 10, 0), HeroTypeID(1), opponent).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	gameState()->players.at(player).human = true;
	gameState()->players.at(player).status = EPlayerStatus::INGAME;
	gameState()->actingPlayers.insert(player);
	gameState()->getStartInfo()->turnTimerInfo.turnTimer = 1;
	gameState()->getStartInfo()->turnTimerInfo.baseTimer = 0;
	handler.turnTimerHandler->onGameplayStart(player);
	handler.turnTimerHandler->onPlayerGetTurn(player);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_TRUE(handler.hasHeroInTransientMovement(player));
	handler.turnTimerHandler->update(1);
	handler.turnTimerHandler->update(1);
	EXPECT_EQ(hero->visitablePos(), land);
	EXPECT_EQ(hero->movementPointsRemaining(), 100);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, AutomaticLandingDoesNotTriggerAnEvent)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), water(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(1, 5, 0), HeroTypeID(0), player).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			map()->getTile(int3(x, y, 0)).terrainType = TerrainId::WATER;
	map()->getTile(land).terrainType = TerrainId::GRASS;
	const std::string text = R"({ "animation": "test.def", "mask": [ "V" ] })";
	JsonNode definition(text.data(), text.size(), "AutomaticLandingEvent");
	auto appearance = std::make_shared<ObjectTemplate>();
	appearance->id = Obj::EVENT;
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto event = std::make_shared<CGObjectInstance>(gameState().get());
	event->ID = Obj::EVENT;
	event->pos = land;
	event->instanceName = "automatic_landing_event";
	event->appearance = appearance;
	map()->addNewObject(event);
	ASSERT_TRUE(map()->getTile(land).visitable());
	ASSERT_FALSE(map()->getTile(land).blocked());
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * landing = paths.getNode(land, EPathfindingLayer::LAND);
	ASSERT_TRUE(landing->reachable());
	ASSERT_EQ(landing->turns, 0);
	ASSERT_EQ(landing->action, EPathNodeAction::NORMAL);
	ASSERT_TRUE(handler.tryAutoLandTransientHero(player));
	EXPECT_EQ(hero->visitablePos(), land);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_NE(map()->getObject(event->id), nullptr);
}

TEST_F(HeroMovementTest, ProposedWaterWalkStartMatchesActualStateAcrossSeam)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), water(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player);
	for(int x = 33; x <= 35; ++x)
		for(int y = 4; y <= 6; ++y)
			builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), land);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	CPathsInfo proposed(gameState()->getMapSize(), hero);
	EXPECT_THROW((SingleHeroPathfinderConfig(proposed, *gameState(), hero, PathfinderStart{water, EPathfindingLayer::LAND, 200})), std::invalid_argument);
	EXPECT_THROW((SingleHeroPathfinderConfig(proposed, *gameState(), hero, PathfinderStart{water, EPathfindingLayer::WATER, -1})), std::invalid_argument);
	auto proposedConfig = std::make_shared<SingleHeroPathfinderConfig>(proposed, *gameState(), hero, PathfinderStart{water, EPathfindingLayer::WATER, 200});
	proposedConfig->options.turnLimit = 0;
	CPathfinder proposedSearch(*gameState(), proposedConfig);
	proposedSearch.calculatePaths();
	ASSERT_EQ(proposed.getNode(water, EPathfindingLayer::WATER)->moveRemains, 200);
	ASSERT_EQ(hero->visitablePos(), land);
	ASSERT_EQ(hero->movementPointsRemaining(), 300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	ASSERT_EQ(hero->movementPointsRemaining(), 200);
	CPathsInfo actual(gameState()->getMapSize(), hero);
	auto actualConfig = std::make_shared<SingleHeroPathfinderConfig>(actual, *gameState(), hero);
	actualConfig->options.turnLimit = 0;
	CPathfinder actualSearch(*gameState(), actualConfig);
	actualSearch.calculatePaths();
	ASSERT_EQ(actual.getNode(water, EPathfindingLayer::WATER)->moveRemains, 200);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			for(const auto layer : {EPathfindingLayer::LAND, EPathfindingLayer::WATER})
			{
				const int3 tile(x, y, 0);
				const auto * expected = actual.getNode(tile, layer);
				const auto * predicted = proposed.getNode(tile, layer);
				SCOPED_TRACE(tile.toString() + " layer " + std::to_string(layer)
					+ " accessibility " + std::to_string(static_cast<int>(predicted->accessible)) + "/" + std::to_string(static_cast<int>(expected->accessible))
					+ " predecessor " + (expected->theNodeBefore ? expected->theNodeBefore->coord.toString() : "none"));
				EXPECT_EQ(predicted->turns == 0, expected->turns == 0);
				if(predicted->turns == 0 && expected->turns == 0)
					EXPECT_EQ(predicted->moveRemains, expected->moveRemains);
			}
}

TEST_F(HeroMovementTest, ProposedFlightStartKeepsUnderlyingObjectAndMatchesActualState)
{
	const PlayerColor player(0);
	const int3 land(0, 5, 0), water(35, 5, 0), hidden(34, 4, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(1, 5, 0), HeroTypeID(0), player).terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), land);
	const std::string text = R"({ "animation": "test.def", "visitableFrom": ["+++", "+-+", "+++"], "mask": [ "A" ] })";
	JsonNode definition(text.data(), text.size(), "ProspectiveFlightObject");
	auto appearance = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	appearance->readJson(definition, resolver, false);
	auto object = std::make_shared<CGObjectInstance>(gameState().get());
	object->pos = land;
	object->instanceName = "prospective_flight_object";
	object->appearance = appearance;
	map()->hideObject(hero);
	map()->addNewObject(object);
	map()->showObject(hero);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	const auto originalObjects = map()->getTile(land).visitableObjects;
	CPathsInfo proposed(gameState()->getMapSize(), hero);
	auto proposedConfig = std::make_shared<SingleHeroPathfinderConfig>(proposed, *gameState(), hero, PathfinderStart{water, EPathfindingLayer::AIR, 200});
	proposedConfig->options.turnLimit = 0;
	const auto * viewTile = proposedConfig->options.mapView->getTile(land);
	EXPECT_TRUE(vstd::contains(viewTile->visitableObjects, object->id));
	EXPECT_FALSE(vstd::contains(viewTile->visitableObjects, hero->id));
	CPathfinder proposedSearch(*gameState(), proposedConfig);
	proposedSearch.calculatePaths();
	EXPECT_EQ(map()->getTile(land).visitableObjects, originalObjects);
	EXPECT_FALSE(proposed.getNode(hidden, EPathfindingLayer::AIR)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_EQ(hero->movementPointsRemaining(), 200);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	CPathsInfo actual(gameState()->getMapSize(), hero);
	auto actualConfig = std::make_shared<SingleHeroPathfinderConfig>(actual, *gameState(), hero);
	actualConfig->options.turnLimit = 0;
	CPathfinder actualSearch(*gameState(), actualConfig);
	actualSearch.calculatePaths();
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			for(const auto layer : {EPathfindingLayer::LAND, EPathfindingLayer::AIR})
			{
				const int3 tile(x, y, 0);
				const auto * expected = actual.getNode(tile, layer);
				const auto * predicted = proposed.getNode(tile, layer);
				SCOPED_TRACE(tile.toString() + " layer " + std::to_string(layer));
				EXPECT_EQ(predicted->turns == 0, expected->turns == 0);
				if(predicted->turns == 0 && expected->turns == 0)
					EXPECT_EQ(predicted->moveRemains, expected->moveRemains);
			}
}

TEST_F(HeroMovementTest, TransientMovementGuardChecksAllOwnedHeroes)
{
	const PlayerColor red(0), blue(1);
	const int3 land(1, 5, 0), water(0, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(red).playerActive(blue)
		.hero(int3(2, 5, 0), HeroTypeID(0), red)
		.hero(int3(6, 6, 0), HeroTypeID(1), red)
		.hero(int3(9, 8, 0), HeroTypeID(2), blue)
		.terrain(water, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(red);
	auto * walker = findHeroAt(int3(2, 5, 0));
	auto * otherHero = findHeroAt(int3(6, 6, 0));
	ASSERT_NE(walker, nullptr);
	ASSERT_NE(otherHero, nullptr);
	ASSERT_EQ(walker->visitablePos(), land);
	walker->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	walker->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), red);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.hasHeroInTransientMovement(red));
	ASSERT_TRUE(handler.moveHero(walker->id, walker->convertFromVisitablePos(water), EMovementMode::STANDARD, true, red, EPathfindingLayer::WATER));
	EXPECT_TRUE(handler.hasHeroInTransientMovement(red));
	EXPECT_FALSE(handler.hasHeroInTransientMovement(blue));
	map()->toroidal = false;
	EXPECT_FALSE(handler.hasHeroInTransientMovement(red));
	map()->toroidal = true;
	ASSERT_TRUE(handler.moveHero(walker->id, walker->convertFromVisitablePos(land), EMovementMode::STANDARD, false, red, EPathfindingLayer::LAND));
	EXPECT_FALSE(handler.hasHeroInTransientMovement(red));
	otherHero->setConfirmedMovementLayer(EPathfindingLayer::AIR);
	EXPECT_TRUE(handler.hasHeroInTransientMovement(red));
	EXPECT_FALSE(handler.hasHeroInTransientMovement(blue));
	otherHero->setConfirmedMovementLayer(EPathfindingLayer::AUTO);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(red));
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
	revealMap(player);
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

TEST_F(HeroMovementTest, WaterWalkingCannotEnterGuardedWaterAcrossSeam)
{
	const PlayerColor player(0);
	const int3 origin(0, 6, 0);
	const int3 destination(0, 5, 0);
	const int3 guardPosition(35, 5, 0);
	const int3 unguarded(1, 6, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 6, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.terrain(destination, TerrainId::WATER).terrain(guardPosition, TerrainId::WATER).terrain(unguarded, TerrainId::WATER)
		.monster(guardPosition, CreatureID(27), 100);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	map()->calculateGuardingGreaturePositions();
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	ASSERT_EQ(gameState()->guardingCreaturePosition(destination), guardPosition);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	EXPECT_FALSE(paths.getNode(destination, EPathfindingLayer::WATER)->reachable());
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_EQ(gameState()->getBattle(player), nullptr);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	const auto * node = paths.getNode(unguarded, EPathfindingLayer::WATER);
	ASSERT_TRUE(node->reachable());
	ASSERT_NE(node->theNodeBefore, nullptr);
	ASSERT_EQ(node->theNodeBefore->coord, origin);
	const int remaining = node->moveRemains;
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(unguarded), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), unguarded);
	EXPECT_EQ(hero->movementPointsRemaining(), remaining);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	CPathsInfo flyingPaths(gameState()->getMapSize(), hero);
	auto flyingConfig = std::make_shared<SingleHeroPathfinderConfig>(flyingPaths, *gameState(), hero);
	CPathfinder flyingPathfinder(*gameState(), flyingConfig);
	flyingPathfinder.calculatePaths();
	EXPECT_TRUE(flyingPaths.getNode(destination, EPathfindingLayer::AIR)->reachable());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_EQ(gameState()->getBattle(player), nullptr);
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

TEST_F(HeroMovementTest, SailingBoatCannotSwitchDirectlyToFlyingAcrossSeam)
{
	const PlayerColor player(0);
	const int3 source(0, 5, 0);
	const int3 destination(35, 5, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 5, 0), HeroTypeID(0), player)
		.terrain(source, TerrainId::WATER).terrain(destination, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->toroidal = true;
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(source, BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(source), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(3000);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * air = paths.getNode(destination, EPathfindingLayer::AIR);
	if(air->reachable() && air->theNodeBefore)
		EXPECT_FALSE(air->theNodeBefore->coord == source && air->theNodeBefore->layer == EPathfindingLayer::SAIL);
	const auto * water = paths.getNode(destination, EPathfindingLayer::WATER);
	if(water->reachable() && water->theNodeBefore)
		EXPECT_FALSE(water->theNodeBefore->coord == source && water->theNodeBefore->layer == EPathfindingLayer::SAIL);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	EXPECT_EQ(hero->visitablePos(), source);
	EXPECT_TRUE(hero->inBoat());
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	EXPECT_EQ(hero->visitablePos(), source);
	EXPECT_TRUE(hero->inBoat());
	EXPECT_EQ(hero->movementPointsRemaining(), 3000);
	const auto * sail = paths.getNode(destination, EPathfindingLayer::SAIL);
	ASSERT_TRUE(sail->reachable());
	ASSERT_NE(sail->theNodeBefore, nullptr);
	ASSERT_EQ(sail->theNodeBefore->coord, source);
	const int remaining = sail->moveRemains;
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_TRUE(hero->inBoat());
	EXPECT_EQ(hero->movementPointsRemaining(), remaining);
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

TEST_F(HeroMovementTest, BoatCanCrossToroidalCornerAfterSaveAndRestore)
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
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.createBoat(int3(0, 0, 0), BoatId::CASTLE, player);
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	hero->setMovementPoints(3000);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(35, 35, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	ASSERT_EQ(hero->visitablePos(), int3(35, 35, 0));
	ASSERT_NE(hero->getBoat(), nullptr);
	const auto heroID = hero->id;
	const auto boatID = hero->getBoat()->id;
	const auto movement = hero->movementPointsRemaining();

	CMemorySerializer stateBytes, headerBytes;
	gameState()->saveGame(stateBytes.oser);
	gameState()->saveHeader(headerBytes.oser);
	const auto path = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-boat-corner-save-%%%%-%%%%.vsgm1");
	struct TemporarySave
	{
		boost::filesystem::path path;
		~TemporarySave() { boost::filesystem::remove(path); }
	} temporarySave{path};
	CSaveFile save;
	save.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*gameState()).toBytes());
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
	ASSERT_TRUE(restoredHero->inBoat());
	ASSERT_NE(restoredHero->getBoat(), nullptr);
	EXPECT_EQ(restoredHero->getBoat()->id, boatID);
	EXPECT_EQ(restoredHero->visitablePos(), int3(35, 35, 0));
	EXPECT_EQ(restoredHero->movementPointsRemaining(), movement);

	GameHandlerTestServer restoredServer(restored, player);
	CGameHandler restoredHandler(restoredServer, restored);
	ASSERT_TRUE(restoredHandler.moveHero(heroID, restoredHero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::SAIL));
	EXPECT_TRUE(restoredHero->inBoat());
	EXPECT_EQ(restoredHero->getBoat()->id, boatID);
	EXPECT_EQ(restoredHero->visitablePos(), int3(0, 0, 0));
	EXPECT_LT(restoredHero->movementPointsRemaining(), movement);
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

TEST_F(HeroMovementTest, HeroCapturesMineAcrossToroidalSeam)
{
	const PlayerColor player(0);
	const int3 minePosition(10, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 0, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), int3(10, 0, 0));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	auto object = handler.createNewObject(minePosition, Obj::MINE, MapObjectSubID(0));
	auto * mine = dynamic_cast<CGMine *>(object.get());
	ASSERT_NE(mine, nullptr);
	ASSERT_EQ(mine->visitablePos(), minePosition);
	for(int x = 0; x < mine->getWidth(); ++x)
		for(int y = 0; y < mine->getHeight(); ++y)
		{
			const int3 tile = mine->anchorPos() - int3(x, y, 0);
			if(!map()->isInTheMap(tile))
				ASSERT_FALSE(mine->coveringAt(tile) || mine->blockingAt(tile) || mine->visitableAt(tile));
		}
	mine->setOwner(PlayerColor::NEUTRAL);
	handler.newObject(object, player);
	ASSERT_EQ(mine->getOwner(), PlayerColor::NEUTRAL);
	map()->toroidal = false;
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(minePosition), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(mine->getOwner(), PlayerColor::NEUTRAL);
	EXPECT_EQ(hero->visitablePos(), int3(10, 0, 0));
	map()->toroidal = true;
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder pathfinder(*gameState(), config);
	pathfinder.calculatePaths();
	const auto * destination = paths.getNode(minePosition, EPathfindingLayer::LAND);
	ASSERT_TRUE(destination->reachable());
	ASSERT_NE(destination->theNodeBefore, nullptr);
	EXPECT_EQ(destination->theNodeBefore->coord, hero->visitablePos());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(minePosition), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(mine->getOwner(), player);
	EXPECT_EQ(hero->visitablePos(), minePosition);
}

TEST_F(HeroMovementTest, HeroCanRevisitMineOnTheSameToroidalTile)
{
	const PlayerColor player(0);
	const int3 minePosition(10, 35, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 0, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	auto object = handler.createNewObject(minePosition, Obj::MINE, MapObjectSubID(0));
	auto * mine = dynamic_cast<CGMine *>(object.get());
	ASSERT_NE(mine, nullptr);
	mine->setOwner(PlayerColor::NEUTRAL);
	handler.newObject(object, player);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(minePosition), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	ASSERT_EQ(hero->visitablePos(), minePosition);
	ASSERT_EQ(mine->getOwner(), player);
	const auto remaining = hero->movementPointsRemaining();
	mine->setOwner(PlayerColor::NEUTRAL);
	ASSERT_TRUE(handler.moveHero(hero->id, hero->anchorPos(), EMovementMode::STANDARD, false, player, EPathfindingLayer::AUTO));
	EXPECT_EQ(mine->getOwner(), player);
	EXPECT_EQ(hero->visitablePos(), minePosition);
	EXPECT_EQ(hero->movementPointsRemaining(), remaining);
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

TEST_F(HeroMovementTest, WrappedMonsterBattleUsesTheCanonicalVisitablePosition)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}})
		.monster(int3(35, 35, 0), CreatureID(27), 100);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	const auto guardID = map()->getTile(int3(35, 35, 0)).topVisitableObj();
	map()->moveObject(guardID, int3(-1, -1, 0));
	map()->calculateGuardingGreaturePositions();
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	auto * guard = dynamic_cast<const CArmedInstance *>(map()->getObject(guardID));
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(guard, nullptr);
	ASSERT_EQ(guard->anchorPos(), int3(-1, -1, 0));
	ASSERT_EQ(guard->visitablePos(), int3(35, 35, 0));
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(0, 0, 0)), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	const auto * battle = gameState()->getBattle(player);
	ASSERT_NE(battle, nullptr);
	ASSERT_EQ(battle->getSideArmy(BattleSide::DEFENDER), guard);
	ASSERT_EQ(battle->getLocation(), guard->visitablePos());
	EXPECT_TRUE(battleMatchesMonsterPosition(*guard, battle->getLocation(), true));
	EXPECT_FALSE(battleMatchesMonsterPosition(*guard, int3(35, 34, 0), true));
	EXPECT_FALSE(battleMatchesMonsterPosition(*guard, int3(35, 35, 1), true));
	EXPECT_FALSE(battleMatchesMonsterPosition(*guard, battle->getLocation(), false));
	EXPECT_TRUE(battleMatchesMonsterPosition(*guard, guard->anchorPos(), false));
}

TEST_F(HeroMovementTest, HiddenMonsterGuardAcrossCornerStillStartsBattle)
{
	const PlayerColor player(0);
	const int3 guardPos(35, 35, 0), destination(0, 0, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 1, 0), HeroTypeID(0), player)
		.heroGarrison({{CreatureID::ARCHER, 1}}).monster(guardPos, CreatureID(27), 100);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	gameState()->getPlayerTeam(player)->fogOfWarMap[guardPos] = 0;
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	const auto * guard = dynamic_cast<const CArmedInstance *>(map()->getObject(map()->getTile(guardPos).topVisitableObj()));
	ASSERT_NE(guard, nullptr);
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(destination), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
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

TEST_F(HeroMovementTest, TownAppearanceUsesWrappedAnchorTerrain)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(player)
		.town(int3(10, 10, 0), FactionID::CASTLE, player)
		.hero(int3(15, 15, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(town, nullptr);
	GameRandomizer randomizer(*gameState());
	const auto originalAnchor = town->anchorPos();
	ASSERT_NO_THROW(town->getObjectHandler()->configureObject(town, randomizer));
	const auto originalAppearance = town->appearance;
	map()->setToroidalAndRebuild(true);
	for(const auto & offset : {int3(map()->width, 0, 0), int3(0, -map()->height, 0),
		int3(map()->width, -map()->height, 0), int3(-2 * map()->width, 2 * map()->height, 0)})
	{
		map()->moveObject(town->id, originalAnchor + offset);
		ASSERT_NO_THROW(town->getObjectHandler()->configureObject(town, randomizer)) << offset.toString();
		EXPECT_EQ(town->appearance, originalAppearance);
	}
	map()->setToroidalAndRebuild(false);
	EXPECT_ANY_THROW(town->getObjectHandler()->configureObject(town, randomizer));
}

TEST_F(HeroMovementTest, EditorMoveTracksOnlyStillSelectedMainTown)
{
	const PlayerColor player(0);
	const int3 original(10, 10, 0), destination(12, 12, 0), other(20, 20, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.town(original, FactionID::CASTLE, player)
		.town(other, FactionID::CASTLE, player)
		.playerMainTown(player, original, false)
		.hero(int3(25, 25, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	auto * town = expectAt<CGTownInstance>(original);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(town, nullptr);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(map()->players[0].posOfMainTown, original);
	auto * editor = map()->getEditManager();
	editor->moveObject(hero, hero->anchorPos() + int3(1, 0, 0));
	EXPECT_EQ(map()->players[0].posOfMainTown, original);
	editor->getUndoManager().undo();
	editor->moveObject(town, destination);
	EXPECT_EQ(map()->players[0].posOfMainTown, destination);
	map()->players[0].posOfMainTown = other;
	editor->getUndoManager().undo();
	EXPECT_EQ(town->anchorPos(), original);
	EXPECT_EQ(map()->players[0].posOfMainTown, other);
}

TEST_F(HeroMovementTest, ObjectiveResolvesTownAnchorAcrossToroidalSeams)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.town(int3(10, 10, 0), FactionID::CASTLE, player)
		.hero(int3(20, 20, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(town, nullptr);
	map()->setToroidalAndRebuild(true);
	map()->moveObject(town->id, int3(-1, -1, 0));
	const auto resolve = [&](EventCondition::EWinLoseType type, const int3 & position)
	{
		EventCondition condition(type);
		if(type == EventCondition::HAVE_BUILDING)
			condition.objectType = BuildingID(0);
		else if(type == EventCondition::TRANSPORT)
			condition.objectType = ArtifactID(0);
		else
			condition.objectType = MapObjectID(Obj::TOWN);
		condition.position = position;
		TriggeredEvent event;
		event.identifier = "wrapped_town_objective";
		event.effect.type = EventEffect::DEFEAT;
		event.trigger = EventExpression(EventExpression::Variant(condition));
		map()->triggeredEvents.clear();
		map()->triggeredEvents.push_back(event);
		map()->checkForObjectives();
		ObjectInstanceID resolved = ObjectInstanceID::NONE;
		map()->triggeredEvents.back().trigger.test([&](const EventCondition & current)
		{
			resolved = current.objectID;
			return true;
		});
		return resolved;
	};
	for(const auto type : {EventCondition::CONTROL, EventCondition::CONTROL_CURRENT,
		EventCondition::DESTROY, EventCondition::HAVE_BUILDING, EventCondition::TRANSPORT})
	{
		EXPECT_EQ(resolve(type, town->anchorPos()), town->id) << type;
		EXPECT_EQ(resolve(type, int3(35, 35, 0)), town->id) << type;
		EXPECT_EQ(resolve(type, town->visitablePos()), town->id) << type;
		EXPECT_EQ(resolve(type, int3(-1, -1, -1)), ObjectInstanceID::NONE) << type;
		EXPECT_EQ(resolve(type, int3(35, 35, 1)), ObjectInstanceID::NONE) << type;
	}
	map()->toroidal = false;
	EXPECT_EQ(resolve(EventCondition::CONTROL_CURRENT, town->anchorPos()), ObjectInstanceID::NONE);
}

TEST_F(HeroMovementTest, EditorMovePreservesObjectTargetedObjectives)
{
	const PlayerColor player(0);
	const PlayerColor opponent(1);
	const int3 original(10, 10, 0), destination(-1, -1, 0), other(20, 20, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).playerActive(opponent)
		.town(original, FactionID::CASTLE, player)
		.town(other, FactionID::CASTLE, opponent)
		.hero(int3(25, 25, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	auto * town = expectAt<CGTownInstance>(original);
	ASSERT_NE(town, nullptr);
	auto * otherTown = expectAt<CGTownInstance>(other);
	ASSERT_NE(otherTown, nullptr);
	map()->setToroidalAndRebuild(true);
	const int3 entrance = town->visitablePos();
	EventExpression::OperatorAll all;
	EventExpression::OperatorAny nested;
	for(const auto type : {EventCondition::CONTROL_CURRENT, EventCondition::HAVE_BUILDING,
		EventCondition::TRANSPORT, EventCondition::DESTROY})
	{
		EventCondition condition(type);
		if(type == EventCondition::HAVE_BUILDING)
			condition.objectType = BuildingID(0);
		else if(type == EventCondition::TRANSPORT)
			condition.objectType = ArtifactID(0);
		else
			condition.objectType = MapObjectID(Obj::TOWN);
		condition.position = type == EventCondition::TRANSPORT ? entrance : original;
		if(type == EventCondition::CONTROL_CURRENT)
			all.expressions.emplace_back(condition);
		else
			nested.expressions.emplace_back(condition);
	}
	all.expressions.emplace_back(nested);
	EventCondition unrelated(EventCondition::CONTROL_CURRENT);
	unrelated.objectType = MapObjectID(Obj::TOWN);
	unrelated.position = other;
	all.expressions.emplace_back(unrelated);
	EventCondition generic(EventCondition::CONTROL_CURRENT);
	generic.objectType = MapObjectID(Obj::TOWN);
	all.expressions.emplace_back(generic);
	TriggeredEvent event;
	event.identifier = "town_move_objectives";
	event.effect.type = EventEffect::VICTORY;
	event.trigger = EventExpression(all);
	map()->triggeredEvents.clear();
	map()->triggeredEvents.push_back(event);
	const auto positions = [&]()
	{
		std::vector<int3> result;
		map()->triggeredEvents[0].trigger.morph([&](const EventCondition & condition) -> EventExpression::Variant
		{
			result.push_back(condition.position);
			return condition;
		});
		return result;
	};
	const std::vector<int3> before{original, original, entrance, original, other, int3(-1, -1, -1)};
	const int3 shiftedEntrance = entrance + destination - original;
	const std::vector<int3> after{destination, destination, shiftedEntrance, destination, other, int3(-1, -1, -1)};
	ASSERT_EQ(positions(), before);
	auto * editor = map()->getEditManager();
	editor->moveObject(town, destination);
	EXPECT_EQ(positions(), after);
	editor->getUndoManager().undo();
	EXPECT_EQ(positions(), before);
	editor->getUndoManager().redo();
	EXPECT_EQ(positions(), after);
	const auto snapshot = gameState()->saveToMemory();
	auto restored = std::make_shared<CGameState>();
	restored->preInit(LIBRARY);
	restored->loadFromMemory(snapshot);
	restored->getMap().checkForObjectives();
	std::vector<EventCondition> resolved;
	restored->getMap().triggeredEvents[0].trigger.morph([&](const EventCondition & condition) -> EventExpression::Variant
	{
		resolved.push_back(condition);
		return condition;
	});
	ASSERT_EQ(resolved.size(), 6u);
	for(size_t i = 0; i < 4; ++i)
		EXPECT_EQ(resolved[i].objectID, town->id);
	EXPECT_EQ(resolved[4].objectID, otherTown->id);
	EXPECT_EQ(resolved[5].objectID, ObjectInstanceID::NONE);
	EXPECT_TRUE(restored->checkForVictory(player, resolved[0]));
	EXPECT_FALSE(restored->checkForVictory(player, resolved[4]));
	map()->triggeredEvents[0].trigger = map()->triggeredEvents[0].trigger.morph([](EventCondition condition) -> EventExpression::Variant
	{
		if(condition.condition == EventCondition::TRANSPORT)
			condition.value = 42;
		return condition;
	});
	editor->getUndoManager().undo();
	auto expected = before;
	expected[2] = shiftedEntrance;
	EXPECT_EQ(positions(), expected);
}

TEST_F(HeroMovementTest, EditorMoveTracksHeroPrisonAndPlaceholderObjectives)
{
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(10, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	const auto check = [&](CGObjectInstance * target, int3 destination)
	{
		EventCondition condition(EventCondition::DESTROY);
		condition.objectType = MapObjectID(Obj::HERO);
		condition.position = target->anchorPos();
		const auto original = condition.position;
		TriggeredEvent event;
		event.identifier = "hero_target";
		event.effect.type = EventEffect::VICTORY;
		event.trigger = EventExpression(EventExpression::Variant(condition));
		map()->triggeredEvents = {event};
		const auto position = [&]()
		{
			int3 result;
			map()->triggeredEvents[0].trigger.test([&](const EventCondition & current)
			{
				result = current.position;
				return true;
			});
			return result;
		};
		auto * editor = map()->getEditManager();
		editor->moveObject(target, destination);
		EXPECT_EQ(position(), destination);
		editor->getUndoManager().undo();
		EXPECT_EQ(position(), original);
	};
	check(hero, int3(-1, 10, 0));
	map()->hideObject(hero);
	hero->ID = Obj::PRISON;
	map()->showObject(hero);
	check(hero, int3(10, -1, 0));
	map()->hideObject(hero);
	hero->ID = Obj::HERO;
	map()->showObject(hero);
	auto placeholder = std::make_shared<CGHeroPlaceholder>(gameState().get());
	placeholder->ID = Obj::HERO_PLACEHOLDER;
	placeholder->appearance = hero->appearance;
	placeholder->pos = int3(15, 15, 0);
	placeholder->instanceName = "objective_placeholder";
	map()->addNewObject(placeholder);
	check(placeholder.get(), int3(-1, -1, 0));
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

void HeroMovementTest::checkDimensionDoorAcrossToroidalSeam(const int3 & start, const int3 & destination)
{
	const PlayerColor player(0);
	const SpellID spell(SpellID::decode("core:dimensionDoor"));
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(start + int3(1, 0, 0), HeroTypeID(0), player)
		.heroSpells({spell})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
		.heroPrimary(0, 0, 20, 20);
	startWithMap(std::move(builder));
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), start);
	hero->mana = 1000;
	hero->setMovementPoints(3000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.castSpell(hero, spell, destination);
	EXPECT_EQ(hero->visitablePos(), start);
	EXPECT_EQ(hero->mana, 1000);
	map()->toroidal = true;
	handler.castSpell(hero, spell, destination);
	EXPECT_EQ(hero->visitablePos(), destination);
	EXPECT_LT(hero->mana, 1000);
	EXPECT_LT(hero->movementPointsRemaining(), 3000);
}

TEST_F(HeroMovementTest, DimensionDoorCastsAcrossToroidalSeam)
{
	checkDimensionDoorAcrossToroidalSeam(int3(0, 10, 0), int3(35, 10, 0));
}

TEST_F(HeroMovementTest, DimensionDoorCastsAcrossToroidalCorner)
{
	checkDimensionDoorAcrossToroidalSeam(int3(0, 0, 0), int3(35, 35, 0));
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

TEST_F(HeroMovementTest, FlightCanLandInPlaceOnOnlyReachableShore)
{
	const PlayerColor player(0);
	const int3 origin(10, 10, 0), water(11, 10, 0), shore(12, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != origin && int3(x, y, 0) != shore)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(shore), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_TRUE(handler.tryLandTransientHeroesInPlace(player));
	EXPECT_EQ(hero->visitablePos(), shore);
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
}

TEST_F(HeroMovementTest, InPlaceFlightLandingPreventsUnsafeDeparture)
{
	const PlayerColor player(0);
	const int3 origin(10, 10, 0), shore(11, 10, 0), water(12, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != origin && int3(x, y, 0) != shore)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(250);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(shore), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	ASSERT_EQ(hero->movementPointsRemaining(), 150);
	EXPECT_FALSE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::AIR));
	EXPECT_EQ(hero->visitablePos(), shore);
	EXPECT_EQ(hero->movementPointsRemaining(), 150);
	ASSERT_TRUE(handler.tryLandTransientHeroesInPlace(player));
}

TEST_F(HeroMovementTest, TransientRecoveryMustNotReturnHeroToAnotherLevel)
{
	const PlayerColor player(0);
	const int3 origin(0, 5, 0), surfaceWater(35, 5, 0), undergroundWater(10, 10, 1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, true).playerActive(player).hero(int3(1, 5, 0), HeroTypeID(0), player)
		.terrain(surfaceWater, TerrainId::WATER).terrain(undergroundWater, TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), origin);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(surfaceWater), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));

	// Simulate a stale recovery origin, as could appear in inconsistent saved
	// state: the hero is now stranded on another level with no points to land.
	map()->moveObject(hero->id, hero->convertFromVisitablePos(undergroundWater));
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			map()->getTile(int3(x, y, 1)).terrainType = TerrainId::WATER;
	hero->setMovementPoints(1);
	ASSERT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::WATER);
	CMemorySerializer archive;
	archive.oser & handler;
	GameHandlerTestServer restoredServer(gameState(), player);
	CGameHandler restored(restoredServer, gameState());
	archive.iser.cb = gameState().get();
	archive.iser & restored;
	ASSERT_TRUE(archive.isFullyRead());
	EXPECT_FALSE(restored.tryRecoverTransientHeroes(player));
	EXPECT_FALSE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(hero->visitablePos(), undergroundWater);
	EXPECT_EQ(hero->movementPointsRemaining(), 1);
	EXPECT_TRUE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, OldSaveCanRecoverToNearestKnownSafeLand)
{
	const PlayerColor player(0);
	const int3 origin(0, 18, 0), water(1, 18, 0), nearest(35, 18, 0), farther(5, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 18, 0), HeroTypeID(0), player)
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}});
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != origin && int3(x, y, 0) != nearest && int3(x, y, 0) != farther)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	const auto army = hero->getArmyStrength();
	const auto * spellbook = hero->getArt(ArtifactPosition::SPELLBOOK);
	ASSERT_GT(army, 0);
	ASSERT_NE(spellbook, nullptr);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(water), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	map()->getTile(origin).terrainType = TerrainId::WATER;
	hero->setMovementPoints(1);

	// The preceding save version cannot contain a recorded transit origin.
	CMemorySerializer archive;
	archive.oser.version = ESerializationVersion::TOROIDAL_REPLICATED_MOVEMENT_LAYER;
	archive.iser.version = ESerializationVersion::TOROIDAL_REPLICATED_MOVEMENT_LAYER;
	archive.oser & handler;
	GameHandlerTestServer restoredServer(gameState(), player);
	CGameHandler restored(restoredServer, gameState());
	archive.iser.cb = gameState().get();
	archive.iser & restored;
	ASSERT_TRUE(archive.isFullyRead());
	ASSERT_TRUE(restored.tryRecoverTransientHeroes(player));
	EXPECT_EQ(hero->visitablePos(), nearest);
	EXPECT_EQ(hero->getConfirmedMovementLayer(), EPathfindingLayer::LAND);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_EQ(hero->getArmyStrength(), army);
	EXPECT_EQ(hero->getArt(ArtifactPosition::SPELLBOOK), spellbook);
	EXPECT_FALSE(restored.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, EmergencyRelocationDoesNotRevealHiddenLand)
{
	const PlayerColor player(0);
	const int3 start(18, 18, 0), hiddenLand(19, 18, 0), knownLand(28, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(19, 18, 0), HeroTypeID(0), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != start && int3(x, y, 0) != hiddenLand && int3(x, y, 0) != knownLand)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	map()->getTile(start).terrainType = TerrainId::WATER;
	hero->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	hero->setMovementPoints(1);
	auto * team = gameState()->getPlayerTeam(player);
	ASSERT_NE(team, nullptr);
	team->fogOfWarMap[hiddenLand] = 0;
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(hero->visitablePos(), knownLand);
	EXPECT_FALSE(team->fogOfWarMap[hiddenLand]);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}

TEST_F(HeroMovementTest, EmergencyRelocationReservesOtherHeroesOrigins)
{
	const PlayerColor player(0);
	const int3 firstWater(9, 10, 0), secondOrigin(11, 10, 0), secondWater(12, 10, 0), fallback(5, 10, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(10, 10, 0), HeroTypeID(0), player)
		.hero(int3(12, 10, 0), HeroTypeID(1), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != firstWater && int3(x, y, 0) != secondOrigin && int3(x, y, 0) != fallback)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * first = findHeroAt(int3(10, 10, 0));
	auto * second = findHeroAt(int3(12, 10, 0));
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	for(auto * hero : {first, second})
		hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	map()->getTile(firstWater).terrainType = TerrainId::WATER;
	first->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	first->setMovementPoints(1);
	second->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(second->id, second->convertFromVisitablePos(secondWater), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	second->setMovementPoints(1);
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(first->visitablePos(), fallback);
	EXPECT_EQ(second->visitablePos(), secondOrigin);
	EXPECT_EQ(first->movementPointsRemaining(), 0);
	EXPECT_EQ(second->movementPointsRemaining(), 0);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, EmergencyRelocationPrioritizesHeroWithLargerSight)
{
	const PlayerColor player(0);
	const int3 firstWater(19, 18, 0), secondWater(21, 18, 0);
	const int3 sharedLand(20, 18, 0), otherLand(5, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(20, 18, 0), HeroTypeID(0), player)
		.hero(int3(22, 18, 0), HeroTypeID(1), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != firstWater && int3(x, y, 0) != secondWater
				&& int3(x, y, 0) != sharedLand && int3(x, y, 0) != otherLand)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * first = findHeroAt(int3(20, 18, 0));
	auto * second = findHeroAt(int3(22, 18, 0));
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	const int smallSight = first->getSightRadius();
	second->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SIGHT_RADIUS, BonusSource::OTHER, 4, BonusSourceID()));
	ASSERT_GE(second->getSightRadius(), smallSight + 4);
	const int3 hidden(5, 18 + smallSight + 1, 0);
	ASSERT_LT(hidden.y, 36);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	for(auto * hero : {first, second})
	{
		map()->getTile(hero->visitablePos()).terrainType = TerrainId::WATER;
		hero->setConfirmedMovementLayer(EPathfindingLayer::WATER);
		hero->setMovementPoints(1);
	}
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(first->visitablePos(), otherLand);
	EXPECT_EQ(second->visitablePos(), sharedLand);
	EXPECT_FALSE(gameState()->getPlayerTeam(player)->fogOfWarMap[hidden]);
}

TEST_F(HeroMovementTest, EmergencyRelocationCanReassignOriginWhenNeededForGroup)
{
	const PlayerColor player(0);
	const int3 firstWater(19, 18, 0), secondWater(21, 18, 0);
	const int3 secondOrigin(20, 18, 0), otherLand(5, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player)
		.hero(int3(20, 18, 0), HeroTypeID(0), player)
		.hero(int3(21, 18, 0), HeroTypeID(1), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != firstWater && int3(x, y, 0) != secondOrigin && int3(x, y, 0) != otherLand)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * first = findHeroAt(int3(20, 18, 0));
	auto * second = findHeroAt(int3(21, 18, 0));
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	const int smallSight = second->getSightRadius();
	first->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::SIGHT_RADIUS, BonusSource::OTHER, 4, BonusSourceID()));
	ASSERT_GE(first->getSightRadius(), smallSight + 4);
	for(auto * hero : {first, second})
		hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	map()->getTile(firstWater).terrainType = TerrainId::WATER;
	first->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	first->setMovementPoints(1);
	second->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(second->id, second->convertFromVisitablePos(secondWater), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	second->setMovementPoints(1);
	const int3 hidden(5, 18 + smallSight + 1, 0);
	ASSERT_LT(hidden.y, 36);
	gameState()->getPlayerTeam(player)->fogOfWarMap[hidden] = 0;
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(first->visitablePos(), secondOrigin);
	EXPECT_EQ(second->visitablePos(), otherLand);
	EXPECT_FALSE(gameState()->getPlayerTeam(player)->fogOfWarMap[hidden]);
}

TEST_F(HeroMovementTest, EmergencyRelocationFindsNearbyLandOnLargeMap)
{
	const PlayerColor player(0);
	const int3 water(126, 126, 0), expectedLand(125, 126, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(252, false).playerActive(player).hero(int3(127, 126, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	map()->getTile(water).terrainType = TerrainId::WATER;
	hero->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	hero->setMovementPoints(1);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(hero->visitablePos(), expectedLand);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}

TEST_F(HeroMovementTest, EmergencyRecoveryPrefersRecordedOrigin)
{
	const PlayerColor player(0);
	const int3 origin(0, 18, 0), departure(1, 18, 0), stranded(9, 18, 0), closerLand(10, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(1, 18, 0), HeroTypeID(0), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != origin && int3(x, y, 0) != closerLand)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(300);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(departure), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	// Isolate recovery preference from ordinary movement by constructing a stranded state.
	map()->moveObject(hero->id, hero->convertFromVisitablePos(stranded));
	hero->setMovementPoints(1);
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(hero->visitablePos(), origin);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}

TEST_F(HeroMovementTest, EmergencyRelocationAvoidsOccupiedLand)
{
	const PlayerColor player(0);
	const int3 start(18, 18, 0), occupied(19, 18, 0), safeLand(25, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(19, 18, 0), HeroTypeID(0), player)
		.resource(occupied, GameResID::GOLD, 10);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != start && int3(x, y, 0) != occupied && int3(x, y, 0) != safeLand)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	map()->getTile(start).terrainType = TerrainId::WATER;
	hero->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	hero->setMovementPoints(1);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	ASSERT_TRUE(handler.tryRecoverTransientHeroes(player));
	EXPECT_EQ(hero->visitablePos(), safeLand);
}

// Reproducer: pathfinding currently offers this route, but the authoritative
// server rejects its final step to preserve a known landing. Enable when both
// sides use compatible route validation.
TEST_F(HeroMovementTest, DISABLED_TransientClientServerRouteAgreement)
{
	const PlayerColor player(0);
	const int3 island(18, 18, 0), destination(25, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(19, 18, 0), HeroTypeID(0), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != island)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	revealMap(player);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(1200);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder finder(*gameState(), config);
	finder.calculatePaths();
	CGPath route;
	ASSERT_TRUE(paths.getPath(route, destination, EPathfindingLayer::WATER));
	ASSERT_EQ(paths.getNode(destination, EPathfindingLayer::WATER)->turns, 0);
	ASSERT_EQ(route.nodes.size(), 8u);
	for(int x = 18; x <= 25; ++x)
		ASSERT_EQ(route.nodes[25 - x].coord, int3(x, 18, 0));
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	for(int x = 19; x <= 25; ++x)
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(x, 18, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER)) << x;
}

void HeroMovementTest::checkSingleIslandTransitStopsBeforeStranding(SpellID spell, BonusType bonus, EPathfindingLayer layer)
{
	const PlayerColor player(0);
	const int3 island(18, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(19, 18, 0), HeroTypeID(0), player)
		.heroSpells({spell})
		.heroEquipped({{ArtifactPosition::SPELLBOOK, ArtifactID::SPELLBOOK}})
		.heroPrimary(1, 1, 10, 10)
		.heroSecondarySkills({{SecondarySkill::WISDOM, 3}, {SecondarySkill::AIR_MAGIC, 3}, {SecondarySkill::WATER_MAGIC, 3}});
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != island)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	hero->mana = 100;
	hero->setMovementPoints(1200);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	handler.castSpell(hero, spell, island);
	ASSERT_TRUE(hero->hasBonusOfType(bonus));

	int accepted = 0;
	bool rejected = false;
	for(int x = 19; x <= 29; ++x)
	{
		if(!handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(x, 18, 0)), EMovementMode::STANDARD, true, player, layer))
		{
			rejected = true;
			break;
		}
		++accepted;
	}
	ASSERT_TRUE(rejected);
	EXPECT_GT(accepted, 0);
	EXPECT_LT(accepted, 11);
	for(int steps = 0; steps < 12 && handler.hasHeroInTransientMovement(player); ++steps)
	{
		const auto before = hero->visitablePos();
		const auto pointsBefore = hero->movementPointsRemaining();
		ASSERT_TRUE(handler.tryAutoLandTransientHero(player));
		EXPECT_EQ(hero->visitablePos(), before + int3(-1, 0, 0));
		EXPECT_LT(hero->movementPointsRemaining(), pointsBefore);
	}
	EXPECT_EQ(hero->visitablePos(), island);
	EXPECT_FALSE(handler.hasHeroInTransientMovement(player));
}

TEST_F(HeroMovementTest, WaterWalkStopsBeforeStrandingNearFog)
{
	checkSingleIslandTransitStopsBeforeStranding(SpellID::decode("core:waterWalk"), BonusType::WATER_WALKING, EPathfindingLayer::WATER);
}

TEST_F(HeroMovementTest, FlightStopsBeforeStrandingNearFog)
{
	checkSingleIslandTransitStopsBeforeStranding(SpellID::decode("core:fly"), BonusType::FLYING_MOVEMENT, EPathfindingLayer::AIR);
}

TEST_F(HeroMovementTest, VisibleShoreRemainsReachableNearFog)
{
	const PlayerColor player(0);
	const int3 origin(1, 18, 0), shore(21, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 18, 0), HeroTypeID(0), player);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != origin && int3(x, y, 0) != shore)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	auto * team = gameState()->getPlayerTeam(player);
	ASSERT_NE(team, nullptr);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			team->fogOfWarMap[int3(x, y, 0)] = 0;
	for(int x = 0; x <= 6; ++x)
		for(int y = 13; y <= 23; ++y)
			team->fogOfWarMap[int3(x, y, 0)] = 1;
	for(int x = 1; x <= 21; ++x)
		team->fogOfWarMap[int3(x, 18, 0)] = 1;
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	hero->setMovementPoints(2200);
	CPathsInfo paths(gameState()->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState(), hero);
	CPathfinder finder(*gameState(), config);
	finder.calculatePaths();
	CGPath route;
	ASSERT_TRUE(paths.getPath(route, shore, EPathfindingLayer::LAND));
	ASSERT_EQ(paths.getNode(shore, EPathfindingLayer::LAND)->turns, 0);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	for(int x = 2; x <= 20; ++x)
	{
		SCOPED_TRACE(x);
		ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(int3(x, 18, 0)), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER));
	}
	ASSERT_TRUE(handler.moveHero(hero->id, hero->convertFromVisitablePos(shore), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));
	EXPECT_EQ(hero->visitablePos(), shore);
}

TEST_F(HeroMovementTest, HiddenGuardDoesNotChangeTransitSafetyDecision)
{
	const PlayerColor player(0);
	const int3 origin(1, 18, 0), shore(21, 18, 0), guardPos(20, 19, 0);
	const int3 start(12, 18, 0), next(13, 18, 0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(2, 18, 0), HeroTypeID(0), player)
		.monster(guardPos, CreatureID(0), 10);
	for(int x = 0; x < 36; ++x)
		for(int y = 0; y < 36; ++y)
			if(int3(x, y, 0) != origin && int3(x, y, 0) != shore && int3(x, y, 0) != guardPos)
				builder.terrain(int3(x, y, 0), TerrainId::WATER);
	startWithMap(std::move(builder));
	map()->setToroidalAndRebuild(true);
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	auto * team = gameState()->getPlayerTeam(player);
	ASSERT_NE(team, nullptr);
	const auto setVisibleArea = [&]()
	{
		for(int x = 0; x < 36; ++x)
			for(int y = 0; y < 36; ++y)
				team->fogOfWarMap[int3(x, y, 0)] = 0;
		for(int x = 1; x <= 21; ++x)
			team->fogOfWarMap[int3(x, 18, 0)] = 1;
		for(int x = 7; x <= 17; ++x)
			for(int y = 13; y <= 23; ++y)
				team->fogOfWarMap[int3(x, y, 0)] = 1;
	};
	setVisibleArea();
	ASSERT_FALSE(team->fogOfWarMap[guardPos]);
	ASSERT_EQ(gameState()->guardingCreaturePosition(shore), guardPos);
	const auto * guard = map()->getObject(map()->getTile(guardPos).topVisitableObj());
	ASSERT_NE(guard, nullptr);
	hero->addNewBonus(std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::WATER_WALKING, BonusSource::OTHER, 0, BonusSourceID()));
	map()->moveObject(hero->id, hero->convertFromVisitablePos(start));
	hero->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	hero->setMovementPoints(1000);
	GameHandlerTestServer server(gameState(), player);
	CGameHandler handler(server, gameState());
	const bool withGuard = handler.moveHero(hero->id, hero->convertFromVisitablePos(next), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER);
	ASSERT_TRUE(handler.removeObject(guard, player));
	map()->moveObject(hero->id, hero->convertFromVisitablePos(start));
	hero->setConfirmedMovementLayer(EPathfindingLayer::WATER);
	hero->setMovementPoints(1000);
	setVisibleArea();
	const bool withoutGuard = handler.moveHero(hero->id, hero->convertFromVisitablePos(next), EMovementMode::STANDARD, true, player, EPathfindingLayer::WATER);
	EXPECT_EQ(withGuard, withoutGuard);
	EXPECT_TRUE(withoutGuard);
}
