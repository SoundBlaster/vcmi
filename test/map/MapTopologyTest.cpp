/*
 * MapTopologyTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/mapping/MapTopology.h"

TEST(MapTopology, WrapsEdgesAndCornersOnTheSameLevel)
{
	const MapTopology topology(100, 80, 2, true);
	EXPECT_EQ(topology.canonicalize(int3(-1, 45, 0)), int3(99, 45, 0));
	EXPECT_EQ(topology.canonicalize(int3(100, 45, 0)), int3(0, 45, 0));
	EXPECT_EQ(topology.canonicalize(int3(28, -1, 1)), int3(28, 79, 1));
	EXPECT_EQ(topology.canonicalize(int3(28, 80, 1)), int3(28, 0, 1));
	EXPECT_EQ(topology.canonicalize(int3(-1, -1, 0)), int3(99, 79, 0));
	EXPECT_EQ(topology.canonicalize(int3(100, -1, 0)), int3(0, 79, 0));
	EXPECT_EQ(topology.canonicalize(int3(-1, 80, 1)), int3(99, 0, 1));
	EXPECT_EQ(topology.canonicalize(int3(100, 80, 1)), int3(0, 0, 1));
	EXPECT_EQ(topology.canonicalize(int3(-201, 161, 0)), int3(99, 1, 0));
	EXPECT_FALSE(topology.canonicalize(int3(0, 0, -1)).has_value());
	EXPECT_FALSE(topology.canonicalize(int3(0, 0, 2)).has_value());
}

TEST(MapTopology, BoundedMapRejectsOutsideCoordinates)
{
	const MapTopology topology(100, 80, 2, false);
	EXPECT_EQ(topology.canonicalize(int3(0, 0, 0)), int3(0, 0, 0));
	EXPECT_EQ(topology.canonicalize(int3(99, 79, 1)), int3(99, 79, 1));
	EXPECT_FALSE(topology.canonicalize(int3(-1, 0, 0)).has_value());
	EXPECT_FALSE(topology.canonicalize(int3(100, 0, 0)).has_value());
	EXPECT_FALSE(topology.canonicalize(int3(0, -1, 0)).has_value());
	EXPECT_FALSE(topology.canonicalize(int3(0, 80, 0)).has_value());
}

TEST(MapTopology, UsesShortDisplacementAcrossSeams)
{
	const MapTopology wrapped(100, 80, 2, true);
	EXPECT_EQ(wrapped.displacement(int3(0, 0, 0), int3(99, 79, 0)), int3(-1, -1, 0));
	EXPECT_EQ(wrapped.displacement(int3(99, 79, 0), int3(0, 0, 0)), int3(1, 1, 0));
	EXPECT_EQ(wrapped.displacement(int3(99, 4, 1), int3(0, 5, 1)), int3(1, 1, 0));
	const MapTopology bounded(100, 80, 2, false);
	EXPECT_EQ(bounded.displacement(int3(0, 0, 0), int3(99, 79, 0)), int3(99, 79, 0));
}

TEST(MapTopology, HorizontalDistanceCanComparePositionsAcrossLevels)
{
	const MapTopology wrapped(36, 36, 2, true);
	const MapTopology bounded(36, 36, 2, false);
	EXPECT_EQ(wrapped.horizontalDistanceSquaredIgnoringLevel({0, 0, 0}, {35, 35, 1}), 2);
	EXPECT_EQ(bounded.horizontalDistanceSquaredIgnoringLevel({0, 0, 0}, {35, 35, 1}), 2450);
	EXPECT_FALSE(bounded.horizontalDistanceSquaredIgnoringLevel({0, 0, 0}, {36, 0, 1}));
	EXPECT_FALSE(wrapped.horizontalDistanceSquaredIgnoringLevel({0, 0, -1}, {35, 0, 0}));
}

TEST(MapTopology, WrapsPixelCoordinatesAcrossMultiplePeriods)
{
	constexpr int extent = 100 * 32;
	EXPECT_EQ(MapTopology::wrapCoordinate(-1, extent), extent - 1);
	EXPECT_EQ(MapTopology::wrapCoordinate(extent, extent), 0);
	EXPECT_EQ(MapTopology::wrapCoordinate(-2 * extent - 17, extent), extent - 17);
	EXPECT_EQ(MapTopology::wrapCoordinate(3 * extent + 17, extent), 17);
}

TEST(MapTopology, WrappedRangesMatchExhaustiveMapAndDoNotRepeatTiles)
{
	for(int width = 3; width <= 8; ++width)
		for(int height = 3; height <= 8; ++height)
			for(int radius = 0; radius <= 10; ++radius)
				for(const auto formula : {int3::DIST_2D, int3::DIST_MANHATTAN, int3::DIST_CHEBYSHEV, int3::DIST_2DSQ})
				{
					const MapTopology topology(width, height, 2, true);
					const int3 position(-width - 1, 2 * height + 1, 1);
					const int centerX = width - 1;
					const int centerY = 1;
					std::set<int3> expected;
					for(int x = 0; x < width; ++x)
						for(int y = 0; y < height; ++y)
						{
							const int rawX = std::abs(x - centerX);
							const int rawY = std::abs(y - centerY);
							const int3 distance(std::min(rawX, width - rawX), std::min(rawY, height - rawY), 0);
							if(int3(0, 0, 0).dist(distance, formula) <= radius)
								expected.insert(int3(x, y, 1));
						}

					std::set<int3> actual;
					int visits = 0;
					topology.forEachWrappedTileInRange(position, radius, formula, [&](const int3 & tile)
					{
						++visits;
						actual.insert(tile);
					});
					EXPECT_EQ(actual, expected) << width << "x" << height << " radius=" << radius << " formula=" << formula;
					EXPECT_EQ(visits, actual.size());
				}
}

TEST(MapTopology, WrappedRangeRejectsInvalidLevelsAndNegativeRadius)
{
	const MapTopology topology(5, 5, 2, true);
	int visits = 0;
	const auto visitor = [&](const int3 &) { ++visits; };
	topology.forEachWrappedTileInRange(int3(0, 0, -1), 2, int3::DIST_2D, visitor);
	topology.forEachWrappedTileInRange(int3(0, 0, 2), 2, int3::DIST_2D, visitor);
	topology.forEachWrappedTileInRange(int3(0, 0, 0), -1, int3::DIST_2D, visitor);
	EXPECT_EQ(visits, 0);
}

TEST(MapTopology, WrappedIntervalsCoverExactlyTheVisibleAxis)
{
	for(int extent = 3; extent <= 12; ++extent)
		for(int start = -2 * extent; start <= 2 * extent; ++start)
			for(int length = 0; length <= 2 * extent; ++length)
			{
				std::set<int> expected;
				for(int offset = 0; offset < length; ++offset)
					expected.insert(((start + offset) % extent + extent) % extent);

				std::set<int> actual;
				int visits = 0;
				const auto parts = MapTopology::splitWrappedInterval(start, length, extent);
				ASSERT_LE(parts.size(), 2);
				for(const auto & [partStart, partLength] : parts)
				{
					ASSERT_GE(partStart, 0);
					ASSERT_GT(partLength, 0);
					ASSERT_LE(partStart + partLength, extent);
					for(int tile = partStart; tile < partStart + partLength; ++tile)
					{
						actual.insert(tile);
						++visits;
					}
				}
				EXPECT_EQ(actual, expected) << "extent=" << extent << " start=" << start << " length=" << length;
				EXPECT_EQ(visits, actual.size());
			}
	EXPECT_TRUE(MapTopology::splitWrappedInterval(0, -1, 10).empty());
}
