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

inline int minimapPixelToTile(int pixel, int displayExtent, int mapExtent, bool toroidal)
{
	assert(displayExtent > 0 && mapExtent > 0);
	if(toroidal)
	{
		const auto inside = std::clamp(pixel, 0, displayExtent - 1);
		// SDL's nearest-neighbor scaling samples at the center of a display
		// pixel. Select the same source tile the minimap actually displays.
		// At an exact boundary SDL's software scaler chooses the earlier tile.
		return static_cast<int>(((2 * static_cast<std::int64_t>(inside) + 1) * mapExtent - 1) / (2 * static_cast<std::int64_t>(displayExtent)));
	}
	const double ratio = static_cast<double>(pixel) / displayExtent;
	return static_cast<int>(std::round(mapExtent * ratio));
}
