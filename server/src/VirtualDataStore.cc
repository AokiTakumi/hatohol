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

#include "VirtualDataStore.h"
using namespace mlpl;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
VirtualDataStore::VirtualDataStore(void)
{
}

VirtualDataStore::~VirtualDataStore(void)
{
}

void VirtualDataStore::start(void)
{
}

HatoholError VirtualDataStore::start(const MonitoringServerInfo &svInfo,
                                     const bool &autoRun)
{
	return HTERR_NOT_IMPLEMENTED;
}

void VirtualDataStore::stop(void)
{
	MLPL_INFO("VirtualDataStore: stop process: started.\n");
	closeAllStores();
}

HatoholError VirtualDataStore::stop(const ServerIdType &serverId)
{
	uint32_t storeId = serverId;
	if (!hasDataStore(storeId))
		return HTERR_NOT_FOUND_PARAMETER;
	remove(storeId);
	return HTERR_OK;
}

