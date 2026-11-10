/*
 * MapViewBattle.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#pragma once

#include "../../lib/mapObjects/CGObjectInstance.h"

inline bool battleMatchesMonsterPosition(const CGObjectInstance & monster, const int3 & battlePosition, bool toroidal)
{
	return (toroidal ? monster.visitablePos() : monster.anchorPos()) == battlePosition;
}
