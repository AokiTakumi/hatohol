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

#include <cppcutter.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <syscall.h>
#include <MutexLock.h>
using namespace mlpl;
#include "Hatohol.h"
#include "Utils.h"
#include "Helpers.h"
#include "HatoholThreadBase.h"

namespace testUtils {

struct TestExecEvtLoop : public HatoholThreadBase {
	pid_t threadId;
	pid_t eventLoopThreadId;
	GMainContext *context;
	GMainLoop *loop;
	bool quitLoop;
	bool useFunctor;
	SyncType syncType;
	MutexLock mutex;

	TestExecEvtLoop(void)
	: threadId(0),
	  eventLoopThreadId(0),
	  context(NULL),
	  loop(NULL),
	  quitLoop(true),
	  useFunctor(false),
	  syncType(SYNC)
	{
		mutex.lock();
		context = g_main_context_default();
		cppcut_assert_not_null(context);

		loop = g_main_loop_new(context, TRUE);
		cppcut_assert_not_null(loop);
	}

	virtual gpointer mainThread(HatoholThreadArg *arg)
	{
		threadId = Utils::getThreadId();
		if (!useFunctor) {
			Utils::executeOnGLibEventLoop<TestExecEvtLoop>(
			  _idleTask, this, syncType, context);
		} else {
			Utils::executeOnGLibEventLoop<TestExecEvtLoop>(
			  *this, syncType, context);
		}
		mutex.unlock();
		return NULL;
	}

	static void _idleTask(TestExecEvtLoop *obj)
	{
		obj->idleTask();
	}

	void idleTask(void)
	{
		eventLoopThreadId = Utils::getThreadId();
		if (quitLoop)
			g_main_loop_quit(loop);
	}

	void operator()(void) {
		idleTask();
	}
};

static void _assertValidateJSMethodName(const string &name, bool expect)
{
	string errMsg;
	cppcut_assert_equal(
	  expect, Utils::validateJSMethodName(name, errMsg),
	  cut_message("%s", errMsg.c_str()));
}
#define assertValidateJSMethodName(N,E) \
cut_trace(_assertValidateJSMethodName(N,E))

static void _assertGetExtension(const string &path, const string &expected)
{
	string actualExt = Utils::getExtension(path);
	cppcut_assert_equal(expected, actualExt);
}
#define assertGetExtension(P, E) cut_trace(_assertGetExtension(P, E))

void cut_setup(void)
{
	hatoholInit();
}

void cut_teardown(void)
{
	releaseDefaultContext();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getCurrTimeAsMicroSecond(void)
{
	const size_t allowedErrorInMicroSec = 100 * 1000; // 100 ms
	struct timeval tv;
	cppcut_assert_equal(0, gettimeofday(&tv, NULL),
	                    cut_message("errno: %d", errno));
	uint64_t currTime = Utils::getCurrTimeAsMicroSecond();
	uint64_t timeError = currTime;
	timeError -= tv.tv_sec * 1000 * 1000;
	timeError -= tv.tv_usec;
	cppcut_assert_equal(true, timeError < allowedErrorInMicroSec);
	if (isVerboseMode())
		cut_notify("timeError: %"PRIu64 " [us]", timeError);
}

void test_validateJSMethodName(void)
{
	assertValidateJSMethodName("IYHoooo_21", true);
}

void test_validateJSMethodNameEmpty(void)
{
	assertValidateJSMethodName("", false);
}

void test_validateJSMethodNameFromNumber(void)
{
	assertValidateJSMethodName("1foo", false);
}

void test_validateJSMethodNameWithSpace(void)
{
	assertValidateJSMethodName("o o", false);
}

void test_validateJSMethodNameWithDot(void)
{
	assertValidateJSMethodName("o.o", false);
}

void test_validateJSMethodNameWithExclamation(void)
{
	assertValidateJSMethodName("o!o", false);
}

void test_getExtension(void)
{
	string extension = "json";
	string path = "/hoge/foo/gator." + extension;
	assertGetExtension(path, extension);
}

void test_getExtensionLastDot(void)
{
	assertGetExtension("/hoge/foo/gator.", "");
}

void test_getExtensionNoDot(void)
{
	assertGetExtension("/hoge/foo/gator", "");
}

void test_getExtensionRoot(void)
{
	assertGetExtension("/", "");
}

void test_getExtensionNull(void)
{
	assertGetExtension("", "");
}

void test_getSelfExeDir(void)
{
	string actual = Utils::getSelfExeDir();
	string selfPath = executeCommand(
	  StringUtils::sprintf("readlink /proc/%d/exe", getpid()));
	string basename =
	   executeCommand(StringUtils::sprintf("basename `%s`",
	                                       selfPath.c_str()));
	static const size_t SIZE_DIR_SEPARATOR = 1;
	size_t expectLen =
	   selfPath.size() - basename.size() - SIZE_DIR_SEPARATOR;
	string expect(selfPath, 0, expectLen);
	cppcut_assert_equal(expect, actual);
}

void test_executeOnGlibEventLoop(void)
{
	acquireDefaultContext();
	TestExecEvtLoop thread;
	thread.start();
	g_main_loop_run(thread.loop);

	cppcut_assert_equal(Utils::getThreadId(), thread.eventLoopThreadId);
	cppcut_assert_not_equal(0, thread.eventLoopThreadId);
	cppcut_assert_not_equal(thread.threadId, thread.eventLoopThreadId);
}

void test_executeOnGlibEventLoopCalledFromSameContext(void)
{
	acquireDefaultContext();
	TestExecEvtLoop task;
	task.quitLoop = false;
	task.mainThread(NULL);
	cppcut_assert_equal(Utils::getThreadId(), task.threadId);
	cppcut_assert_equal(Utils::getThreadId(), task.eventLoopThreadId);
}

void test_executeOnGlibEventLoopForFunctor(void)
{
	acquireDefaultContext();
	TestExecEvtLoop thread;
	thread.useFunctor = true;
	thread.start();
	g_main_loop_run(thread.loop);

	cppcut_assert_equal(Utils::getThreadId(), thread.eventLoopThreadId);
	cppcut_assert_not_equal(0, thread.eventLoopThreadId);
	cppcut_assert_not_equal(thread.threadId, thread.eventLoopThreadId);
}

void test_executeOnGlibEventLoopFromSameContextForFunctor(void)
{
	acquireDefaultContext();
	TestExecEvtLoop task;
	task.useFunctor = true;
	task.quitLoop = false;
	task.mainThread(NULL);
	cppcut_assert_equal(Utils::getThreadId(), task.threadId);
	cppcut_assert_equal(Utils::getThreadId(), task.eventLoopThreadId);
}

void test_executeOnGlibEventLoopAsync(void)
{
	acquireDefaultContext();

	TestExecEvtLoop thread;
	thread.start();
	thread.syncType = ASYNC;
	const size_t timeoutInMSec = 5000;
	cppcut_assert_equal(MutexLock::STAT_OK,
	                    thread.mutex.timedlock(timeoutInMSec));
	g_main_loop_run(thread.loop);

	cppcut_assert_equal(Utils::getThreadId(), thread.eventLoopThreadId);
	cppcut_assert_not_equal(0, thread.eventLoopThreadId);
	cppcut_assert_not_equal(thread.threadId, thread.eventLoopThreadId);
}

} // namespace testUtils
