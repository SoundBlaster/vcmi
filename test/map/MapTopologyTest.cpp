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
#include "../../client/adventureMap/MinimapPosition.h"
#include "../../mapeditor/TerrainViewTopology.h"
#include "../../mapeditor/ObjectViewTopology.h"

TEST(MapTopology, EditorClipboardPlacementWrapsBothAxesAndPreservesBoundedFallback)
{
	const MapTopology torus(7, 5, 2, true), bounded(7, 5, 2, false);
	EXPECT_EQ(ObjectViewTopology::clipboardPosition(torus, {6, 4, 1}, {1, 1, 0}), int3(0, 0, 1));
	EXPECT_EQ(ObjectViewTopology::clipboardPosition(torus, {0, 0, 0}, {-1, -1, 0}), int3(6, 4, 0));
	EXPECT_EQ(ObjectViewTopology::clipboardPosition(bounded, {6, 4, 1}, {1, 1, 0}), int3(6, 4, 1));
	EXPECT_EQ(ObjectViewTopology::clipboardPosition(bounded, {2, 2, 0}, {1, -1, 0}), int3(3, 1, 0));
}

TEST(MapTopology, EditorObjectCopiesCoverTheCanonicalAxis)
{
	for(int extent = 3; extent <= 9; ++extent)
		for(int length = 1; length <= 2 * extent; ++length)
			for(int start = -2 * extent; start <= 2 * extent; ++start)
			{
				std::vector<int> actual(extent, 0), expected(extent, 0);
				for(int position = start; position < start + length; ++position)
					++expected[MapTopology::wrapCoordinate(position, extent)];
				for(int copy : ObjectViewTopology::visibleStarts(start, length, extent))
					for(int position = std::max(0, copy); position < std::min(extent, copy + length); ++position)
						++actual[position];
				EXPECT_EQ(actual, expected) << extent << ", " << start << ", " << length;
			}
	EXPECT_TRUE(ObjectViewTopology::visibleStarts(0, 0, 3).empty());
}

TEST(MapTopology, EditorRoadAndRedrawCrossToroidalSeams)
{
	const MapTopology wrapped(7, 5, 2, true);
	const MapTopology bounded(7, 5, 2, false);
	EXPECT_EQ(TerrainViewTopology::roadSourceAbove(wrapped, {3, 0, 1}), int3(3, 4, 1));
	EXPECT_FALSE(TerrainViewTopology::roadSourceAbove(bounded, {3, 0, 1}));
	EXPECT_EQ(TerrainViewTopology::roadSourceAbove(bounded, {3, 1, 1}), int3(3, 0, 1));
	EXPECT_FALSE(TerrainViewTopology::roadSourceAbove(wrapped, {3, 0, 2}));
	EXPECT_TRUE(TerrainViewTopology::wrappedRedrawAreas({}, 7, 5).empty());
	for(const auto & selection : std::vector<std::vector<int3>>{
		{{0, 0, 0}}, {{6, 4, 0}}, {{0, 2, 0}}, {{1, 1, 0}, {5, 3, 0}}})
	{
		int minX = selection.front().x, maxX = minX, minY = selection.front().y, maxY = minY;
		for(const auto & tile : selection)
		{
			minX = std::min(minX, tile.x);
			maxX = std::max(maxX, tile.x);
			minY = std::min(minY, tile.y);
			maxY = std::max(maxY, tile.y);
		}
		std::set<int3> expected;
		for(int x = minX - 2; x < maxX + 2; ++x)
			for(int y = minY - 2; y < maxY + 2; ++y)
				expected.insert(*wrapped.canonicalize({x, y, 0}));
		std::set<int3> actual;
		int count = 0;
		for(const auto & area : TerrainViewTopology::wrappedRedrawAreas(selection, 7, 5))
			for(int x = area.x; x < area.x + area.width; ++x)
				for(int y = area.y; y < area.y + area.height; ++y)
				{
					actual.insert({x, y, 0});
					++count;
				}
		EXPECT_EQ(actual, expected);
		EXPECT_EQ(count, actual.size());
	}
}

TEST(MapTopology, EditorCameraAndPointerCoordinatesWrapAtSubtileBoundaries)
{
	EXPECT_DOUBLE_EQ(TerrainViewTopology::wrapSceneCoordinate(-0.25, 224), 223.75);
	EXPECT_DOUBLE_EQ(TerrainViewTopology::wrapSceneCoordinate(224.25, 224), 0.25);
	EXPECT_DOUBLE_EQ(TerrainViewTopology::wrapSceneCoordinate(-448.5, 224), 223.5);
	EXPECT_EQ(TerrainViewTopology::sceneTileCoordinate(-0.25, true, 32), -1);
	EXPECT_EQ(TerrainViewTopology::sceneTileCoordinate(-32.25, true, 32), -2);
	EXPECT_EQ(TerrainViewTopology::sceneTileCoordinate(-0.25, false, 32), 0);
	EXPECT_EQ(TerrainViewTopology::sceneTileCoordinate(32.25, true, 32), 1);
}

