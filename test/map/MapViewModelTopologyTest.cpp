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

TEST(MapViewModelTopologyTest, CachedBandsPlaceEveryVisibleTileExactlyOnce)
{
	constexpr int extent = 36;
	for(const Point dimensions : {Point(384, 256), Point(1600, 1200)})
	for(const int tileSize : {16, 32})
	for(const bool nativeCache : {false, true})
	for(const Point center : {Point(0, 0), Point(extent * tileSize - 1, 0),
		Point(0, extent * tileSize - 1), Point(extent * tileSize - 1, extent * tileSize - 1)})
	{
		MapViewModel model;
		model.setTileSize(Point(tileSize, tileSize));
		model.setCacheAtNativeSize(nativeCache);
		model.setViewDimensions(dimensions);
		model.setViewCenter(center);
		model.setLevel(1);
		const Point cacheTileSize = model.getCacheTileSize();
		const Rect visible = model.getTilesTotalRect();
		std::set<std::pair<int, int>> visited;
		int bands = 0;
		model.forEachCachedBand([&](const Rect & cacheArea, const Point & target, const Point & size)
		{
			++bands;
			ASSERT_EQ(cacheArea.w % cacheTileSize.x, 0);
			ASSERT_EQ(cacheArea.h % cacheTileSize.y, 0);
			const int columns = cacheArea.w / cacheTileSize.x;
			const int rows = cacheArea.h / cacheTileSize.y;
			EXPECT_EQ(size, Point(columns * tileSize, rows * tileSize));
			for(int y = 0; y < rows; ++y)
				for(int x = 0; x < columns; ++x)
				{
					const Point screen = target + Point(x * tileSize, y * tileSize);
					const int3 tile = model.getTileAtPoint(screen + Point(tileSize / 2, tileSize / 2));
					EXPECT_EQ(model.getTargetTileArea(tile).topLeft(), screen);
					EXPECT_EQ(model.getCacheTileArea(tile).topLeft(), cacheArea.topLeft() + Point(x * cacheTileSize.x, y * cacheTileSize.y));
					EXPECT_TRUE(visited.emplace(tile.x, tile.y).second);
				}
		});
		EXPECT_GE(bands, 1);
		EXPECT_LE(bands, 4);
		EXPECT_EQ(visited.size(), static_cast<size_t>(visible.w * visible.h));
	}
}
