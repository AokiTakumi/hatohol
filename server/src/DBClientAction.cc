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

#include "ConfigManager.h"
#include "DBAgentFactory.h"
#include "DBClientAction.h"
#include "DBClientUtils.h"
#include "MutexLock.h"

using namespace mlpl;

const char *TABLE_NAME_ACTIONS     = "actions";
const char *TABLE_NAME_ACTION_LOGS = "action_logs";

int DBClientAction::ACTION_DB_VERSION = 1;
const char *DBClientAction::DEFAULT_DB_NAME = "action";

static const ColumnDef COLUMN_DEF_ACTIONS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"action_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"server_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"host_id",                         // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"host_group_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_id",                      // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_status",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_severity",                // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"trigger_severity_comp_type",      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"action_type",                     // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"path",                            // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"working_dir",                     // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTIONS,                // tableName
	"timeout",                         // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};
static const size_t NUM_COLUMNS_ACTIONS =
  sizeof(COLUMN_DEF_ACTIONS) / sizeof(ColumnDef);

enum {
	IDX_ACTIONS_ACTION_ID,
	IDX_ACTIONS_SERVER_ID,
	IDX_ACTIONS_HOST_ID,
	IDX_ACTIONS_HOST_GROUP_ID,
	IDX_ACTIONS_TRIGGER_ID,
	IDX_ACTIONS_TRIGGER_STATUS,
	IDX_ACTIONS_TRIGGER_SEVERITY,
	IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE,
	IDX_ACTIONS_ACTION_TYPE,
	IDX_ACTIONS_PATH,
	IDX_ACTIONS_WORKING_DIR,
	IDX_ACTIONS_TIMEOUT,
	NUM_IDX_ACTIONS,
};

static const ColumnDef COLUMN_DEF_ACTION_LOGS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"action_log_id",                   // columnName
	SQL_COLUMN_TYPE_BIGUINT,           // type
	20,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"action_id",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
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
	TABLE_NAME_ACTION_LOGS,            // tableName
	"starter_id",                      // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"queuing_time",                    // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"start_time",                      // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"end_time",                        // columnName
	SQL_COLUMN_TYPE_DATETIME,          // type
	0,                                 // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"exec_failure_code",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_ACTION_LOGS,            // tableName
	"exit_code",                       // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	true,                              // canBeNull
	SQL_KEY_MUL,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
},
};

static const size_t NUM_COLUMNS_ACTION_LOGS =
  sizeof(COLUMN_DEF_ACTION_LOGS) / sizeof(ColumnDef);

static const DBClient::DBSetupTableInfo DB_TABLE_INFO[] = {
{
	TABLE_NAME_ACTIONS,
	NUM_COLUMNS_ACTIONS,
	COLUMN_DEF_ACTIONS,
}, {
	TABLE_NAME_ACTION_LOGS,
	NUM_COLUMNS_ACTION_LOGS,
	COLUMN_DEF_ACTION_LOGS,
}
};
static const size_t NUM_TABLE_INFO =
sizeof(DB_TABLE_INFO) / sizeof(DBClient::DBSetupTableInfo);

static DBClient::DBSetupFuncArg DB_ACTION_SETUP_FUNC_ARG = {
	DBClientAction::ACTION_DB_VERSION,
	NUM_TABLE_INFO,
	DB_TABLE_INFO,
};

