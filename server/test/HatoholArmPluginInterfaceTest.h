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

#ifndef HatoholArmPluginInterfaces_h
#define HatoholArmPluginInterfaces_h

#include <string>
#include <SimpleSemaphore.h>
#include <SmartBuffer.h>
#include <AtomicValue.h>
#include <MutexLock.h>
#include <qpid/messaging/Connection.h>
#include "HatoholArmPluginInterface.h"

struct HapiTestCtx {
	mlpl::SimpleSemaphore   sem;
	mlpl::AtomicValue<bool> connected;
	mlpl::AtomicValue<bool> quitOnConnected;
	mlpl::AtomicValue<bool> quitOnReceived;

	HapiTestCtx(void);
	std::string getReceivedMessage(void);
	void setReceivedMessage(const mlpl::SmartBuffer &smbuf);

private:
	mlpl::MutexLock lock;
	std::string     receivedMessage;
};

class HatoholArmPluginInterfaceTestBasic : public HatoholArmPluginInterface {
	struct OtherSide {
		HapiTestCtx ctx;
		HatoholArmPluginInterfaceTestBasic *obj;

		OtherSide(const std::string &queueAddr);
		virtual ~OtherSide();
	};

public:
	static const size_t TIMEOUT = 5000; // ms

	HatoholArmPluginInterfaceTestBasic(
	  HapiTestCtx &ctx,
	  const std::string &addr = "test-hatohol-arm-plugin-interface",
	  const bool workInServer = true);

	virtual void onConnected(qpid::messaging::Connection &conn) override;

	HapiTestCtx &getHapiTestCtx(void);
	void sendAsOther(const std::string &msg);
	void sendAsOther(const mlpl::SmartBuffer &smbuf);

private:
	HapiTestCtx &m_testCtx;
};

class HatoholArmPluginInterfaceTest : public HatoholArmPluginInterfaceTestBasic {
public:
	HatoholArmPluginInterfaceTest(HapiTestCtx &ctx);
	virtual void onReceived(mlpl::SmartBuffer &smbuf) override;
	mlpl::SimpleSemaphore &getRcvSem(void);

private:
	mlpl::SimpleSemaphore m_rcvSem;
};

#endif // HatoholArmPluginInterfaces_h
