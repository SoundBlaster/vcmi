/*
* FindObj.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Goals/CGoal.h"
#include "../Analyzers/ObjectClusterizer.h"


namespace NK2AI
{

struct HeroPtr;
class AIGateway;
class FuzzyHelper;

namespace Goals
{
	class DLL_EXPORT UnlockCluster : public CGoal<UnlockCluster>
	{
	private:
		std::shared_ptr<ObjectCluster> cluster;
		AIPath pathToCenter;
		std::string blockerName;

	public:
		UnlockCluster(std::shared_ptr<ObjectCluster> cluster, const AIPath & pathToCenter, const CGObjectInstance * blocker)
			: CGoal(Goals::UNLOCK_CLUSTER), cluster(cluster), pathToCenter(pathToCenter), blockerName(blocker->getObjectNameTextID())
		{
			tile = blocker->visitablePos();
			hero = pathToCenter.targetHero;
			objid = blocker->id.getNum();
		}

		bool operator==(const UnlockCluster & other) const override;
		std::string toString() const override;
		std::shared_ptr<ObjectCluster> getCluster() const { return cluster; }
		const AIPath & getPathToCenter() { return pathToCenter; }
	};
}

}
