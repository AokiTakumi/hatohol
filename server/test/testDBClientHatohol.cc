#include <cppcutter.h>
#include <cutter.h>

#include "Hatohol.h"
#include "DBClientHatohol.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBClientTest.h"

namespace testDBClientHatohol {

static void addTriggerInfo(TriggerInfo *triggerInfo)
{
	DBClientHatohol dbHatohol;
	dbHatohol.addTriggerInfo(triggerInfo);
}
#define assertAddTriggerToDB(X) \
cut_trace(_assertAddToDB<TriggerInfo>(X, addTriggerInfo))

static string makeExpectedOutput(TriggerInfo *triggerInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%"PRIu32"|%"PRIu64"|%d|%d|%ld|%lu|%"PRIu64"|%s|%s\n",
	    triggerInfo->serverId,
	    triggerInfo->id,
	    triggerInfo->status, triggerInfo->severity,
	    triggerInfo->lastChangeTime.tv_sec,
	    triggerInfo->lastChangeTime.tv_nsec,
	    triggerInfo->hostId,
	    triggerInfo->hostName.c_str(),
	    triggerInfo->brief.c_str());
	return expectedOut;
}

static void _assertGetTriggers(uint32_t serverId = ALL_SERVERS)
{
	TriggerInfoList triggerInfoList;
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerInfoList, serverId);
	size_t numExpectedTestTriggers = getNumberOfTestTriggers(serverId);
	cppcut_assert_equal(numExpectedTestTriggers, triggerInfoList.size());

	string expectedText;
	string actualText;
	TriggerInfoListIterator it = triggerInfoList.begin();
	for (size_t i = 0; i < numExpectedTestTriggers; i++, ++it) {
		expectedText += makeExpectedOutput(&testTriggerInfo[i]);
		actualText += makeExpectedOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetTriggers(...) cut_trace(_assertGetTriggers(__VA_ARGS__))

static void _assertGetTriggerInfoList(uint32_t serverId)
{
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		assertAddTriggerToDB(&testTriggerInfo[i]);
	assertGetTriggers(serverId);
}
#define assertGetTriggerInfoList(SERVER_ID) \
cut_trace(_assertGetTriggerInfoList(SERVER_ID))


// TODO: The names of makeExpectedOutput() and makeExpectedItemOutput()
//       will be changed to be the similar of this function.
static string makeEventOutput(EventInfo &eventInfo)
{
	string output =
	  StringUtils::sprintf(
	    "%"PRIu32"|%"PRIu64"|%ld|%ld|%d|%u|%"PRIu64"|%s|%s\n",
	    eventInfo.serverId, eventInfo.id,
	    eventInfo.time.tv_sec, eventInfo.time.tv_nsec,
	    eventInfo.status, eventInfo.severity,
	    eventInfo.hostId,
	    eventInfo.hostName.c_str(),
	    eventInfo.brief.c_str());
	return output;
}

static void _assertGetEvents(void)
{
	EventInfoList eventInfoList;
	DBClientHatohol dbHatohol;
	dbHatohol.getEventInfoList(eventInfoList);
	cppcut_assert_equal(NumTestEventInfo, eventInfoList.size());

	string expectedText;
	string actualText;
	EventInfoListIterator it = eventInfoList.begin();
	for (size_t i = 0; i < NumTestEventInfo; i++, ++it) {
		expectedText += makeEventOutput(testEventInfo[i]);
		actualText += makeEventOutput(*it);
	}
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetEvents() cut_trace(_assertGetEvents())

static string makeExpectedItemOutput(ItemInfo *itemInfo)
{
	string expectedOut =
	  StringUtils::sprintf(
	    "%"PRIu32"|%"PRIu64"|%"PRIu64"|%s|%ld|%lu|%s|%s|%s\n",
	    itemInfo->serverId, itemInfo->id,
	    itemInfo->hostId,
	    itemInfo->brief.c_str(),
	    itemInfo->lastValueTime.tv_sec,
	    itemInfo->lastValueTime.tv_nsec,
	    itemInfo->lastValue.c_str(),
	    itemInfo->prevValue.c_str(),
	    itemInfo->itemGroupName.c_str());
	return expectedOut;
}

static void _assertGetItems(uint32_t serverId)
{
	ItemInfoList itemInfoList;
	DBClientHatohol dbHatohol;
	dbHatohol.getItemInfoList(itemInfoList, serverId);
	size_t numExpectedTestItems = getNumberOfTestItems(serverId);
	cppcut_assert_equal(numExpectedTestItems, itemInfoList.size());

	string expectedText;
	string actualText;
	ItemInfoListIterator it = itemInfoList.begin();
	for (size_t i = 0; i < numExpectedTestItems; i++, ++it) {
		expectedText += makeExpectedItemOutput(&testItemInfo[i]);
		actualText += makeExpectedItemOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
}
#define assertGetItems(SERVER_ID) cut_trace(_assertGetItems(SERVER_ID))

void _assertItemInfoList(uint32_t serverId)
{
	deleteDBClientDB(DB_DOMAIN_ID_HATOHOL);

	DBClientHatohol dbHatohol;
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbHatohol.addItemInfoList(itemInfoList);

	assertGetItems(serverId);
}
#define assertItemInfoList(SERVER_ID) cut_trace(_assertItemInfoList(SERVER_ID))

void test_addTriggerInfoList(void);
static void _assertGetHostInfoList(uint32_t serverId)
{
	// We have to insert test trigger data in DB first. Because current
	// implementation of DBClientHatohol creates hostInfoList
	// from trigger table.
	// TODO: The implementation will be fixed in the future. The DB table
	//       for host will be added. After that, we will fix this setup.
	test_addTriggerInfoList();

	HostInfoList actualHostList;
	HostInfoList expectedHostList;
	ServerIdHostIdMap svIdHostIdMap;
	DBClientHatohol dbHatohol;
	dbHatohol.getHostInfoList(actualHostList, serverId);
	getTestHostInfoList(expectedHostList, serverId, &svIdHostIdMap);

	// comapre two lists
	cppcut_assert_equal(expectedHostList.size(), actualHostList.size());

	HostInfoListIterator actualHost = actualHostList.begin();
	for (; actualHost != actualHostList.end(); ++actualHost) {
		// server ID
		ServerIdHostIdMapIterator svIt =
		   svIdHostIdMap.find(actualHost->serverId);
		cppcut_assert_equal(true, svIt != svIdHostIdMap.end());

		// Host ID
		HostIdSet &hostIdSet = svIt->second;
		HostIdSetIterator hostIt = hostIdSet.find(actualHost->id);
		cppcut_assert_equal(true, hostIt != hostIdSet.end());

		// delete the element from svIdHostIdMap.
		// This is needed to check the duplication of hosts in
		// actualHostList
		hostIdSet.erase(hostIt);
		if (hostIdSet.empty())
			svIdHostIdMap.erase(svIt);
	}
	cppcut_assert_equal((size_t)0, svIdHostIdMap.size());
}
#define assertGetHostInfoList(SERVER_ID) \
cut_trace(_assertGetHostInfoList(SERVER_ID))

void setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createDB(void)
{
	// remove the DB that already exists
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_HATOHOL);

	// create an instance (the database will be automatically created)
	DBClientHatohol dbHatohol;
	cut_assert_exist_path(dbPath.c_str());

	// check the version
	string statement = "select * from _dbclient";
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_HATOHOL, statement);
	string expectedOut = StringUtils::sprintf
	                       ("%d|%d\n", DBClient::DBCLIENT_DB_VERSION,
	                                   DBClientHatohol::HATOHOL_DB_VERSION);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableTrigger(void)
{
	const string tableName = "triggers";
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_HATOHOL);
	DBClientHatohol dbHatohol;
	string command = "sqlite3 " + dbPath + " \".table\"";
	assertExist(tableName, executeCommand(command));

	// check content
	string statement = "select * from " + tableName;
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_HATOHOL, statement);
	string expectedOut = ""; // currently no data
	cppcut_assert_equal(expectedOut, output);
}

