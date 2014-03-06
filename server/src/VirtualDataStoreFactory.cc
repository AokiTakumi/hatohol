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

#include "VirtualDataStoreFactory.h"
#include "VirtualDataStoreFake.h"
#include "VirtualDataStoreZabbix.h"
#include "VirtualDataStoreNagios.h"

using namespace std;

struct VirtualDataStoreFactory::PrivateContext
{
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
VirtualDataStore *VirtualDataStoreFactory::create(
  const MonitoringSystemType &monSysType)
{
	if (monSysType == MONITORING_SYSTEM_FAKE)
		return new VirtualDataStoreFake();
	if (monSysType == MONITORING_SYSTEM_ZABBIX)
		return new VirtualDataStoreZabbix();
	if (monSysType == MONITORING_SYSTEM_NAGIOS)
		return new VirtualDataStoreNagios();
	return NULL;
}
