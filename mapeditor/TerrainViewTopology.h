/*
 * TerrainViewTopology.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../lib/mapping/MapTopology.h"

namespace TerrainViewTopology
{
struct TileRectangle
{
	int x;
	int y;
	int width;
	int height;
};

inline std::optional<int3> roadSourceAbove(const MapTopology & topology, const int3 & tile)
{
	return topology.canonicalize(tile + int3(0, -1, 0));
}

/// Matches the editor's existing two-tile margin, splitting it at the seams.
inline std::vector<TileRectangle> wrappedRedrawAreas(const std::vector<int3> & tiles, int width, int height)
{
	if(tiles.empty())
		return {};
	int minX = tiles.front().x, maxX = minX;
	int minY = tiles.front().y, maxY = minY;
	for(const auto & tile : tiles)
	{
		minX = std::min(minX, tile.x);
		maxX = std::max(maxX, tile.x);
		minY = std::min(minY, tile.y);
		maxY = std::max(maxY, tile.y);
	}
	const auto xs = MapTopology::splitWrappedInterval(minX - 2, maxX - minX + 4, width);
	const auto ys = MapTopology::splitWrappedInterval(minY - 2, maxY - minY + 4, height);
	std::vector<TileRectangle> result;
	for(const auto & [x, w] : xs)
		for(const auto & [y, h] : ys)
			result.push_back({x, y, w, h});
	return result;
}
}
