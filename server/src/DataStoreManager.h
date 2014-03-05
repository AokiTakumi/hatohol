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

#include <list>
#include "DataStore.h"
#include "Utils.h"

struct DataStoreEventProc {
public:
	DataStoreEventProc(void);
	virtual ~DataStoreEventProc();
	virtual void onAdded(DataStore *dataStore);
	virtual void onRemoved(DataStore *dataStore);
};

typedef std::list<DataStoreEventProc *>  DataStoreEventProcList;
typedef DataStoreEventProcList::iterator DataStoreEventProcListIterator;

class DataStoreManager {
public:
	DataStoreManager(void);
	virtual ~DataStoreManager();
	virtual void passCommandLineArg(const CommandLineArg &cmdArg);

	/**
	 * regist an event handler.
	 *
	 * The events registered by this method are executed in series.
	 * 
	 * @param eventProc
	 * A pointer to a DataStoreEventProc instance.
	 * The owner of the instance is passed to DataStoreManager.
	 * So the caller doesn't use it or free it after calling this method.
	 */
	void registEventProc(DataStoreEventProc *eventProc);

	bool hasDataStore(uint32_t storeId);

	/**
	 * add a DataStore instance.
	 *
	 * When a DataStore is sucessfully added, its used counter is
	 * incremented.
	 *
	 * @param storeId A store ID.
	 * @param dataStore A pointer to a DataStore instance to be added.
	 *
	 * @return
	 * true if the dataStore is successfully added. Otherwise false is
	 * returned.
	 */
	bool add(uint32_t storeId, DataStore *dataStore);

	/**
	 * remove a DataStore instance.
	 *
	 * When a DataStore is sucessfully removed, its used counter is
	 * decremented.
	 *
	 * @param storeId A store ID.
	 */
	void remove(uint32_t storeId);

	/**
	 * get a vector of pointers of DataStore instance.
	 *
	 * @return
	 * A DataStoreVector instance. A used counter of each DataStore
	 * instance in it is increamented. So the caller must be call unref()
	 * for each DataStore instance.
	 */
	DataStoreVector getDataStoreVector(void);

protected:
	void closeAllStores(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DataStoreManager_h
