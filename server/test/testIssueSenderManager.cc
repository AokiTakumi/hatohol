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

#include <cutter.h>
#include <cppcutter.h>
#include "IssueSenderManager.h"
#include "Hatohol.h"
#include "Helpers.h"
#include "DBClientTest.h"
#include "RedmineAPIEmulator.h"

using namespace std;

namespace testIssueSenderManager {

class TestIssueSenderManager : public IssueSenderManager
{
public:
	// Enable to create a object to test destructor
	TestIssueSenderManager(void)
	{
	}

	IssueSender *getSender(const IssueTrackerIdType &id)
	{
		return IssueSenderManager::getSender(id);
	}
};

void cut_setup(void)
{
	hatoholInit();
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
	deleteDBClientHatoholDB();
}

void cut_teardown(void)
{
	g_redmineEmulator.reset();
}

void test_instanceIsSingleton(void)
{
	IssueSenderManager &manager1 = IssueSenderManager::getInstance();
	IssueSenderManager &manager2 = IssueSenderManager::getInstance();
	cppcut_assert_equal(&manager1, &manager2);
}

static void statusCallback(const EventInfo &info,
			   const IssueSender::JobStatus &status,
			   void *userData)
{
	bool *succeeded = static_cast<bool*>(userData);

	switch (status) {
	case IssueSender::JOB_SUCCEEDED:
		*succeeded = true;
		break;
	default:
		break;
	}
}

void test_sendRedmineIssue(void)
{
	setupTestDBConfig(true, true);
	IssueTrackerIdType trackerId = 3;
	IssueTrackerInfo &tracker = testIssueTrackerInfo[trackerId - 1];
	g_redmineEmulator.addUser(tracker.userName, tracker.password);
	TestIssueSenderManager manager;
	bool succeeded = false;
	manager.queue(trackerId, testEventInfo[0],
		      statusCallback, (void*)&succeeded);
	while (!manager.isIdling())
		usleep(100 * 1000);
	const string &json = g_redmineEmulator.getLastResponse();
	cppcut_assert_equal(true, succeeded);
	cppcut_assert_equal(false, json.empty());
}

void test_createMultiThreads(void)
{
	setupTestDBConfig(true, true);
	IssueTrackerIdType trackerId1 = 3;
	IssueTrackerIdType trackerId2 = 4;
	IssueTrackerInfo &tracker1 = testIssueTrackerInfo[trackerId1 - 1];
	IssueTrackerInfo &tracker2 = testIssueTrackerInfo[trackerId2 - 1];
	g_redmineEmulator.addUser(tracker1.userName, tracker1.password);
	g_redmineEmulator.addUser(tracker2.userName, tracker2.password);
	TestIssueSenderManager manager;
	bool succeeded1 = false;
	bool succeeded2 = false;
	manager.queue(trackerId1, testEventInfo[0],
		      statusCallback, (void*)&succeeded1);
	manager.queue(trackerId2, testEventInfo[0],
		      statusCallback, (void*)&succeeded2);
	while (!manager.isIdling())
		usleep(100 * 1000);
	const string &json = g_redmineEmulator.getLastResponse();
	cppcut_assert_equal(true, succeeded1 && succeeded2);
	cppcut_assert_equal(false, json.empty());
	IssueSender *sender1 = manager.getSender(trackerId1);
	IssueSender *sender2 = manager.getSender(trackerId2);
	cppcut_assert_equal(true, sender1 && sender2);
	cppcut_assert_equal(trackerId1, sender1->getIssueTrackerInfo().id);
	cppcut_assert_equal(trackerId2, sender2->getIssueTrackerInfo().id);
}

} // namespace testIssueSenderManager