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

#include <cppcutter.h>
#include <SimpleSemaphore.h>
#include "Hatohol.h"
#include "HapZabbixAPI.h"
#include "ZabbixAPIEmulator.h"
#include "HatoholArmPluginTestPair.h"

using namespace mlpl;
using namespace qpid::messaging;

namespace testHapZabbixAPI {

static const ServerIdType DEFAULT_SERVER_ID = 5;
static const guint EMULATOR_PORT = 33333;
static ZabbixAPIEmulator g_apiEmulator;

class HapZabbixAPITest :
  public HapZabbixAPI, public HapiTestHelper {
public:
	HapZabbixAPITest(void)
	: m_readySem(0)
	{
	}

	void assertWaitReady(void)
	{
		assertWaitSemaphore(m_readySem);
	}

	void callWorkOnTriggers(void)
	{
		updateAuthTokenIfNeeded();
		workOnTriggers();
	}

protected:
	void onConnected(Connection &conn) override
	{
		HapiTestHelper::onConnected(conn);
	}

	void onInitiated(void) override
	{
		HapZabbixAPI::onInitiated();
		HapiTestHelper::onInitiated();
	}

	void onReady(void) override
	{
		m_readySem.post();
	}

private:
	SimpleSemaphore m_readySem;
};

void cut_setup(void)
{
	hatoholInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
	else
		g_apiEmulator.setOperationMode(OPE_MODE_NORMAL);
}

void cut_teardown(void)
{
	g_apiEmulator.reset();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getTriggers(void)
{
	deleteDBClientHatoholDB();
	HatoholArmPluginTestPair<HapZabbixAPITest> pair(
	  DEFAULT_SERVER_ID, "127.0.0.1", EMULATOR_PORT);
	pair.plugin->assertWaitReady();
	pair.plugin->callWorkOnTriggers();

	// TODO: check the DB content
	sleep(1);
}

} // namespace testHapZabbixAPI
