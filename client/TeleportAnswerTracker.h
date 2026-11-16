/*
 * TeleportAnswerTracker.h, part of VCMI engine
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include "../lib/constants/EntityIdentifiers.h"

#include <mutex>
#include <optional>

/// Keep an unrelated dialog acknowledgement from resuming a teleport route.
class TeleportAnswerTracker
{
	mutable std::mutex mutex;
	std::optional<QuestionID> question;
	std::optional<int> request;
	std::optional<ObjectInstanceID> hero;
	bool teleportObserved = false;

public:
	enum class Result { UNRELATED, CONTINUE, STOP };

	void begin(QuestionID id, ObjectInstanceID heroID)
	{
		std::lock_guard lock(mutex);
		question = id;
		hero = heroID;
		request.reset();
		teleportObserved = false;
	}

	void teleported(ObjectInstanceID heroID)
	{
		std::lock_guard lock(mutex);
		if(question && hero && *hero == heroID)
			teleportObserved = true;
	}

	void sent(QuestionID id, int requestID)
	{
		std::lock_guard lock(mutex);
		if(question && *question == id)
			request = requestID;
	}

	Result consume(int requestID, bool accepted)
	{
		std::lock_guard lock(mutex);
		if(!request || *request != requestID)
			return Result::UNRELATED;
		const bool canContinue = accepted && teleportObserved;
		question.reset();
		request.reset();
		hero.reset();
		teleportObserved = false;
		return canContinue ? Result::CONTINUE : Result::STOP;
	}

	bool waiting() const
	{
		std::lock_guard lock(mutex);
		return question.has_value();
	}
};
