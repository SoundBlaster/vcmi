/*
 * MapViewMovement.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../../lib/Point.h"
#include "../../lib/int3.h"

class MapTopology;

Point movingObjectImageOffset(const MapTopology & topology, bool toroidal,
	const int3 & tileFrom, const int3 & tileDest, const int3 & tile, double progress);