struct DBClientAction::PrivateContext
{
	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientAction::init(void)
{
	HATOHOL_ASSERT(NUM_COLUMNS_ACTIONS == NUM_IDX_ACTIONS,
	  "NUM_COLUMNS_ACTIONS: %zd, NUM_IDX_ACTIONS: %d",
	  NUM_COLUMNS_ACTIONS, NUM_IDX_ACTIONS);

	HATOHOL_ASSERT(NUM_COLUMNS_ACTION_LOGS == NUM_IDX_ACTION_LOGS,
	  "NUM_COLUMNS_ACTION_LOGS: %zd, NUM_IDX_ACTION_LOGS: %d",
	  NUM_COLUMNS_ACTION_LOGS, NUM_IDX_ACTION_LOGS);

	addDefaultDBInfo(
	  DB_DOMAIN_ID_ACTION, DEFAULT_DB_NAME, &DB_ACTION_SETUP_FUNC_ARG);
}

const char *DBClientAction::getTableNameActions(void)
{
	return TABLE_NAME_ACTIONS;
}

const char *DBClientAction::getTableNameActionLogs(void)
{
	return TABLE_NAME_ACTION_LOGS;
}

DBClientAction::DBClientAction(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientAction::~DBClientAction()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientAction::addAction(ActionDef &actionDef)
{
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.numColumns = NUM_COLUMNS_ACTIONS;
	arg.columnDefs = COLUMN_DEF_ACTIONS;

	row->ADD_NEW_ITEM(Int, 0); // This is automaticall set (0 is dummy)
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.serverId,
	                  getNullFlag(actionDef, ACTCOND_SERVER_ID));
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.hostId,
	                  getNullFlag(actionDef, ACTCOND_HOST_ID));
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.hostGroupId,
	                  getNullFlag(actionDef, ACTCOND_HOST_GROUP_ID));
	row->ADD_NEW_ITEM(Uint64, actionDef.condition.triggerId,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_ID));
	row->ADD_NEW_ITEM(Int, actionDef.condition.triggerStatus,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_STATUS));
	row->ADD_NEW_ITEM(Int, actionDef.condition.triggerSeverity,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	row->ADD_NEW_ITEM(Int, actionDef.condition.triggerSeverityCompType,
	                  getNullFlag(actionDef, ACTCOND_TRIGGER_SEVERITY));
	row->ADD_NEW_ITEM(Int, actionDef.type);
	row->ADD_NEW_ITEM(String, actionDef.path);
	row->ADD_NEW_ITEM(String, actionDef.workingDir);
	row->ADD_NEW_ITEM(Int, actionDef.timeout);

	arg.row = row;

	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		actionDef.id = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAction::getActionList(const EventInfo &eventInfo,
                                   ActionDefList &actionDefList)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_ACTIONS;
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_ID]);

	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_SERVER_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_HOST_GROUP_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_ID]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY]);
	arg.pushColumn(
	  COLUMN_DEF_ACTIONS[IDX_ACTIONS_TRIGGER_SEVERITY_COMP_TYPE]);

	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_ACTION_TYPE]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_PATH]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_WORKING_DIR]);
	arg.pushColumn(COLUMN_DEF_ACTIONS[IDX_ACTIONS_TIMEOUT]);

	// TODO: append where section

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// get the result
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		actionDefList.push_back(ActionDef());
		ActionDef &actionDef = actionDefList.back();

		actionDef.id = GET_INT_FROM_GRP(itemGroup, idx++);

		// conditions
		bool isNull;
		actionDef.condition.serverId =
		   GET_INT_FROM_GRP(itemGroup, idx++, &isNull);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_SERVER_ID);

		actionDef.condition.hostId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_HOST_ID);

		actionDef.condition.hostGroupId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_HOST_GROUP_ID);

		actionDef.condition.triggerId =
		   GET_UINT64_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_ID);

		actionDef.condition.triggerStatus =
		   GET_INT_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_STATUS);

		actionDef.condition.triggerSeverity =
		   GET_INT_FROM_GRP(itemGroup, idx++);
		if (!isNull)
			actionDef.condition.enable(ACTCOND_TRIGGER_SEVERITY);

		actionDef.condition.triggerSeverityCompType =
		  static_cast<ComparisonType>
		    (GET_INT_FROM_GRP(itemGroup, idx++));

		actionDef.type =
		   static_cast<ActionType>(GET_INT_FROM_GRP(itemGroup, idx++));
		actionDef.path       = GET_STRING_FROM_GRP(itemGroup, idx++);
		actionDef.workingDir = GET_STRING_FROM_GRP(itemGroup, idx++);
		actionDef.timeout    = GET_INT_FROM_GRP(itemGroup, idx++);
	}
}

uint64_t DBClientAction::logStartExecAction(const ActionDef &actionDef)
{
	VariableItemGroupPtr row;
	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	arg.numColumns = NUM_COLUMNS_ACTION_LOGS;
	arg.columnDefs = COLUMN_DEF_ACTION_LOGS;

	row->ADD_NEW_ITEM(Uint64, 0); // action_log_id (automatically set)
	row->ADD_NEW_ITEM(Int, actionDef.id);
	row->ADD_NEW_ITEM(Int, ACTLOG_STAT_STARTED);
	// TODO: set the appropriate the following starter ID.
	row->ADD_NEW_ITEM(Int, 0);  // status
	row->ADD_NEW_ITEM(Int, 0, ITEM_DATA_NULL); // queuing_time
	row->ADD_NEW_ITEM(Int, CURR_DATETIME);     // start_time
	row->ADD_NEW_ITEM(Int, 0, ITEM_DATA_NULL); // end_time
	row->ADD_NEW_ITEM(Int, ACTLOG_EXECFAIL_NONE);
	row->ADD_NEW_ITEM(Int, 0, ITEM_DATA_NULL); // exit_code

	arg.row = row;
	uint64_t logId;
	DBCLIENT_TRANSACTION_BEGIN() {
		insert(arg);
		logId = getLastInsertId();
	} DBCLIENT_TRANSACTION_END();
	return logId;
}

void DBClientAction::logEndExecAction(const ExitChildInfo &exitChildInfo)
{
	VariableItemGroupPtr row;
	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_ACTION_LOGS;
	arg.columnDefs = COLUMN_DEF_ACTION_LOGS;

	const char *actionLogIdColumnName = 
	  COLUMN_DEF_ACTION_LOGS[IDX_ACTION_LOGS_ACTION_LOG_ID].columnName;
	arg.condition = StringUtils::sprintf("%s=%"PRIu64,
	                                     actionLogIdColumnName,
	                                     exitChildInfo.logId);
	// status
	row->ADD_NEW_ITEM(Int, ACTLOG_STAT_SUCCEEDED);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_STATUS);

	// end_time
	row->ADD_NEW_ITEM(Int, CURR_DATETIME);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_END_TIME);

	// exit_code
	row->ADD_NEW_ITEM(Int, exitChildInfo.exitCode);
	arg.columnIndexes.push_back(IDX_ACTION_LOGS_EXIT_CODE);

	arg.row = row;
	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

void DBClientAction::logErrExecAction(const ActionDef &actionDef,
                                      const string &msg)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemDataNullFlagType DBClientAction::getNullFlag
  (const ActionDef &actionDef, ActionConditionEnableFlag enableFlag)
{
	if (actionDef.condition.isEnable(enableFlag))
		return ITEM_DATA_NOT_NULL;
	else
		return ITEM_DATA_NULL;
}
