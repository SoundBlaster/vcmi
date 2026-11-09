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
		return static_cast<int>(static_cast<std::int64_t>(inside) * mapExtent / displayExtent);
	}
	return static_cast<int>(std::round(static_cast<double>(mapExtent) * pixel / displayExtent));
}
