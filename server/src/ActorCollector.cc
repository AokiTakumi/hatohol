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

#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <cstring>
#include "ActorCollector.h"
#include "DBClientAction.h"
#include "Logger.h"
#include "MutexLock.h"
#include "SessionManager.h"
#include "Reaper.h"
using namespace std;
using namespace mlpl;

typedef map<pid_t, ActorInfo *>      WaitChildSet;
typedef WaitChildSet::iterator       WaitChildSetIterator;
typedef WaitChildSet::const_iterator WaitChildSetConstIterator;

struct ActorCollector::PrivateContext {
	static MutexLock lock;
	static WaitChildSet waitChildSet;
	static ActorCollector *collector;
	static sem_t collectorSem;
	static bool  collectorExitRequest;

	// for reset
	static bool  inReset;
	static sem_t resetCompletionSem;
};

MutexLock ActorCollector::PrivateContext::lock;
WaitChildSet ActorCollector::PrivateContext::waitChildSet;
ActorCollector *ActorCollector::PrivateContext::collector = NULL;
sem_t           ActorCollector::PrivateContext::collectorSem;
bool            ActorCollector::PrivateContext::collectorExitRequest = false;

sem_t           ActorCollector::PrivateContext::resetCompletionSem;
bool            ActorCollector::PrivateContext::inReset = false;

// ---------------------------------------------------------------------------
// ActorInfo
// ---------------------------------------------------------------------------
ActorInfo::ActorInfo(void)
: pid(0),
  logId(-1),
  dontLog(false),
  collectedCb(NULL),
  postCollectedCb(NULL),
  collectedCbPriv(NULL),
  timerTag(INVALID_EVENT_ID)
{
}

ActorInfo &ActorInfo::operator=(const ActorInfo &actorInfo)
{
	pid             = actorInfo.pid;
	logId           = actorInfo.logId;
	dontLog         = actorInfo.dontLog;
	// Don't copy sessionId to avoid multiple call of
	// SessionManager::remove() for the session.
	collectedCb     = actorInfo.collectedCb;
	postCollectedCb = actorInfo.postCollectedCb;
	collectedCbPriv = actorInfo.collectedCbPriv;
	timerTag        = actorInfo.timerTag;

	return *this;
}

ActorInfo::~ActorInfo()
{
	// If the following function fails, MPL_ERR() is called in it.
	// So we do nothing here.
	Utils::removeEventSourceIfNeeded(timerTag);

	if (!sessionId.empty()) {
		SessionManager *sessionMgr = SessionManager::getInstance();
		if (!sessionMgr->remove(sessionId)) {
			MLPL_WARN("Failed to remove session: %s\n",
			          sessionId.c_str());
		}
	}
}

// ---------------------------------------------------------------------------
// ActorCollector::Locker
// ---------------------------------------------------------------------------
ActorCollector::Locker::Locker(void)
{
	ActorCollector::lock();
}

