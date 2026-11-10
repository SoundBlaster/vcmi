/*
 * MapScrollAccumulator.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include <cstdint>

/// Carries fractional pixels between updates while scrolling in one direction.
class MapScrollAccumulator
{
public:
	int advance(int x, int y, double pixelsPerSecond, uint32_t elapsedMs)
	{
		if (x != directionX || y != directionY)
		{
			remaining = 0;
			directionX = x;
			directionY = y;
		}
		if (x == 0 && y == 0)
			return 0;

		remaining += pixelsPerSecond * elapsedMs / 1000.0;
		const int pixels = static_cast<int>(remaining);
		remaining -= pixels;
		return pixels;
	}

	void reset()
	{
		directionX = directionY = 0;
		remaining = 0;
	}

private:
	int directionX = 0;
	int directionY = 0;
	double remaining = 0;
};
