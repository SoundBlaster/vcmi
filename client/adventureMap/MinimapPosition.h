/*
 * MinimapPosition.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>

inline int minimapPixelToTile(int pixel, int displayExtent, int mapExtent, bool toroidal)
{
	assert(displayExtent > 0 && mapExtent > 0);
	if(toroidal)
	{
		const auto inside = std::clamp(pixel, 0, displayExtent - 1);
		// Sample the center of the display pixel, resolving exact boundaries
		// toward the earlier tile. The minimap raster uses this same mapping.
		return static_cast<int>(((2 * static_cast<std::int64_t>(inside) + 1) * mapExtent - 1) / (2 * static_cast<std::int64_t>(displayExtent)));
	}
	const double ratio = static_cast<double>(pixel) / displayExtent;
	return static_cast<int>(std::round(mapExtent * ratio));
}

/// Half-open range of display pixels representing this tile; it may be empty.
inline std::pair<int, int> minimapTilePixelInterval(int tile, int displayExtent, int mapExtent)
{
	assert(tile >= 0 && tile < mapExtent && displayExtent > 0);
	const auto lowerBound = [=](int target)
	{
		int first = 0, last = displayExtent;
		while(first < last)
		{
			const int middle = first + (last - first) / 2;
			if(minimapPixelToTile(middle, displayExtent, mapExtent, true) < target)
				first = middle + 1;
			else
				last = middle;
		}
		return first;
	};
	return {lowerBound(tile), lowerBound(tile + 1)};
}

/// Display pixels whose sampled tiles lie in the half-open tile interval.
inline std::pair<int, int> minimapTileRangePixelInterval(int firstTile, int endTile, int displayExtent, int mapExtent)
{
	assert(firstTile >= 0 && firstTile < endTile && endTile <= mapExtent);
	return {minimapTilePixelInterval(firstTile, displayExtent, mapExtent).first,
		minimapTilePixelInterval(endTile - 1, displayExtent, mapExtent).second};
}
