#include <cppcutter.h>
#include <cutter.h>

#include "Asura.h"
#include "DBClientConfig.h"
#include "ConfigManager.h"
#include "Helpers.h"
#include "DBClientTest.h"

namespace testDBClientConfig {

static void addTargetServer(MonitoringServerInfo *serverInfo)
{
	DBClientConfig dbConfig;
	dbConfig.addTargetServer(serverInfo);
}
#define assertAddServerToDB(X) \
cut_trace(_assertAddToDB<MonitoringServerInfo>(X, addTargetServer))

static string makeExpectedOutput(MonitoringServerInfo *serverInfo)
{
	string expectedOut = StringUtils::sprintf
	                       ("%u|%d|%s|%s|%s\n",
	                        serverInfo->id, serverInfo->type,
	                        serverInfo->hostName.c_str(),
	                        serverInfo->ipAddress.c_str(),
	                        serverInfo->nickname.c_str());
	return expectedOut;
}

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createDB(void)
{
	// remove the DB that already exists
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_CONFIG);

	// create an instance (the database will be automatically created)
	DBClientConfig dbConfig;
	cut_assert_exist_path(dbPath.c_str());

	// check the version
	string statement = "select * from _dbclient";
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_CONFIG, statement);
	string expectedOut = StringUtils::sprintf("%d|%d\n",
	                                          DBClient::DB_VERSION,
	                                          DBClientConfig::DB_VERSION);
	cppcut_assert_equal(expectedOut, output);
}

void test_createTableServers(void)
{
	const string tableName = "servers";
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_CONFIG);
	DBClientConfig dbConfig;
	string command = "sqlite3 " + dbPath + " \".table\"";
	assertExist(tableName, executeCommand(command));

	// check content
	string statement = "select * from " + tableName;
	string output = execSqlite3ForDBClient(DB_DOMAIN_ID_CONFIG, statement);
	string expectedOut = StringUtils::sprintf(""); // currently no data
	cppcut_assert_equal(expectedOut, output);
}

void test_testAddTargetServer(void)
{
	string dbPath = deleteDBClientDB(DB_DOMAIN_ID_CONFIG);

	// added a record
	MonitoringServerInfo *testInfo = serverInfo;
	assertAddServerToDB(testInfo);

	// confirm with the command line tool
	string cmd = StringUtils::sprintf(
	               "sqlite3 %s \"select * from servers\"", dbPath.c_str());
	string result = executeCommand(cmd);
	string expectedOut = makeExpectedOutput(testInfo);
	cppcut_assert_equal(expectedOut, result);
}

void test_testGetTargetServers(void)
{
	for (size_t i = 0; i < NumServerInfo; i++)
		assertAddServerToDB(&serverInfo[i]);

	MonitoringServerInfoList monitoringServers;
	DBClientConfig dbConfig;
	dbConfig.getTargetServers(monitoringServers);
	cppcut_assert_equal(NumServerInfo, monitoringServers.size());

	string expectedText;
	string actualText;
	MonitoringServerInfoListIterator it = monitoringServers.begin();
	for (size_t i = 0; i < NumServerInfo; i++, ++it) {
		expectedText += makeExpectedOutput(&serverInfo[i]);
		actualText += makeExpectedOutput(&(*it));
	}
	cppcut_assert_equal(expectedText, actualText);
}

} // namespace testDBClientConfig
