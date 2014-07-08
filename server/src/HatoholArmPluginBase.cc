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

#include <cstdio>
#include <SimpleSemaphore.h>
#include <MutexLock.h>
#include <Reaper.h>
#include "HatoholArmPluginBase.h"

using namespace mlpl;

const size_t HatoholArmPluginBase::WAIT_INFINITE = 0;

struct HatoholArmPluginBase::AsyncCbData {
	HatoholArmPluginBase *obj;
	void                 *priv;

	AsyncCbData(HatoholArmPluginBase *_obj, void *_priv)
	: obj(_obj),
	  priv(_priv)
	{
	}
};

typedef void (*AsyncCallback)(HatoholArmPluginBase::AsyncCbData *data);

struct HatoholArmPluginBase::PrivateContext {
	SimpleSemaphore replyWaitSem;
	SmartBuffer     responseBuf;
	AsyncCallback   currAsyncCb;
	AsyncCbData    *currAsyncCbData;

	MutexLock       waitMutex;
	bool            inResetForInitiated;
	bool            waitInitiatedAck;
	SimpleSemaphore initiatedAckSem;

	PrivateContext(void)
	: replyWaitSem(0),
	  currAsyncCb(NULL),
	  currAsyncCbData(NULL),
	  inResetForInitiated(false),
	  waitInitiatedAck(false),
	  initiatedAckSem(0)
	{
	}
};

// ---------------------------------------------------------------------------
// GetMonitoringServerInfoAsyncArg
// ---------------------------------------------------------------------------
HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg::GetMonitoringServerInfoAsyncArg(MonitoringServerInfo *serverInfo)
: m_serverInfo(serverInfo)
{
}

void HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg::doneCb(
  const bool &succeeded)
{
}

MonitoringServerInfo &
  HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg::getMonitoringServerInfo(void)
{
	return *m_serverInfo;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginBase::HatoholArmPluginBase(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
	const char *env = getenv(ENV_NAME_QUEUE_ADDR);
	if (env)
		setQueueAddress(env);

	registerCommandHandler(
	  HAPI_CMD_REQ_TERMINATE,
	  (CommandHandler)
	    &HatoholArmPluginBase::cmdHandlerTerminate);
}

HatoholArmPluginBase::~HatoholArmPluginBase()
{
	if (m_ctx)
		delete m_ctx;
}

bool HatoholArmPluginBase::getMonitoringServerInfo(
  MonitoringServerInfo &serverInfo)
{
	sendCmdGetMonitoringServerInfo();
	waitResponseAndCheckHeader();
	return parseReplyGetMonitoringServerInfo(serverInfo);
}

void HatoholArmPluginBase::getMonitoringServerInfoAsync(
  GetMonitoringServerInfoAsyncArg *arg)
{
	HATOHOL_ASSERT(!m_ctx->currAsyncCb,
	               "Async. process is already running.");
	sendCmdGetMonitoringServerInfo();
	m_ctx->currAsyncCb = _getMonitoringServerInfoAsyncCb;
	m_ctx->currAsyncCbData = new AsyncCbData(this, arg);
}

SmartTime HatoholArmPluginBase::getTimestampOfLastTrigger(void)
{
	struct Callback : public CommandCallbacks {
		HatoholArmPluginBase *obj;
		SimpleSemaphore sem;
		timespec ts;
		bool succeeded;

		Callback(HatoholArmPluginBase *_obj)
		: CommandCallbacks(false), // autoDelete
		  obj(_obj),
		  sem(0),
		  succeeded(false)
		{
		}

		virtual void onGotReply(
		  const mlpl::SmartBuffer &replyBuf,
		  const HapiCommandHeader &cmdHeader) override
		{
			const HapiResTimestampOfLastTrigger *body =
			  obj->getResponseBody
			    <HapiResTimestampOfLastTrigger>(replyBuf);
			ts.tv_sec  = LtoN(body->timestamp);
			ts.tv_nsec = LtoN(body->nanosec);
			succeeded = true;
			sem.post();
		}

		virtual void onError(
		  const HapiResponseCode &code,
		  const HapiCommandHeader &cmdHeader) override
		{
			sem.post();
		}
	} cb(this);

	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER);
	send(cmdBuf, &cb);
	cb.sem.wait();
	if (!cb.succeeded) {
		THROW_HATOHOL_EXCEPTION(
		  "Failed to call HAPI_CMD_GET_TIMESTAMP_OF_LAST_TRIGGER\n");
	}
	return SmartTime(cb.ts);
}

