/*
 * MapViewModel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapViewModel.h"

#include "../../lib/int3.h"

Point MapViewModel::getNativeTileSize()
{
	return Point(32, 32);
}

void MapViewModel::setTileSize(const Point & newValue)
{
	tileSize = newValue;
}

void MapViewModel::setCacheAtNativeSize(bool newValue)
{
	cacheAtNativeSize = newValue;
}

void MapViewModel::setViewCenter(const Point & newValue)
{
	viewCenter = newValue;
}

void MapViewModel::setViewDimensions(const Point & newValue)
{
	viewDimensions = newValue;
}

void MapViewModel::setLevel(int newLevel)
{
	mapLevel = newLevel;
}

Point MapViewModel::getSingleTileSizeUpperLimit() const
{
	// arbitrary-seleted upscaling limit
	return Point(256, 256);
}

Point MapViewModel::getSingleTileSizeLowerLimit() const
{
	// arbitrary-seleted downscaling limit
	return Point(4, 4);
}

Point MapViewModel::getSingleTileSize() const
{
	return tileSize;
}

/// Below this the unscaled cache would hold several times the pixels the window shows, since the
/// tiles keep their native size while their number grows - there it stores them ready-scaled
static constexpr int minimalNativeCacheTileSize = 16;

Point MapViewModel::getCacheTileSize() const
{
	if(!cacheAtNativeSize || tileSize.x < minimalNativeCacheTileSize || tileSize.y < minimalNativeCacheTileSize)
		return tileSize;

	return getNativeTileSize();
}

Point MapViewModel::getMapViewCenter() const
{
	return viewCenter;
}

Point MapViewModel::getPixelsVisibleDimensions() const
{
	return viewDimensions;
}

int MapViewModel::getLevel() const
{
	return mapLevel;
}

Point MapViewModel::getTilesVisibleDimensions() const
{
	// total number of potentially visible tiles is:
	// 1) number of completely visible tiles
	// 2) additional tile that might be partially visible from left/top size
	// 3) additional tile that might be partially visible from right/bottom size
	return {
		getPixelsVisibleDimensions().x / getSingleTileSize().x + 2,
		getPixelsVisibleDimensions().y / getSingleTileSize().y + 2,
	};
}

Rect MapViewModel::getTilesTotalRect() const
{
	return Rect(
		Point(getTileAtPoint(Point(0,0))),
		getTilesVisibleDimensions()
	);
}

int3 MapViewModel::getTileAtPoint(const Point & position) const
{
	Point topLeftOffset = getMapViewCenter() - getPixelsVisibleDimensions() / 2;

	Point absolutePosition = position + topLeftOffset;

	// NOTE: using division via double in order to use std::floor
	// which rounds to negative infinity and not towards zero (like integer division)
	return {
		static_cast<int>(std::floor(static_cast<double>(absolutePosition.x) / getSingleTileSize().x)),
		static_cast<int>(std::floor(static_cast<double>(absolutePosition.y) / getSingleTileSize().y)),
		getLevel()
	};
}

Point MapViewModel::getCacheDimensionsPixels() const
{
	// a cache at native size holds exactly the visible tiles, so it follows their number rather
	// than the window - zooming out asks for more tiles and thus for a larger canvas
	if(cacheAtNativeSize)
		return getTilesVisibleDimensions() * getCacheTileSize();

	return getPixelsVisibleDimensions() + getSingleTileSizeUpperLimit() * 2;
}

Rect MapViewModel::getCacheTileArea(const int3 & coordinates) const
{
	assert(mapLevel == coordinates.z);
	assert(getTilesVisibleDimensions().x + coordinates.x >= 0);
	assert(getTilesVisibleDimensions().y + coordinates.y >= 0);

	Point tileIndex{
		(getTilesVisibleDimensions().x + coordinates.x) % getTilesVisibleDimensions().x,
		(getTilesVisibleDimensions().y + coordinates.y) % getTilesVisibleDimensions().y
	};

	return Rect(tileIndex * getCacheTileSize(), getCacheTileSize());
}

void MapViewModel::forEachCachedBand(const std::function<void(const Rect &, const Point &, const Point &)> & visit) const
{
	const Rect tilesRect = getTilesTotalRect();
	const Point tileSize = getSingleTileSize();
	const Point cacheTileSize = getCacheTileSize();
	const int width = tilesRect.w;
	const int height = tilesRect.h;
	if(width <= 0 || height <= 0)
		return;

	const Point origin = getTargetTileArea(int3(tilesRect.x, tilesRect.y, getLevel())).topLeft();
	const int firstSlotX = ((tilesRect.x % width) + width) % width;
	const int firstSlotY = ((tilesRect.y % height) + height) % height;
	// A circular cache has at most two contiguous bands along either axis.
	const std::array<std::pair<int, int>, 2> columns = {{{firstSlotX, width - firstSlotX}, {0, firstSlotX}}};
	const std::array<std::pair<int, int>, 2> rows = {{{firstSlotY, height - firstSlotY}, {0, firstSlotY}}};

	int offsetX = 0;
	for(const auto & column : columns)
	{
		int offsetY = 0;
		for(const auto & row : rows)
		{
			if(column.second > 0 && row.second > 0)
			{
				const Rect cacheArea(column.first * cacheTileSize.x, row.first * cacheTileSize.y,
					column.second * cacheTileSize.x, row.second * cacheTileSize.y);
				const Point targetPosition = origin + Point(offsetX * tileSize.x, offsetY * tileSize.y);
				const Point targetSize(column.second * tileSize.x, row.second * tileSize.y);
				visit(cacheArea, targetPosition, targetSize);
			}
			offsetY += row.second;
		}
		offsetX += column.second;
	}
}

Rect MapViewModel::getTargetTileArea(const int3 & coordinates) const
{
	Point topLeftOffset = getMapViewCenter() - getPixelsVisibleDimensions() / 2;
	Point tilePosAbsolute = Point(coordinates) * getSingleTileSize();
	Point tilePosRelative = tilePosAbsolute - topLeftOffset;

	return Rect(tilePosRelative, getSingleTileSize());
}
