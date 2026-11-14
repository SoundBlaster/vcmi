/*
 * CGPathNode.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGPathNode.h"

#include "CPathfinder.h"
#include "PathfinderOptions.h"
#include "TransientLanding.h"

#include "../callback/IGameInfoCallback.h"
#include "../gameState/CGameState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapping/TerrainTile.h"

static bool canSeeObj(const CGObjectInstance * obj)
{
	/// Pathfinder should ignore placed events
	return obj != nullptr && obj->ID != Obj::EVENT;
}

const CGPathNode & CGPath::currNode() const
{
	assert(nodes.size() > 1);
	return nodes[nodes.size()-1];
}

const CGPathNode & CGPath::nextNode() const
{
	assert(nodes.size() > 1);
	return nodes[nodes.size()-2];
}

const CGPathNode & CGPath::lastNode() const
{
	assert(nodes.size() > 1);
	return nodes[0];
}

bool CGPath::hasNextNode() const
{
	return nodes.size() >= 2;
}

int3 CGPath::startPos() const
{
	return currNode().coord;
}

CGPath::ELayer CGPath::startLayer() const
{
	return nodes[nodes.size()-1].layer;
}

int3 CGPath::endPos() const
{
	return lastNode().coord;
}

CGPath::ELayer CGPath::endLayer() const
{
	return nodes[0].layer;
}

CPathsInfo::CPathsInfo(const int3 & Sizes, const CGHeroInstance * hero_)
	: sizes(Sizes), hero(hero_)
{
	nodes.resize(boost::extents[ELayer::NUM_LAYERS][sizes.z][sizes.x][sizes.y]);
	heroBonusTreeVersion = hero->getTreeVersion();
}

CPathsInfo::~CPathsInfo() = default;

void CPathsInfo::prepareForReuse(const CGHeroInstance * hero_)
{
	std::lock_guard lock(routeSafetyMutex);
	hero = hero_;
	heroBonusTreeVersion = hero->getTreeVersion();
	routeSafetyState = nullptr;
	routeSafetyCache.clear();
}

void CPathsInfo::configureRouteSafety(const CGameState * state)
{
	std::lock_guard lock(routeSafetyMutex);
	routeSafetyState = state;
	routeSafetyCache.clear();
}

void CPathsInfo::beginSearch()
{
	std::lock_guard lock(routeSafetyMutex);
	routeSafetyCache.clear();
	++currentGeneration;
	if(currentGeneration != 0)
		return;

	for(auto * node = nodes.data(); node != nodes.data() + nodes.num_elements(); ++node)
		node->generation = 0;
	currentGeneration = 1;
}

bool CPathsInfo::isKnownSafeRoute(const CGPathNode * destination) const
{
	if(!routeSafetyState || !destination->theNodeBefore || hero->inBoat())
		return true;
	std::lock_guard lock(routeSafetyMutex);
	const auto cached = routeSafetyCache.find(destination);
	if(cached != routeSafetyCache.end())
		return cached->second;
	std::vector<const CGPathNode *> steps;
	for(auto * node = destination; node; node = node->theNodeBefore)
		steps.push_back(node);
	std::reverse(steps.begin(), steps.end());
	// Search is deferred until a route or cursor is requested. The nested
	// landing search reads raw nodes and cannot recursively validate routes.
	const auto layer = hero->getConfirmedMovementLayer();
	bool hadKnownLanding = layer == ELayer::LAND;
	if(layer == ELayer::AUTO)
	{
		PathfinderMapView view(*routeSafetyState, hero);
		const auto * tile = view.getTile(hero->visitablePos());
		hadKnownLanding = tile->isLand() && !tile->blocked();
	}
	else if(layer == ELayer::AIR || layer == ELayer::WATER)
		hadKnownLanding = findKnownTransientLanding(*routeSafetyState, hero, std::nullopt, nullptr, LandingKnowledge::PLAYER_ROUTE) == KnownLanding::FOUND;
	bool safe = true;
	for(size_t index = 1; index < steps.size(); ++index)
	{
		const auto * previous = steps[index - 1];
		const auto * step = steps[index];
		// Future turns and interactive actions may change the world or reveal
		// fog. Validate the current uninterrupted movement prefix only.
		if(step->turns != 0 || (index > 1 && previous->action != EPathNodeAction::NORMAL))
			break;
		if(step->layer == ELayer::LAND)
		{
			hadKnownLanding = true;
			continue;
		}
		if(step->layer != ELayer::AIR && step->layer != ELayer::WATER)
			break;
		if(step->isTeleportAction())
			break;
		if(step->moveRemains <= 0 && previous->coord != step->coord)
		{
			safe = false;
			break;
		}
		const bool hasLanding = findKnownTransientLanding(*routeSafetyState, hero,
			PathfinderStart{step->coord, step->layer, step->moveRemains}, nullptr, LandingKnowledge::PLAYER_ROUTE) == KnownLanding::FOUND;
		if(hadKnownLanding && !hasLanding)
		{
			safe = false;
			break;
		}
		hadKnownLanding = hasLanding;
	}
	routeSafetyCache.emplace(destination, safe);
	return safe;
}

const CGPathNode * CPathsInfo::getPathInfo(const int3 & tile, const ELayer layer) const
{
	assert(vstd::iswithin(tile.x, 0, sizes.x));
	assert(vstd::iswithin(tile.y, 0, sizes.y));
	assert(vstd::iswithin(tile.z, 0, sizes.z));
	const auto * node = layer < ELayer::NUM_LAYERS ? getNode(tile, layer) : getNode(tile);
	return isKnownSafeRoute(node) ? node : &unreachableNode;
}

bool CPathsInfo::getPath(CGPath & out, const int3 & dst, const ELayer layer) const
{
	out.nodes.clear();
	const CGPathNode * curnode = getPathInfo(dst, layer);
	if(!curnode->theNodeBefore)
		return false;

	while(curnode)
	{
		const CGPathNode cpn = * curnode;
		curnode = curnode->theNodeBefore;
		out.nodes.push_back(cpn);
	}
	return true;
}

const CGPathNode * CPathsInfo::getNode(const int3 & coord) const
{
	const auto * landNode = getNode(coord, ELayer::LAND);
	if(landNode->reachable())
		return landNode;
	return getNode(coord, ELayer::SAIL);
}

PathNodeInfo::PathNodeInfo()
	: node(nullptr), nodeObject(nullptr), tile(nullptr), coord(-1, -1, -1), guarded(false),	isInitialPosition(false)
{
}

void PathNodeInfo::setNode(const IGameInfoCallback & gameInfo, CGPathNode * n, const PathfinderMapView * mapView)
{
	node = n;
	guarded = false;

	if(coord != node->coord)
	{
		assert(node->coord.isValid());

		coord = node->coord;
		tile = mapView ? mapView->getTile(coord) : gameInfo.getTile(coord);
		nodeObject = nullptr;
		nodeHero = nullptr;

		ObjectInstanceID topObjectID = tile->topVisitableObj();
		if (topObjectID.hasValue())
		{
			nodeObject = gameInfo.getObjInstance(topObjectID);

			if (nodeObject->ID == Obj::HERO)
			{
				nodeHero = dynamic_cast<const CGHeroInstance *>(nodeObject);
				ObjectInstanceID bottomObjectID = tile->topVisitableObj(true);

				if (bottomObjectID.hasValue())
					nodeObject = gameInfo.getObjInstance(bottomObjectID);
			}
		}
	}

}

void PathNodeInfo::updateInfo(CPathfinderHelper * hlp, const IGameInfoCallback & gameInfo)
{
	if(hlp->getKnownGuardingCreaturePosition(node->coord).isValid() && !isInitialPosition)
	{
		guarded = true;
	}

	if(nodeObject)
	{
		objectRelations = gameInfo.getPlayerRelations(hlp->owner, nodeObject->tempOwner);
	}

	if(nodeHero)
	{
		heroRelations = gameInfo.getPlayerRelations(hlp->owner, nodeHero->tempOwner);
	}
}

bool PathNodeInfo::isNodeObjectVisitable() const
{
	/// Hero can't visit objects while walking on water or flying
	return (node->layer == EPathfindingLayer::LAND || node->layer == EPathfindingLayer::SAIL)
		&& (canSeeObj(nodeObject) || canSeeObj(nodeHero));
}


CDestinationNodeInfo::CDestinationNodeInfo():
	blocked(false),
	action(EPathNodeAction::UNKNOWN)
{
}

void CDestinationNodeInfo::setNode(const IGameInfoCallback & gameInfo, CGPathNode * n, const PathfinderMapView * mapView)
{
	PathNodeInfo::setNode(gameInfo, n, mapView);

	blocked = false;
	action = EPathNodeAction::UNKNOWN;
}

bool CDestinationNodeInfo::isBetterWay() const
{
	if(node->turns == 0xff) //we haven't been here before
		return true;
	else
		return cost < node->getCost(); //this route is faster
}
