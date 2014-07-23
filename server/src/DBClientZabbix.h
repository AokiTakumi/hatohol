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

#ifndef DBClientZabbix_h
#define DBClientZabbix_h

#include "DBClient.h"
#include "DBClientHatohol.h"

class DBClientZabbix : public DBClient {
public:
	typedef void (DBClientZabbix::*AbsentItemPicker)
	               (std::vector<uint64_t> &absentHostIdVector,
	                const std::vector<uint64_t> &hostIdVector);
	typedef void (DBClientZabbix::*TableSaver)(ItemTablePtr tablePtr);

	static const int ZABBIX_DB_VERSION;
	static const uint64_t EVENT_ID_NOT_FOUND;
	static const int TRIGGER_CHANGE_TIME_NOT_FOUND;

	static void init(void);
	static DBDomainId getDBDomainId(const ServerIdType zabbixServerId);

	/**
	 * create a DBClientZabbix instance.
	 *
	 * Different from other DBClient sub classes, instances of this class 
	 * have to be created with the factory function.
	 *
	 * @param A server ID of the zabbix server.
	 * @return A new DBclientZabbix instance.
	 */
	static DBClientZabbix *create(const ServerIdType zabbixServerId);
	virtual ~DBClientZabbix();

	void addTriggersRaw2_0(ItemTablePtr tablePtr);
	void addFunctionsRaw2_0(ItemTablePtr tablePtr);
	void addItemsRaw2_0(ItemTablePtr tablePtr);
	void addHostsRaw2_0(ItemTablePtr tablePtr);
	void addEventsRaw2_0(ItemTablePtr tablePtr);
	void addApplicationsRaw2_0(ItemTablePtr tablePtr);
	void addGroupsRaw2_0(ItemTablePtr tablePtr);
	void addHostsGroupsRaw2_0(ItemTablePtr tablePtr);

	void getTriggersAsHatoholFormat(TriggerInfoList &triggerInfoList);
	void getEventsAsHatoholFormat(EventInfoList &eventInfoList);

	/**
	 * get the last (biggest) event ID in the database.
	 * @return
	 * The last event ID. If the database has no event data,
	 * EVENT_ID_NOT_FOUND is returned.
	 */
	uint64_t getLastEventId(void);

	/**
	 * get the last trigger change time in the database.
	 * @return
	 * The last change time. If the database has no event data,
	 * TRIGGER_CHANGE_TIME_NOT_FOUND is returned.
	 */
	int getTriggerLastChange(void);

	std::string getApplicationName(uint64_t applicationId);

	void pickupAbsentHostIds(std::vector<uint64_t> &absentHostIdVector,
	                         const std::vector<uint64_t> &hostIdVector);
	void pickupAbsentApplcationIds(
	  std::vector<uint64_t> &absentAppIdVector,
	  const std::vector<uint64_t> &appIdVector);

protected:
	static std::string getDBName(const ServerIdType zabbixServerId);
	static void updateDBIfNeeded(DBAgent *dbAgent, int oldVer, void *data);

	DBClientZabbix(const ServerIdType zabbixServerId);
	void addItems(
	  ItemTablePtr tablePtr, const DBAgent::TableProfile &tableProfile,
	  int updateCheckIndex);
	void makeSelectExArgForTriggerAsHatoholFormat(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientZabbix_h
