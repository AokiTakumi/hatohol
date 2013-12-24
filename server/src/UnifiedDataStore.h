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

#ifndef UnifiedDataStore_h
#define UnifiedDataStore_h

#include "ArmBase.h"
#include "DBClientHatohol.h"
#include "DBClientAction.h"
#include "DBClientConfig.h"
#include "Utils.h"
#include "Closure.h"

class UnifiedDataStore
{
public:
	UnifiedDataStore(void);
	virtual ~UnifiedDataStore(void);

	static UnifiedDataStore *getInstance(void);
	virtual void parseCommandLineArgument(CommandLineArg &cmdArg);
	virtual void start(void);
	virtual void stop(void);
	virtual void fetchItems(uint32_t targetServerId = ALL_SERVERS);

	virtual void getTriggerList(TriggerInfoList &triggerList,
				    TriggersQueryOption &option,
	                            uint64_t targetTriggerId = ALL_TRIGGERS);
	virtual HatoholError getEventList(EventInfoList &eventList,
	                                  EventsQueryOption &option);
	virtual void getItemList(ItemInfoList &itemList,
	                         ItemsQueryOption &option,
	                         uint64_t targetItemId = ALL_ITEMS);
	virtual bool getItemListAsync(ClosureBase *closure,
				      uint32_t targetServerId = ALL_SERVERS);
	virtual void getHostList(HostInfoList &hostInfoList,
	                         uint32_t targetServerId = ALL_SERVERS,
	                         uint64_t targetHostId = ALL_HOSTS);
	virtual void getActionList(ActionDefList &actionList);
	virtual void deleteActionList(const ActionIdList &actionIdList);
	virtual size_t getNumberOfTriggers
	                 (uint32_t serverId, uint64_t hostGroupId,
	                  TriggerSeverityType severity);
	virtual size_t getNumberOfGoodHosts(uint32_t serverId,
	                                    uint64_t hostGroupId);
	virtual size_t getNumberOfBadHosts(uint32_t serverId,
	                                   uint64_t hostGroupId);

	virtual bool getCopyOnDemandEnabled(void) const;
	virtual void setCopyOnDemandEnabled(bool enable);
	virtual void addAction(ActionDef &actionDef);

	/**
	 * Add events in the Hatohol DB and executes action if needed. 
	 * 
	 * @param eventList A list of EventInfo.
	 */
	virtual void addEventList(const EventInfoList &eventList);

	virtual void getUserList(UserInfoList &userList,
                                 UserQueryOption &option);
	virtual HatoholError addUser(
	  UserInfo &userInfo, const OperationPrivilege &privilege);
	virtual HatoholError updateUser(
	  UserInfo &userInfo, const OperationPrivilege &privilege);
	virtual HatoholError deleteUser(
	  UserIdType userId, const OperationPrivilege &privilege);
	virtual HatoholError getAccessInfoMap(
	  ServerAccessInfoMap &srvAccessInfoMap, AccessInfoQueryOption &option);
	virtual HatoholError addAccessInfo(
	  AccessInfo &userInfo, const OperationPrivilege &privilege);
	virtual HatoholError deleteAccessInfo(
	  AccessInfoIdType userId, const OperationPrivilege &privilege);
	virtual HatoholError addTargetServer(
	  MonitoringServerInfo &svInfo, const OperationPrivilege &privilege);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // UnifiedDataStore_h
