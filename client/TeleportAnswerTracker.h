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
public:
	enum class Result { UNRELATED, WAIT, CONTINUE, STOP };

private:
	mutable std::mutex mutex;
	std::optional<QuestionID> question;
	std::optional<int> request;
	std::optional<ObjectInstanceID> hero;
	bool teleportObserved = false;
	bool acknowledgementReceived = false;
	bool resolutionRequired = false;
	bool resolutionReceived = false;

	Result finishIfReady()
	{
		if(!acknowledgementReceived || (resolutionRequired && !resolutionReceived))
			return Result::WAIT;
		const bool canContinue = teleportObserved;
		question.reset();
		request.reset();
		hero.reset();
		teleportObserved = false;
		acknowledgementReceived = false;
		resolutionReceived = false;
		resolutionRequired = false;
		return canContinue ? Result::CONTINUE : Result::STOP;
	}

public:
	void begin(QuestionID id, ObjectInstanceID heroID, bool waitForResolution)
	{
		std::lock_guard lock(mutex);
		question = id;
		hero = heroID;
		request.reset();
		teleportObserved = false;
		acknowledgementReceived = false;
		resolutionRequired = waitForResolution;
		resolutionReceived = false;
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
		if(!accepted)
		{
			question.reset();
			request.reset();
			hero.reset();
			teleportObserved = false;
			acknowledgementReceived = false;
			resolutionRequired = false;
			resolutionReceived = false;
			return Result::STOP;
		}
		request.reset(); // Ignore duplicate acknowledgements while waiting for resolution.
		acknowledgementReceived = true;
		return finishIfReady();
	}

	Result resolved(QuestionID id)
	{
		std::lock_guard lock(mutex);
		if(!question || *question != id || !resolutionRequired)
			return Result::UNRELATED;
		resolutionReceived = true;
		return finishIfReady();
	}

	bool waiting() const
	{
		std::lock_guard lock(mutex);
		return question.has_value();
	}
};
