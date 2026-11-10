/*
 * MapAudioSources.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapping/MapTopology.h"

#include <set>

/// Canonical map tiles that act as ambient sound sources for one object.
inline std::set<int3> mapAmbientSourceTiles(const CGObjectInstance & object, const MapTopology & topology)
{
	std::set<int3> result;
	const auto add = [&](const int3 & position)
	{
		if(const auto tile = topology.canonicalize(position))
			result.insert(*tile);
	};
	if(object.isTile2Terrain())
	{
		for(int fx = 0; fx < object.getWidth(); ++fx)
			for(int fy = 0; fy < object.getHeight(); ++fy)
			{
				const auto position = object.anchorPos() - int3(fx, fy, 0);
				if(object.coveringAt(position))
					add(position);
			}
	}
	else if(object.isVisitable())
		add(object.visitablePos());
	else
		for(const auto & offset : object.getBlockedOffsets())
			add(object.anchorPos() + offset);
	return result;
}
