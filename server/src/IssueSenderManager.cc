/*
 * Copyright (C) 2014 Project Hatohol
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

#include <map>
#include "MutexLock.h"
#include "Reaper.h"
#include "IssueSenderManager.h"
#include "IssueSenderRedmine.h"
#include "CacheServiceDBClient.h"

using namespace std;
using namespace mlpl;

struct IssueSenderManager::PrivateContext
{
	static IssueSenderManager instance;
	map<IssueTrackerIdType, IssueSender*> sendersMap;
	MutexLock sendersLock;

	~PrivateContext()
	{
		sendersLock.lock();
		Reaper<MutexLock> unlocker(&sendersLock, MutexLock::unlock);
		map<IssueTrackerIdType, IssueSender*>::iterator it;
		for (it = sendersMap.begin(); it != sendersMap.end(); it++) {
			IssueSender *sender = it->second;
			delete sender;
		}
		sendersMap.clear();
	}

	IssueSender *createIssueSender(const IssueTrackerIdType &id)
	{
		CacheServiceDBClient cache;
		DBClientConfig *dbConfig = cache.getConfig();
		IssueTrackerInfoVect issueTrackerVect;
		IssueTrackerQueryOption option(USER_ID_SYSTEM);
		//TODO: not implemented yet
		//option.setTragetId(id);
		dbConfig->getIssueTrackers(issueTrackerVect, option);

		if (issueTrackerVect.size() <= 0) {
			MLPL_ERR("Not found IssueTrackerInfo: %d\n", id);
			return NULL;
		}
		if (issueTrackerVect.size() > 1) {
			MLPL_ERR("Too many IssueTrackerInfo for ID:%d\n", id);
			return NULL;
		}

		IssueTrackerInfo &tracker = *issueTrackerVect.begin();
		IssueSender *sender = NULL;
		switch (tracker.type) {
		case ISSUE_TRACKER_REDMINE:
			sender = new IssueSenderRedmine(tracker);
			break;
		default:
			MLPL_ERR("Invalid IssueTracker type: %d\n",
				 tracker.type);
			break;
		}
		return sender;
	}

	IssueSender *getSender(const IssueTrackerIdType &id)
	{
		sendersLock.lock();
		Reaper<MutexLock> unlocker(&sendersLock, MutexLock::unlock);
		if (sendersMap.find(id) != sendersMap.end() && sendersMap[id])
			return sendersMap[id];

		IssueSender *sender = createIssueSender(id);
		if (sender)
			sendersMap[id] = sender;

		return sender;
	}
};

IssueSenderManager IssueSenderManager::PrivateContext::instance;

IssueSenderManager &IssueSenderManager::getInstance(void)
{
	return PrivateContext::instance;
}

void IssueSenderManager::queue(
  const IssueTrackerIdType &trackerId, const EventInfo &eventInfo)
{
	IssueSender *sender = m_ctx->getSender(trackerId);
	if (!sender) {
		MLPL_ERR("Failed to queue sending an issue"
			 " for the event: %" FMT_EVENT_ID,
			 eventInfo.id);
		return;
	}
	sender->queue(eventInfo);
}

IssueSenderManager::IssueSenderManager(void)
{
	m_ctx = new PrivateContext();
}

IssueSenderManager::~IssueSenderManager()
{
	delete m_ctx;
}
