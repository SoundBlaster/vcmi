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
#include "../../lib/Rect.h"
#include "../../lib/int3.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

class MapTopology;

Point movingObjectImageOffset(const MapTopology & topology, bool toroidal,
	const int3 & tileFrom, const int3 & tileDest, const int3 & tile, double progress);

/// Visits every clipped image copy that intersects a tile on a repeating map.
template<typename Visitor>
void forEachWrappedImageFragment(const Point & offset, const Point & imageSize, const Point & mapPeriod, Visitor visit)
{
	assert(mapPeriod.x > 0 && mapPeriod.y > 0);
	constexpr int tileSize = 32;
	const auto firstOffset = [](int value, int period) -> int64_t
	{
		const int64_t remainder = (static_cast<int64_t>(value) + tileSize - 1) % period;
		return (remainder < 0 ? remainder + period : remainder) - tileSize + 1;
	};
	for(int64_t x = firstOffset(offset.x, mapPeriod.x); x < imageSize.x; x += mapPeriod.x)
		for(int64_t y = firstOffset(offset.y, mapPeriod.y); y < imageSize.y; y += mapPeriod.y)
		{
			const int sourceX = imageSize.x - static_cast<int>(x) - tileSize;
			const int sourceY = imageSize.y - static_cast<int>(y) - tileSize;
			const int left = std::max(0, sourceX);
			const int top = std::max(0, sourceY);
			const int right = std::min(imageSize.x, sourceX + tileSize);
			const int bottom = std::min(imageSize.y, sourceY + tileSize);
			visit(Point(left - sourceX, top - sourceY), Rect(left, top, right - left, bottom - top));
		}
}
