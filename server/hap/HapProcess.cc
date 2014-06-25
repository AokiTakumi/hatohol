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

#include <glib.h>
#include <cstdio>
#include <AtomicValue.h>
#include "HapProcess.h"
#include "HatoholException.h"

using namespace mlpl;

const int HapProcess::DEFAULT_EXCEPTION_SLEEP_TIME_MS = 60 * 1000;

struct HapProcess::PrivateContext {
	GMainLoop *loop;
	AtomicValue<int> exceptionSleepTimeMS;
	ArmStatus        armStatus;

	PrivateContext(void)
	: loop(NULL),
	  exceptionSleepTimeMS(DEFAULT_EXCEPTION_SLEEP_TIME_MS)
	{
	}

	~PrivateContext()
	{
		if (loop)
			g_main_loop_unref(loop);
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HapProcess::HapProcess(int argc, char *argv[])
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

HapProcess::~HapProcess()
{
	if (m_ctx)
		delete m_ctx;
}

void HapProcess::initGLib(void)
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
#ifndef GLIB_VERSION_2_32
	g_thread_init(NULL);
#endif // GLIB_VERSION_2_32
}

GMainLoop *HapProcess::getGMainLoop(void)
{
	if (!m_ctx->loop)
		m_ctx->loop = g_main_loop_new(NULL, FALSE);
	HATOHOL_ASSERT(m_ctx->loop, "Failed to create GMainLoop.");
	return m_ctx->loop;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer HapProcess::mainThread(HatoholThreadArg *arg)
{
	// This class changes the virtual method name for the thread.
	// If a sub class also inherits HatoholArmPluginBase (that inherits
	// HatoholThreadBase) or its sub classes, mainThread() is ambiguous.
	return hapMainThread(arg);
}

int HapProcess::onCaughtException(const std::exception &e)
{
	return m_ctx->exceptionSleepTimeMS;
}

gpointer HapProcess::hapMainThread(HatoholThreadArg *arg)
{
	// The implementation will be done in the sub class.
	return NULL;
}

void HapProcess::setExceptionSleepTime(int sleepTimeMS)
{
	m_ctx->exceptionSleepTimeMS = sleepTimeMS;
}

ArmStatus &HapProcess::getArmStatus(void)
{
	return m_ctx->armStatus;
}
