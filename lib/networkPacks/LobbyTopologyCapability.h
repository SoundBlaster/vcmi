/*
 * LobbyTopologyCapability.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../serializer/ESerializationVersion.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace LobbyTopologyCapability
{
constexpr uint32_t TAG = 0x54520000u;
constexpr uint32_t VERSION_MASK = 0x0000ffffu;
static_assert(static_cast<uint32_t>(ESerializationVersion::CURRENT) <= VERSION_MASK);

struct Greeting
{
	ESerializationVersion version;
	bool toroidal;
};

inline ESerializationVersion encode(ESerializationVersion version)
{
	const auto raw = static_cast<int32_t>(version);
	assert(raw >= 0 && static_cast<uint32_t>(raw) <= VERSION_MASK);
	return static_cast<ESerializationVersion>(TAG | static_cast<uint32_t>(raw));
}

inline Greeting decode(ESerializationVersion wireVersion)
{
	const auto raw = static_cast<uint32_t>(static_cast<int32_t>(wireVersion));
	if((raw & ~VERSION_MASK) == TAG)
		return {static_cast<ESerializationVersion>(raw & VERSION_MASK), true};
	return {wireVersion, false};
}

inline Greeting negotiateClient(ESerializationVersion wireVersion)
{
	const auto offered = decode(wireVersion);
	auto version = std::min(offered.version, ESerializationVersion::CURRENT);
	if(!offered.toroidal && version >= ESerializationVersion::TOROIDAL_MAP)
		version = ESerializationVersion::CAMPAIGN_ENEMY_HERO_CARRIED_OVER;
	return {version, offered.toroidal && version >= ESerializationVersion::TOROIDAL_MAP};
}

inline bool isSafeServerGreeting(const Greeting & greeting)
{
	return greeting.version >= ESerializationVersion::MINIMAL_LOBBY
		&& greeting.version <= ESerializationVersion::CURRENT
		&& ((greeting.toroidal && greeting.version >= ESerializationVersion::TOROIDAL_MAP)
			|| (!greeting.toroidal && greeting.version < ESerializationVersion::TOROIDAL_MAP));
}
}
