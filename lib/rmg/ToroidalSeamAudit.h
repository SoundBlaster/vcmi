/*
 * ToroidalSeamAudit.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include "../mapping/CMap.h"
#include "../mapping/MapTopology.h"
#include "../mapping/TerrainTile.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

namespace rmg
{
struct SeamContact
{
	int3 first;
	int3 second;
	int firstZone;
	int secondZone;
	bool firstToSecond;
	bool secondToFirst;
};

/// Identify possible unguarded ground contacts between distinct zones. When
/// seamOnly is false, include both internal and wrapped contacts.
/// This is a static audit, not a replacement for player pathfinding: it does
/// not model visits, fog, boats, spells or changes to guards. Nonblocking
/// visitable objects remain candidates rather than being silently excluded.
template<class ZoneAt>
std::vector<SeamContact> findUnguardedZoneContacts(const CMap & map, ZoneAt zoneAt, bool seamOnly)
{
	if(!map.toroidal)
		throw std::invalid_argument("seam audit requires toroidal topology");
	std::vector<SeamContact> result;
	std::set<std::pair<size_t, size_t>> seen;
	const auto index = [&](const int3 & p)
	{
		return (static_cast<size_t>(p.z) * map.height + p.y) * map.width + p.x;
	};
	const auto eligible = [&](const int3 & p)
	{
		const auto & tile = map.getTile(p);
		return tile.isLand() && tile.entrableTerrain() && !tile.blocked()
			&& !map.guardingCreaturePosition(p).isValid();
	};
	const auto topology = map.getTopology();
	for(int z = 0; z < map.levels(); ++z)
		for(int y = 0; y < map.height; ++y)
			for(int x = 0; x < map.width; ++x)
			{
				if(seamOnly && x != 0 && x != map.width - 1 && y != 0 && y != map.height - 1)
					continue;
				const int3 first(x, y, z);
				if(!eligible(first))
					continue;
				for(const auto & direction : int3::getDirs())
				{
					const int3 raw = first + direction;
					if(seamOnly && map.isInTheMap(raw))
						continue;
					const auto second = topology.canonicalize(raw);
					if(!second || index(first) >= index(*second) || zoneAt(first) == zoneAt(*second) || !eligible(*second))
						continue;
					const bool forward = map.canMoveBetween(first, *second);
					const bool backward = map.canMoveBetween(*second, first);
					if((forward || backward) && seen.emplace(index(first), index(*second)).second)
						result.push_back({first, *second, zoneAt(first), zoneAt(*second), forward, backward});
				}
			}
	return result;
}

template<class ZoneAt>
std::vector<SeamContact> findUnguardedSeamContacts(const CMap & map, ZoneAt zoneAt)
{
	return findUnguardedZoneContacts(map, zoneAt, true);
}

/// Plan a set of contact endpoints to block while preserving protected cells.
/// The result covers all supplied edges, but does not modify a map or prove
/// that the selected cells can be covered by obstacles without cutting routes.
inline std::optional<std::set<int3>> planSeamClosures(
	const std::vector<SeamContact> & contacts, const std::set<int3> & protectedCells)
{
	std::set<int3> blocked;
	for(const auto & contact : contacts)
	{
		const bool firstProtected = protectedCells.contains(contact.first);
		const bool secondProtected = protectedCells.contains(contact.second);
		if(firstProtected && secondProtected)
			return std::nullopt;
		if(firstProtected)
			blocked.insert(contact.second);
		if(secondProtected)
			blocked.insert(contact.first);
	}
	while(true)
	{
		std::map<int3, size_t> degree;
		for(const auto & contact : contacts)
		{
			if(blocked.contains(contact.first) || blocked.contains(contact.second))
				continue;
			++degree[contact.first];
			++degree[contact.second];
		}
		if(degree.empty())
			return blocked;
		const auto best = std::max_element(degree.begin(), degree.end(), [](const auto & a, const auto & b)
		{
			return a.second < b.second;
		});
		blocked.insert(best->first);
	}
}
}