void test_addTriggerInfo(void)
{
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_HATOHOL);

	// added a record
	TriggerInfo *testInfo = testTriggerInfo;
	assertAddTriggerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from triggers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeExpectedOutput(testInfo);
	cppcut_assert_equal(expectedOut, result);
}

void test_getTriggerInfoList(void)
{
	assertGetTriggerInfoList(ALL_SERVERS);
}

void test_getTriggerInfoListForOneServer(void)
{
	uint32_t targetServerId = testTriggerInfo[0].serverId;
	assertGetTriggerInfoList(targetServerId);
}

void test_setTriggerInfoList(void)
{
	deleteDBClientDB(DB_DOMAIN_ID_HATOHOL);

	DBClientHatohol dbHatohol;
	TriggerInfoList triggerInfoList;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		triggerInfoList.push_back(testTriggerInfo[i]);
	uint32_t serverId = testTriggerInfo[0].serverId;
	dbHatohol.setTriggerInfoList(triggerInfoList, serverId);

	assertGetTriggers();
}

void test_addTriggerInfoList(void)
{
	deleteDBClientDB(DB_DOMAIN_ID_HATOHOL);

	size_t i;
	DBClientHatohol dbHatohol;

	// First call
	size_t numFirstAdd = NumTestTriggerInfo / 2;
	TriggerInfoList triggerInfoList0;
	for (i = 0; i < numFirstAdd; i++)
		triggerInfoList0.push_back(testTriggerInfo[i]);
	dbHatohol.addTriggerInfoList(triggerInfoList0);

	// Second call
	TriggerInfoList triggerInfoList1;
	for (; i < NumTestTriggerInfo; i++)
		triggerInfoList1.push_back(testTriggerInfo[i]);
	dbHatohol.addTriggerInfoList(triggerInfoList1);

	// Check
	assertGetTriggers();
}

void test_itemInfoList(void)
{
	assertItemInfoList(ALL_SERVERS);
}

void test_itemInfoListForOneServer(void)
{
	uint32_t targetServerId = testItemInfo[0].serverId;
	assertItemInfoList(targetServerId);
}

void test_addEventInfoList(void)
{
	// DBClientHatohol internally joins the trigger table and the event table.
	// So we also have to add trigger data.
	// When the internal join is removed, the following line will not be
	// needed.
	test_setTriggerInfoList();

	DBClientHatohol dbHatohol;
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		eventInfoList.push_back(testEventInfo[i]);
	dbHatohol.addEventInfoList(eventInfoList);

	assertGetEvents();
}

void test_getLastEventId(void)
{
	test_addEventInfoList();
	DBClientHatohol dbHatohol;
	const int serverid = 3;
	cppcut_assert_equal(findLastEventId(serverid),
	                    dbHatohol.getLastEventId(serverid));
}

void test_getHostInfoList(void)
{
	assertGetHostInfoList(ALL_SERVERS);
}

void test_getHostInfoListForOneServer(void)
{
	uint32_t targetServerId = testTriggerInfo[0].serverId;
	assertGetHostInfoList(targetServerId);
}

} // namespace testDBClientHatohol
