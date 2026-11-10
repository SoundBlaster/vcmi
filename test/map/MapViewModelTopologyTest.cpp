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
#include "../../client/mapView/MapViewOverlay.h"
#include "../../client/mapView/MapViewObservation.h"
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

TEST(MapViewModelTopologyTest, ObservationDistinguishesVisibleCopiesAndHonorsFogAndBudget)
{
	MapViewModel model;
	model.setTileSize(Point(10, 10));
	model.setViewDimensions(Point(40, 40));
	model.setViewCenter(Point(0, 0));
	model.setLevel(0);
	const MapTopology torus(3, 3, 1, true);
	std::vector<std::pair<int3, int3>> observations;
	const auto visible = [](const int3 & tile) { return tile != int3(-1, -1, 0); };
	const auto visit = [&](const int3 & displayed, const int3 & canonical, const Rect & area)
	{
		observations.emplace_back(displayed, canonical);
		EXPECT_EQ(area.topLeft(), model.getTargetTileArea(displayed).topLeft());
	};
	EXPECT_TRUE(forEachObservedMapTile(model, torus, 16, visible, visit));
	EXPECT_EQ(observations.size(), 15u);
	EXPECT_EQ(std::count(observations.begin(), observations.end(), std::make_pair(int3(-2, -2, 0), int3(1, 1, 0))), 1);
	EXPECT_EQ(std::count(observations.begin(), observations.end(), std::make_pair(int3(1, 1, 0), int3(1, 1, 0))), 1);
	EXPECT_EQ(std::count_if(observations.begin(), observations.end(), [](const auto & pair) { return pair.first == int3(-1, -1, 0); }), 0);
	observations.clear();
	EXPECT_FALSE(forEachObservedMapTile(model, torus, 2, visible, visit));
	EXPECT_EQ(observations.size(), 2u);
	observations.clear();
	EXPECT_TRUE(forEachObservedMapTile(model, MapTopology(3, 3, 1, false), 16, visible, visit));
	EXPECT_EQ(observations.size(), 4u);
}

TEST(MapViewModelTopologyTest, ObservationRecordsVisibleRunsAndFogChanges)
{
	MapViewModel model;
	model.setTileSize(Point(10, 10));
	model.setViewDimensions(Point(40, 40));
	model.setViewCenter(Point(0, 0));
	model.setLevel(0);
	const MapTopology torus(3, 3, 1, true);
	bool revealOne = false;
	const auto capture = [&]()
	{
		std::vector<MapObservationVisibleRun> runs;
		const bool complete = forEachObservedMapTile(model, torus, 16,
			[&](const int3 & displayed)
			{
				return displayed.y == -1 && (displayed.x == -2 || displayed.x == 0 || (revealOne && displayed.x == -1));
			},
			[&](const int3 & displayed, const int3 &, const Rect &)
			{
				appendObservedVisibleTile(runs, displayed);
			});
		EXPECT_TRUE(complete);
		return formatObservedVisibleRuns(runs);
	};
	MapObservationGate gate;
	const auto hidden = capture();
	EXPECT_EQ(hidden, "[[-1,-2,-1],[-1,0,1]]");
	EXPECT_TRUE(gate.shouldRecord(hidden, Point(0, 0)));
	EXPECT_FALSE(gate.shouldRecord(hidden, Point(0, 0)));
	revealOne = true;
	const auto revealed = capture();
	EXPECT_EQ(revealed, "[[-1,-2,1]]");
	EXPECT_TRUE(gate.shouldRecord(revealed, Point(0, 0)));
	EXPECT_FALSE(gate.shouldRecord(revealed, Point(0, 0)));
}

TEST(MapViewModelTopologyTest, ObservationWidgetTraversalKeepsParentsAndReportsLimits)
{
	struct Widget { std::vector<Widget *> children; };
	Widget leaf, sibling, child{{&leaf}}, root{{&child, &sibling}};
	std::vector<std::pair<int, int>> nodes;
	const auto visit = [&](const Widget &, int index, int parent) { nodes.emplace_back(index, parent); };
	EXPECT_TRUE(forEachObservedWidget(&root, 4, 2, visit));
	EXPECT_EQ(nodes, (std::vector<std::pair<int, int>>{{0, -1}, {1, 0}, {2, 1}, {3, 0}}));
	nodes.clear();
	EXPECT_FALSE(forEachObservedWidget(&root, 2, 2, visit));
	EXPECT_EQ(nodes.size(), 2u);
	nodes.clear();
	EXPECT_FALSE(forEachObservedWidget(&root, 4, 1, visit));
	EXPECT_EQ(nodes, (std::vector<std::pair<int, int>>{{0, -1}, {1, 0}, {2, 0}}));
}

TEST(MapViewModelTopologyTest, ObservationEmitsOnlyForChangesAndCameraSettling)
{
	MapObservationGate gate;
	EXPECT_TRUE(gate.shouldRecord("state-a", Point(0, 0)));
	EXPECT_FALSE(gate.shouldRecord("state-a", Point(0, 0)));
	EXPECT_FALSE(gate.shouldRecord("state-a", Point(1, 0)));
	EXPECT_FALSE(gate.shouldRecord("state-a", Point(2, 0)));
	EXPECT_TRUE(gate.shouldRecord("state-a", Point(2, 0)));
	EXPECT_FALSE(gate.shouldRecord("state-a", Point(2, 0)));
	EXPECT_TRUE(gate.shouldRecord("state-b", Point(2, 0)));
	EXPECT_FALSE(gate.shouldRecord("state-b", Point(2, 0)));
}

TEST(MapViewModelTopologyTest, ObservationLimitsInspectionFrequency)
{
	MapObservationSampler sampler;
	const auto start = std::chrono::steady_clock::time_point{};
	EXPECT_TRUE(sampler.shouldInspect(start));
	EXPECT_FALSE(sampler.shouldInspect(start + std::chrono::milliseconds(99)));
	EXPECT_TRUE(sampler.shouldInspect(start + std::chrono::milliseconds(100)));
	EXPECT_FALSE(sampler.shouldInspect(start + std::chrono::milliseconds(101)));
}

TEST(MapViewModelTopologyTest, RepeatedCopiesUseTheSameOverlayTileAndLabelParity)
{
	const MapTopology torus(5, 7, 2, true);
	const MapTopology bounded(5, 7, 2, false);
	const int3 canonical(4, 0, 1);
	const std::map<int3, int> spellIcons{{canonical, 42}};
	for(const int3 displayed : {canonical, int3(-1, 0, 1), int3(9, 0, 1),
		int3(-1, -7, 1), int3(9, 14, 1)})
	{
		const auto tile = mapOverlayTile(torus, displayed);
		ASSERT_EQ(tile, canonical);
		EXPECT_EQ(spellIcons.at(tile), 42);
		EXPECT_EQ(tile.x % 2, canonical.x % 2);
	}
	EXPECT_EQ(mapOverlayTile(torus, int3(-1, 0, 2)), int3(-1, 0, 2));
	EXPECT_EQ(mapOverlayTile(bounded, int3(-1, 0, 1)), int3(-1, 0, 1));
	EXPECT_EQ(mapOverlayTile(bounded, canonical), canonical);
}

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
