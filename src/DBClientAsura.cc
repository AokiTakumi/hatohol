/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <memory>

#include <MutexLock.h>
using namespace mlpl;

#include "DBAgentFactory.h"
#include "DBClientAsura.h"
#include "ConfigManager.h"
#include "DBClientUtils.h"

static const char *TABLE_NAME_TRIGGERS = "triggers";
static const char *TABLE_NAME_EVENTS   = "events";

int DBClientAsura::ASURA_DB_VERSION = 1;

static const ColumnDef COLUMN_DEF_TRIGGERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"status",                          // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"severity",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"last_change_time_sec",            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"last_change_time_ns",             // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"hostname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_TRIGGERS,               // tableName
	"brief",                           // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};

static const size_t NUM_COLUMNS_TRIGGERS =
  sizeof(COLUMN_DEF_TRIGGERS) / sizeof(ColumnDef);

enum {
	IDX_TRIGGERS_SERVER_ID,
	IDX_TRIGGERS_ID,
	IDX_TRIGGERS_STATUS,
	IDX_TRIGGERS_SEVERITY,
	IDX_TRIGGERS_LAST_CHANGE_TIME_SEC,
	IDX_TRIGGERS_LAST_CHANGE_TIME_NS,
	IDX_TRIGGERS_HOST_ID,
	IDX_TRIGGERS_HOSTNAME,
	IDX_TRIGGERS_BRIEF,
	NUM_IDX_TRIGGERS,
};

static const ColumnDef COLUMN_DEF_EVENTS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"time_sec",                        // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"time_ns",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"event_value",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_EVENTS,                 // tableName
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const size_t NUM_COLUMNS_EVENTS =
  sizeof(COLUMN_DEF_EVENTS) / sizeof(ColumnDef);

enum {
	IDX_EVENTS_SERVER_ID,
	IDX_EVENTS_ID,
	IDX_EVENTS_TIME_SEC,
	IDX_EVENTS_TIME_NS,
	IDX_EVENTS_EVENT_TYPE,
	IDX_EVENTS_TRIGGER_ID,
	NUM_IDX_EVENTS,
};

struct DBClientAsura::PrivateContext
{
	static MutexLock mutex;
	static bool   initialized;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}

	static void lock(void)
	{
		mutex.lock();
	}

	static void unlock(void)
	{
		mutex.unlock();
	}
};
MutexLock DBClientAsura::PrivateContext::mutex;
bool   DBClientAsura::PrivateContext::initialized = false;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientAsura::init(void)
{
	ASURA_ASSERT(NUM_COLUMNS_TRIGGERS == NUM_IDX_TRIGGERS,
	  "NUM_COLUMNS_TRIGGERS: %zd, NUM_IDX_TRIGGERS: %zd",
	  NUM_COLUMNS_TRIGGERS, NUM_IDX_TRIGGERS);

	ASURA_ASSERT(NUM_COLUMNS_EVENTS == NUM_IDX_EVENTS,
	  "NUM_COLUMNS_EVENTS: %zd, NUM_IDX_EVENTS: %zd",
	  NUM_COLUMNS_EVENTS, NUM_IDX_EVENTS);
}

void DBClientAsura::reset(void)
{
	resetDBInitializedFlags();
}

DBClientAsura::DBClientAsura(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	m_ctx->lock();
	if (!m_ctx->initialized) {
		// The setup function: dbSetupFunc() is called from
		// the creation of DBAgent instance below.
		prepareSetupFunction();
	}
	m_ctx->unlock();
	setDBAgent(DBAgentFactory::create(DB_DOMAIN_ID_ASURA));
}

