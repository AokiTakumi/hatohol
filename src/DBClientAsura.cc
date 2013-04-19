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

#include "DBAgentFactory.h"
#include "DBClientAsura.h"
#include "ConfigManager.h"

static const char *TABLE_NAME_SERVERS = "servers";

static const ColumnDef COLUMN_DEF_SERVERS[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"id",                              // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_PRI,                       // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"type",                            // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
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
	TABLE_NAME_SERVERS,                // tableName
	"ip_address",                      // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"nickname",                        // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}
};
static const size_t NUM_COLUMNS_SERVERS =
  sizeof(COLUMN_DEF_SERVERS) / sizeof(ColumnDef);

enum {
	IDX_SERVERS_ID,
	IDX_SERVERS_TYPE,
	IDX_SERVERS_HOSTNAME,
	IDX_SERVERS_IP_ADDRESS,
	IDX_SERVERS_NICKNAME,
	NUM_IDX_SERVERS,
};

struct DBClientAsura::PrivateContext
{
	static GMutex mutex;
	static bool   initialized;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}

	static void lock(void)
	{
		g_mutex_lock(&mutex);
	}

	static void unlock(void)
	{
		g_mutex_unlock(&mutex);
	}
};
GMutex DBClientAsura::PrivateContext::mutex;
bool   DBClientAsura::PrivateContext::initialized = false;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
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
	setDBAgent(DBAgentFactory::create(DB_DOMAIN_ID_OFFSET_ASURA));
}

DBClientAsura::~DBClientAsura()
{
	if (m_ctx)
		delete m_ctx;
}

void DBClientAsura::addTargetServer(MonitoringServerInfo *monitoringServerInfo)
{
	string condition = StringUtils::sprintf("id=%u",
	                                        monitoringServerInfo->id);
	DBCLIENT_TRANSACTION_BEGIN() {
		if (!isRecordExisting(TABLE_NAME_SERVERS, condition)) {
			DBAgentInsertArg arg;
			arg.tableName = TABLE_NAME_SERVERS;
			arg.numColumns = NUM_COLUMNS_SERVERS;
			arg.columnDefs = COLUMN_DEF_SERVERS;
			arg.row->ADD_NEW_ITEM(Int, monitoringServerInfo->id);
			arg.row->ADD_NEW_ITEM(Int, monitoringServerInfo->type);
			arg.row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->hostName);
			arg.row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->ipAddress);
			arg.row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->nickname);
			insert(arg);
		} else {
			DBAgentUpdateArg arg;
			arg.tableName = TABLE_NAME_SERVERS;
			arg.columnDefs = COLUMN_DEF_SERVERS;

			arg.row->ADD_NEW_ITEM(Int, monitoringServerInfo->type);
			arg.columnIndexes.push_back(IDX_SERVERS_TYPE);

			arg.row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->hostName);
			arg.columnIndexes.push_back(IDX_SERVERS_HOSTNAME);

			arg.row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->ipAddress);
			arg.columnIndexes.push_back(IDX_SERVERS_IP_ADDRESS);

			arg.row->ADD_NEW_ITEM(String,
			                      monitoringServerInfo->nickname);
			arg.columnIndexes.push_back(IDX_SERVERS_NICKNAME);
			update(arg);
		}
	} DBCLIENT_TRANSACTION_END();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
void DBClientAsura::prepareSetupFunction(void)
{
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_SERVERS,
		NUM_COLUMNS_SERVERS,
		COLUMN_DEF_SERVERS,
	}
	};
	static const size_t NUM_TABLE_INFO =
	sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
	};

	DBAgent::addSetupFunction(DB_DOMAIN_ID_OFFSET_ASURA,
	                          dbSetupFunc, (void *)&DB_SETUP_FUNC_ARG);
}