ActorCollector::Locker::~Locker()
{
	ActorCollector::unlock();
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void ActorCollector::init(void)
{
	HATOHOL_ASSERT(sem_init(&PrivateContext::collectorSem, 0, 0) == 0,
	               "Failed to call sem_init(): %d\n", errno);
	HATOHOL_ASSERT(sem_init(&PrivateContext::resetCompletionSem, 0, 0) == 0,
	               "Failed to call sem_init(): %d\n", errno);
}

void ActorCollector::reset(void)
{
	// Some tests calls g_child_watch_add() and g_spawn_sync() families
	// that internally call it. They set their own handler for SIGCHLD.
	// So we reset it here. This implies that a test that uses
	// ActorCollector has to call hatoholInit() or ActorCollector::reset()
	// in cut_setup().
	registerSIGCHLD();

	CppReaper<Locker> lockReaper(new Locker());
	if (!PrivateContext::collector)
		return;
	incWaitingActor(); // to unblock wait_sem().
	PrivateContext::inReset = true;
	lockReaper.reap();

	// Wait for the completion of reset() on the collected thread
	while (true) {
		int ret = sem_trywait(&PrivateContext::resetCompletionSem);
		if (ret == -1 && errno == EAGAIN) {
			bool worked = false;
			while (g_main_context_iteration(NULL, FALSE))
				worked = true;
			if (!worked)
				sched_yield();
			continue;
		}
		if (ret == -1 && errno == EINTR)
			continue;
		HATOHOL_ASSERT(ret == 0,
		               "Failed to call sem_wait(): %d\n", errno);
		break;
	}
}

void ActorCollector::resetOnCollectorThread(void)
{
	// This function is mainly for the test. In the normal use,
	// waitChildSet is of course empty when this function is called
	// at the start-up.
	while (!PrivateContext::waitChildSet.empty()) {
		WaitChildSetIterator it = PrivateContext::waitChildSet.begin();
		delete it->second; // actorInfo;
		pid_t pid = it->first;
		PrivateContext::waitChildSet.erase(it);
		int ret = kill(pid, SIGKILL);
		if (ret == -1 && errno == ESRCH) {
			MLPL_INFO("No process w/ pid: %d\n", pid);
			continue;
		}
		HATOHOL_ASSERT(ret == 0, "Failed to send kill (%d): %d\n",
		               pid, errno);
	}

	PrivateContext::inReset = false;
	HATOHOL_ASSERT(sem_init(&PrivateContext::collectorSem, 0, 0) == 0,
	               "Failed to call sem_init(): %d\n", errno);
	HATOHOL_ASSERT(sem_post(&PrivateContext::resetCompletionSem) == 0,
	               "Failed to call sem_post(): %d\n", errno);
}

void ActorCollector::quit(void)
{
	lock();
	if (!PrivateContext::collector) {
		unlock();
		return;
	}
	MLPL_INFO("stop process: started.\n");
	PrivateContext::collectorExitRequest = true;;
	unlock();
	incWaitingActor(); 
}

void ActorCollector::addActor(ActorInfo *actorInfo)
{
	// This function is currently called only from
	// ActionManager::execCommandAction(). Because lock() and unlock() are
	// called in it, so they aren't called here.
	pair<WaitChildSetIterator, bool> result =
	  PrivateContext::waitChildSet.insert
	    (pair<pid_t, ActorInfo *>(actorInfo->pid, actorInfo));
	if (!result.second) {
		MLPL_BUG("pid: %d (logId: %"PRIu64 ") is already regstered.\n",
		         actorInfo->pid, actorInfo->logId);
		return;
	}

	incWaitingActor();
}

ActorCollector::ActorCollector(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ActorCollector::~ActorCollector()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void ActorCollector::registerSIGCHLD(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = SIG_DFL;
	sa.sa_flags |= (SA_RESTART|SA_SIGINFO);
	HATOHOL_ASSERT(sigaction(SIGCHLD, &sa, NULL ) == 0,
	               "Failed to set SIGCHLD, errno: %d\n", errno);
}

void ActorCollector::incWaitingActor(void)
{
	// Currently this functions calls from addActor() and
	// notifyChildSiginfo(). In the former case, addActor() is called
	// only from ActorManager::spawn() with the lock. So a race never
	// happens. In the later case, it is ensured that 'collector' is not
	// NULL and not changed, because notifyChildSiginfo() is a class
	// method of 'collector'.
	if (!PrivateContext::collector) {
		PrivateContext::collector = new ActorCollector();
		PrivateContext::collector->start();
	}

	HATOHOL_ASSERT(
	  sem_post(&PrivateContext::collectorSem) == 0,
	  "Failed to call sem_post(): %d\n", errno);
}

void ActorCollector::lock(void)
{
	PrivateContext::lock.lock();
}

void ActorCollector::unlock(void)
{
	PrivateContext::lock.unlock();
}

bool ActorCollector::isWatching(pid_t pid)
{
	bool found = false;
	lock();
	WaitChildSetIterator it = PrivateContext::waitChildSet.find(pid);
	if (it != PrivateContext::waitChildSet.end())
		found = true;
	unlock();
	return found;
}

void ActorCollector::setDontLog(pid_t pid)
{
	bool found = false;
	lock();
	WaitChildSetIterator it = PrivateContext::waitChildSet.find(pid);
	if (it != PrivateContext::waitChildSet.end()) {
		it->second->dontLog = true;
		found = true;
	}
	unlock();
	if (!found)
		MLPL_WARN("Not found pid: %d for setDontLog().\n", pid);
}

size_t ActorCollector::getNumberOfWaitingActors(void)
{
	lock();
	size_t num = PrivateContext::waitChildSet.size();
	unlock();
	return num;
}

void ActorCollector::start(void)
{
	bool autoDelete = true;
	HatoholThreadBase::start(autoDelete);
}

//
// These methods are executed on a collector thread
//
gpointer ActorCollector::mainThread(HatoholThreadArg *arg)
{
	int ret;
	siginfo_t siginfo;
	while (true) {
		ret = sem_wait(&PrivateContext::collectorSem);
		if (ret == -1 && errno == EINTR)
			continue;
		lock();
		if (PrivateContext::inReset) {
			unlock();
			resetOnCollectorThread();
			continue;
		}
		bool shouldExit = PrivateContext::collectorExitRequest;
		unlock();
		if (shouldExit)
			break;

		while (true) {
			ret = waitid(P_ALL, 0, &siginfo, WEXITED);
			if (ret == -1 && errno == EINTR)
				continue;
			break;
		}
		if (ret == -1) {
			MLPL_ERR("Failed to call wait_id: %d\n", errno);
			continue;
		}
		notifyChildSiginfo(&siginfo);
	}
	MLPL_INFO("stop process: completed.\n");
	return NULL;
}

void ActorCollector::notifyChildSiginfo(siginfo_t *info)
{
	ChildSigInfo childSigInfo;
	childSigInfo.pid      = info->si_pid;
	childSigInfo.code     = info->si_code;
	childSigInfo.status   = info->si_status;

	// check the reason of the signal.
	DBClientAction::LogEndExecActionArg logArg;
	if (childSigInfo.code == CLD_EXITED) {
		logArg.status = ACTLOG_STAT_SUCCEEDED;
		// failureCode is set to ACTLOG_EXECFAIL_NONE in the
		// LogEndExecActionArg's constructor.
	} else if (childSigInfo.code == CLD_KILLED) {
		logArg.status = ACTLOG_STAT_FAILED;
		logArg.failureCode = ACTLOG_EXECFAIL_KILLED_SIGNAL;
	} else if (childSigInfo.code == CLD_DUMPED) {
		logArg.status = ACTLOG_STAT_FAILED;
		logArg.failureCode = ACTLOG_EXECFAIL_DUMPED_SIGNAL;
	} else {
		// The received-signal candidates are
		// CLD_TRAPPED, CLD_STOPPED, and CLD_CONTINUED.
		MLPL_INFO("Actor: %d, status: %d\n",
		          childSigInfo.pid, childSigInfo.status);
		incWaitingActor();
		return;
	}
	logArg.exitCode = childSigInfo.status;

	// try to find the action log.
	ActorInfo *actorInfo = NULL;
	lock();
	WaitChildSetIterator it =
	   PrivateContext::waitChildSet.find(childSigInfo.pid);
	if (it != PrivateContext::waitChildSet.end()) {
		actorInfo = it->second;
		PrivateContext::waitChildSet.erase(it);
	}

	// return if the actorInfo instance was not found
	if (!actorInfo) {
		unlock();
		incWaitingActor();
		return;
	}

	// execute the callback function
	if (actorInfo->collectedCb)
		(*actorInfo->collectedCb)(actorInfo);
	unlock();

	// log the action result if needed
	if (!actorInfo->dontLog) {
		DBClientAction dbAction;
		logArg.logId = actorInfo->logId;
		dbAction.logEndExecAction(logArg);
	}

	// execute the callback function without the lock
	if (actorInfo->postCollectedCb)
		(*actorInfo->postCollectedCb)(actorInfo);
	
	// ActionManager::commandActionTimeoutCb() may be running on the
	// default GLib event loop. So we delete actorInfo on that.
	Utils::deleteOnGLibEventLoop<ActorInfo>(actorInfo, ASYNC);
}