DBClientAsura::~DBClientAsura()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientAsura::addTriggerInfo(TriggerInfo *triggerInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addTriggerInfoBare(*triggerInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAsura::getTriggerInfoList(TriggerInfoList &triggerInfoList)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_TRIGGERS;
	arg.columnDefs = COLUMN_DEF_TRIGGERS;
	arg.columnIndexes.push_back(IDX_TRIGGERS_SERVER_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_STATUS);
	arg.columnIndexes.push_back(IDX_TRIGGERS_SEVERITY);
	arg.columnIndexes.push_back(IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);
	arg.columnIndexes.push_back(IDX_TRIGGERS_LAST_CHANGE_TIME_NS);
	arg.columnIndexes.push_back(IDX_TRIGGERS_HOST_ID);
	arg.columnIndexes.push_back(IDX_TRIGGERS_HOSTNAME);
	arg.columnIndexes.push_back(IDX_TRIGGERS_BRIEF);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		triggerInfoList.push_back(TriggerInfo());
		TriggerInfo &trigInfo = triggerInfoList.back();

		trigInfo.serverId  = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.id        = GET_UINT64_FROM_GRP(itemGroup, idx++);
		int status         = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.status    = static_cast<TriggerStatusType>(status);
		int severity       = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.severity  = static_cast<TriggerSeverityType>(severity);
		trigInfo.lastChangeTime.tv_sec = 
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.lastChangeTime.tv_nsec =
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.hostId    = GET_STRING_FROM_GRP(itemGroup, idx++);
		trigInfo.hostName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		trigInfo.brief     = GET_STRING_FROM_GRP(itemGroup, idx++);
	}
}

void DBClientAsura::setTriggerInfoList(const TriggerInfoList &triggerInfoList,
                                       uint32_t serverId)
{
	DBAgentDeleteArg deleteArg;
	deleteArg.tableName = TABLE_NAME_TRIGGERS;
	deleteArg.condition =
	  StringUtils::sprintf("%s=%u",
	    COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID].columnName,
	    serverId);

	TriggerInfoListConstIterator it = triggerInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != triggerInfoList.end(); ++it)
			addTriggerInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAsura::addEventInfo(EventInfo *eventInfo)
{
	DBCLIENT_TRANSACTION_BEGIN() {
		addEventInfoBare(*eventInfo);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAsura::addEventInfoList(const EventInfoList &eventInfoList)
{
	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		for (; it != eventInfoList.end(); ++it)
			addEventInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAsura::getEventInfoList(EventInfoList &eventInfoList)
{
	const ColumnDef &eventsServerId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID];
	const ColumnDef &eventsId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_ID];
	const ColumnDef &eventsTimeSec =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_SEC];
	const ColumnDef &eventsTimeNs =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TIME_NS];
	const ColumnDef &eventsEventValue = 
	  COLUMN_DEF_EVENTS[IDX_EVENTS_EVENT_TYPE];
	const ColumnDef &eventsTriggerId =
	  COLUMN_DEF_EVENTS[IDX_EVENTS_TRIGGER_ID];

	const ColumnDef &triggersServerId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SERVER_ID];
	const ColumnDef &triggersTriggerId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_ID];
	const ColumnDef &triggersStatus =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_STATUS];
	const ColumnDef &triggersSeverity =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_SEVERITY];
	const ColumnDef &triggersLastChangeTimeSec = 
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_SEC];
	const ColumnDef &triggersLastChangeTimeNs = 
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_LAST_CHANGE_TIME_NS];
	const ColumnDef &triggersHostId =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOST_ID];
	const ColumnDef &triggersHostName =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_HOSTNAME];
	const ColumnDef &triggersBrief =
	  COLUMN_DEF_TRIGGERS[IDX_TRIGGERS_BRIEF];

	DBAgentSelectExArg arg;

	// Tables
	const static char *VAR_EVENTS = "e";
	const static char *VAR_TRIGGERS = "t";
	arg.tableName = StringUtils::sprintf(
	  " %s %s inner join %s %s on %s.%s=%s.%s",
	  TABLE_NAME_EVENTS, VAR_EVENTS,
	  TABLE_NAME_TRIGGERS, VAR_TRIGGERS,
	  VAR_EVENTS, eventsTriggerId.columnName,
	  VAR_TRIGGERS, triggersTriggerId.columnName);

	// Columns
	arg.pushColumn(eventsServerId,   VAR_EVENTS);
	arg.pushColumn(eventsId,         VAR_EVENTS);
	arg.pushColumn(eventsTimeSec,    VAR_EVENTS);
	arg.pushColumn(eventsTimeNs,     VAR_EVENTS);
	arg.pushColumn(eventsEventValue, VAR_EVENTS);
	arg.pushColumn(eventsTriggerId,  VAR_EVENTS);

	arg.pushColumn(triggersStatus,   VAR_TRIGGERS);
	arg.pushColumn(triggersSeverity, VAR_TRIGGERS);
	arg.pushColumn(triggersLastChangeTimeSec, VAR_TRIGGERS);
	arg.pushColumn(triggersLastChangeTimeNs,  VAR_TRIGGERS);
	arg.pushColumn(triggersHostId,   VAR_TRIGGERS);
	arg.pushColumn(triggersHostName, VAR_TRIGGERS);
	arg.pushColumn(triggersBrief,    VAR_TRIGGERS);

	// Condition
	arg.condition = StringUtils::sprintf(
	  "%s.%s=%s.%s", 
	  VAR_EVENTS, eventsServerId.columnName,
	  VAR_TRIGGERS, triggersServerId.columnName);

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		eventInfoList.push_back(EventInfo());
		EventInfo &eventInfo = eventInfoList.back();

		eventInfo.serverId   = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.id         = GET_UINT64_FROM_GRP(itemGroup, idx++);
		eventInfo.time.tv_sec  = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.time.tv_nsec = GET_INT_FROM_GRP(itemGroup, idx++);
		int type             = GET_INT_FROM_GRP(itemGroup, idx++);
		eventInfo.type       = static_cast<EventType>(type);
		eventInfo.triggerId  = GET_UINT64_FROM_GRP(itemGroup, idx++);

		TriggerInfo &trigInfo = eventInfo.triggerInfo;
		trigInfo.serverId  = eventInfo.serverId;
		trigInfo.id        = eventInfo.triggerId;
		int status         = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.status    = static_cast<TriggerStatusType>(status);
		int severity       = GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.severity  = static_cast<TriggerSeverityType>(severity);
		trigInfo.lastChangeTime.tv_sec = 
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.lastChangeTime.tv_nsec =
		  GET_INT_FROM_GRP(itemGroup, idx++);
		trigInfo.hostId    = GET_STRING_FROM_GRP(itemGroup, idx++);
		trigInfo.hostName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		trigInfo.brief     = GET_STRING_FROM_GRP(itemGroup, idx++);
	}
}

