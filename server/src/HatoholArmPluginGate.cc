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

#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include <string>
#include "HatoholArmPluginGate.h"
#include "CacheServiceDBClient.h"
#include "ChildProcessManager.h"
#include "StringUtils.h"

using namespace std;
using namespace mlpl;
using namespace qpid::messaging;

struct HatoholArmPluginGate::PrivateContext
{
	MonitoringServerInfo serverInfo; // we have a copy.
	ArmStatus            armStatus;
	GPid                 pid;

	PrivateContext(const MonitoringServerInfo &_serverInfo)
	: serverInfo(_serverInfo),
	  pid(0)
	{
	}
};

const string HatoholArmPluginGate::PassivePluginQuasiPath = "#PASSIVE_PLUGIN#";

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::HatoholArmPluginGate(
  const MonitoringServerInfo &serverInfo)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext(serverInfo);
}

bool HatoholArmPluginGate::start(const MonitoringSystemType &type)
{
	CacheServiceDBClient cache;
	DBClientConfig *dbConfig = cache.getConfig();
	ArmPluginInfo armPluginInfo;
	if (!dbConfig->getArmPluginInfo(armPluginInfo, type)) {
		MLPL_ERR("Failed to get ArmPluginInfo: type: %d\n", type);
		return false;
	}
	if (armPluginInfo.path == PassivePluginQuasiPath) {
		MLPL_INFO("Started: passive plugin (%d) %s\n",
		          armPluginInfo.type, armPluginInfo.path.c_str());
	} else {
		// launch a plugin process
		if (!launchPluginProcess(armPluginInfo))
			return false;
	}

	// start a thread
	m_ctx->armStatus.setRunningStatus(true);
	HatoholThreadBase::start();
	return true;
}

const ArmStatus &HatoholArmPluginGate::getArmStatus(void) const
{
	return m_ctx->armStatus;
}

gpointer HatoholArmPluginGate::mainThread(HatoholThreadArg *arg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);

	// The following lines is for checking if a build succeeds
	// and don't do a meaningful job.
	const string broker = "localhost:5672";
	const string address = "hapi";
	const string connectionOptions;

	Connection connection(broker, connectionOptions);
	connection.open();
	Session session = connection.createSession();

	Receiver receiver = session.createReceiver(address);
	Message message = receiver.fetch();
	session.acknowledge();
	connection.close();
	
	return NULL;
}

void HatoholArmPluginGate::waitExit(void)
{
	HatoholThreadBase::waitExit();
	m_ctx->armStatus.setRunningStatus(false);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGate::~HatoholArmPluginGate()
{
	if (m_ctx)
		delete m_ctx;
}

bool HatoholArmPluginGate::launchPluginProcess(
  const ArmPluginInfo &armPluginInfo)
{
	struct EventCb : public ChildProcessManager::EventCallback {

		bool succeededInCreation;

		EventCb(void)
		: succeededInCreation(false)
		{
		}

		virtual void onExecuted(const bool &succeeded,
		                        GError *gerror) // override
		{
			succeededInCreation = succeeded;
		}

		virtual void onCollected(const siginfo_t *siginfo) // override
		{
			// TODO: Implemented
			MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
		}
	} *eventCb = new EventCb();

	ChildProcessManager::CreateArg arg;
	arg.args.push_back(armPluginInfo.path);
	arg.eventCb = eventCb;
	ChildProcessManager::getInstance()->create(arg);
	if (!eventCb->succeededInCreation) {
		MLPL_ERR("Failed to execute: (%d) %s\n",
		         armPluginInfo.type, armPluginInfo.path.c_str());
		return false;
	}

	MLPL_INFO("Started: plugin (%d) %s\n",
	          armPluginInfo.type, armPluginInfo.path.c_str());
	return true;
}

string HatoholArmPluginGate::generateBrokerAddress(
  const MonitoringServerInfo &serverInfo)
{
	return StringUtils::sprintf("hatohol-arm-plugin.%"FMT_SERVER_ID,
	                            serverInfo.id);
}
