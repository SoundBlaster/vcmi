/*
 * MapViewOverlay.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#pragma once

#include "../../lib/mapping/MapTopology.h"

/// Map copies share overlay content and label placement with their canonical tile.
/// Keep out-of-map coordinates unchanged on bounded maps.
inline int3 mapOverlayTile(const MapTopology & topology, const int3 & displayedTile)
{
	return topology.canonicalize(displayedTile).value_or(displayedTile);
}