EventIdType HatoholArmPluginBase::getLastEventId(void)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, HAPI_CMD_GET_LAST_EVENT_ID);
	send(cmdBuf);
	waitResponseAndCheckHeader();

	const HapiResLastEventId *body =
	  getResponseBody<HapiResLastEventId>(m_ctx->responseBuf);
	return body->lastEventId;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void HatoholArmPluginBase::onGotResponse(
  const HapiResponseHeader *header, SmartBuffer &resBuf)
{
	resBuf.handOver(m_ctx->responseBuf);
	if (m_ctx->currAsyncCb) {
		(*m_ctx->currAsyncCb)(m_ctx->currAsyncCbData);
		m_ctx->currAsyncCb = NULL;
		m_ctx->currAsyncCbData = NULL;
		return;
	}
	m_ctx->replyWaitSem.post();
}

void HatoholArmPluginBase::onInitiated(void)
{
	m_ctx->waitMutex.lock();
	if (!m_ctx->waitInitiatedAck) {
		m_ctx->waitMutex.unlock();
		return;
	}

	m_ctx->inResetForInitiated = true;
	m_ctx->replyWaitSem.post();
	m_ctx->waitMutex.unlock();
	onPreWaitInitiatedAck();
	m_ctx->initiatedAckSem.wait();
	m_ctx->inResetForInitiated = false;
	onPostWaitInitiatedAck();
}

void HatoholArmPluginBase::onReceivedTerminate(void)
{
	MLPL_INFO("Got the teminate command.\n");
	exit(EXIT_SUCCESS);
}

void HatoholArmPluginBase::onPreWaitInitiatedAck(void)
{
}

void HatoholArmPluginBase::onPostWaitInitiatedAck(void)
{
}

void HatoholArmPluginBase::enableWaitInitiatedAck(const bool &enable)
{
	Reaper<MutexLock> unlocker(&m_ctx->waitMutex, MutexLock::unlock);
	m_ctx->waitMutex.lock();
	m_ctx->waitInitiatedAck = enable;
	throwInitiatedExceptionIfNeeded();
}

void HatoholArmPluginBase::ackInitiated(void)
{
	m_ctx->initiatedAckSem.post();
}

void HatoholArmPluginBase::throwInitiatedExceptionIfNeeded(void)
{
	if (m_ctx->inResetForInitiated) {
		m_ctx->inResetForInitiated = false;
		throw HapInitiatedException();
	}
}

void HatoholArmPluginBase::sendCmdGetMonitoringServerInfo(void)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(
	  cmdBuf, HAPI_CMD_GET_MONITORING_SERVER_INFO);
	send(cmdBuf);
}

bool HatoholArmPluginBase::parseReplyGetMonitoringServerInfo(
  MonitoringServerInfo &serverInfo)
{
	const HapiResMonitoringServerInfo *svInfo =
	  getResponseBody<HapiResMonitoringServerInfo>(m_ctx->responseBuf);
	serverInfo.id   = LtoN(svInfo->serverId);
	serverInfo.type = static_cast<MonitoringSystemType>(LtoN(svInfo->type));

	const char *str;
	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->hostNameOffset, svInfo->hostNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: hostName.\n");
		return false;
	}
	serverInfo.hostName = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->ipAddressOffset, svInfo->ipAddressLength);
	if (!str) {
		MLPL_ERR("Broken packet: ipAddress.\n");
		return false;
	}
	serverInfo.ipAddress = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->nicknameOffset, svInfo->nicknameLength);
	if (!str) {
		MLPL_ERR("Broken packet: nickname.\n");
		return false;
	}
	serverInfo.nickname = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->userNameOffset, svInfo->userNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: userName.\n");
		return false;
	}
	serverInfo.userName = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->passwordOffset, svInfo->passwordLength);
	if (!str) {
		MLPL_ERR("Broken packet: password.\n");
		return false;
	}
	serverInfo.password = str;

	str = getString(m_ctx->responseBuf, svInfo,
	                svInfo->dbNameOffset, svInfo->dbNameLength);
	if (!str) {
		MLPL_ERR("Broken packet: dbName.\n");
		return false;
	}
	serverInfo.dbName = str;

	serverInfo.port               = LtoN(svInfo->port);
	serverInfo.pollingIntervalSec = LtoN(svInfo->pollingIntervalSec);
	serverInfo.retryIntervalSec   = LtoN(svInfo->retryIntervalSec);

	return true;
}

