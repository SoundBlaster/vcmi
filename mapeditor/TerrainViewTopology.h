/*
 * TerrainViewTopology.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../lib/mapping/MapTopology.h"

#include <array>
#include <set>

namespace TerrainViewTopology
{
struct TileRectangle
{
	int x;
	int y;
	int width;
	int height;
};

/// Visible portions of a viewport, in the canonical minimap rectangle.
inline std::vector<TileRectangle> wrappedViewportAreas(int x, int y, int viewportWidth, int viewportHeight, int width, int height)
{
	if(viewportWidth <= 0 || viewportHeight <= 0 || width <= 0 || height <= 0)
		return {};
	const auto xs = MapTopology::splitWrappedInterval(x, viewportWidth, width);
	const auto ys = MapTopology::splitWrappedInterval(y, viewportHeight, height);
	std::vector<TileRectangle> result;
	for(const auto & [left, w] : xs)
		for(const auto & [top, h] : ys)
			result.push_back({left, top, w, h});
	return result;
}

inline std::optional<int3> roadSourceAbove(const MapTopology & topology, const int3 & tile)
{
	return topology.canonicalize(tile + int3(0, -1, 0));
}

/// Map a square brush to physical tiles; toroidal callers reject an outside pointer.
inline std::vector<int3> brushSelection(const MapTopology & topology, const int3 & center, int firstOffset, int size, bool toroidal)
{
	const auto canonicalCenter = topology.canonicalize(center);
	if(size <= 0 || (toroidal && (!canonicalCenter || *canonicalCenter != center)))
		return {};
	std::set<int3> tiles;
	for(int dx = firstOffset; dx < firstOffset + size; ++dx)
		for(int dy = firstOffset; dy < firstOffset + size; ++dy)
			if(const auto tile = topology.canonicalize(center + int3(dx, dy, 0)))
				tiles.insert(*tile);
	return {tiles.begin(), tiles.end()};
}

/// Flood the four-connected selection using canonical map neighbors.
template<typename CanTraverse>
std::set<int3> floodSelection(const MapTopology & topology, const int3 & start, CanTraverse canTraverse)
{
	const auto first = topology.canonicalize(start);
	if(!first || *first != start)
		return {};

	const std::array<int3, 4> directions{int3(1, 0, 0), int3(-1, 0, 0), int3(0, 1, 0), int3(0, -1, 0)};
	std::set<int3> result{start};
	std::vector<int3> queue{start};
	while(!queue.empty())
	{
		const auto current = queue.back();
		queue.pop_back();
		for(const auto & direction : directions)
		{
			const auto neighbor = topology.canonicalize(current + direction);
			if(!neighbor || result.count(*neighbor) || !canTraverse(current, *neighbor))
				continue;
			result.insert(*neighbor);
			queue.push_back(*neighbor);
		}
	}
	return result;
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
