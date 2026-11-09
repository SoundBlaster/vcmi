/*
 * ToroidalDemoMap.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "TinyH3MBuilder.h"

/// Sparse, deliberately adapted map for exercising seams, not a balanced scenario.
inline TinyH3M::TinyH3MBuilder makeToroidalDemoMap()
{
	const PlayerColor red(0), blue(1);
	return TinyH3M::TinyH3MBuilder(EMapFormat::SOD)
		.size(36, false)
		.name("Toroid Seam Laboratory")
		.description("Experimental open map for testing opposite edges and corners.")
		.playerActive(red).playerActive(blue)
		.playerMainTown(red, int3(8, 8, 0), false)
		.playerMainTown(blue, int3(27, 27, 0), false)
		.town(int3(8, 8, 0), FactionID(0), red)
		.town(int3(27, 27, 0), FactionID(0), blue)
		.hero(int3(1, 0, 0), HeroTypeID(0), red)
		.hero(int3(19, 18, 0), HeroTypeID(1), blue)
		.resource(int3(35, 35, 0), GameResID::GOLD, 10)
		.resource(int3(35, 1, 0), GameResID::WOOD, 5)
		.resource(int3(1, 35, 0), GameResID::ORE, 5);
}
