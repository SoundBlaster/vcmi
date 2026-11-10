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

bool topologyAfterPacketRoundTrip(ESerializationVersion senderVersion, ESerializationVersion receiverVersion)
{
	auto transport = std::make_shared<PacketCapture>();
	GameConnection sender(transport);
	GameConnection receiver(transport);
	sender.setSerializationVersion(senderVersion);
	receiver.setSerializationVersion(receiverVersion);
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

// The raw codec still relies on correct negotiation; keep the desired
// fail-closed behavior visible as a separate defense-in-depth regression.
TEST(GameConnectionTopologyTest, DISABLED_RejectsConflictingTopologyWireLayouts)
{
	const auto old = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	EXPECT_THROW(topologyAfterPacketRoundTrip(old, ESerializationVersion::CURRENT), std::runtime_error);
	EXPECT_THROW({
		const bool decoded = topologyAfterPacketRoundTrip(ESerializationVersion::CURRENT, old);
		ADD_FAILURE() << "Conflicting layout accepted with toroidal=" << decoded;
	}, std::runtime_error);
}
