/*
 * MovementRequestTracker.h, part of VCMI engine
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include "../lib/constants/EntityIdentifiers.h"

#include <mutex>
#include <optional>
#include <set>

/// Associate movement acknowledgements with the currently followed route.
class MovementRequestTracker
{
	std::mutex mutex;
	std::optional<ObjectInstanceID> activeHero;
	std::set<int> requests;
	inline static thread_local MovementRequestTracker * sending = nullptr;

public:
	class SendScope
	{
		MovementRequestTracker * previous;
	public:
		explicit SendScope(MovementRequestTracker & tracker) : previous(sending) { sending = &tracker; }
		~SendScope() { sending = previous; }
		SendScope(const SendScope &) = delete;
		SendScope & operator=(const SendScope &) = delete;
	};

	void start(ObjectInstanceID hero)
	{
		std::lock_guard lock(mutex);
		activeHero = hero;
		requests.clear();
	}

	void stop()
	{
		std::lock_guard lock(mutex);
		activeHero.reset();
		requests.clear();
	}

	void sent(ObjectInstanceID hero, int requestID)
	{
		if(sending != this)
			return;
		std::lock_guard lock(mutex);
		if(activeHero && *activeHero == hero)
			requests.insert(requestID);
	}

	bool consume(int requestID)
	{
		std::lock_guard lock(mutex);
		return requests.erase(requestID) != 0;
	}
};