void DBClientAsura::setEventInfoList(const EventInfoList &eventInfoList,
                                     uint32_t serverId)
{
	DBAgentDeleteArg deleteArg;
	deleteArg.tableName = TABLE_NAME_EVENTS;
	deleteArg.condition =
	  StringUtils::sprintf("%s=%u",
	    COLUMN_DEF_EVENTS[IDX_EVENTS_SERVER_ID].columnName,
	    serverId);

	EventInfoListConstIterator it = eventInfoList.begin();
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(deleteArg);
		for (; it != eventInfoList.end(); ++it)
			addEventInfoBare(*it);
	} DBCLIENT_TRANSACTION_END();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientAsura::resetDBInitializedFlags(void)
{
	PrivateContext::initialized = false;
}

void DBClientAsura::prepareSetupFunction(void)
{
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_TRIGGERS,
		NUM_COLUMNS_TRIGGERS,
		COLUMN_DEF_TRIGGERS,
	}, {
		TABLE_NAME_EVENTS,
		NUM_COLUMNS_EVENTS,
		COLUMN_DEF_EVENTS,
	}
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		ASURA_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	DBAgent::addSetupFunction(DB_DOMAIN_ID_ASURA,
	                          dbSetupFunc, (void *)&DB_SETUP_FUNC_ARG);
}

