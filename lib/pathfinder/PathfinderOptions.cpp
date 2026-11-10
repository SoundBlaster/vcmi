/*
 * CPathfinder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PathfinderOptions.h"

#include "NodeStorage.h"
#include "PathfindingRules.h"
#include "CPathfinder.h"

#include "../IGameSettings.h"
#include "../GameLibrary.h"
#include "../callback/IGameInfoCallback.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapping/TerrainTile.h"

struct PathfinderMapView::Impl
{
	const IGameInfoCallback & gameInfo;
	ObjectInstanceID hero;
	mutable std::map<int3, TerrainTile> tiles;
};

PathfinderMapView::PathfinderMapView(const IGameInfoCallback & gameInfo, const CGHeroInstance * hero)
	: impl(std::make_unique<Impl>(gameInfo, hero->id))
{
}

PathfinderMapView::~PathfinderMapView() = default;

const TerrainTile * PathfinderMapView::getTile(const int3 & position) const
{
	const auto found = impl->tiles.find(position);
	if(found != impl->tiles.end())
		return &found->second;
	const auto * tile = impl->gameInfo.getTile(position);
	if(!vstd::contains(tile->visitableObjects, impl->hero) && !vstd::contains(tile->blockingObjects, impl->hero))
		return tile;
	auto it = impl->tiles.emplace(position, *tile).first;
	vstd::erase_if(it->second.visitableObjects, [&](ObjectInstanceID id) { return id == impl->hero; });
	vstd::erase_if(it->second.blockingObjects, [&](ObjectInstanceID id) { return id == impl->hero; });
	return &it->second;
}

PathfinderOptions::PathfinderOptions(const IGameInfoCallback & cb)
	: useFlying(true)
	, useWaterWalking(true)
	, ignoreGuards(cb.getSettings().getBoolean(EGameSettings::PATHFINDER_IGNORE_GUARDS))
	, useEmbarkAndDisembark(cb.getSettings().getBoolean(EGameSettings::PATHFINDER_USE_BOAT))
	, useTeleportTwoWay(cb.getSettings().getBoolean(EGameSettings::PATHFINDER_USE_MONOLITH_TWO_WAY))
	, useTeleportOneWay(cb.getSettings().getBoolean(EGameSettings::PATHFINDER_USE_MONOLITH_ONE_WAY_UNIQUE))
	, useTeleportOneWayRandom(cb.getSettings().getBoolean(EGameSettings::PATHFINDER_USE_MONOLITH_ONE_WAY_RANDOM))
	, useTeleportWhirlpool(cb.getSettings().getBoolean(EGameSettings::PATHFINDER_USE_WHIRLPOOL))
	, originalFlyRules(cb.getSettings().getBoolean(EGameSettings::PATHFINDER_ORIGINAL_FLY_RULES))
	, useCastleGate(false)
	, lightweightFlyingMode(false)
	, oneTurnSpecialLayersLimit(true)
	, turnLimit(std::numeric_limits<uint8_t>::max())
	, canUseCast(false)
	, useDimensionDoor(false)
	, allowLayerTransitioningAfterBattle(false)
	, forceUseTeleportWhirlpool(false)
{
}

PathfinderConfig::PathfinderConfig(std::shared_ptr<INodeStorage> nodeStorage, const IGameInfoCallback & callback, std::vector<std::shared_ptr<IPathfindingRule>> rules):
	nodeStorage(std::move(nodeStorage)),
	rules(std::move(rules)),
	options(callback)
{
}

std::vector<std::shared_ptr<IPathfindingRule>> SingleHeroPathfinderConfig::buildRuleSet()
{
	return std::vector<std::shared_ptr<IPathfindingRule>>{
		std::make_shared<LayerTransitionRule>(),
			std::make_shared<DestinationActionRule>(),
			std::make_shared<MovementToDestinationRule>(),
			std::make_shared<MovementCostRule>(),
			std::make_shared<MovementAfterDestinationRule>()
	};
}

SingleHeroPathfinderConfig::~SingleHeroPathfinderConfig() = default;

SingleHeroPathfinderConfig::SingleHeroPathfinderConfig(CPathsInfo & out, const IGameInfoCallback & gs, const CGHeroInstance * hero)
	: PathfinderConfig(std::make_shared<NodeStorage>(out, hero), gs, buildRuleSet())
	, hero(hero)
{
}

SingleHeroPathfinderConfig::SingleHeroPathfinderConfig(CPathsInfo & out, const IGameInfoCallback & gs, const CGHeroInstance * hero, const PathfinderStart & start)
	: PathfinderConfig(std::make_shared<NodeStorage>(out, hero, start), gs, buildRuleSet())
	, hero(hero)
{
	if(hero->inBoat() || !gs.isInTheMap(start.position) || start.position.z != hero->visitablePos().z
		|| (start.layer != EPathfindingLayer::AIR && start.layer != EPathfindingLayer::WATER)
		|| start.movementPoints < 0)
		throw std::invalid_argument("Invalid prospective on-foot transient pathfinder start");
	options.mapView = std::make_shared<PathfinderMapView>(gs, hero);
}

CPathfinderHelper * SingleHeroPathfinderConfig::getOrCreatePathfinderHelper(const PathNodeInfo & source, const IGameInfoCallback & gameInfo)
{
	if (!pathfinderHelper)
		pathfinderHelper = std::make_unique<CPathfinderHelper>(gameInfo, hero, options);

	return pathfinderHelper.get();
}
