/*
 * Copyright (C) 2013-2014 Project Hatohol
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

#include "Synchronizer.h"
#include "RedmineAPIEmulator.h"
#include "Hatohol.h"
#include "IssueSenderRedmine.h"
#include "LabelUtils.h"
#include "DBClientTest.h"
#include "Helpers.h"
#include "JsonParserAgent.h"
#include <cppcutter.h>
#include <gcutter.h>

using namespace std;
using namespace mlpl;

namespace testIssueSenderRedmine {

static Synchronizer g_sync;

class TestRedmineSender : public IssueSenderRedmine {
public:
	TestRedmineSender(const IssueTrackerInfo &tracker)
	: IssueSenderRedmine(tracker)
	{
	}
	virtual ~TestRedmineSender()
	{
	}
	string buildTitle(const EventInfo &event,
			  const MonitoringServerInfo *server)
	{
		return IssueSender::buildTitle(event, server);
	}
	string buildDescription(const EventInfo &event,
				const MonitoringServerInfo *server)
	{
		return IssueSender::buildDescription(event, server);
	}
	string buildJson(const EventInfo &event)
	{
		return IssueSenderRedmine::buildJson(event);
	}
	string getIssuesJsonURL(void)
	{
		return IssueSenderRedmine::getIssuesJsonURL();
	}
	HatoholError parseResponse(IssueInfo &issueInfo,const string &response)
	{
		return IssueSenderRedmine::parseResponse(issueInfo, response);
	}
};

void cut_setup(void)
{
	cppcut_assert_equal(false, g_sync.isLocked(),
	                    cut_message("g_sync is locked."));

	hatoholInit();
	if (!g_redmineEmulator.isRunning())
		g_redmineEmulator.start(EMULATOR_PORT);
	deleteDBClientHatoholDB();
}

void cut_teardown(void)
{
	g_sync.reset();
	g_redmineEmulator.reset();
}

string expectedJson(const EventInfo &event)
{
	MonitoringServerInfo &server = testServerInfo[event.serverId - 1];

	char timeString[128];
	struct tm eventTime;
	localtime_r(&event.time.tv_sec, &eventTime);
	strftime(timeString, sizeof(timeString),
		 "%a, %d %b %Y %T %z", &eventTime);

	string expected =
	  StringUtils::sprintf(
	    "{\"issue\":{"
	    "\"subject\":\"[%s %s] %s\","
	    "\"description\":\""
	    "Server ID: %" FMT_SERVER_ID "\\n"
	    "    Hostname:   \\\"%s\\\"\\n"
	    "    IP Address: \\\"%s\\\"\\n"
	    "    Nickname:   \\\"%s\\\"\\n"
	    "Host ID: %" FMT_HOST_ID "\\n"
	    "    Hostname:   \\\"%s\\\"\\n"
	    "Event ID: %" FMT_EVENT_ID "\\n"
	    "    Time:       \\\"%ld.%09ld (%s)\\\"\\n"
	    "    Type:       \\\"%d (%s)\\\"\\n"
	    "    Brief:      \\\"%s\\\"\\n"
	    "Trigger ID: %" FMT_TRIGGER_ID "\\n"
	    "    Status:     \\\"%d (%s)\\\"\\n"
	    "    Severity:   \\\"%d (%s)\\\"\\n"
	    "\""
	    "}}",
	    // subject
	    server.getHostAddress().c_str(),
	    event.hostName.c_str(),
	    event.brief.c_str(),
	    // description
	    server.id,
	    server.hostName.c_str(),
	    server.ipAddress.c_str(),
	    server.nickname.c_str(),
	    event.hostId,
	    event.hostName.c_str(),
	    event.id,
	    event.time.tv_sec, event.time.tv_nsec, timeString,
	    event.type,
	    LabelUtils::getEventTypeLabel(event.type).c_str(),
	    event.brief.c_str(),
	    event.triggerId,
	    event.status,
	    LabelUtils::getTriggerStatusLabel(event.status).c_str(),
	    event.severity,
	    LabelUtils::getTriggerSeverityLabel(event.severity).c_str());
	return expected;
}

void test_buildJson(void)
{
	setupTestDBConfig(true, true);
	IssueTrackerInfo tracker;
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(expectedJson(testEventInfo[0]),
			    sender.buildJson(testEventInfo[0]));
}

void test_getIssuesJsonURL(void)
{
	IssueTrackerInfo &tracker = testIssueTrackerInfo[0];
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(
	  string("http://localhost/projects/1/issues.json"),
	  sender.getIssuesJsonURL());
}

void test_getIssuesJsonURLWithStringProjectId(void)
{
	IssueTrackerInfo &tracker = testIssueTrackerInfo[1];
	TestRedmineSender sender(tracker);
	cppcut_assert_equal(
	  string("http://localhost/projects/hatohol/issues.json"),
	  sender.getIssuesJsonURL());
}

static void makeExpectedIssueInfo(IssueInfo &issue,
				  const IssueTrackerInfo &tracker,
				  const EventInfo &event,
				  const RedmineIssue &postedIssue)
{
	issue.serverId = event.serverId;
	issue.eventId = event.id;
	issue.trackerId = tracker.id;
	issue.identifier = StringUtils::toString((int)postedIssue.id);
	issue.location = tracker.baseURL + "/issues/" + issue.identifier;
	issue.status = postedIssue.getStatusName();
	issue.createdAt = postedIssue.createdOn;
	issue.updatedAt = postedIssue.updatedOn;
}

void _assertSend(const HatoholErrorCode &expected,
		 const IssueTrackerInfo &tracker,
		 const EventInfo &event)
{
	setupTestDBConfig(true, true);
	TestRedmineSender sender(tracker);
	g_redmineEmulator.addUser(tracker.userName, tracker.password);
	HatoholError result = sender.send(event);
	cppcut_assert_equal(expected, result.getCode());

	if (expected != HTERR_OK)
		return;

	// verify the reply
	MonitoringServerInfo &server = testServerInfo[event.serverId - 1];
	JsonParserAgent agent(g_redmineEmulator.getLastResponse());
	cppcut_assert_equal(true, agent.startObject("issue"));
	string title, description, trackerId;
	agent.read("subject", title);
	agent.read("description", description);
	cppcut_assert_equal(sender.buildTitle(event, &server), title);
	cppcut_assert_equal(sender.buildDescription(event, &server),
			    description);
	if (!tracker.trackerId.empty()) {
		agent.startObject("tracker");
		int64_t trackerId = 0;
		agent.read("id", trackerId);
		cppcut_assert_equal(tracker.trackerId,
				    StringUtils::toString((int)trackerId));
		agent.endObject();
	}

	// verify the saved information
	IssueInfo issue;
	makeExpectedIssueInfo(issue, tracker, event,
			      g_redmineEmulator.getLastIssue());
	DBClientHatohol dbClientHatohol;
	DBAgent *dbAgent = dbClientHatohol.getDBAgent();
	string statement = "select * from issues;";
	string expect = makeIssueOutput(issue);
	assertDBContent(dbAgent, statement, expect);
}
#define assertSend(E,T,V) \
cut_trace(_assertSend(E,T,V))

void test_send(void)
{
	assertSend(HTERR_OK, testIssueTrackerInfo[2], testEventInfo[0]);
}

void test_sendWithUnknownTracker(void)
{
	IssueTrackerInfo tracker = testIssueTrackerInfo[2];
	tracker.trackerId = "100";
	assertSend(HTERR_FAILED_TO_SEND_ISSUE, tracker, testEventInfo[0]);
}

void test_parseResponse(void)
{
	IssueTrackerInfo tracker = testIssueTrackerInfo[2];
	IssueInfo expected = testIssueInfo[0], actual;
	RedmineIssue issue;
	issue.id = atoi(expected.identifier.c_str());
	issue.trackerId = atoi(tracker.trackerId.c_str());
	issue.assigneeId = 1;
	issue.assigneeName = expected.assignee;
	issue.startDate = expected.createdAt;
	issue.createdOn = expected.createdAt;
	issue.updatedOn = expected.updatedAt;

	TestRedmineSender sender(tracker);
	actual.trackerId = expected.trackerId;
	actual.serverId = expected.serverId;
	actual.eventId = expected.eventId;
	HatoholError result = sender.parseResponse(actual, issue.toJson());
	cppcut_assert_equal(HTERR_OK, result.getCode());
	cppcut_assert_equal(makeIssueOutput(expected),
			    makeIssueOutput(actual));
}

struct CallbackData
{
	size_t errorsCount;
	bool   succeeded;
	bool   failed;

	CallbackData()
	: errorsCount(0), succeeded(false), failed(false)
	{
	}
};

static void statusCallback(const EventInfo &info,
			   const IssueSender::JobStatus &status,
			   void *userData)
{
	CallbackData *data = static_cast<CallbackData*>(userData);
	switch (status) {
	case IssueSender::JOB_RETRYING:
		data->errorsCount++;
		break;
	case IssueSender::JOB_SUCCEEDED:
		data->succeeded = true;
		break;
	case IssueSender::JOB_FAILED:
		data->failed = true;
		break;
	default:
		break;
	}
}

void _assertThread(size_t numErrors, bool shouldSuccess = true)
{
	setupTestDBConfig(true, true);
	const IssueTrackerInfo tracker = testIssueTrackerInfo[2];
	const EventInfo &event = testEventInfo[0];
	TestRedmineSender sender(tracker);
	CallbackData cbData;
	sender.setRetryInterval(10);
	g_redmineEmulator.addUser(tracker.userName, tracker.password);
	for (size_t i = 0; i < numErrors; i++)
		g_redmineEmulator.queueDummyResponse(
		  SOUP_STATUS_INTERNAL_SERVER_ERROR);

	sender.start();
	sender.queue(event, statusCallback, (void*)&cbData);
	while (!sender.isIdling())
		usleep(100 * 1000);
	sender.exitSync();

	size_t expectedErrorsCount = shouldSuccess ? numErrors : 3;
	cppcut_assert_equal(expectedErrorsCount, cbData.errorsCount);
	cppcut_assert_equal(shouldSuccess, cbData.succeeded);
	cppcut_assert_equal(!shouldSuccess, cbData.failed);

	// check the posted issue
	string expect;
	if (shouldSuccess) {
		IssueInfo issue;
		makeExpectedIssueInfo(issue, tracker, event,
				      g_redmineEmulator.getLastIssue());
		expect = makeIssueOutput(issue);
	}
	DBClientHatohol dbClientHatohol;
	DBAgent *dbAgent = dbClientHatohol.getDBAgent();
	string statement = "select * from issues;";
	assertDBContent(dbAgent, statement, expect);
}
#define assertThread(N, ...) \
cut_trace(_assertThread(N, ##__VA_ARGS__))

void test_thread(void)
{
	assertThread(0);
}

void test_threadWithWithInLimitErrors(void)
{
	const size_t retryLimit = 3;
	assertThread(retryLimit);
}

void test_threadWithLimitOverErrors(void)
{
	const size_t retryLimit = 3;
	bool shouldSuccessSending = true;
	assertThread(retryLimit + 1, !shouldSuccessSending);
}

}
