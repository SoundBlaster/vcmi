/*
 * MapViewObservation.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include "MapViewModel.h"
#include "../../lib/mapping/MapTopology.h"

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
