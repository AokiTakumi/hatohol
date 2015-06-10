/*
 * Copyright (C) 2015 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <StringUtils.h>
#include <JSONParser.h>
#include <HatoholArmPluginInterfaceHAPI2.h>
#include "HatoholArmPluginGateHAPI2.h"
#include "ThreadLocalDBCache.h"
#include "UnifiedDataStore.h"
#include "ArmFake.h"

using namespace std;
using namespace mlpl;

struct HatoholArmPluginGateHAPI2::Impl
{
	// We have a copy. The access to the object is MT-safe.
	const MonitoringServerInfo m_serverInfo;
	HatoholArmPluginGateHAPI2 &m_hapi2;
	ArmPluginInfo m_pluginInfo;
	ArmFake m_armFake;
	ArmStatus m_armStatus;
	set<string> m_supportedProcedureNameSet;
	HostInfoCache hostInfoCache;
	map<string, Closure0 *> m_fetchClosureMap;
	map<string, Closure1<HistoryInfoVect> *> m_fetchHistoryClosureMap;

	Impl(const MonitoringServerInfo &_serverInfo,
	     HatoholArmPluginGateHAPI2 &hapi2)
	: m_serverInfo(_serverInfo),
	  m_hapi2(hapi2),
	  m_armFake(m_serverInfo),
	  m_armStatus(),
	  hostInfoCache(&_serverInfo.id)
	{
		ArmPluginInfo::initialize(m_pluginInfo);
		ThreadLocalDBCache cache;
		DBTablesConfig &dbConfig = cache.getConfig();
		const ServerIdType &serverId = m_serverInfo.id;
		if (!dbConfig.getArmPluginInfo(m_pluginInfo, serverId)) {
			MLPL_ERR("Failed to get ArmPluginInfo: serverId: %d\n",
				 serverId);
			return;
		}
		m_hapi2.setArmPluginInfo(m_pluginInfo);
	}

	~Impl()
	{
		for (auto pair: m_fetchClosureMap) {
			Closure0 *closure = pair.second;
			if (closure)
				(*closure)();
			delete closure;
		}
		for (auto pair: m_fetchHistoryClosureMap) {
			Closure1<HistoryInfoVect> *closure = pair.second;
			HistoryInfoVect historyInfoVect;
			if (closure)
				(*closure)(historyInfoVect);
			delete closure;
		}
		m_armStatus.setRunningStatus(false);
	}

	void start(void)
	{
		m_armStatus.setRunningStatus(true);
		callExchangeProfile();
	}

	void callExchangeProfile(void) {
		JSONBuilder builder;
		builder.startObject();
		builder.add("jsonrpc", "2.0");
		builder.add("name", PACKAGE_NAME);
		builder.add("method", HAPI2_EXCHANGE_PROFILE);
		builder.startObject("params");
		builder.startArray("procedures");
		for (auto procedureDef : m_hapi2.getProcedureDefList()) {
			if (procedureDef.type == PROCEDURE_HAP)
				continue;
			builder.add(procedureDef.name);
		}
		builder.endArray(); // procedures
		builder.endObject(); // params
		std::mt19937 random = m_hapi2.getRandomEngine();
		int64_t id = random();
		builder.add("id", id);
		builder.endObject();
		// TODO: add callback
		m_hapi2.send(builder.generate(), id, NULL);
	}

	void queueFetchCallback(const string &fetchId, Closure0 *closure)
	{
		if (fetchId.empty())
			return;
		m_fetchClosureMap[fetchId] = closure;
	}

	void runFetchCallback(const string &fetchId)
	{
		if (fetchId.empty())
			return;

		auto it = m_fetchClosureMap.find(fetchId);
		if (it != m_fetchClosureMap.end()) {
			Closure0 *closure = it->second;
			if (closure)
				(*closure)();
			m_fetchClosureMap.erase(it);
			delete closure;
		}
	}

	void queueFetchHistoryCallback(const string &fetchId,
				       Closure1<HistoryInfoVect> *closure)
	{
		if (fetchId.empty())
			return;
		m_fetchHistoryClosureMap[fetchId] = closure;
	}

	void runFetchHistoryCallback(const string &fetchId,
				     const HistoryInfoVect &historyInfoVect)
	{
		if (fetchId.empty())
			return;

		auto it = m_fetchHistoryClosureMap.find(fetchId);
		if (it != m_fetchHistoryClosureMap.end()) {
			Closure1<HistoryInfoVect> *closure = it->second;
			(*closure)(historyInfoVect);
			m_fetchHistoryClosureMap.erase(it);
			delete closure;
		}
	}

	bool hasProcedure(const string procedureName)
	{
		return m_supportedProcedureNameSet.find(procedureName) !=
			m_supportedProcedureNameSet.end();
	}

	struct FetchProcedureCallback : public ProcedureCallback {
		HatoholArmPluginGateHAPI2::Impl &m_impl;
		const string m_fetchId;
		const string m_methodName;
		FetchProcedureCallback(HatoholArmPluginGateHAPI2::Impl &impl,
				       const string &fetchId,
				       const string &methodName)
		: m_impl(impl), m_fetchId(fetchId), m_methodName(methodName)
		{
		}

		bool isSucceeded(JSONParser &parser)
		{
			if (parser.isMember("error"))
				return false;

			string result;
			parser.read("result", result);
			return result == "SUCCESS";
		}

		virtual void onGotResponse(JSONParser &parser) override
		{
			if (isSucceeded(parser)) {
				// The callback function will be executed on
				// put* or update* procedures.
				return;
			}

			// The fetch* procedure has not been accepted by the
			// plugin. The closure for it should be expired
			// immediately.
			if (m_methodName == HAPI2_FETCH_HISTORY) {
				HistoryInfoVect historyInfoVect;
				m_impl.runFetchHistoryCallback(m_fetchId,
							       historyInfoVect);
			} else {
				m_impl.runFetchCallback(m_fetchId);
			}

			// TODO: output error log
		}
	};
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::HatoholArmPluginGateHAPI2(
  const MonitoringServerInfo &serverInfo, const bool &autoStart)
: HatoholArmPluginInterfaceHAPI2(MODE_SERVER),
  m_impl(new Impl(serverInfo, *this))
{
	registerProcedureHandler(
	  HAPI2_EXCHANGE_PROFILE,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile);
	registerProcedureHandler(
	  HAPI2_MONITORING_SERVER_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo);
	registerProcedureHandler(
	  HAPI2_LAST_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerLastInfo);
	registerProcedureHandler(
	  HAPI2_PUT_ITEMS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutItems);
	registerProcedureHandler(
	  HAPI2_PUT_HISTORY,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerPutHistory);
	registerProcedureHandler(
	  HAPI2_UPDATE_HOSTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHosts);
	registerProcedureHandler(
	  HAPI2_UPDATE_HOST_GROUPS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroups);
	registerProcedureHandler(
	  HAPI2_UPDATE_HOST_GROUP_MEMEBRSHIP,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroupMembership);
	registerProcedureHandler(
	  HAPI2_UPDATE_TRIGGERS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateTriggers);
	registerProcedureHandler(
	  HAPI2_UPDATE_EVENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateEvents);
	registerProcedureHandler(
	  HAPI2_UPDATE_HOST_PARENTS,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostParents);
	registerProcedureHandler(
	  HAPI2_UPDATE_ARM_INFO,
	  (ProcedureHandler)
	    &HatoholArmPluginGateHAPI2::procedureHandlerUpdateArmInfo);

	if (autoStart)
		start();
}

void HatoholArmPluginGateHAPI2::start(void)
{
	HatoholArmPluginInterfaceHAPI2::start();
	m_impl->start();
}

bool HatoholArmPluginGateHAPI2::parseTimeStamp(
  const string &timeStampString, timespec &timeStamp)
{
	timeStamp.tv_sec = 0;
	timeStamp.tv_nsec = 0;

	StringVector list;
	StringUtils::split(list, timeStampString, '.', false);

	if (list.empty() || list.size() > 2)
		goto ERR;
	struct tm tm;
	if (!strptime(list[0].c_str(), "%4Y%2m%2d%2H%2M%2S", &tm))
		goto ERR;
	timeStamp.tv_sec = timegm(&tm); // as UTC

	if (list.size() == 1)
		return true;

	if (list[1].size() > 9)
		goto ERR;
	for (size_t i = 0; i < list[1].size(); i++) {
		unsigned int ch = list[1][i];
		if (ch < '0' || ch > '9')
			goto ERR;
	}
	for (size_t i = list[1].size(); i < 9; i++)
		list[1] += '0';
	timeStamp.tv_nsec = atol(list[1].c_str());

	return true;

 ERR:
	MLPL_ERR("Invalid timestamp format: %s\n",
		 timeStampString.c_str());
	return false;
}

static bool parseTimeStamp(
  JSONParser &parser, const string &member, timespec &timeStamp)
{
	string timeStampString;
	parser.read(member, timeStampString);
	return HatoholArmPluginGateHAPI2::parseTimeStamp(timeStampString,
							 timeStamp);
}

bool HatoholArmPluginGateHAPI2::isFetchItemsSupported(void)
{
	return m_impl->hasProcedure(HAPI2_FETCH_ITEMS);
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchItem(Closure0 *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_ITEMS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_ITEMS);
	builder.startObject("params");
	if (false) { // TODO: Pass requested hostIds
		builder.startArray("hostIds");
		builder.endArray();
	}
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_ITEMS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
}

string buildTimeStamp(const time_t &timeValue)
{
	struct tm tm;;
	gmtime_r(&timeValue, &tm);
	char buf[32];
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tm);
	string timeString;
	timeString = buf;
	return timeString;
}

void HatoholArmPluginGateHAPI2::startOnDemandFetchHistory(
  const ItemInfo &itemInfo, const time_t &beginTime, const time_t &endTime,
  Closure1<HistoryInfoVect> *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_HISTORY)) {
		HistoryInfoVect historyInfoVect;
		if (closure)
			(*closure)(historyInfoVect);
		delete closure;
		return;
	}

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_HISTORY);
	builder.startObject("params");
	builder.add("hostId", itemInfo.hostIdInServer);
	builder.add("itemId", itemInfo.id);
	builder.add("beginTime", buildTimeStamp(beginTime));
	builder.add("endTime", buildTimeStamp(endTime));
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchHistoryCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_HISTORY);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
}

bool HatoholArmPluginGateHAPI2::startOnDemandFetchTrigger(Closure0 *closure)
{
	if (!m_impl->hasProcedure(HAPI2_FETCH_TRIGGERS))
		return false;

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("method", HAPI2_FETCH_TRIGGERS);
	builder.startObject("params");
	if (false) { // TODO: Pass requested hostIds
		builder.startArray("hostIds");
		builder.endArray();
	}
	std::mt19937 random = getRandomEngine();
	int64_t fetchId = random(), id = random();
	string fetchIdString = StringUtils::sprintf("%" PRId64, fetchId);
	m_impl->queueFetchCallback(fetchIdString, closure);
	builder.add("fetchId", fetchIdString);
	builder.endObject();
	builder.add("id", id);
	builder.endObject();
	ProcedureCallback *callback =
	  new Impl::FetchProcedureCallback(*m_impl, fetchIdString,
					   HAPI2_FETCH_TRIGGERS);
	ProcedureCallbackPtr callbackPtr(callback, false);
	send(builder.generate(), id, callbackPtr);
	return true;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
HatoholArmPluginGateHAPI2::~HatoholArmPluginGateHAPI2()
{
}

const MonitoringServerInfo &
HatoholArmPluginGateHAPI2::getMonitoringServerInfo(void) const
{
	return m_impl->m_armFake.getServerInfo();
}

const ArmStatus &HatoholArmPluginGateHAPI2::getArmStatus(void) const
{
	return m_impl->m_armStatus;
}

static bool parseExchangeProfileParams(
  JSONParser &parser, set<string> &supportedProcedureNameSet)
{
	parser.startObject("procedures");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		string supportedProcedureName;
		parser.read(i, supportedProcedureName);

		supportedProcedureNameSet.insert(supportedProcedureName);
	}
	parser.endObject(); // procedures
	return true;
}

static bool hapProcessLogger(JSONParser &parser)
{
	string hapProcessName;
	parser.read("name", hapProcessName);
	MLPL_INFO("HAP Process connecting done. Connected HAP process name: \"%s\"\n",
		  hapProcessName.c_str());
	return true;
}

string HatoholArmPluginGateHAPI2::procedureHandlerExchangeProfile(
  JSONParser &parser)
{
	m_impl->m_supportedProcedureNameSet.clear();
	parser.startObject("params");
	bool succeeded = parseExchangeProfileParams(
			   parser, m_impl->m_supportedProcedureNameSet);
	hapProcessLogger(parser);
	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.startObject("result");
	builder.add("name", PACKAGE_NAME);
	builder.startArray("procedures");
	for (auto procedureDef : getProcedureDefList()) {
		if (procedureDef.type == PROCEDURE_HAP)
			continue;
		builder.add(procedureDef.name);
	}
	builder.endArray(); // procedures
	builder.endObject(); // result
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

string HatoholArmPluginGateHAPI2::procedureHandlerMonitoringServerInfo(
  JSONParser &parser)
{
	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.startObject("result");
	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	builder.add("serverId", serverInfo.id);
	builder.add("url", serverInfo.baseURL);
	builder.add("type", m_impl->m_pluginInfo.uuid);
	builder.add("nickName", serverInfo.nickname);
	builder.add("userName", serverInfo.userName);
	builder.add("password", serverInfo.password);
	builder.add("pollingIntervalSec", serverInfo.pollingIntervalSec);
	builder.add("retryIntervalSec", serverInfo.retryIntervalSec);
	builder.add("extendedInfo", serverInfo.extendedInfo);
	builder.endObject(); // result
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseLastInfoParams(JSONParser &parser, LastInfoType &lastInfoType)
{
	string type;
	parser.read("params", type);
	if (type == "host")
		lastInfoType = LAST_INFO_HOST;
	else if (type == "hostGroup")
		lastInfoType = LAST_INFO_HOST_GROUP;
	else if (type == "hostGroupMembership")
		lastInfoType = LAST_INFO_HOST_GROUP_MEMBERSHIP;
	else if (type == "trigger")
		lastInfoType = LAST_INFO_TRIGGER;
	else if (type == "event")
		lastInfoType = LAST_INFO_EVENT;
	else if (type == "hostParent")
		lastInfoType = LAST_INFO_HOST_PARENT;
	else
		lastInfoType = LAST_INFO_ALL; // TODO: should return an error

	return true;
}

string HatoholArmPluginGateHAPI2::procedureHandlerLastInfo(JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
	LastInfoQueryOption option(USER_ID_SYSTEM);
	LastInfoType lastInfoType;
	bool succeeded = parseLastInfoParams(parser, lastInfoType);

	option.setLastInfoType(lastInfoType);
	option.setTargetServerId(m_impl->m_serverInfo.id);
	LastInfoDefList lastInfoList;
	dbLastInfo.getLastInfoList(lastInfoList, option);
	string lastInfoValue = "";
	for (auto lastInfo : lastInfoList) {
		lastInfoValue = lastInfo.value;
		break;
	}

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", lastInfoValue);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseItemParams(JSONParser &parser, ItemInfoList &itemInfoList,
			    const MonitoringServerInfo &serverInfo)
{
	parser.startObject("items");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse item contents.\n");
			return false;
		}

		ItemInfo itemInfo;
		itemInfo.serverId = serverInfo.id;
		parser.read("itemId", itemInfo.id);
		parser.read("hostId", itemInfo.hostIdInServer);
		parser.read("brief", itemInfo.brief);
		parseTimeStamp(parser, "lastValueTime", itemInfo.lastValueTime);
		parser.read("lastValue", itemInfo.lastValue);
		parser.read("itemGroupName", itemInfo.itemGroupName);
		parser.read("unit", itemInfo.unit);
		parser.endElement();

		itemInfoList.push_back(itemInfo);
	}
	parser.endObject(); // items
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutItems(JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ItemInfoList itemList;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseItemParams(parser, itemList, serverInfo);
	dataStore->addItemList(itemList);
	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		m_impl->runFetchCallback(fetchId);
	}
	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", "");
	setResponseId(parser, builder);
	builder.endObject();

	return builder.generate();
}

static bool parseHistoryParams(JSONParser &parser, HistoryInfoVect &historyInfoVect,
			       const MonitoringServerInfo &serverInfo)
{
	ItemIdType itemId = "";
	parser.read("itemId", itemId);
	parser.startObject("histories");
	size_t num = parser.countElements();

	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse histories contents.\n");
			return false;
		}

		HistoryInfo historyInfo;
		historyInfo.itemId = itemId;
		historyInfo.serverId = serverInfo.id;
		parser.read("value", historyInfo.value);
		parseTimeStamp(parser, "time", historyInfo.clock);
		parser.endElement();

		historyInfoVect.push_back(historyInfo);
	}
	parser.endObject(); // histories
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerPutHistory(
  JSONParser &parser)
{
	HistoryInfoVect historyInfoVect;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHistoryParams(parser, historyInfoVect,
					    serverInfo);
	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		m_impl->runFetchHistoryCallback(fetchId, historyInfoVect);
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", "");
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostsParams(JSONParser &parser, ServerHostDefVect &hostInfoVect,
			     const MonitoringServerInfo &serverInfo)
{
	parser.startObject("hosts");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		ServerHostDef hostInfo;
		hostInfo.serverId = serverInfo.id;
		int64_t hostId;
		parser.read("hostId", hostId);
		hostInfo.hostIdInServer = hostId;
		parser.read("hostName", hostInfo.name);
		parser.endElement();

		hostInfoVect.push_back(hostInfo);
	}
	parser.endObject(); // hosts
	return true;
};

static bool parseUpdateType(JSONParser &parser, string &updateType)
{
	parser.read("updateType", updateType);
	if (updateType == "ALL") {
		return true;
	} else if (updateType == "UPDATED") {
		return false;
	} else {
		MLPL_WARN("Invalid update type: %s\n", updateType.c_str());
		return false;
	}
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHosts(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	ServerHostDefVect hostInfoVect;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHostsParams(parser, hostInfoVect, serverInfo);
	string updateType;
	bool checkInvalidHosts = parseUpdateType(parser, updateType);
	// TODO: reflect error in response
	if (checkInvalidHosts) {
		dataStore->syncHosts(hostInfoVect, serverInfo.id,
				     m_impl->hostInfoCache);
	} else {
		dataStore->upsertHosts(hostInfoVect);
	}

	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST);
	}

	string result = succeeded ? "SUCCESS" : "FAILURE";

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostGroupsParams(JSONParser &parser,
				  HostgroupVect &hostgroupVect,
				  const MonitoringServerInfo &serverInfo)
{
	parser.startObject("hostGroups");
	size_t num = parser.countElements();
	for (size_t j = 0; j < num; j++) {
		if (!parser.startElement(j)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		Hostgroup hostgroup;
		hostgroup.serverId = serverInfo.id;
		parser.read("groupId", hostgroup.idInServer);
		parser.read("groupName", hostgroup.name);
		parser.endElement();

		hostgroupVect.push_back(hostgroup);
	}
	parser.endObject(); // hostsGroups
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroups(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupVect hostgroupVect;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHostGroupsParams(parser, hostgroupVect, serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostGroups = parseUpdateType(parser, updateType);
	// TODO: reflect error in response
	if (checkInvalidHostGroups) {
		dataStore->syncHostgroups(hostgroupVect, serverInfo.id);
	} else {
		dataStore->upsertHostgroups(hostgroupVect);
	}
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST_GROUP);
	}
	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}


static bool parseHostGroupMembershipParams(
  JSONParser &parser,
  HostgroupMemberVect &hostgroupMemberVect,
  const MonitoringServerInfo &serverInfo)
{
	parser.startObject("hostGroupsMembership");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse hosts contents.\n");
			return false;
		}

		HostgroupMember hostgroupMember;
		hostgroupMember.serverId = serverInfo.id;
		string hostId;
		parser.read("hostId", hostId);
		hostgroupMember.hostId = StringUtils::toUint64(hostId);
		parser.startObject("groupIds");
		size_t groupIdNum = parser.countElements();
		for (size_t j = 0; j < groupIdNum; j++) {
			parser.read(j, hostgroupMember.hostgroupIdInServer);
			hostgroupMemberVect.push_back(hostgroupMember);
		}
		parser.endObject(); // groupIds
		parser.endElement();
	}
	parser.endObject(); // hostGroupsMembership
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostGroupMembership(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	HostgroupMemberVect hostgroupMembershipVect;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseHostGroupMembershipParams(parser, hostgroupMembershipVect,
							serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostGroupMembership =
	  parseUpdateType(parser, updateType);
	// TODO: reflect error in response
	if (checkInvalidHostGroupMembership) {
		dataStore->syncHostgroupMembers(hostgroupMembershipVect, serverInfo.id);
	} else {
		dataStore->upsertHostgroupMembers(hostgroupMembershipVect);
	}
	string lastInfo;
	if (!parser.read("lastInfo", lastInfo) ) {
		upsertLastInfo(lastInfo, LAST_INFO_HOST_GROUP_MEMBERSHIP);
	}
	dataStore->upsertHostgroupMembers(hostgroupMembershipVect);

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static void parseTriggerStatus(JSONParser &parser, TriggerStatusType &status)
{
	string statusString;
	parser.read("status", statusString);
	if (statusString == "OK") {
		status = TRIGGER_STATUS_OK;
	} else if (statusString == "NG") {
		status = TRIGGER_STATUS_PROBLEM;
	} else if (statusString == "UNKNOWN") {
		status = TRIGGER_STATUS_UNKNOWN;
	} else {
		MLPL_WARN("Unknown trigger status: %s\n", statusString.c_str());
		status = TRIGGER_STATUS_UNKNOWN;
	}
}

static void parseTriggerSeverity(JSONParser &parser,
				 TriggerSeverityType &severity)
{
	string severityString;
	parser.read("severity", severityString);
	if (severityString == "ALL") {
		severity = TRIGGER_SEVERITY_ALL;
	} else if (severityString == "UNKNOWN") {
		severity = TRIGGER_SEVERITY_UNKNOWN;
	} else if (severityString == "INFO") {
		severity = TRIGGER_SEVERITY_INFO;
	} else if (severityString == "WARNING") {
		severity = TRIGGER_SEVERITY_WARNING;
	} else if (severityString == "ERROR") {
		severity = TRIGGER_SEVERITY_ERROR;
	} else if (severityString == "CRITICAL") {
		severity = TRIGGER_SEVERITY_CRITICAL;
	} else if (severityString == "EMERGENCY") {
		severity = TRIGGER_SEVERITY_EMERGENCY;
	} else {
		MLPL_WARN("Unknown trigger severity: %s\n",
			  severityString.c_str());
		severity = TRIGGER_SEVERITY_UNKNOWN;
	}
}

static bool parseTriggersParams(JSONParser &parser, TriggerInfoList &triggerInfoList,
				const MonitoringServerInfo &serverInfo)
{
	parser.startObject("triggers");
	size_t num = parser.countElements();

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse event contents.\n");
			return false;
		}

		TriggerInfo triggerInfo;
		triggerInfo.serverId = serverInfo.id;
		parseTriggerStatus(parser, triggerInfo.status);
		parseTriggerSeverity(parser, triggerInfo.severity);
		parseTimeStamp(parser, "lastChangeTime", triggerInfo.lastChangeTime);
		parser.read("hostId",       triggerInfo.hostIdInServer);
		parser.read("hostName",     triggerInfo.hostName);
		parser.read("brief",        triggerInfo.brief);
		parser.read("extendedInfo", triggerInfo.extendedInfo);
		parser.endElement();

		triggerInfoList.push_back(triggerInfo);
	}
	parser.endObject(); // triggers
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateTriggers(
  JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesMonitoring &dbMonitoring = cache.getMonitoring();
	TriggerInfoList triggerInfoList;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseTriggersParams(parser, triggerInfoList, serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidTriggers = parseUpdateType(parser, updateType);
	// TODO: reflect error in response
	if (checkInvalidTriggers) {
		dbMonitoring.syncTriggers(triggerInfoList, serverInfo.id);
	} else {
		dbMonitoring.addTriggerInfoList(triggerInfoList);
	}

	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_TRIGGER);
	}

	if (parser.isMember("mayMoreFlag")) {
		bool mayMoreFlag;
		parser.read("mayMoreFlag", mayMoreFlag);
		// TODO: What should we do?
	}
	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		m_impl->runFetchCallback(fetchId);
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static void parseEventType(JSONParser &parser, EventInfo &eventInfo)
{
	string eventType;
	parser.read("type", eventType);

	if (eventType == "GOOD") {
		eventInfo.type = EVENT_TYPE_GOOD;
	} else if (eventType == "BAD") {
		eventInfo.type = EVENT_TYPE_BAD;
	} else if (eventType == "UNKNOWN") {
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	} else if (eventType == "NOTIFICATION") {
		eventInfo.type = EVENT_TYPE_NOTIFICATION;
	} else {
		MLPL_WARN("Invalid event type: %s\n", eventType.c_str());
		eventInfo.type = EVENT_TYPE_UNKNOWN;
	}
};

static bool parseEventsParams(JSONParser &parser, EventInfoList &eventInfoList,
			      const MonitoringServerInfo &serverInfo)
{
	parser.startObject("events");
	size_t num = parser.countElements();
	constexpr const size_t numLimit = 1000;
	static const TriggerIdType DO_NOT_ASSOCIATE_TRIGGER_ID =
		SPECIAL_TRIGGER_ID_PREFIX "DO_NOT_ASSOCIATE_TRIGGER";

	if (num > numLimit) {
		MLPL_ERR("Event Object is too large. "
			 "Object size limit(%zd) exceeded.\n", numLimit);
		return false;
	}

	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse event contents.\n");
			return false;
		}

		EventInfo eventInfo;
		eventInfo.serverId = serverInfo.id;
		parser.read("eventId",      eventInfo.id);
		parseTimeStamp(parser, "time", eventInfo.time);
		parseEventType(parser, eventInfo);
		TriggerIdType triggerId = DO_NOT_ASSOCIATE_TRIGGER_ID;
		if (!parser.read("triggerId", triggerId)) {
			eventInfo.triggerId = triggerId;
		}
		parseTriggerStatus(parser, eventInfo.status);
		parseTriggerSeverity(parser, eventInfo.severity);
		parser.read("hostId",       eventInfo.hostIdInServer);
		parser.read("hostName",     eventInfo.hostName);
		parser.read("brief",        eventInfo.brief);
		parser.read("extendedInfo", eventInfo.extendedInfo);
		parser.endElement();

		eventInfoList.push_back(eventInfo);
	}
	parser.endObject(); // events
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateEvents(
  JSONParser &parser)
{
	UnifiedDataStore *dataStore = UnifiedDataStore::getInstance();
	EventInfoList eventInfoList;
	parser.startObject("params");

	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	bool succeeded = parseEventsParams(parser, eventInfoList, serverInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";
	dataStore->addEventList(eventInfoList);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_EVENT);
	}

	if (parser.isMember("fetchId")) {
		string fetchId;
		parser.read("fetchId", fetchId);
		// TODO: callback
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseHostParentsParams(
  JSONParser &parser,
  VMInfoVect &vmInfoVect)
{
	parser.startObject("hostParents");
	size_t num = parser.countElements();
	for (size_t i = 0; i < num; i++) {
		if (!parser.startElement(i)) {
			MLPL_ERR("Failed to parse vm_info contents.\n");
			return false;
		}

		VMInfo vmInfo;
		string childHostId, parentHostId;
		parser.read("childHostId", childHostId);
		vmInfo.hostId = StringUtils::toUint64(childHostId);
		parser.read("parentHostId", parentHostId);
		vmInfo.hypervisorHostId = StringUtils::toUint64(parentHostId);
		parser.endElement();

		vmInfoVect.push_back(vmInfo);
	}
	parser.endObject(); // hostsParents
	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateHostParents(
  JSONParser &parser)
{
	ThreadLocalDBCache cache;
	DBTablesHost &dbHost = cache.getHost();
	VMInfoVect vmInfoVect;
	parser.startObject("params");

	bool succeeded = parseHostParentsParams(parser, vmInfoVect);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	string updateType;
	bool checkInvalidHostParents = parseUpdateType(parser, updateType);
	// TODO: implement validation for hostParents
	for (auto vmInfo : vmInfoVect)
		dbHost.upsertVMInfo(vmInfo);
	string lastInfoValue;
	if (!parser.read("lastInfo", lastInfoValue) ) {
		upsertLastInfo(lastInfoValue, LAST_INFO_HOST_PARENT);
	}

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

static bool parseArmInfoParams(JSONParser &parser, ArmInfo &armInfo)
{
	string status;
	parser.read("lastStatus", status);
	if (status == "INIT") {
		armInfo.stat = ARM_WORK_STAT_INIT;
	} else if (status == "OK") {
		armInfo.stat = ARM_WORK_STAT_OK;
	} else if (status == "NG") {
		armInfo.stat = ARM_WORK_STAT_FAILURE;
	} else {
		MLPL_WARN("Invalid status: %s\n", status.c_str());
		armInfo.stat = ARM_WORK_STAT_FAILURE;
	}
	parser.read("failureReason", armInfo.failureComment);
	timespec successTime, failureTime;
	parseTimeStamp(parser, "lastSuccessTime", successTime);
	parseTimeStamp(parser, "lastFailureTime", failureTime);
	SmartTime lastSuccessTime(successTime);
	SmartTime lastFailureTime(failureTime);
	armInfo.statUpdateTime = lastSuccessTime;
	armInfo.lastFailureTime = lastFailureTime;

	int64_t numSuccess, numFailure;
	parser.read("numSuccess", numSuccess);
	parser.read("numFailure", numFailure);
	armInfo.numUpdate = (size_t)numSuccess;
	armInfo.numFailure = (size_t)numFailure;

	return true;
};

string HatoholArmPluginGateHAPI2::procedureHandlerUpdateArmInfo(
  JSONParser &parser)
{
	ArmStatus status;
	ArmInfo armInfo;
	parser.startObject("params");

	bool succeeded = parseArmInfoParams(parser, armInfo);
	status.setArmInfo(armInfo);
	string result = succeeded ? "SUCCESS" : "FAILURE";

	parser.endObject(); // params

	JSONBuilder builder;
	builder.startObject();
	builder.add("jsonrpc", "2.0");
	builder.add("result", result);
	setResponseId(parser, builder);
	builder.endObject();
	return builder.generate();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

void HatoholArmPluginGateHAPI2::upsertLastInfo(string lastInfoValue, LastInfoType type)
{
	ThreadLocalDBCache cache;
	DBTablesLastInfo &dbLastInfo = cache.getLastInfo();
	OperationPrivilege privilege(USER_ID_SYSTEM);
	const MonitoringServerInfo &serverInfo = m_impl->m_serverInfo;
	LastInfoDef lastInfo;
	lastInfo.id = AUTO_INCREMENT_VALUE;
	lastInfo.dataType = type;
	lastInfo.value = lastInfoValue;
	lastInfo.serverId = serverInfo.id;
	dbLastInfo.upsertLastInfo(lastInfo, privilege);
}
