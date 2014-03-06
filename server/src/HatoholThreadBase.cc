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

#include <glib.h>
#include <glib-object.h>
#include <Logger.h>
#include <MutexLock.h>
#include <ReadWriteLock.h>
#include <Reaper.h>
#include <exception>
#include <stdexcept>
#include "Utils.h"
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "CacheServiceDBClient.h"
using namespace std;
using namespace mlpl;

struct HatoholThreadBase::PrivateContext {
	GThread *thread;
	ReadWriteLock rwlock;
	ExceptionCallbackInfoList exceptionCbList;
	ExitCallbackInfoList      exitCbList;
	MutexLock                 mutexForThreadStart;
	MutexLock                 mutexForThreadExit;

	// methods
	PrivateContext(void)
	: thread(NULL)
	{
		mutexForThreadStart.lock();
	}

	virtual ~PrivateContext()
	{
		if (thread) {
#ifdef GLIB_VERSION_2_32
			g_thread_unref(thread);
#else
			// no need to code for freeing
#endif // GLIB_VERSION_2_32
		}
	}

	void read_lock(void)
	{
		rwlock.readLock();
	}

	void read_unlock(void)
	{
		rwlock.unlock();
	}

	void write_lock(void)
	{
		rwlock.writeLock();
	}

	void write_unlock(void)
	{
		rwlock.unlock();
	}

	void waitThreadStarted(void)
	{
		mutexForThreadStart.lock();
	}

	void notifyThreadStarted(void)
	{
		mutexForThreadExit.lock();
		mutexForThreadStart.unlock();
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholThreadBase::HatoholThreadBase(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

HatoholThreadBase::~HatoholThreadBase()
{
	waitExit();
	if (m_ctx)
		delete m_ctx;
}

void HatoholThreadBase::start(bool autoDeleteObject, void *userData)
{
	HatoholThreadArg *arg = new HatoholThreadArg();
	arg->obj = this;
	arg->userData = userData;
	arg->autoDeleteObject = autoDeleteObject;
	GError *error = NULL;
	m_ctx->thread =
#ifdef GLIB_VERSION_2_32
	  g_thread_try_new("HatoholThread", threadStarter, arg, &error);
#else
	  g_thread_create(threadStarter, arg, TRUE, &error);
#endif // GLIB_VERSION_2_32
	if (m_ctx->thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
		g_error_free(error);
	}
	m_ctx->waitThreadStarted();
}

void HatoholThreadBase::waitExit(void)
{
	m_ctx->mutexForThreadExit.lock();
	m_ctx->mutexForThreadExit.unlock();
}

void HatoholThreadBase::addExceptionCallback(ExceptionCallbackFunc func,
                                           void *data)
{
	ExceptionCallbackInfo exceptionInfo;
	exceptionInfo.func = func;
	exceptionInfo.data = data;

	m_ctx->write_lock();
	m_ctx->exceptionCbList.push_back(exceptionInfo);
	m_ctx->write_unlock();
}

void HatoholThreadBase::addExitCallback(ExitCallbackFunc func, void *data)
{
	ExitCallbackInfo exitInfo;
	exitInfo.func = func;
	exitInfo.data = data;

	m_ctx->write_lock();
	m_ctx->exitCbList.push_back(exitInfo);
	m_ctx->write_unlock();
}

bool HatoholThreadBase::isStarted(void) const
{
	return m_ctx->thread;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HatoholThreadBase::doExceptionCallback(const exception &e)
{
	m_ctx->read_lock();
	ExceptionCallbackInfoListIterator it = m_ctx->exceptionCbList.begin();
	for (; it != m_ctx->exceptionCbList.end(); ++it) {
		ExceptionCallbackInfo &exceptionInfo = *it;
		(*exceptionInfo.func)(e, exceptionInfo.data);
	}
	m_ctx->read_unlock();
}

void HatoholThreadBase::doExitCallback(void)
{
	m_ctx->read_lock();
	ExitCallbackInfoListIterator it = m_ctx->exitCbList.begin();
	for (; it != m_ctx->exitCbList.end(); ++it) {
		ExitCallbackInfo &exitInfo = *it;
		(*exitInfo.func)(exitInfo.data);
	}
	m_ctx->read_unlock();
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
void HatoholThreadBase::threadCleanup(HatoholThreadArg *arg)
{
	arg->obj->doExitCallback();
	CacheServiceDBClient::cleanup();
	arg->obj->m_ctx->thread = NULL; // TODO: Use an atomic way.
	arg->obj->m_ctx->mutexForThreadExit.unlock();
	if (arg->autoDeleteObject)
		delete arg->obj;
	delete arg;
}

gpointer HatoholThreadBase::threadStarter(gpointer data)
{
	gpointer ret = NULL;
	HatoholThreadArg *arg = static_cast<HatoholThreadArg *>(data);

	// threadCleanup() is called when the this function returns,
	// even if it is due to an exception.
	Reaper<HatoholThreadArg> threadCleaner(arg, threadCleanup);

	arg->obj->m_ctx->notifyThreadStarted();
	try {
		ret = arg->obj->mainThread(arg);
	} catch (const HatoholException &e) {
		MLPL_ERR("Got Hatohol Exception: %s\n",
		         e.getFancyMessage().c_str());
		arg->obj->doExceptionCallback(e);
	} catch (const exception &e) {
		MLPL_ERR("Got Exception: %s\n", e.what());
		arg->obj->doExceptionCallback(e);
	} catch (...) {
		throw;
	}
	return ret;
}
