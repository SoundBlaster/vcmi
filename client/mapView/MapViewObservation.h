/*
 * MapViewObservation.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include "MapViewModel.h"
#include "../../lib/mapping/MapTopology.h"
#include <cstddef>
#include <chrono>
#include <sstream>
#include <string>
#include <vector>

/// Consecutive visible displayed tiles in one row, with an exclusive end.
struct MapObservationVisibleRun
{
	int y;
	int beginX;
	int endX;
};

/// forEachObservedMapTile visits tiles in row-major order.
inline void appendObservedVisibleTile(std::vector<MapObservationVisibleRun> & runs, const int3 & displayed)
{
	if(!runs.empty() && runs.back().y == displayed.y && runs.back().endX == displayed.x)
		++runs.back().endX;
	else
		runs.push_back({displayed.y, displayed.x, displayed.x + 1});
}

inline std::string formatObservedVisibleRuns(const std::vector<MapObservationVisibleRun> & runs)
{
	std::ostringstream out;
	out << '[';
	bool first = true;
	for(const auto & run : runs)
	{
		if(!first)
			out << ',';
		first = false;
		out << '[' << run.y << ',' << run.beginX << ',' << run.endX << ']';
	}
	out << ']';
	return out.str();
}

class MapObservationSampler
{
	std::chrono::steady_clock::time_point last{};
	bool initialized = false;
public:
	bool shouldInspect(std::chrono::steady_clock::time_point now)
	{
		if(initialized && now - last < std::chrono::milliseconds(100))
			return false;
		initialized = true;
		last = now;
		return true;
	}
};

/// Emit a snapshot for a changed observable state or when a moving camera
/// stops, so its exact final pixel position is not left stale.
class MapObservationGate
{
	bool initialized = false;
	bool wasMoving = false;
	Point lastCenter;
	std::string lastKey;
public:
	bool shouldRecord(const std::string & key, const Point & center)
	{
		const bool moving = initialized && center != lastCenter;
		const bool record = !initialized || key != lastKey || (wasMoving && !moving);
		initialized = true;
		wasMoving = moving;
		lastCenter = center;
		lastKey = key;
		return record;
	}
};

/// Visits visible displayed tile copies within a fixed work budget. The
/// caller provides visibility from the active renderer context, not the map.
template<typename Visible, typename Visit>
bool forEachObservedMapTile(const MapViewModel & model, const MapTopology & topology,
	std::size_t maxTiles, Visible visible, Visit visit)
{
	const Rect tiles = model.getTilesTotalRect();
	const Point dimensions = model.getPixelsVisibleDimensions();
	std::size_t count = 0;
	for(int y = tiles.y; y < tiles.y + tiles.h; ++y)
		for(int x = tiles.x; x < tiles.x + tiles.w; ++x)
		{
			const int3 displayed(x, y, model.getLevel());
			const Rect area = model.getTargetTileArea(displayed);
			if(area.x >= dimensions.x || area.y >= dimensions.y || area.x + area.w <= 0 || area.y + area.h <= 0)
				continue;
			if(count == maxTiles)
				return false;
			++count;
			const auto canonical = topology.canonicalize(displayed);
			if(canonical && visible(displayed))
				visit(displayed, *canonical, area);
		}
	return true;
}

/// Walk a widget tree without exposing text or assuming that nodes are
/// rendered, unobscured or actionable. Parent indices belong to this record.
template<typename Node, typename Visit>
bool forEachObservedWidget(const Node * root, std::size_t maxNodes, std::size_t maxDepth, Visit visit)
{
	std::size_t count = 0;
	bool complete = true;
	const auto walk = [&](const auto & self, const Node * node, int parent, std::size_t depth) -> void
	{
		if(!node)
			return;
		if(count == maxNodes || depth > maxDepth)
		{
			complete = false;
			return;
		}
		const int index = static_cast<int>(count++);
		visit(*node, index, parent);
		for(const auto * child : node->children)
		{
			if(count == maxNodes && child)
			{
				complete = false;
				break;
			}
			self(self, child, index, depth + 1);
		}
	};
	walk(walk, root, -1, 0);
	return complete;
}
