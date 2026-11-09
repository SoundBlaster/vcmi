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

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

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

	/// Compares horizontal positions while deliberately ignoring their level difference.
	/// Callers must exclude sentinel coordinates before requesting toroidal wrapping.
	std::optional<ui32> horizontalDistanceSquaredIgnoringLevel(const int3 & from, int3 to) const
	{
		to.z = from.z;
		const auto canonicalFrom = canonicalize(from);
		const auto canonicalTo = canonicalize(to);
		if(!canonicalFrom || !canonicalTo)
			return std::nullopt;
		return int3(0, 0, 0).dist2dSQ(displacement(*canonicalFrom, *canonicalTo));
	}

	/// Visits each tile in a rectangular range once, preserving the center's level.
	/// Callers must exclude sentinel positions before requesting toroidal wrapping.
	template<typename Visitor>
	void forEachTileInRectangle(const int3 & position, int rangeX, int rangeY, Visitor visit) const
	{
		const auto center = canonicalize(position);
		if(!center || rangeX < 0 || rangeY < 0)
			return;

		const auto intervals = [this](int coordinate, int range, int extent)
		{
			if(wraps)
			{
				if(range >= extent / 2)
					return std::vector<std::pair<int, int>>{{0, extent}};
				return splitWrappedInterval(coordinate - range, 2 * range + 1, extent);
			}
			const int start = std::max(0, coordinate - std::min(range, extent));
			const int end = std::min(extent - 1, coordinate + std::min(range, extent));
			return std::vector<std::pair<int, int>>{{start, end - start + 1}};
		};
		const auto xs = intervals(center->x, rangeX, width);
		const auto ys = intervals(center->y, rangeY, height);
		for(const auto & [xStart, xLength] : xs)
			for(int x = xStart; x < xStart + xLength; ++x)
				for(const auto & [yStart, yLength] : ys)
					for(int y = yStart; y < yStart + yLength; ++y)
						visit(int3(x, y, center->z));
	}

	/// Visits each toroidal tile in range once, preserving the center's level.
	/// The center may itself be a repeated image outside the canonical rectangle.
	template<typename Visitor>
	void forEachWrappedTileInRange(const int3 & position, int radius, int3::EDistanceFormula formula, Visitor visit) const
	{
		assert(wraps);
		const auto center = canonicalize(position);
		if(!center || radius < 0)
			return;

		const int minX = -std::min(radius, width / 2);
		const int maxX = std::min(radius, (width - 1) / 2);
		const int minY = -std::min(radius, height / 2);
		const int maxY = std::min(radius, (height - 1) / 2);
		for(int dx = minX; dx <= maxX; ++dx)
			for(int dy = minY; dy <= maxY; ++dy)
			{
				const int3 offset(dx, dy, 0);
				if(int3(0, 0, 0).dist(offset, formula) <= radius)
					visit(*canonicalize(*center + offset));
			}
	}

	static int wrapCoordinate(int value, int extent)
	{
		assert(extent > 0);
		const int remainder = value % extent;
		return remainder < 0 ? remainder + extent : remainder;
	}

	/// Splits a wrapped interval into non-overlapping canonical portions.
	/// Intervals spanning a full period cover the whole axis only once.
	static std::vector<std::pair<int, int>> splitWrappedInterval(int start, int length, int extent)
	{
		assert(extent > 0);
		std::vector<std::pair<int, int>> parts;
		if(length <= 0)
			return parts;
		if(length >= extent)
			return {{0, extent}};
		start = wrapCoordinate(start, extent);
		const int firstLength = std::min(length, extent - start);
		parts.emplace_back(start, firstLength);
		if(firstLength < length)
			parts.emplace_back(0, length - firstLength);
		return parts;
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
