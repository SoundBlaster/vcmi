/*
 * PathfinderUtil.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../mapObjects/CGObjectInstance.h"
#include "../mapping/TerrainTile.h"
#include "../mapping/MapTilesStorage.h"
#include "../mapping/CMapHeader.h"
#include "../mapping/MapTopology.h"
#include "../callback/IGameInfoCallback.h"
#include "CGPathNode.h"

namespace PathfinderUtil
{
	using FoW = MapTilesStorage<uint8_t>;
	using ELayer = EPathfindingLayer;

	inline std::vector<int3> knownGuardians(const int3 & pos, const FoW & fow, const IGameInfoCallback & gameInfo)
	{
		std::vector<int3> result;
		const auto topology = gameInfo.getMapHeader()->getTopology();
		for(const auto * guard : gameInfo.getGuardingCreatures(pos))
		{
			const auto guardPos = topology.canonicalize(guard->visitablePos());
			if(guardPos && fow[*guardPos])
				result.push_back(*guardPos);
		}
		return result;
	}

	inline int3 knownGuardingCreaturePosition(const int3 & pos, const FoW & fow, const IGameInfoCallback & gameInfo)
	{
		const auto guardPos = gameInfo.guardingCreaturePosition(pos);
		if(!gameInfo.getMapHeader()->toroidal || !guardPos.isValid() || fow[guardPos])
			return guardPos;
		const auto guards = knownGuardians(pos, fow, gameInfo);
		return guards.empty() ? int3(-1, -1, -1) : guards.front();
	}

	inline bool layerMatchesTerrain(const ELayer & layer, const TerrainTile & tile)
	{
		return !((layer == ELayer::LAND && tile.isWater())
			|| ((layer == ELayer::SAIL || layer == ELayer::WATER) && !tile.isWater()));
	}

	inline bool layerBlockedByObjects(const ELayer & layer, const TerrainTile & tile)
	{
		return (layer == ELayer::WATER && tile.blocked())
			|| (layer == ELayer::AVIATE && (tile.blocked() || tile.visitable()));
	}

	template<EPathfindingLayer::Type layer>
	EPathAccessibility evaluateAccessibility(const int3 & pos, const TerrainTile & tinfo, const FoW & fow, const PlayerColor player, const IGameInfoCallback & gameInfo)
	{
		if(!fow[pos])
			return EPathAccessibility::BLOCKED;

		switch(layer)
		{
		case ELayer::LAND:
		case ELayer::SAIL:
			if(tinfo.visitable())
			{
				if (tinfo.visitableObjects.size() > 1)
				{
					auto frontVisitable = gameInfo.getObjInstance(tinfo.visitableObjects.front());
					auto backVisitable = gameInfo.getObjInstance(tinfo.visitableObjects.back());
					if(frontVisitable->ID == Obj::SANCTUARY && backVisitable->ID == Obj::HERO && backVisitable->getOwner() != player)
					{
						return EPathAccessibility::BLOCKED;
					}
				}

				bool hasBlockedVisitable = false;
				bool hasVisitable = false;

				for(const auto objID : tinfo.visitableObjects)
				{
					auto obj = gameInfo.getObjInstance(objID);

					if(obj->isBlockedVisitable())
						hasBlockedVisitable = true;
					else if(!obj->passableFor(player) && obj->ID != Obj::EVENT)
						hasVisitable = true;
				}

				if(hasBlockedVisitable)
					return EPathAccessibility::BLOCKVIS;
				if(hasVisitable)
					return EPathAccessibility::VISITABLE;

				return EPathAccessibility::ACCESSIBLE;
			}
			else if(tinfo.blocked())
			{
				return EPathAccessibility::BLOCKED;
			}
			else if(knownGuardingCreaturePosition(pos, fow, gameInfo).isValid())
			{
				// Monster close by; blocked visit for battle
				return EPathAccessibility::GUARDED;
			}

			break;

		case ELayer::WATER:
			if(layerBlockedByObjects(ELayer::WATER, tinfo) || tinfo.isLand())
				return EPathAccessibility::BLOCKED;

			break;

		case ELayer::AVIATE:
			//cannot aviate over blocked and visitable tiles (use FLY layer instead)
			if(layerBlockedByObjects(ELayer::AVIATE, tinfo))
				return EPathAccessibility::BLOCKED;
			return EPathAccessibility::ACCESSIBLE;
			break;

		case ELayer::AIR:
			return EPathAccessibility::FLYABLE;

			break;
		}

		return EPathAccessibility::ACCESSIBLE;
	}
}
