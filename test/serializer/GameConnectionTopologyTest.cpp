/*
 * GameConnectionTopologyTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/network/NetworkInterface.h"
#include "../../lib/networkPacks/LobbyTopologyCapability.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/serializer/ESerializationVersion.h"
#include "../../lib/serializer/GameConnection.h"
#include "../../server/CVCMIServer.h"
#include "../../server/LobbyNetPackVisitors.h"

namespace
{
class PacketCapture final : public INetworkConnection
{
public:
	void sendPacket(const std::vector<std::byte> & message) override { bytes = message; }
	void setAsyncWritesEnabled(bool) override {}
	void close() override { closed = true; }
	std::vector<std::byte> bytes;
	bool closed = false;
};

class LobbyProbe final : public INetworkClientListener
{
public:
	LobbyProbe(std::string uuid, ESerializationVersion offered) : uuid(std::move(uuid)), offered(offered) {}
	void onConnectionFailed(const std::string & error) override { failure = error; }
	void onDisconnected(const std::shared_ptr<INetworkConnection> &, const std::string & error) override { failure = error; }
	void onConnectionEstablished(const std::shared_ptr<INetworkConnection> & connection) override
	{
		transport = connection;
		codec = std::make_unique<GameConnection>(transport);
		LobbyClientConnected greeting;
		greeting.uuid = uuid;
		greeting.names = {uuid};
		greeting.mode = EStartMode::NEW_GAME;
		greeting.version = offered;
		codec->sendPack(greeting);
	}
	void onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & bytes) override
	{
		auto packet = codec->retrievePack(bytes);
		if(!ack && !dynamic_cast<LobbyClientConnected *>(packet.get()))
			++packetsBeforeOwnAcknowledgement;
		if(ack && dynamic_cast<LobbyClientConnected *>(packet.get()))
			++otherAcknowledgements;
		if(dynamic_cast<LobbyChatMessage *>(packet.get()))
			++receivedChats;
		const auto * connected = dynamic_cast<const LobbyClientConnected *>(packet.get());
		if(connected && connected->uuid == uuid)
		{
			ack = LobbyTopologyCapability::decode(connected->version);
			codec->setSerializationVersion(ack->version);
			if(ack->toroidal && ack->version >= ESerializationVersion::TOROIDAL_FRAMED_PACKETS)
				codec->enableFramedPackets();
		}
	}
	std::string uuid;
	ESerializationVersion offered;
	std::string failure;
	std::optional<LobbyTopologyCapability::Greeting> ack;
	int packetsBeforeOwnAcknowledgement = 0;
	int otherAcknowledgements = 0;
	int receivedChats = 0;
	std::shared_ptr<INetworkConnection> transport;
	std::unique_ptr<GameConnection> codec;
};

bool topologyAfterPacketRoundTrip(ESerializationVersion senderVersion, ESerializationVersion receiverVersion)
{
	auto transport = std::make_shared<PacketCapture>();
	GameConnection sender(transport);
	GameConnection receiver(transport);
	sender.setSerializationVersion(senderVersion);
	receiver.setSerializationVersion(receiverVersion);
	if(senderVersion >= ESerializationVersion::TOROIDAL_FRAMED_PACKETS)
		sender.enableFramedPackets();
	if(receiverVersion >= ESerializationVersion::TOROIDAL_FRAMED_PACKETS)
		receiver.enableFramedPackets();
	LobbyUpdateMapInfo outgoing;
	outgoing.mapInfo = std::make_shared<CMapInfo>();
	outgoing.mapInfo->mapHeader = std::make_unique<CMapHeader>();
	outgoing.mapInfo->mapHeader->toroidal = true;
	sender.sendPack(outgoing);
	auto incoming = receiver.retrievePack(transport->bytes);
	const auto * mapUpdate = dynamic_cast<const LobbyUpdateMapInfo *>(incoming.get());
	if(!mapUpdate || !mapUpdate->mapInfo || !mapUpdate->mapInfo->mapHeader)
		throw std::runtime_error("Lobby map header was not received");
	return mapUpdate->mapInfo->mapHeader->toroidal;
}
}

TEST(GameConnectionTopologyTest, CurrentVersionCarriesToroidalMapHeader)
{
	EXPECT_TRUE(topologyAfterPacketRoundTrip(ESerializationVersion::CURRENT, ESerializationVersion::CURRENT));
}

TEST(GameConnectionTopologyTest, PreviousVersionOmitsToroidalMapHeader)
{
	EXPECT_FALSE(topologyAfterPacketRoundTrip(ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER, ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER));
}

TEST(GameConnectionTopologyTest, PreviousTaggedToroidalPeerNegotiatesItsOwnVersion)
{
	const auto prior = ESerializationVersion::TOROIDAL_MAP;
	const auto accepted = LobbyTopologyCapability::negotiateClient(LobbyTopologyCapability::encode(prior));
	EXPECT_TRUE(accepted.toroidal);
	EXPECT_EQ(accepted.version, prior);
	EXPECT_FALSE(LobbyTopologyCapability::isSafeServerGreeting(accepted));
	EXPECT_TRUE(topologyAfterPacketRoundTrip(prior, prior));
	auto transport = std::make_shared<PacketCapture>();
	auto connection = std::make_shared<GameConnection>(transport);
	connection->setSerializationVersion(prior);
	connection->setToroidalTopologySupported(true);
	CVCMIServer server(0, true);
	server.activeConnections.push_back(connection);
	CMapHeader map;
	map.toroidal = true;
	EXPECT_FALSE(server.supportsMapTopology(map));
	map.toroidal = false;
	EXPECT_TRUE(server.supportsMapTopology(map));
}

TEST(GameConnectionTopologyTest, PreFramingToroidalPeerCannotStartToroidalGame)
{
	const auto old = ESerializationVersion::TOROIDAL_TRANSIENT_RETURN;
	const auto greeting = LobbyTopologyCapability::negotiateClient(LobbyTopologyCapability::encode(old));
	EXPECT_TRUE(greeting.toroidal);
	EXPECT_FALSE(LobbyTopologyCapability::isSafeServerGreeting(greeting));
	auto transport = std::make_shared<PacketCapture>();
	auto connection = std::make_shared<GameConnection>(transport);
	connection->setSerializationVersion(old);
	connection->setToroidalTopologySupported(true);
	CVCMIServer server(0, true);
	server.activeConnections.push_back(connection);
	CMapHeader map;
	map.toroidal = true;
	EXPECT_FALSE(server.supportsMapTopology(map));
	map.toroidal = false;
	EXPECT_TRUE(server.supportsMapTopology(map));
}

TEST(GameConnectionTopologyTest, MovementLayerIsVersionedInPackets)
{
	for(const auto version : {ESerializationVersion::TOROIDAL_MOVEMENT_SOURCE_LAYER, ESerializationVersion::CURRENT})
	{
		auto transport = std::make_shared<PacketCapture>();
		GameConnection sender(transport), receiver(transport);
		sender.setSerializationVersion(version);
		receiver.setSerializationVersion(version);
		TryMoveHero outgoing;
		outgoing.movementLayer = EPathfindingLayer::AIR;
		sender.sendPack(outgoing);
		auto packet = receiver.retrievePack(transport->bytes);
		const auto * movement = dynamic_cast<const TryMoveHero *>(packet.get());
		ASSERT_NE(movement, nullptr);
		EXPECT_EQ(movement->movementLayer, version == ESerializationVersion::CURRENT ? EPathfindingLayer::AIR : EPathfindingLayer::AUTO);
	}
}

TEST(GameConnectionTopologyTest, CapabilityGreetingSurvivesLobbyPacketSerialization)
{
	auto transport = std::make_shared<PacketCapture>();
	GameConnection sender(transport);
	GameConnection receiver(transport);
	LobbyClientConnected outgoing;
	outgoing.uuid = "test-client";
	outgoing.version = LobbyTopologyCapability::encode(ESerializationVersion::CURRENT);
	sender.sendPack(outgoing);
	auto packet = receiver.retrievePack(transport->bytes);
	const auto * greeting = dynamic_cast<const LobbyClientConnected *>(packet.get());
	ASSERT_NE(greeting, nullptr);
	const auto decoded = LobbyTopologyCapability::decode(greeting->version);
	EXPECT_TRUE(decoded.toroidal);
	EXPECT_EQ(decoded.version, ESerializationVersion::CURRENT);
}

TEST(GameConnectionTopologyTest, ServerNegotiatesCapabilityAndRejectsMixedPeersForToroidalMaps)
{
	CVCMIServer server(0, true);
	const auto connect = [&](ESerializationVersion wireVersion, const std::string & uuid)
	{
		auto transport = std::make_shared<PacketCapture>();
		auto connection = std::make_shared<GameConnection>(transport);
		server.activeConnections.push_back(connection);
		LobbyClientConnected packet;
		packet.uuid = uuid;
		packet.mode = EStartMode::NEW_GAME;
		packet.version = wireVersion;
		ApplyOnServerNetPackVisitor visitor(server, connection);
		visitor.visitLobbyClientConnected(packet);
		EXPECT_TRUE(visitor.getResult());
		return std::tuple{transport, connection, packet.version};
	};

	const auto [firstTransport, first, firstAck] = connect(LobbyTopologyCapability::encode(ESerializationVersion::CURRENT), "tagged");
	EXPECT_TRUE(first->supportsToroidalTopology());
	EXPECT_EQ(first->getSerializationVersion(), ESerializationVersion::CURRENT);
	const auto firstGreeting = LobbyTopologyCapability::decode(firstAck);
	EXPECT_TRUE(firstGreeting.toroidal);
	EXPECT_TRUE(LobbyTopologyCapability::isSafeServerGreeting(firstGreeting));
	CMapHeader map;
	map.toroidal = true;
	EXPECT_TRUE(server.supportsMapTopology(map));
	const auto [anotherTransport, another, anotherAck] = connect(LobbyTopologyCapability::encode(ESerializationVersion::CURRENT), "second-tagged");
	EXPECT_TRUE(another->supportsToroidalTopology());
	EXPECT_TRUE(LobbyTopologyCapability::decode(anotherAck).toroidal);
	EXPECT_TRUE(server.supportsMapTopology(map));

	// An unrelated client with the same ordinal but no tag must be downgraded
	// and cannot participate in a toroidal game.
	const auto [secondTransport, second, secondAck] = connect(ESerializationVersion::CURRENT, "untagged");
	EXPECT_FALSE(second->supportsToroidalTopology());
	EXPECT_EQ(second->getSerializationVersion(), ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER);
	EXPECT_EQ(secondAck, ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER);
	EXPECT_FALSE(server.supportsMapTopology(map));
	const auto [legacyTransport, legacy, legacyAck] = connect(ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER, "legacy");
	EXPECT_FALSE(legacy->supportsToroidalTopology());
	EXPECT_EQ(legacyAck, ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER);
	EXPECT_FALSE(server.supportsMapTopology(map));
	map.toroidal = false;
	EXPECT_TRUE(server.supportsMapTopology(map));
}

TEST(GameConnectionTopologyTest, LegacyGreetingIsAllowedButAmbiguousServerVersionIsRejected)
{
	const auto old = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	const auto wire = LobbyTopologyCapability::encode(ESerializationVersion::CURRENT);
	// The previous server responds with min(client version, server version).
	const auto legacyAck = std::min(wire, old);
	EXPECT_EQ(legacyAck, old);
	EXPECT_TRUE(LobbyTopologyCapability::isSafeServerGreeting(LobbyTopologyCapability::decode(legacyAck)));
	const auto taggedLegacy = LobbyTopologyCapability::negotiateClient(LobbyTopologyCapability::encode(old));
	EXPECT_EQ(taggedLegacy.version, old);
	EXPECT_FALSE(taggedLegacy.toroidal);
	EXPECT_FALSE(LobbyTopologyCapability::isSafeServerGreeting(LobbyTopologyCapability::decode(LobbyTopologyCapability::encode(old))));
	EXPECT_FALSE(LobbyTopologyCapability::isSafeServerGreeting(LobbyTopologyCapability::decode(ESerializationVersion::CURRENT)));
	const auto unknownFuture = static_cast<ESerializationVersion>(static_cast<int32_t>(ESerializationVersion::CURRENT) + 1);
	EXPECT_FALSE(LobbyTopologyCapability::isSafeServerGreeting(LobbyTopologyCapability::decode(unknownFuture)));
	const auto downgradedFuture = LobbyTopologyCapability::negotiateClient(unknownFuture);
	EXPECT_EQ(downgradedFuture.version, old);
	EXPECT_FALSE(downgradedFuture.toroidal);
}

TEST(GameConnectionTopologyTest, ServerRejectsClientBelowMinimumLobbyVersion)
{
	CVCMIServer server(0, true);
	auto transport = std::make_shared<PacketCapture>();
	auto connection = std::make_shared<GameConnection>(transport);
	server.activeConnections.push_back(connection);
	LobbyClientConnected packet;
	packet.version = ESerializationVersion::NONE;
	ApplyOnServerNetPackVisitor visitor(server, connection);
	visitor.visitLobbyClientConnected(packet);
	EXPECT_FALSE(visitor.getResult());
	EXPECT_TRUE(transport->closed);
	EXPECT_TRUE(server.activeConnections.empty());
}

TEST(GameConnectionTopologyTest, InternalConnectionsNegotiateBeforeToroidalStartGuard)
{
	CVCMIServer server(0, true);
	server.prepare(false, false);
	LobbyProbe tagged("tagged-internal", LobbyTopologyCapability::encode(ESerializationVersion::CURRENT));
	LobbyProbe untagged("untagged-internal", ESerializationVersion::CURRENT);
	auto clientNetwork = INetworkHandler::createHandler();
	const auto pump = [&]
	{
		clientNetwork->getContext().restart();
		clientNetwork->getContext().poll();
		server.getNetworkHandler().getContext().restart();
		server.getNetworkHandler().getContext().poll();
		clientNetwork->getContext().restart();
		clientNetwork->getContext().poll();
	};
	clientNetwork->createInternalConnection(tagged, server.getNetworkServer());
	pump();
	ASSERT_TRUE(tagged.failure.empty()) << tagged.failure;
	ASSERT_TRUE(tagged.ack.has_value());
	EXPECT_GT(tagged.packetsBeforeOwnAcknowledgement, 0);
	EXPECT_TRUE(tagged.ack->toroidal);
	EXPECT_EQ(tagged.ack->version, ESerializationVersion::CURRENT);
	CMapHeader map;
	map.toroidal = true;
	EXPECT_TRUE(server.supportsMapTopology(map));
	clientNetwork->createInternalConnection(untagged, server.getNetworkServer());
	pump();
	ASSERT_TRUE(untagged.failure.empty()) << untagged.failure;
	ASSERT_TRUE(untagged.ack.has_value());
	EXPECT_GT(untagged.packetsBeforeOwnAcknowledgement, 0);
	EXPECT_GT(tagged.otherAcknowledgements, 0);
	EXPECT_FALSE(untagged.ack->toroidal);
	EXPECT_EQ(untagged.ack->version, ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER);
	EXPECT_FALSE(server.supportsMapTopology(map));
	map.toroidal = false;
	EXPECT_TRUE(server.supportsMapTopology(map));
}

TEST(GameConnectionTopologyTest, TcpConnectionsNegotiateBeforeToroidalStartGuard)
{
	CVCMIServer server(0, true);
	const auto port = server.prepare(false, true);
	ASSERT_NE(port, 0);
	auto clientNetwork = INetworkHandler::createHandler();
	LobbyProbe tagged("tagged-tcp", LobbyTopologyCapability::encode(ESerializationVersion::CURRENT));
	LobbyProbe untagged("untagged-tcp", ESerializationVersion::CURRENT);
	const auto waitForGreeting = [&](const LobbyProbe & probe)
	{
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(!probe.ack && probe.failure.empty() && std::chrono::steady_clock::now() < deadline)
		{
			clientNetwork->getContext().restart();
			clientNetwork->getContext().poll();
			server.getNetworkHandler().getContext().restart();
			server.getNetworkHandler().getContext().poll();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	};
	clientNetwork->connectToRemote(tagged, "127.0.0.1", port);
	waitForGreeting(tagged);
	ASSERT_TRUE(tagged.failure.empty()) << tagged.failure;
	ASSERT_TRUE(tagged.ack.has_value());
	EXPECT_TRUE(tagged.ack->toroidal);
	EXPECT_EQ(tagged.ack->version, ESerializationVersion::CURRENT);
	CMapHeader map;
	map.toroidal = true;
	EXPECT_TRUE(server.supportsMapTopology(map));
	clientNetwork->connectToRemote(untagged, "127.0.0.1", port);
	waitForGreeting(untagged);
	ASSERT_TRUE(untagged.failure.empty()) << untagged.failure;
	ASSERT_TRUE(untagged.ack.has_value());
	EXPECT_FALSE(untagged.ack->toroidal);
	EXPECT_EQ(untagged.ack->version, ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER);
	EXPECT_FALSE(server.supportsMapTopology(map));
	map.toroidal = false;
	EXPECT_TRUE(server.supportsMapTopology(map));
}

TEST(GameConnectionTopologyTest, RejectsConflictingTopologyWireLayouts)
{
	const auto old = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	EXPECT_THROW(topologyAfterPacketRoundTrip(old, ESerializationVersion::CURRENT), std::runtime_error);
	EXPECT_THROW({
		const bool decoded = topologyAfterPacketRoundTrip(ESerializationVersion::CURRENT, old);
		ADD_FAILURE() << "Conflicting layout accepted with toroidal=" << decoded;
	}, std::runtime_error);
}

TEST(GameConnectionTopologyTest, RejectsDamagedOrWrongVersionFrame)
{
	auto transport = std::make_shared<PacketCapture>();
	GameConnection sender(transport);
	sender.setSerializationVersion(ESerializationVersion::CURRENT);
	sender.enableFramedPackets();
	LobbyUpdateMapInfo outgoing;
	outgoing.mapInfo = std::make_shared<CMapInfo>();
	outgoing.mapInfo->mapHeader = std::make_unique<CMapHeader>();
	outgoing.mapInfo->mapHeader->toroidal = true;
	sender.sendPack(outgoing);
	ASSERT_GT(transport->bytes.size(), 8u);
	const auto original = transport->bytes;
	for(int corruption = 0; corruption < 3; ++corruption)
	{
		auto bytes = original;
		if(corruption == 0)
			bytes.resize(7);
		else if(corruption == 1)
			bytes[1] ^= std::byte{1};
		else
			bytes[4] ^= std::byte{1};
		GameConnection receiver(transport);
		receiver.setSerializationVersion(ESerializationVersion::CURRENT);
		receiver.enableFramedPackets();
		EXPECT_THROW(receiver.retrievePack(bytes), std::runtime_error) << "corruption=" << corruption;
	}
}

TEST(GameConnectionTopologyTest, InvalidFramedPeerIsDisconnectedWithoutDroppingHost)
{
	CVCMIServer server(0, true);
	server.prepare(false, false);
	const auto connect = [&](const std::string & uuid)
	{
		auto transport = std::make_shared<PacketCapture>();
		server.onNewConnection(transport);
		auto connection = server.activeConnections.back();
		LobbyClientConnected greeting;
		greeting.uuid = uuid;
		greeting.mode = EStartMode::NEW_GAME;
		greeting.version = LobbyTopologyCapability::encode(ESerializationVersion::CURRENT);
		ApplyOnServerNetPackVisitor visitor(server, connection);
		visitor.visitLobbyClientConnected(greeting);
		EXPECT_TRUE(visitor.getResult());
		server.announcePack(greeting);
		return transport;
	};
	const auto host = connect("valid-host");
	const auto badPeer = connect("invalid-peer");
	ASSERT_EQ(server.activeConnections.size(), 2u);
	const std::vector<std::byte> wrongVersion = {
		std::byte{1}, std::byte{'T'}, std::byte{'R'}, std::byte{'F'},
		std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}
	};
	EXPECT_NO_THROW(server.onPacketReceived(badPeer, wrongVersion));
	EXPECT_TRUE(badPeer->closed);
	ASSERT_EQ(server.activeConnections.size(), 1u);
	EXPECT_FALSE(host->closed);
	GameConnection client(host);
	client.setSerializationVersion(ESerializationVersion::CURRENT);
	client.enableFramedPackets();
	LobbyChatMessage chat;
	chat.playerName = "valid-host";
	chat.message.appendRawString("still connected");
	client.sendPack(chat);
	const auto request = host->bytes;
	EXPECT_NO_THROW(server.onPacketReceived(host, request));
	EXPECT_FALSE(host->closed);
	EXPECT_EQ(server.activeConnections.size(), 1u);
}

TEST(GameConnectionTopologyTest, TcpInvalidFramedPeerDoesNotInterruptHost)
{
	CVCMIServer server(0, true);
	const auto port = server.prepare(false, true);
	ASSERT_NE(port, 0);
	auto network = INetworkHandler::createHandler();
	LobbyProbe host("tcp-host", LobbyTopologyCapability::encode(ESerializationVersion::CURRENT));
	LobbyProbe badPeer("tcp-bad", LobbyTopologyCapability::encode(ESerializationVersion::CURRENT));
	const auto pumpUntil = [&](const auto & predicate)
	{
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(!predicate() && std::chrono::steady_clock::now() < deadline)
		{
			network->getContext().restart();
			network->getContext().poll();
			server.getNetworkHandler().getContext().restart();
			server.getNetworkHandler().getContext().poll();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return predicate();
	};
	network->connectToRemote(host, "127.0.0.1", port);
	ASSERT_TRUE(pumpUntil([&] { return host.ack.has_value() || !host.failure.empty(); }));
	ASSERT_TRUE(host.failure.empty()) << host.failure;
	network->connectToRemote(badPeer, "127.0.0.1", port);
	ASSERT_TRUE(pumpUntil([&] { return badPeer.ack.has_value() || !badPeer.failure.empty(); }));
	ASSERT_TRUE(badPeer.failure.empty()) << badPeer.failure;
	ASSERT_EQ(server.activeConnections.size(), 2u);
	badPeer.transport->sendPacket({std::byte{1}, std::byte{'T'}, std::byte{'R'}, std::byte{'F'},
		std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}});
	ASSERT_TRUE(pumpUntil([&] { return server.activeConnections.size() == 1; }));
	ASSERT_TRUE(host.failure.empty()) << host.failure;
	LobbyChatMessage chat;
	chat.playerName = "tcp-host";
	chat.message.appendRawString("still connected");
	host.codec->sendPack(chat);
	EXPECT_TRUE(pumpUntil([&] { return host.receivedChats > 0 || !host.failure.empty(); }));
	EXPECT_TRUE(host.failure.empty()) << host.failure;
	EXPECT_GT(host.receivedChats, 0);
}
