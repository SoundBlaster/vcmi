/*
 * TransientLanding.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "PathfinderOptions.h"

class CGameState;
class CGHeroInstance;

enum class KnownLanding { FOUND, NONE, UNKNOWN };
enum class LandingKnowledge { PLAYER_ROUTE, SAFE_RECOVERY };
using LandingStep = std::pair<int3, EPathfindingLayer>;

DLL_LINKAGE bool canLandTransientHeroAt(const CGameState & gameState, const CGHeroInstance * hero, const int3 & position, EPathfindingLayer layer);
DLL_LINKAGE bool canLandTransientHeroInPlace(const CGameState & gameState, const CGHeroInstance * hero);
DLL_LINKAGE KnownLanding findKnownTransientLanding(const CGameState & gameState, const CGHeroInstance * hero, const std::optional<PathfinderStart> & start, std::vector<LandingStep> * plainRoute = nullptr, LandingKnowledge knowledge = LandingKnowledge::SAFE_RECOVERY);
