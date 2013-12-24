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

#ifndef DataStoreManager_h
#define DataStoreManager_h

#include <map>
#include <string>
using namespace std;

#include "DataStore.h"
#include "DataStoreEventProc.h"
#include "Utils.h"

class DataStoreManager {
	// Currently multi-thread unsafe.
public:
	DataStoreManager(void);
	virtual ~DataStoreManager();
	virtual void passCommandLineArg(const CommandLineArg &cmdArg);
	void registEventProc(DataStoreEventProc *proc);

	// Elements regisgtered in this function should be freed by
	// this class (i.e. owner is changed).
	bool add(uint32_t storeId, DataStore *dataStore);

	DataStoreVector &getDataStoreVector(void);

protected:
	void closeAllStores(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DataStoreManager_h
