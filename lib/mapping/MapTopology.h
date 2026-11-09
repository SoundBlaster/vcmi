/*
 * MapTopology.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../int3.h"

#include <optional>

/// Converts explicitly requested map positions without changing level identity.
class MapTopology
{
public:
	MapTopology(int width, int height, int levels, bool wraps)
		: width(width), height(height), levels(levels), wraps(wraps)
	{
		assert(width > 0 && height > 0 && levels > 0);
	}

	/// Returns no position for an invalid level or an outside position on a bounded map.
	std::optional<int3> canonicalize(const int3 & position) const
	{
		if(position.z < 0 || position.z >= levels)
			return std::nullopt;

		if(!wraps)
		{
			if(position.x < 0 || position.x >= width || position.y < 0 || position.y >= height)
				return std::nullopt;
			return position;
		}

		return int3(wrapCoordinate(position.x, width), wrapCoordinate(position.y, height), position.z);
	}

	/// Returns the shortest displacement between canonical positions on the same level.
	int3 displacement(const int3 & from, const int3 & to) const
	{
		assert(canonicalize(from) == from && canonicalize(to) == to && from.z == to.z);
		int3 result = to - from;
		if(wraps)
		{
			result.x = shortestOffset(result.x, width);
			result.y = shortestOffset(result.y, height);
		}
		return result;
	}

	static int wrapCoordinate(int value, int extent)
	{
		assert(extent > 0);
		const int remainder = value % extent;
		return remainder < 0 ? remainder + extent : remainder;
	}

private:
	int width;
	int height;
	int levels;
	bool wraps;

	static int shortestOffset(int offset, int extent)
	{
		if(offset > extent / 2)
			return offset - extent;
		if(offset < -(extent / 2))
			return offset + extent;
		return offset;
	}
};