void DBClientAsura::addTriggerInfoBare(const TriggerInfo &triggerInfo)
{
	string condition = StringUtils::sprintf("id=%"PRIu64, triggerInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_TRIGGERS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_TRIGGERS;
		arg.numColumns = NUM_COLUMNS_TRIGGERS;
		arg.columnDefs = COLUMN_DEF_TRIGGERS;
		row->ADD_NEW_ITEM(Int, triggerInfo.serverId);
		row->ADD_NEW_ITEM(Uint64, triggerInfo.id);
		row->ADD_NEW_ITEM(Int, triggerInfo.status);
		row->ADD_NEW_ITEM(Int, triggerInfo.severity),
		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_sec); 
		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_nsec); 
		row->ADD_NEW_ITEM(String, triggerInfo.hostId);
		row->ADD_NEW_ITEM(String, triggerInfo.hostName);
		row->ADD_NEW_ITEM(String, triggerInfo.brief);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_TRIGGERS;
		arg.columnDefs = COLUMN_DEF_TRIGGERS;

		row->ADD_NEW_ITEM(Int, triggerInfo.serverId);
		arg.columnIndexes.push_back(IDX_TRIGGERS_SERVER_ID);

		row->ADD_NEW_ITEM(Int, triggerInfo.status);
		arg.columnIndexes.push_back(IDX_TRIGGERS_STATUS);

		row->ADD_NEW_ITEM(Int, triggerInfo.severity);
		arg.columnIndexes.push_back(IDX_TRIGGERS_SEVERITY);

		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_sec); 
		arg.columnIndexes.push_back
		  (IDX_TRIGGERS_LAST_CHANGE_TIME_SEC);

		row->ADD_NEW_ITEM(Int, triggerInfo.lastChangeTime.tv_nsec); 
		arg.columnIndexes.push_back
		  (IDX_TRIGGERS_LAST_CHANGE_TIME_NS);

		row->ADD_NEW_ITEM(String, triggerInfo.hostId);
		arg.columnIndexes.push_back(IDX_TRIGGERS_HOST_ID);

		row->ADD_NEW_ITEM(String, triggerInfo.hostName);
		arg.columnIndexes.push_back(IDX_TRIGGERS_HOSTNAME);

		row->ADD_NEW_ITEM(String, triggerInfo.brief);
		arg.columnIndexes.push_back(IDX_TRIGGERS_BRIEF);
		arg.row = row;
		update(arg);
	}
}

void DBClientAsura::addEventInfoBare(const EventInfo &eventInfo)
{
	string condition = StringUtils::sprintf("id=%"PRIu64, eventInfo.id);
	VariableItemGroupPtr row;
	if (!isRecordExisting(TABLE_NAME_EVENTS, condition)) {
		DBAgentInsertArg arg;
		arg.tableName = TABLE_NAME_EVENTS;
		arg.numColumns = NUM_COLUMNS_EVENTS;
		arg.columnDefs = COLUMN_DEF_EVENTS;
		row->ADD_NEW_ITEM(Int, eventInfo.serverId),
		row->ADD_NEW_ITEM(Uint64, eventInfo.id);
		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_sec); 
		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_nsec); 
		row->ADD_NEW_ITEM(Int, eventInfo.type);
		row->ADD_NEW_ITEM(Uint64, eventInfo.triggerId);
		arg.row = row;
		insert(arg);
	} else {
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_EVENTS;
		arg.columnDefs = COLUMN_DEF_EVENTS;

		row->ADD_NEW_ITEM(Int, eventInfo.serverId);
		arg.columnIndexes.push_back(IDX_EVENTS_SERVER_ID);

		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_sec); 
		arg.columnIndexes.push_back(IDX_EVENTS_TIME_SEC);

		row->ADD_NEW_ITEM(Int, eventInfo.time.tv_nsec); 
		arg.columnIndexes.push_back(IDX_EVENTS_TIME_NS);

		row->ADD_NEW_ITEM(Int, eventInfo.type);
		arg.columnIndexes.push_back(IDX_EVENTS_EVENT_TYPE);

		row->ADD_NEW_ITEM(Uint64, eventInfo.triggerId);
		arg.columnIndexes.push_back(IDX_EVENTS_TRIGGER_ID);
		arg.row = row;

		update(arg);
	}
}
