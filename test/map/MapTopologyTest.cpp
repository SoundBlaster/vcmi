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

TEST(MapTopology, WrapsPixelCoordinatesAcrossMultiplePeriods)
{
	constexpr int extent = 100 * 32;
	EXPECT_EQ(MapTopology::wrapCoordinate(-1, extent), extent - 1);
	EXPECT_EQ(MapTopology::wrapCoordinate(extent, extent), 0);
	EXPECT_EQ(MapTopology::wrapCoordinate(-2 * extent - 17, extent), extent - 17);
	EXPECT_EQ(MapTopology::wrapCoordinate(3 * extent + 17, extent), 17);
}
