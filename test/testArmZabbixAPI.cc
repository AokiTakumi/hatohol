#include <cppcutter.h>
#include <sys/time.h>
#include <errno.h>
#include "Asura.h"
#include "ZabbixAPIEmulator.h"
#include "ArmZabbixAPI.h"
#include "Helpers.h"
#include "Synchronizer.h"

namespace testArmZabbixAPI {
static Synchronizer g_sync;

class ArmZabbixAPITestee :  public ArmZabbixAPI {

typedef bool (ArmZabbixAPITestee::*ThreadOneProc)(void);

	static const size_t NUM_TEST_TRIGGER_READ = 3;

public:
	ArmZabbixAPITestee(int zabbixServerId, const char *server)
	: ArmZabbixAPI(zabbixServerId, server),
	  m_result(false),
	  m_threadOneProc(&ArmZabbixAPITestee::defaultThreadOneProc),
	  m_countThreadOneProc(0)
	{
		addExceptionCallback(_exceptionCb, this);
	}

	bool getResult(void) const
	{
		return m_result;
	}

	const string errorMessage(void) const
	{
		return m_errorMessage;
	}

	bool testGetTriggers(void)
	{
		m_countThreadOneProc = 0;
		g_sync.lock();
		setPollingInterval(0);
		m_threadOneProc = &ArmZabbixAPITestee::threadOneProcTriggers;
		addExitCallback(exitCbTriggers, this);
		start();
		g_sync.wait();
		return m_result;
	}

protected:
	static void _exceptionCb(const exception &e, void *data)
	{
		ArmZabbixAPITestee *obj =
		   static_cast<ArmZabbixAPITestee *>(data);
		obj->exceptionCb(e);
	}

	static void exitCbTriggers(void *)
	{
		g_sync.unlock();
	}

	void exceptionCb(const exception &e)
	{
		m_result = false;
		m_errorMessage = e.what();
		g_sync.unlock();
	}

	bool defaultThreadOneProc(void)
	{
		THROW_ASURA_EXCEPTION("m_threadOneProc is not set.");
		return false;
	}

	bool threadOneProcTriggers(void)
	{
		bool succeeded = ArmZabbixAPI::mainThreadOneProc();
		if (!succeeded) {
			m_result = false;
			m_errorMessage = "Failed: mainThreadOneProc()";
		} else if (m_countThreadOneProc == NUM_TEST_TRIGGER_READ) {
			m_result = true;
			requestExit();
		}
		return succeeded;
	}

	// virtual function
	bool mainThreadOneProc(void)
	{
		m_countThreadOneProc++;;
		return (this->*m_threadOneProc)();
	}

private:
	bool m_result;
	string m_errorMessage;
	ThreadOneProc m_threadOneProc;
	size_t        m_countThreadOneProc;
};

static const guint EMULATOR_PORT = 33333;

ZabbixAPIEmulator g_apiEmulator;

void setup(void)
{
	if (!g_sync.trylock())
		cut_fail("g_sync is not unlocked.");
	g_sync.unlock();

	asuraInit();
	if (!g_apiEmulator.isRunning())
		g_apiEmulator.start(EMULATOR_PORT);
}

void teardown(void)
{
	if (!g_sync.trylock())
		g_sync.unlock();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getTriggers(void)
{
	int svId = 0;
	deleteDBClientZabbixDB(svId);
	ArmZabbixAPITestee armZbxApiTestee(svId, "localhost");
	cppcut_assert_equal
	  (true, armZbxApiTestee.testGetTriggers(),
	   cut_message("%s\n", armZbxApiTestee.errorMessage().c_str()));
}

} // namespace testArmZabbixAPI
