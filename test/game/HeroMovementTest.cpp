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
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/filesystem/CFilesystemLoader.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/AdapterLoaders.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGResource.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/mapping/MapTopology.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/CMapInfo.h"
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
	auto source = TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false).name("Toroid TCP Lobby")
		.playerActive(red).playerActive(blue)
		.playerMainTown(red, int3(8, 8, 0), false)
		.playerMainTown(blue, int3(27, 27, 0), false)
		.town(int3(8, 8, 0), FactionID(0), red)
		.town(int3(27, 27, 0), FactionID(0), blue)
		.hero(int3(1, 0, 0), HeroTypeID(0), red)
		.hero(int3(1, 5, 0), HeroTypeID(1), blue)
		.resource(int3(35, 35, 0), GameResID::GOLD, 10).build();
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
		explicit LobbyPeer(std::string name) : name(std::move(name)) {}
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
			}
		}
		std::string name;
		std::string failure;
		bool negotiated = false;
		std::map<uint32_t, bool> results;
		std::shared_ptr<INetworkConnection> transport;
		std::unique_ptr<GameConnection> codec;
		std::shared_ptr<CGameState> state;
	};

	CVCMIServer server(0, true);
	const auto port = server.prepare(false, true);
	ASSERT_NE(port, 0);
	auto network = INetworkHandler::createHandler();
	LobbyPeer host("lobby-host"), guest("lobby-guest");
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
	network->connectToRemote(guest, "127.0.0.1", port);
	pumpUntil([&] { return guest.negotiated; });
	ASSERT_TRUE(guest.negotiated) << guest.failure;
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
	LobbyStartGame start;
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
		EXPECT_EQ(peer->state->getHero(blueHero->id)->visitablePos(), blueDestination) << peer->failure;
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
	for(const bool toroidal : {false, true})
	{
		map()->toroidal = toroidal;
		const std::string name = "Saves/" + directory.filename().string() + (toroidal ? "-toroidal" : "-bounded");
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
		const auto expectedVersion = SaveFileFormat::versionForTopology(toroidal);
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
	const PlayerColor player(0);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(player).hero(int3(11, 10, 0), HeroTypeID(0), player)
		.terrain(int3(11, 10, 0), TerrainId::ROCK);
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

	MoveHero allowed({hero->convertFromVisitablePos(int3(9, 10, 0))}, EPathfindingLayer::LAND, hero->id, false);
	allowed.player = player;
	allowed.requestID = 2;
	handler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, allowed);
	ASSERT_TRUE(server.results.contains(2));
	EXPECT_TRUE(server.results.at(2));
	EXPECT_EQ(hero->visitablePos(), int3(9, 10, 0));
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
