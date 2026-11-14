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
struct CPathsInfo;

enum class KnownLanding { FOUND, NONE, UNKNOWN };
enum class LandingKnowledge { PLAYER_ROUTE, SAFE_RECOVERY };
using LandingStep = std::pair<int3, EPathfindingLayer>;

DLL_LINKAGE bool canLandTransientHeroAt(const CGameState & gameState, const CGHeroInstance * hero, const int3 & position, EPathfindingLayer layer);
DLL_LINKAGE bool canLandTransientHeroInPlace(const CGameState & gameState, const CGHeroInstance * hero);
// An optional scratch graph is exclusively borrowed and reset for each search.
// It must match the map dimensions and must not be the graph being inspected.
DLL_LINKAGE KnownLanding findKnownTransientLanding(const CGameState & gameState, const CGHeroInstance * hero, const std::optional<PathfinderStart> & start, std::vector<LandingStep> * plainRoute = nullptr, LandingKnowledge knowledge = LandingKnowledge::SAFE_RECOVERY, CPathsInfo * scratch = nullptr);