void HatoholArmPluginBase::_getMonitoringServerInfoAsyncCb(
  HatoholArmPluginBase::AsyncCbData *data)
{
	GetMonitoringServerInfoAsyncArg *arg =
	  static_cast<GetMonitoringServerInfoAsyncArg *>(data->priv);
	data->obj->getMonitoringServerInfoAsyncCb(arg);
	delete data;
}

void HatoholArmPluginBase::getMonitoringServerInfoAsyncCb(
  HatoholArmPluginBase::GetMonitoringServerInfoAsyncArg *arg)
{
	bool succeeded = parseReplyGetMonitoringServerInfo(
	                   arg->getMonitoringServerInfo());
	arg->doneCb(succeeded);
}

void HatoholArmPluginBase::waitResponseAndCheckHeader(void)
{
	HATOHOL_ASSERT(!m_ctx->currAsyncCb,
	               "Async. process is already running.");
	sleepInitiatedExceptionThrowable(WAIT_INFINITE);

	// To check the sainity of the header
	getResponseHeader(m_ctx->responseBuf);
}

bool HatoholArmPluginBase::sleepInitiatedExceptionThrowable(size_t timeoutInMS)
{
	bool wakedUp = true;
	if (timeoutInMS == WAIT_INFINITE) {
		m_ctx->replyWaitSem.wait();
	} else {
		SimpleSemaphore::Status stat =
		  m_ctx->replyWaitSem.timedWait(timeoutInMS);
		HATOHOL_ASSERT(stat != SimpleSemaphore::STAT_ERROR_UNKNOWN,
		               "timedWait() returns STAT_ERROR_UNKNOWN");
		if (stat == SimpleSemaphore::STAT_TIMEDOUT)
			wakedUp = false;
	}
	Reaper<MutexLock> unlocker(&m_ctx->waitMutex, MutexLock::unlock);
	m_ctx->waitMutex.lock();
	throwInitiatedExceptionIfNeeded();
	return wakedUp;
}

void HatoholArmPluginBase::wake(void)
{
	m_ctx->replyWaitSem.post();
}

void HatoholArmPluginBase::sendTable(
  const HapiCommandCode &code, const ItemTablePtr &tablePtr)
{
	SmartBuffer cmdBuf;
	setupCommandHeader<void>(cmdBuf, code);
	appendItemTable(cmdBuf, tablePtr);
	send(cmdBuf);
}

void HatoholArmPluginBase::sendArmInfo(const ArmInfo &armInfo)
{
	SmartBuffer cmdBuf;
	const size_t failureCommentLen = armInfo.failureComment.size();
	const size_t additionalSize = failureCommentLen + 1;
	HapiArmInfo *body =
	  setupCommandHeader<HapiArmInfo>(cmdBuf, HAPI_CMD_SEND_ARM_INFO,
	                                  additionalSize);
	body->running = NtoL(armInfo.running);
	body->stat    = NtoL(armInfo.stat);

	const timespec *ts = &armInfo.statUpdateTime.getAsTimespec();
	body->statUpdateTime = NtoL(ts->tv_sec);
	body->statUpdateTimeNanosec = NtoL(ts->tv_nsec);

	ts = &armInfo.lastSuccessTime.getAsTimespec();
	body->lastSuccessTime = NtoL(ts->tv_sec);
	body->lastSuccessTimeNanosec = NtoL(ts->tv_nsec);

	ts = &armInfo.lastFailureTime.getAsTimespec();
	body->lastFailureTime = NtoL(ts->tv_sec);
	body->lastFailureTimeNanosec = NtoL(ts->tv_nsec);

	body->numUpdate  = NtoL(armInfo.numUpdate);
	body->numFailure = NtoL(armInfo.numFailure);

	char *buf = reinterpret_cast<char *>(body) + sizeof(HapiArmInfo);
	buf = putString(buf, body, armInfo.failureComment,
	                &body->failureCommentOffset,
	                &body->failureCommentLength);
	send(cmdBuf);
}

void HatoholArmPluginBase::cmdHandlerTerminate(const HapiCommandHeader *header)
{
	onReceivedTerminate();
}
