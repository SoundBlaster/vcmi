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
#include "../../client/adventureMap/MapScrollAccumulator.h"

TEST(MapScrollAccumulatorTest, SlowScrollingAccumulatesSubpixelUpdates)
{
	MapScrollAccumulator scroll;
	int moved = 0;
	for (int frame = 0; frame < 200; ++frame)
		moved += scroll.advance(-1, 0, 100.0, 5);
	EXPECT_EQ(moved, 100);
}

TEST(MapScrollAccumulatorTest, DirectionChangesAndResetDiscardOldRemainder)
{
	MapScrollAccumulator scroll;
	EXPECT_EQ(scroll.advance(-1, 0, 100.0, 5), 0);
	EXPECT_EQ(scroll.advance(0, -1, 100.0, 5), 0);
	EXPECT_EQ(scroll.advance(0, -1, 100.0, 5), 1);
	EXPECT_EQ(scroll.advance(0, -1, 100.0, 5), 0);
	scroll.reset();
	EXPECT_EQ(scroll.advance(0, -1, 100.0, 5), 0);
}
#include "../../client/mapView/MapViewMovement.h"
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

TEST(MapViewModelTopologyTest, MovingSpriteTakesOneStepAcrossEachSeam)
{
	constexpr int extent = 36;
	constexpr int tileSize = 32;
	const MapTopology wrapped(extent, extent, 2, true);
	for(int dx = -1; dx <= 1; ++dx)
	for(int dy = -1; dy <= 1; ++dy)
	{
		if(dx == 0 && dy == 0)
			continue;
		const int3 from(dx < 0 ? 0 : dx > 0 ? extent - 1 : 10,
			dy < 0 ? 0 : dy > 0 ? extent - 1 : 10, 1);
		const int3 dest = *wrapped.canonicalize(from + int3(dx, dy, 0));
		for(int pixels = 0; pixels <= tileSize; pixels += 8)
		{
			const double progress = static_cast<double>(pixels) / tileSize;
			const Point sourceOffset(dx * pixels, dy * pixels);
			const Point destinationOffset(-dx * (tileSize - pixels), -dy * (tileSize - pixels));
			EXPECT_EQ(movingObjectImageOffset(wrapped, true, from, dest, from, progress), sourceOffset);
			EXPECT_EQ(movingObjectImageOffset(wrapped, true, from, dest, dest, progress), destinationOffset);
			EXPECT_EQ(movingObjectImageOffset(wrapped, true, from, dest, from + int3(extent, -extent, 0), progress), sourceOffset);

			MapViewModel view;
			view.setTileSize(Point(tileSize, tileSize));
			view.setViewDimensions(Point(800, 600));
			view.setLevel(1);
			const int period = extent * tileSize;
			view.setViewCenter(Point(MapTopology::wrapCoordinate(from.x * tileSize + tileSize / 2 + dx * pixels, period),
				MapTopology::wrapCoordinate(from.y * tileSize + tileSize / 2 + dy * pixels, period)));
			const int3 centerTile = view.getTileAtPoint(Point(400, 300));
			const Point screenPosition = view.getTargetTileArea(centerTile).topLeft()
				+ movingObjectImageOffset(wrapped, true, from, dest, centerTile, progress);
			EXPECT_EQ(screenPosition, Point(384, 284));
		}
	}
	const MapTopology bounded(extent, extent, 2, false);
	EXPECT_EQ(movingObjectImageOffset(bounded, false, int3(0, 0, 1), int3(35, 35, 1), int3(0, 0, 1), 0.5), Point(560, 560));
}

TEST(MapViewModelTopologyTest, MovingSpriteDrawsBothCopiesInSmallMapTile)
{
	const MapTopology topology(3, 3, 1, true);
	for(const int dx : {-1, 1})
	for(const int dy : {-1, 1})
	{
		const int3 from(dx < 0 ? 0 : 2, dy < 0 ? 0 : 2, 0);
		const int3 destination = *topology.canonicalize(from + int3(dx, dy, 0));
		for(int progressPixels = 0; progressPixels <= 32; progressPixels += 8)
		{
			const auto offset = movingObjectImageOffset(topology, true, from, destination, destination, progressPixels / 32.0);
			int coverage[32][32]{};
			int fragments = 0;
			forEachWrappedImageFragment(offset, Point(96, 96), Point(96, 96), [&](const Point & target, const Rect & source)
			{
				++fragments;
				ASSERT_GT(source.w, 0);
				ASSERT_GT(source.h, 0);
				for(int y = 0; y < source.h; ++y)
					for(int x = 0; x < source.w; ++x)
					{
						const int screenX = target.x + x;
						const int screenY = target.y + y;
						ASSERT_GE(screenX, 0);
						ASSERT_LT(screenX, 32);
						ASSERT_GE(screenY, 0);
						ASSERT_LT(screenY, 32);
						++coverage[screenY][screenX];
						EXPECT_EQ(source.x + x, MapTopology::wrapCoordinate(96 - offset.x - 32 + screenX, 96));
						EXPECT_EQ(source.y + y, MapTopology::wrapCoordinate(96 - offset.y - 32 + screenY, 96));
					}
			});
			for(const auto & row : coverage)
				for(const int count : row)
					EXPECT_EQ(count, 1);
			if(progressPixels > 0 && progressPixels < 32)
				EXPECT_EQ(fragments, (offset.x < 0 ? 2 : 1) * (offset.y < 0 ? 2 : 1));
		}
	}
	int fragments = 0;
	forEachWrappedImageFragment(Point(-16, 0), Point(64, 32), Point(96, 96), [&](const Point & target, const Rect & source)
	{
		++fragments;
		EXPECT_EQ(target, Point(0, 0));
		EXPECT_EQ(source.topLeft(), Point(48, 0));
		EXPECT_EQ(source.dimensions(), Point(16, 32));
	});
	EXPECT_EQ(fragments, 1);
}
