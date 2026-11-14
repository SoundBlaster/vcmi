/*
 * TransientLanding.cpp, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"
#include "TransientLanding.h"
#include "CPathfinder.h"
#include "CGPathNode.h"
#include "../CPlayerState.h"
#include "../TerrainHandler.h"
#include "../gameState/CGameState.h"
#include "../mapping/CMap.h"
#include "../mapping/MapTopology.h"
#include "../mapping/TerrainTile.h"
#include "../mapObjects/CGHeroInstance.h"

bool canLandTransientHeroAt(const CGameState & gameState, const CGHeroInstance * hero, const int3 & position, EPathfindingLayer layer)
{
	if(hero->inBoat() || layer != EPathfindingLayer::AIR)
		return false;
	const auto & tile = gameState.getMap().getTile(position);
	const auto * team = gameState.getPlayerTeam(hero->getOwner());
	if(!team)
		return false;
	const auto topology = gameState.getMap().getTopology();
	for(int dx = -1; dx <= 1; ++dx)
		for(int dy = -1; dy <= 1; ++dy)
		{
			const auto neighbor = topology.canonicalize(position + int3(dx, dy, 0));
			if(!neighbor || !team->fogOfWarMap[*neighbor])
				return false;
		}
	if(!tile.isLand() || !tile.getTerrain()->isPassable()
		|| tile.objectVisitedBy(hero->id).hasValue()
		|| gameState.guardingCreaturePosition(position).isValid())
		return false;
	if(!std::all_of(tile.blockingObjects.begin(), tile.blockingObjects.end(),
		[hero](const auto & id) { return id == hero->id; }))
		return false;
	return true;
}

bool canLandTransientHeroInPlace(const CGameState & gameState, const CGHeroInstance * hero)
{
	return canLandTransientHeroAt(gameState, hero, hero->visitablePos(), hero->getConfirmedMovementLayer());
}

// Movement validation may use a route over explored tiles with player-known
// guards. Automatic recovery additionally requires known guard neighborhoods.
KnownLanding findKnownTransientLanding(const CGameState & gameState, const CGHeroInstance * hero, const std::optional<PathfinderStart> & start, std::vector<LandingStep> * plainRoute, LandingKnowledge knowledge)
{
	const auto * team = gameState.getPlayerTeam(hero->getOwner());
	if(!team)
		return KnownLanding::UNKNOWN;
	const auto position = start ? start->position : hero->visitablePos();
	const auto layer = start ? start->layer : hero->getConfirmedMovementLayer();
	if(!plainRoute && canLandTransientHeroAt(gameState, hero, position, layer))
		return KnownLanding::FOUND;
	CPathsInfo paths(gameState.getMapSize(), hero);
	auto config = start
		? std::make_shared<SingleHeroPathfinderConfig>(paths, gameState, hero, *start)
		: std::make_shared<SingleHeroPathfinderConfig>(paths, gameState, hero);
	config->options.turnLimit = 0;
	config->options.ignoreGuards = false;
	config->options.useTeleportTwoWay = false;
	config->options.useTeleportOneWay = false;
	config->options.useTeleportOneWayRandom = false;
	config->options.useTeleportWhirlpool = false;
	CPathfinder pathfinder(gameState, config);
	pathfinder.calculatePaths();
	const auto topology = gameState.getMap().getTopology();
	const auto pathIsKnown = [&](const CGPathNode * node)
	{
		for(const auto * step = node; step; step = step->theNodeBefore)
		{
			if(!team->fogOfWarMap[step->coord])
				return false;
			if((step->layer == EPathfindingLayer::AIR || step->layer == EPathfindingLayer::WATER) && step->moveRemains <= 0)
				return false;
			if(step != node && step->theNodeBefore && step->action != EPathNodeAction::NORMAL)
				return false;
			if(knowledge == LandingKnowledge::PLAYER_ROUTE)
				continue;
			for(int dx = -1; dx <= 1; ++dx)
				for(int dy = -1; dy <= 1; ++dy)
				{
					const auto pos = topology.canonicalize(step->coord + int3(dx, dy, 0));
					if(!pos || !team->fogOfWarMap[*pos])
						return false;
				}
		}
		return true;
	};
	const int z = start ? start->position.z : hero->visitablePos().z;
	bool unknownFrontier = !team->fogOfWarMap[start ? start->position : hero->visitablePos()];
	const CGPathNode * bestPlainLanding = nullptr;
	for(int x = 0; x < paths.sizes.x; ++x)
		for(int y = 0; y < paths.sizes.y; ++y)
		{
			const int3 tile(x, y, z);
			const auto * node = paths.getNode(tile, EPathfindingLayer::LAND);
			if(node->turns == 0 && node->theNodeBefore
				&& (node->action == EPathNodeAction::NORMAL || node->action == EPathNodeAction::VISIT || node->action == EPathNodeAction::BATTLE)
				&& pathIsKnown(node))
			{
				if(!plainRoute)
					return KnownLanding::FOUND;
				bool plain = node->action == EPathNodeAction::NORMAL;
				for(const auto * step = node; plain && step; step = step->theNodeBefore)
				{
					const auto * stepTile = gameState.getTile(step->coord);
					if(stepTile->objectVisitedBy(hero->id).hasValue()
						|| gameState.guardingCreaturePosition(step->coord).isValid())
						plain = false;
				}
				if(plain && (!bestPlainLanding || node->getCost() < bestPlainLanding->getCost()))
					bestPlainLanding = node;
			}
			if(!unknownFrontier && team->fogOfWarMap[tile])
				for(const auto layer : {EPathfindingLayer::AIR, EPathfindingLayer::WATER})
				{
					const auto * transient = paths.getNode(tile, layer);
					if(transient->turns != 0 || transient->moveRemains <= 0)
						continue;
					for(int dx = -1; dx <= 1 && !unknownFrontier; ++dx)
						for(int dy = -1; dy <= 1; ++dy)
						{
							const auto pos = topology.canonicalize(tile + int3(dx, dy, 0));
							if(!pos || !team->fogOfWarMap[*pos])
							{
								unknownFrontier = true;
								break;
							}
						}
				}
		}
	if(bestPlainLanding)
	{
		plainRoute->clear();
		for(const auto * step = bestPlainLanding; step->theNodeBefore; step = step->theNodeBefore)
			plainRoute->emplace_back(step->coord, step->layer);
		std::reverse(plainRoute->begin(), plainRoute->end());
		return KnownLanding::FOUND;
	}
	return unknownFrontier ? KnownLanding::UNKNOWN : KnownLanding::NONE;
}
