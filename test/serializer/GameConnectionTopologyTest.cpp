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
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/serializer/ESerializationVersion.h"
#include "../../lib/serializer/GameConnection.h"

namespace
{
class PacketCapture final : public INetworkConnection
{
public:
	void sendPacket(const std::vector<std::byte> & message) override { bytes = message; }
	void setAsyncWritesEnabled(bool) override {}
	void close() override {}
	std::vector<std::byte> bytes;
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

// A version collision can make different layouts appear negotiated; keep the
// desired fail-closed behavior visible until topology capability is negotiated.
TEST(GameConnectionTopologyTest, DISABLED_RejectsConflictingTopologyWireLayouts)
{
	const auto old = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	EXPECT_THROW(topologyAfterPacketRoundTrip(old, ESerializationVersion::CURRENT), std::runtime_error);
	EXPECT_THROW({
		const bool decoded = topologyAfterPacketRoundTrip(ESerializationVersion::CURRENT, old);
		ADD_FAILURE() << "Conflicting layout accepted with toroidal=" << decoded;
	}, std::runtime_error);
}
