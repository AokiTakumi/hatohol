#include <string>
#include <vector>
#include <map>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include <glib.h>
#include "SQLProcessor.h"

namespace testMySQLWorkerZabbix {

typedef map<size_t, string> NumberStringMap;
typedef NumberStringMap::iterator NumberStringMapIterator;

gchar *g_standardOutput = NULL;
gchar *g_standardError = NULL;
GError *g_error = NULL;
gboolean g_spawnRet;
gint     g_exitStatus;

void setup()
{
}

void teardown()
{
	if (g_standardOutput) {
		g_free(g_standardOutput);
		g_standardOutput = NULL;
	}
	if (g_standardError) {
		g_free(g_standardError);
		g_standardError = NULL;
	}
	if (g_error) {
		g_error_free(g_error);
		g_error = NULL;
	}
}

static const char *execResult(void)
{
	string str;
	str = StringUtils::sprintf("ret: %d, exit status: %d\n"
	                           "<<stdout>>\n%s\n<<stderr>>\n%s",
	                           g_spawnRet, g_exitStatus,
	                           g_standardOutput, g_standardError);
	return str.c_str();
}

static void _assertRecord(int numExpectedLines, NumberStringMap &nsmap,
                          vector<string> *linesOut = NULL)
{
	vector<string> lines;
	string stdOutStr(g_standardOutput);
	StringUtils::split(lines, stdOutStr, '\n');
	cut_assert_equal_int(numExpectedLines, lines.size());

	NumberStringMapIterator it = nsmap.begin();
	for (; it != nsmap.end(); ++it) {
		size_t lineNum = it->first;
		string &expected = it->second;
		cut_assert_equal_string(expected.c_str(),
		                        lines[lineNum].c_str());
	}
	if (linesOut)
		*linesOut = lines;
}
#define assertRecord(NUM, NSMAP, ...) \
cut_trace(_assertRecord(NUM, NSMAP, ##__VA_ARGS__))

static void executeCommand(const char *cmd)
{
	const gchar *working_directory = NULL;
	const char *portStr = getenv("TEST_MYSQL_PORT");
	if (!portStr)
		portStr = "3306";
	const gchar *argv[] = {
	  "mysql", "-h", "127.0.0.1", "-P", NULL /* portStr */, "-B",
	  "-e", NULL /* cmd */, NULL};
	argv[4] = portStr;
	argv[7] = cmd;
	gchar *envp = NULL;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GSpawnChildSetupFunc child_setup = NULL;
	gpointer user_data = NULL;
	g_spawnRet = g_spawn_sync(working_directory,
	                          (gchar **)argv, (gchar **)envp, flags,
	                          child_setup, user_data,
	                          &g_standardOutput, &g_standardError,
	                          &g_exitStatus, &g_error);
	cppcut_assert_equal((gboolean)TRUE, g_spawnRet,
	                    cut_message("%s", execResult()));
	cppcut_assert_equal(0, g_exitStatus, cut_message("%s", execResult()));
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_atAtVersionComment(void)
{
	const char *cmd = "select @@version_comment";
	executeCommand(cmd);
	NumberStringMap nsmap;
	nsmap[1] = "ASURA";
	assertRecord(2, nsmap);
}

void test_selectNodes(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT n.* FROM nodes n WHERE n.nodetype=1 ORDER BY n.nodeid";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

void test_selectConfig(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT c.* FROM config c "
	  "WHERE c.configid BETWEEN 000000000000000 AND 099999999999999";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(2, nsmap);
}

void test_selectUseridNoCondition(void)
{
	const char *cmd = "use zabbix; SELECT u.userid FROM users u";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(3, nsmap, &lines);
}

void test_selectUserid(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT u.userid,u.attempt_failed,u.attempt_clock,u.attempt_ip "
	  "FROM users u WHERE u.alias='guest' AND "
	  "u.userid BETWEEN 000000000000000 AND 099999999999999";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(2, nsmap, &lines);

	StringVector splitResult;
	StringUtils::split(splitResult, lines[0], '\t');
	cppcut_assert_equal((size_t)4, splitResult.size());
	cppcut_assert_equal(string("userid"),         splitResult[0]);
	cppcut_assert_equal(string("attempt_failed"), splitResult[1]);
	cppcut_assert_equal(string("attempt_clock"),  splitResult[2]);
	cppcut_assert_equal(string("attempt_ip"),     splitResult[3]);

	splitResult.clear();
	StringUtils::split(splitResult, lines[1], '\t', false);
	cppcut_assert_equal((size_t)4, splitResult.size());
	cppcut_assert_equal(string("2"), splitResult[0]);
}

void test_selectUsrgrpid(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT g.usrgrpid FROM usrgrp g,users_groups ug "
	  "WHERE ug.userid=1 AND g.usrgrpid=ug.usrgrpid AND "
	  "g.users_status=1 LIMIT 1 OFFSET 0";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

void test_selectMaxGuiAccess(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT MAX(g.gui_access) AS gui_access "
	  "FROM usrgrp g,users_groups ug "
	  "WHERE ug.userid=2 AND g.usrgrpid=ug.usrgrpid";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(2, nsmap, &lines);

	StringVector splitResult;
	StringUtils::split(splitResult, lines[0], '\t');
	cppcut_assert_equal((size_t)1, splitResult.size());
	cppcut_assert_equal(string("gui_access"), splitResult[0]);

	splitResult.clear();
	StringUtils::split(splitResult, lines[1], '\t', false);
	cppcut_assert_equal((size_t)1, splitResult.size());
	cppcut_assert_equal(string("0"), splitResult[0]);
}

void test_selectUseridAutoLogoutLastAccess(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT u.userid,u.autologout,s.lastaccess FROM sessions s,users u "
	  "WHERE s.sessionid='9331b5c345021fa3879caa8922586199' AND "
	  "s.status=0 AND s.userid=u.userid AND "
	  "((s.lastaccess+u.autologout>1357641517) OR (u.autologout=0)) "
	  "AND u.userid BETWEEN 000000000000000 AND 099999999999999";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

void test_insertSessions(void)
{
	const char *cmd = "use zabbix;"
	  "INSERT INTO sessions (sessionid,userid,lastaccess,status) "
	  "VALUES ('9ba4d7eb67f2025db7d3e881ee64702c',2,1361063975,0)";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

void test_updateSessions(void)
{
	cut_trace(test_insertSessions());
	const char *cmd = "use zabbix;"
	  "UPDATE sessions SET lastaccess=1361275141 "
	  "WHERE userid=2 AND sessionid='9ba4d7eb67f2025db7d3e881ee64702c'";
	executeCommand(cmd);
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

void test_selectProfiles(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT p.* FROM profiles p WHERE p.userid=1 AND "
	  "p.profileid BETWEEN 000000000000000 AND 099999999999999 "
	  "ORDER BY p.userid,p.profileid";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(103, nsmap);
}

void test_selectUserHistory(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT uh.title1,uh.url1,uh.title2,uh.url2,uh.title3,uh.url3,"
	  "uh.title4,uh.url4,uh.title5,uh.url5 FROM user_history uh "
	  "WHERE uh.userid=1";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(2, nsmap);
}

void test_selectCountTrigger(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT COUNT(DISTINCT t.triggerid) AS cnt,t.status,t.value "
	  "FROM triggers t INNER JOIN functions f ON t.triggerid=f.triggerid "
	  "INNER JOIN items i ON f.itemid=i.itemid INNER JOIN hosts h "
	  "ON i.hostid=h.hostid WHERE i.status=0 AND h.status=0 "
	  "GROUP BY t.status,t.value";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

void test_selectCountItemsHosts(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT COUNT(*) AS cnt,i.status FROM items i "
	  "INNER JOIN hosts h ON i.hostid=h.hostid "
	  "WHERE h.status=0 AND  (i.status IN ('0','1','3'))  "
	  "GROUP BY i.status";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(0, nsmap);
}

#if 0
void test_selectGroupHostGroupHosts(void)
{
	const char *cmd = "use zabbix;"
	  "SELECT  DISTINCT  g.* FROM groups g,hosts_groups hg,hosts h "
	  "WHERE hg.groupid=g.groupid AND h.hostid=hg.hostid AND h.status=0 "
	  "AND EXISTS "
	    "(SELECT t.triggerid FROM items i,functions f,triggers t "
	    "WHERE i.hostid=hg.hostid AND i.status=0 AND i.itemid=f.itemid "
	    "AND f.triggerid=t.triggerid AND t.status=0)"
	  "AND g.groupid BETWEEN 000000000000000 AND 099999999999999";
	executeCommand(cmd);
	vector<string> lines;
	NumberStringMap nsmap;
	assertRecord(2, nsmap);
}
#endif

} // namespace testMySQLWorkerZabbix
