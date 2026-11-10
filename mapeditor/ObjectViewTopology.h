/*
 * ObjectViewTopology.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../lib/mapping/MapTopology.h"

namespace ObjectViewTopology
{
/// Starts of periodic copies that intersect the canonical half-open axis.
inline std::vector<int> visibleStarts(int start, int length, int extent)
{
	assert(extent > 0);
	std::vector<int> result;
	if(length <= 0)
		return result;
	for(int position = MapTopology::wrapCoordinate(start, extent); position > -length; position -= extent)
		result.push_back(position);
	return result;
}
}
