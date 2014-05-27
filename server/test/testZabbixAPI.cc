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

#include <string>
#include <gcutter.h>
#include "ZabbixAPITestUtils.h"

using namespace std;

namespace testZabbixAPI {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getAPIVersion_2_2_0(void)
{
	assertTestGet(ZabbixAPITestee::GET_TEST_TYPE_API_VERSION,
		      ZabbixAPIEmulator::API_VERSION_2_2_0);
}

void test_getAPIVersion(void)
{
	assertTestGet(ZabbixAPITestee::GET_TEST_TYPE_API_VERSION);
}

void data_checkAPIVersion(void)
{
	gcut_add_datum("Equal",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Lower micro version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 3,
		       NULL);
	gcut_add_datum("Higher micro version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 5,
		       NULL);
	gcut_add_datum("Higher minor version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 2,
		       "minor", G_TYPE_INT, 1,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Lower major version",
		       "expected", G_TYPE_BOOLEAN, TRUE,
		       "major", G_TYPE_INT, 1,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
	gcut_add_datum("Higher major version",
		       "expected", G_TYPE_BOOLEAN, FALSE,
		       "major", G_TYPE_INT, 3,
		       "minor", G_TYPE_INT, 0,
		       "micro", G_TYPE_INT, 4,
		       NULL);
}

void test_checkAPIVersion(gconstpointer data)
{
	assertCheckAPIVersion(gcut_data_get_boolean(data, "expected"),
			      gcut_data_get_int(data, "major"),
			      gcut_data_get_int(data, "minor"),
			      gcut_data_get_int(data, "micro"));
}

void test_openSession(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);
	zbxApiTestee.testOpenSession();
}

void test_getAuthToken(void)
{
	MonitoringServerInfo serverInfo;
	ZabbixAPITestee::initServerInfoWithDefaultParam(serverInfo);
	ZabbixAPITestee zbxApiTestee(serverInfo);

	string firstToken, secondToken;
	firstToken = zbxApiTestee.callAuthToken();
	secondToken = zbxApiTestee.callAuthToken();
	cppcut_assert_equal(firstToken, secondToken);
}

} // namespace testZabbixAPI