TEST(MapTopology, EditorRepeatedSectionsCoverDisplayedAndCanonicalCoordinates)
{
	const MapTopology topology(7, 5, 1, true);
	for(const auto & [x, y, width, height] : std::vector<std::array<int, 4>>{
		{1, 1, 3, 2}, {-1, 1, 3, 2}, {6, 4, 3, 3}, {-9, -7, 16, 12}, {0, 0, 7, 5}})
	{
		std::map<int3, int3> actual;
		for(const auto & area : TerrainViewTopology::repeatedSections(x, y, width, height, 7, 5))
			for(int dx = 0; dx < area.width; ++dx)
				for(int dy = 0; dy < area.height; ++dy)
				{
					const int3 displayed(area.displayedX + dx, area.displayedY + dy, 0);
					const int3 canonical(area.canonicalX + dx, area.canonicalY + dy, 0);
					EXPECT_TRUE(actual.emplace(displayed, canonical).second);
				}
		ASSERT_EQ(actual.size(), static_cast<size_t>(width * height));
		for(int dx = 0; dx < width; ++dx)
			for(int dy = 0; dy < height; ++dy)
			{
				const int3 displayed(x + dx, y + dy, 0);
				ASSERT_TRUE(actual.contains(displayed));
				EXPECT_EQ(actual.at(displayed), *topology.canonicalize(displayed));
			}
	}
	EXPECT_TRUE(TerrainViewTopology::repeatedSections(0, 0, 0, 1, 7, 5).empty());
	EXPECT_TRUE(TerrainViewTopology::repeatedSections(0, 0, 1, 1, 7, 0).empty());
}

TEST(MapTopology, EditorMinimapViewportCoversWrappedTiles)
{
	const MapTopology topology(7, 5, 1, true);
	for(const auto & [x, y, width, height] : std::vector<std::array<int, 4>>{
		{1, 1, 3, 2}, {-1, 1, 3, 2}, {6, 4, 3, 3}, {-9, -7, 10, 8}, {0, 0, 7, 5}})
	{
		std::set<int3> expected, actual;
		for(int dx = 0; dx < width; ++dx)
			for(int dy = 0; dy < height; ++dy)
				expected.insert(*topology.canonicalize({x + dx, y + dy, 0}));
		for(const auto & area : TerrainViewTopology::wrappedViewportAreas(x, y, width, height, 7, 5))
			for(int dx = 0; dx < area.width; ++dx)
				for(int dy = 0; dy < area.height; ++dy)
					EXPECT_TRUE(actual.insert({area.x + dx, area.y + dy, 0}).second);
		EXPECT_EQ(actual, expected);
	}
	EXPECT_TRUE(TerrainViewTopology::wrappedViewportAreas(0, 0, 0, 1, 7, 5).empty());
	EXPECT_TRUE(TerrainViewTopology::wrappedViewportAreas(0, 0, 1, 1, 0, 5).empty());
}

