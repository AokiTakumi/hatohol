/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ActorCollector_h
#define ActorCollector_h

#include "HatoholThreadBase.h"
#include "HatoholError.h"

struct ActorInfo;
typedef void (*ActorCollectedFunc)(const ActorInfo *actorInfo);

struct ActorInfo {
	pid_t    pid;
	uint64_t logId;
	bool     dontLog;
	std::string sessionId;

	// collectedCb is called with taking ActorCollector::lock().
	// postCollectedCb is called after calling ActorCollector::unlock().
	ActorCollectedFunc collectedCb;
	mutable ActorCollectedFunc postCollectedCb;
	mutable void              *collectedCbPriv;
	guint timerTag;
	
	// constructor and destructor
	ActorInfo(void);
	ActorInfo &operator=(const ActorInfo &actorInfo);
	virtual ~ActorInfo();
};

class ActorCollector : public HatoholThreadBase
{
public:
	struct Locker
	{
		Locker(void);
		virtual ~Locker();
	};

	struct Profile {
		mlpl::StringVector args;
		mlpl::StringVector envs;
		std::string workingDirectory;

		/**
		 * These callbacks are called in debut() synchronusly.
		 */
		virtual ActorInfo *successCb(const pid_t &pid) = 0;
		virtual void postSuccessCb(void) = 0;
		virtual void errorCb(GError *error) = 0;
	};

	static void init(void);
	static void reset(void);
	static void resetOnCollectorThread(void);
	static void quit(void);

	/**
	 * Create a new actor.
	 *
	 * @param profile An actor's profile.
	 * @return A HatoholError instance.
	 */
	static HatoholError debut(Profile &profile);

	/**
	 * lock() has to be called before this function is used.
	 */
	static void addActor(ActorInfo *actorInfo);

	ActorCollector(void);
	virtual ~ActorCollector();

	/**
	 * Check if ActorCollector is watching the process.
	 *
	 * @param pid A process ID of a checked process.
	 */
	static bool isWatching(pid_t pid);

	static void setDontLog(pid_t pid);

	static size_t getNumberOfWaitingActors(void);

protected:
	static void registerSIGCHLD(void);
	static void incWaitingActor(void);
	static void lock(void);
	static void unlock(void);

	// we redefine start() as protected so that HatoholThreadBase::start()
	// cannot be called from other classes.
	void start(void);
	static void notifyChildSiginfo(const siginfo_t *info);

	// overriden virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ActorCollector_h

