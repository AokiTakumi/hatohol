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
#include "ChildProcessManager.h"
#include "HatoholError.h"
#include "Helpers.h"

namespace testChildProcessManager {

static void _assertCreate(ChildProcessManager::CreateArg &arg)
{
	arg.args.push_back("/bin/cat");
	assertHatoholError(HTERR_OK,
	                   ChildProcessManager::getInstance()->create(arg));
	cppcut_assert_not_equal(0, arg.pid);
}
#define assertCreate(A) cut_trace(_assertCreate(A))

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createWithEmptyParameter(void)
{
	ChildProcessManager::CreateArg arg;
	assertHatoholError(HTERR_INVALID_ARGS,
	                   ChildProcessManager::getInstance()->create(arg));
}

void test_createWithInvalidPath(void)
{
	ChildProcessManager::CreateArg arg;
	arg.args.push_back("non-exisiting-command");
	assertHatoholError(HTERR_FAILED_TO_SPAWN,
	                   ChildProcessManager::getInstance()->create(arg));
}

void test_create(void)
{
	ChildProcessManager::CreateArg arg;
	assertCreate(arg);
}

void test_collectedCb(void)
{
	struct Ctx : public ChildProcessManager::EventCallback {

		GMainLoopWithTimeout mainLoop;
		virtual void onCollected(const siginfo_t *siginfo) // override
		{
			mainLoop.quit();
		}
	} ctx;

	ChildProcessManager::CreateArg arg;
	arg.eventCb = &ctx;
	assertCreate(arg);
	cppcut_assert_equal(0, kill(arg.pid, SIGKILL));
	ctx.mainLoop.run();
}

} // namespace testChildProcessManager
