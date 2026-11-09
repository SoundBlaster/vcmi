/*
 * MapViewModelTopologyTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "../../client/mapView/MapViewModel.h"
#include "../../lib/mapping/MapTopology.h"

TEST(MapViewModelTopologyTest, WrappedCameraPreservesPixelGeometryAcrossSeams)
{
	constexpr int extent = 36;
	constexpr int tileSize = 32;
	constexpr int period = extent * tileSize;
	const MapTopology topology(extent, extent, 2, true);
	struct CameraMove
	{
		Point from;
		Point to;
		Point delta;
	};
	const CameraMove moves[] = {
		{Point(0, 0), Point(period - 1, 0), Point(-1, 0)},
		{Point(0, 0), Point(0, period - 1), Point(0, -1)},
		{Point(0, 0), Point(period - 1, period - 1), Point(-1, -1)},
		{Point(period - 1, period - 1), Point(0, 0), Point(1, 1)},
	};
	for(const Point dimensions : {Point(800, 600), Point(1600, 1200)})
	for(const auto & move : moves)
	{
		MapViewModel before;
		MapViewModel after;
		for(auto * view : {&before, &after})
		{
			view->setTileSize(Point(tileSize, tileSize));
			view->setViewDimensions(dimensions);
			view->setLevel(1);
		}
		before.setViewCenter(move.from);
		after.setViewCenter(move.to);
		for(int y = 0; y < dimensions.y; y += 17)
			for(int x = 0; x < dimensions.x; x += 19)
			{
				const Point pixel(x, y);
				const auto oldTile = before.getTileAtPoint(pixel + move.delta);
				const auto newTile = after.getTileAtPoint(pixel);
				ASSERT_EQ(topology.canonicalize(oldTile), topology.canonicalize(newTile));
				const auto oldArea = before.getTargetTileArea(oldTile);
				const auto newArea = after.getTargetTileArea(newTile);
				EXPECT_EQ(newArea.topLeft(), oldArea.topLeft() - move.delta);
				EXPECT_EQ(newArea.dimensions(), oldArea.dimensions());
			}
	}
}
