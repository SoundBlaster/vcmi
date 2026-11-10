/*
 * MapViewMovement.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"
#include "MapViewMovement.h"

#include "../../lib/mapping/MapTopology.h"

Point movingObjectImageOffset(const MapTopology & topology, bool toroidal,
	const int3 & tileFrom, const int3 & tileDest, const int3 & tile, double progress)
{
	int3 offsetFrom = tileFrom - tile;
	int3 offsetDest = tileDest - tile;
	if(toroidal)
	{
		const auto from = topology.canonicalize(tileFrom);
		const auto dest = topology.canonicalize(tileDest);
		const auto canonicalTile = topology.canonicalize(tile);
		assert(from && dest && canonicalTile);
		offsetFrom = topology.displacement(*canonicalTile, *from);
		offsetDest = offsetFrom + topology.displacement(*from, *dest);
	}
	return vstd::lerp(Point(offsetFrom) * Point(32, 32), Point(offsetDest) * Point(32, 32), progress);
}