TEST(MapTopology, EditorSquareBrushWrapsOnlyOnToroidalMaps)
{
	for(int level = 0; level < 2; ++level)
		for(const auto & center : std::vector<int3>{{0, 0, level}, {6, 0, level}, {0, 4, level}, {6, 4, level}, {3, 2, level}})
			for(const auto & [first, size] : {std::pair{0, 2}, std::pair{-1, 4}})
			{
				const MapTopology wrapped(7, 5, 2, true), bounded(7, 5, 2, false);
				std::set<int3> expectedWrapped, expectedBounded;
				for(int dx = first; dx < first + size; ++dx)
					for(int dy = first; dy < first + size; ++dy)
					{
						const int x = center.x + dx, y = center.y + dy;
						expectedWrapped.insert({(x + 7) % 7, (y + 5) % 5, level});
						if(x >= 0 && x < 7 && y >= 0 && y < 5)
							expectedBounded.insert({x, y, level});
					}
				const auto toroidalTiles = TerrainViewTopology::brushSelection(wrapped, center, first, size, true);
				const auto boundedTiles = TerrainViewTopology::brushSelection(bounded, center, first, size, false);
				EXPECT_EQ(std::set<int3>(toroidalTiles.begin(), toroidalTiles.end()), expectedWrapped);
				EXPECT_EQ(std::set<int3>(boundedTiles.begin(), boundedTiles.end()), expectedBounded);
				EXPECT_EQ(toroidalTiles.size(), expectedWrapped.size());
			}
	const MapTopology small(3, 3, 1, true), bounded(7, 5, 1, false);
	EXPECT_EQ(TerrainViewTopology::brushSelection(small, {0, 0, 0}, -1, 4, true).size(), 9u);
	EXPECT_EQ(TerrainViewTopology::brushSelection(small, {-1, 0, 0}, -1, 4, true).size(), 9u);
	EXPECT_TRUE(TerrainViewTopology::brushSelection(small, {0, 0, 1}, -1, 4, true).empty());
	// Bounded brushes retain their prior behavior when their pointer is just outside.
	const auto outside = TerrainViewTopology::brushSelection(bounded, {-1, 0, 0}, -1, 4, false);
	EXPECT_EQ(std::set<int3>(outside.begin(), outside.end()), (std::set<int3>{{0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {1, 0, 0}, {1, 1, 0}, {1, 2, 0}}));
}

TEST(MapTopology, EditorFloodSelectionRespectsTopologyAndBarriers)
{
	for(int level = 0; level < 2; ++level)
	{
		const MapTopology wrapped(7, 5, 2, true);
		const MapTopology bounded(7, 5, 2, false);
		const int3 start(0, 0, level);
		const std::set<int3> region{{0, 0, level}, {6, 0, level}, {0, 4, level}, {6, 4, level}};
		const auto sameRegion = [&](const int3 &, const int3 & neighbor) { return region.count(neighbor) != 0; };
		EXPECT_EQ(TerrainViewTopology::floodSelection(wrapped, start, sameRegion), region);
		EXPECT_EQ(TerrainViewTopology::floodSelection(bounded, start, sameRegion), (std::set<int3>{start}));

		// The selection predicate models the editor's add and erase barriers.
		const std::set<int3> selected{{6, 0, level}, {6, 4, level}};
		const auto add = [&](const int3 &, const int3 & neighbor)
		{
			return region.count(neighbor) && !selected.count(neighbor);
		};
		EXPECT_EQ(TerrainViewTopology::floodSelection(wrapped, start, add),
			(std::set<int3>{{0, 0, level}, {0, 4, level}}));
		const auto erase = [&](const int3 &, const int3 & neighbor) { return selected.count(neighbor) != 0; };
		EXPECT_EQ(TerrainViewTopology::floodSelection(wrapped, int3(6, 0, level), erase), selected);

		// A uniform region terminates and visits each physical tile once.
		const auto all = TerrainViewTopology::floodSelection(wrapped, start, [](const int3 &, const int3 &) { return true; });
		EXPECT_EQ(all.size(), 35u);
		for(const auto & tile : all)
			EXPECT_EQ(tile.z, level);
	}
	const MapTopology wrapped(7, 5, 2, true);
	EXPECT_TRUE(TerrainViewTopology::floodSelection(wrapped, {-1, 0, 0}, [](const auto &, const auto &) { return true; }).empty());
	EXPECT_TRUE(TerrainViewTopology::floodSelection(wrapped, {0, 0, 2}, [](const auto &, const auto &) { return true; }).empty());
}

TEST(MapTopology, ToroidalMinimapIconCopiesCoverEdges)
{
	EXPECT_EQ(minimapWrappedIconOrigins(-1, 7, 144), (std::vector<int>{143, -1}));
	EXPECT_EQ(minimapWrappedIconOrigins(-2, 8, 144), (std::vector<int>{142, -2}));
	EXPECT_TRUE(minimapWrappedIconOrigins(0, 0, 3).empty());
	for(int extent = 1; extent <= 20; ++extent)
		for(int length = 1; length <= 16; ++length)
			for(int origin = -2 * extent; origin <= 2 * extent; ++origin)
			{
				std::vector<int> actual(extent, 0), expected(extent, 0);
				for(int pixel = origin; pixel < origin + length; ++pixel)
					++expected[MapTopology::wrapCoordinate(pixel, extent)];
				for(int copy : minimapWrappedIconOrigins(origin, length, extent))
					for(int pixel = std::max(0, copy); pixel < std::min(extent, copy + length); ++pixel)
						++actual[pixel];
				EXPECT_EQ(actual, expected) << "extent=" << extent << " origin=" << origin << " length=" << length;
			}
}

TEST(MapTopology, ToroidalMinimapClicksAddressTheTileUnderThePixel)
{
	EXPECT_EQ(minimapPixelToTile(0, 150, 36, true), 0);
	EXPECT_EQ(minimapPixelToTile(75, 150, 36, true), 18);
	EXPECT_EQ(minimapPixelToTile(149, 150, 36, true), 35);
	EXPECT_EQ(minimapPixelToTile(4, 150, 36, true), 1);
	EXPECT_EQ(minimapPixelToTile(12, 150, 36, true), 2); // exact boundary selects the earlier tile
	EXPECT_EQ(minimapPixelToTile(143, 144, 252, true), 251);
	EXPECT_EQ(minimapPixelToTile(99, 100, 60, true), 59);
	EXPECT_EQ(minimapPixelToTile(-1, 150, 36, true), 0);
	EXPECT_EQ(minimapPixelToTile(150, 150, 36, true), 35);
	EXPECT_EQ(minimapPixelToTile(149, 150, 36, false), 36);
	for(const auto & [mapExtent, displayExtent] : {std::pair{36, 150}, std::pair{60, 100}, std::pair{72, 72}})
		for(int tile = 0; tile < mapExtent; ++tile)
		{
			const int pixelNearCenter = (2 * tile + 1) * displayExtent / (2 * mapExtent);
			EXPECT_EQ(minimapPixelToTile(pixelNearCenter, displayExtent, mapExtent, true), tile);
		}
	for(int mapExtent : {36, 72, 144})
		for(int displayExtent : {73, 100, 150})
			for(int pixel = 0; pixel < displayExtent; ++pixel)
			{
				const double oldRatio = static_cast<double>(pixel) / displayExtent;
				EXPECT_EQ(minimapPixelToTile(pixel, displayExtent, mapExtent, false), static_cast<int>(std::round(mapExtent * oldRatio)));
			}
}

TEST(MapTopology, ToroidalMinimapTileIntervalsPartitionDisplayPixels)
{
	for(int mapExtent = 3; mapExtent <= 252; ++mapExtent)
		for(int displayExtent = 1; displayExtent <= 150; ++displayExtent)
		{
			int previousEnd = 0;
			for(int tile = 0; tile < mapExtent; ++tile)
			{
				const auto [first, end] = minimapTilePixelInterval(tile, displayExtent, mapExtent);
				if(first != previousEnd || end < first || end > displayExtent)
					FAIL() << "invalid interval: map=" << mapExtent << " display=" << displayExtent << " tile=" << tile;
				for(int pixel = first; pixel < end; ++pixel)
					if(minimapPixelToTile(pixel, displayExtent, mapExtent, true) != tile)
						FAIL() << "wrong tile: map=" << mapExtent << " display=" << displayExtent << " pixel=" << pixel;
				previousEnd = end;
			}
			if(previousEnd != displayExtent)
				FAIL() << "uncovered pixels: map=" << mapExtent << " display=" << displayExtent;
		}
}

TEST(MapTopology, ToroidalMinimapRadarMatchesDisplayedTiles)
{
	// At this fractional scale pixel 12 belongs to tile 2, not tile 3.
	EXPECT_EQ(minimapTileRangePixelInterval(3, 4, 150, 36), (std::pair{13, 17}));
	for(const auto & [mapExtent, displayExtent] : {std::pair{36, 150}, std::pair{23, 144}, std::pair{252, 144}, std::pair{3, 1}})
		for(int start = 0; start < mapExtent; ++start)
			for(int end = start + 1; end <= mapExtent; ++end)
			{
				const auto [first, last] = minimapTileRangePixelInterval(start, end, displayExtent, mapExtent);
				for(int pixel = 0; pixel < displayExtent; ++pixel)
				{
					const int tile = minimapPixelToTile(pixel, displayExtent, mapExtent, true);
					if((first <= pixel && pixel < last) != (start <= tile && tile < end))
						FAIL() << "map=" << mapExtent << " display=" << displayExtent << " range=" << start << ":" << end << " pixel=" << pixel;
				}
			}
}

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

TEST(MapTopology, RectangleEnumerationMatchesBoundedAndWrappedDistancesWithoutDuplicates)
{
	for(int width = 3; width <= 7; ++width)
		for(int height = 3; height <= 7; ++height)
			for(int rangeX = 0; rangeX <= 8; ++rangeX)
				for(int rangeY = 0; rangeY <= 8; ++rangeY)
					for(bool wraps : {false, true})
					{
						const MapTopology topology(width, height, 2, wraps);
						const int3 center(0, height - 1, 1);
						std::set<int3> expected;
						for(int x = 0; x < width; ++x)
							for(int y = 0; y < height; ++y)
							{
								const int dx = wraps ? std::min(x, width - x) : x;
								const int rawY = height - 1 - y;
								const int dy = wraps ? std::min(rawY, height - rawY) : rawY;
								if(dx <= rangeX && dy <= rangeY)
									expected.insert(int3(x, y, 1));
							}
						std::set<int3> actual;
						int visits = 0;
						topology.forEachTileInRectangle(center, rangeX, rangeY, [&](const int3 & tile)
						{
							++visits;
							actual.insert(tile);
						});
						EXPECT_EQ(actual, expected) << width << "x" << height << " ranges=" << rangeX << "," << rangeY << " wraps=" << wraps;
						EXPECT_EQ(visits, actual.size());
					}
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
