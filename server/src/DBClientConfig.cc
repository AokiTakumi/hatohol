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

#include <memory>

#include <MutexLock.h>
using namespace std;

#include "DBAgentFactory.h"
#include "DBClientConfig.h"
#include "DBClientUtils.h"
#include "CacheServiceDBClient.h"
#include "HatoholError.h"
#include "Params.h"

static const char *TABLE_NAME_SYSTEM  = "system";
static const char *TABLE_NAME_SERVERS = "servers";

int DBClientConfig::CONFIG_DB_VERSION = 8;
const char *DBClientConfig::DEFAULT_DB_NAME = "hatohol";
const char *DBClientConfig::DEFAULT_USER_NAME = "hatohol";
const char *DBClientConfig::DEFAULT_PASSWORD  = "hatohol";

static const ColumnDef COLUMN_DEF_SYSTEM[] = {
{
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"database_dir",                    // columnName
	SQL_COLUMN_TYPE_VARCHAR,           // type
	255,                               // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"",                                // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"enable_face_mysql",               // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	NULL,                              // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"face_rest_port",                  // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"0",                               // defaultValue
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SYSTEM,                 // tableName
	"enable_copy_on_demand",           // columnName
	SQL_COLUMN_TYPE_INT,               // type
	11,                                // columnLength
	0,                                 // decFracLength
	false,                             // canBeNull
	SQL_KEY_NONE,                      // keyType
	0,                                 // flags
	"1",                               // defaultValue
},
};
static const size_t NUM_COLUMNS_SYSTEM =
  sizeof(COLUMN_DEF_SYSTEM) / sizeof(ColumnDef);

enum {
	IDX_SYSTEM_DATABASE_DIR,
	IDX_SYSTEM_ENABLE_FACE_MYSQL,
	IDX_SYSTEM_FACE_REST_PORT,
	IDX_SYSTEM_ENABLE_COPY_ON_DEMAND,
	NUM_IDX_SYSTEM,
};

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
	SQL_COLUMN_FLAG_AUTO_INC,          // flags
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
}, {
	ITEM_ID_NOT_SET,                   // itemId
	TABLE_NAME_SERVERS,                // tableName
	"port",                            // columnName
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
	"polling_interval_sec",            // columnName
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
	"retry_interval_sec",              // columnName
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
	"user_name",                       // columnName
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
	"password",                        // columnName
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
	"db_name",                         // columnName
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
	IDX_SERVERS_PORT,
	IDX_SERVERS_POLLING_INTERVAL_SEC,
	IDX_SERVERS_RETRY_INTERVAL_SEC,
	IDX_SERVERS_USER_NAME,
	IDX_SERVERS_PASSWORD,
	IDX_SERVERS_DB_NAME,
	NUM_IDX_SERVERS,
};

const char *MonitoringServerInfo::getHostAddress(void) const
{
	if (!ipAddress.empty())
		return ipAddress.c_str();
	if (!hostName.empty())
		return hostName.c_str();
	return NULL;
}

struct DBClientConfig::PrivateContext
{
	static DBConnectInfo connInfo;

	PrivateContext(void)
	{
	}

	virtual ~PrivateContext()
	{
	}
};
DBConnectInfo DBClientConfig::PrivateContext::connInfo;

static bool updateDB(DBAgent *dbAgent, int oldVer, void *data)
{
	if (oldVer <= 5) {
		DBAgentAddColumnsArg addColumnsArg;
		addColumnsArg.tableName = TABLE_NAME_SYSTEM;
		addColumnsArg.columnDefs = COLUMN_DEF_SYSTEM;
		addColumnsArg.columnIndexes.push_back(
		  IDX_SYSTEM_ENABLE_COPY_ON_DEMAND);
		dbAgent->addColumns(addColumnsArg);
	}
	if (oldVer <= 7) {
		// enable copy-on-demand by default
		DBAgentUpdateArg arg;
		arg.tableName = TABLE_NAME_SYSTEM;
		arg.columnDefs = COLUMN_DEF_SYSTEM;
		arg.columnIndexes.push_back(IDX_SYSTEM_ENABLE_COPY_ON_DEMAND);
		VariableItemGroupPtr row;
		row->ADD_NEW_ITEM(Int, 1);
		arg.row = row;
		dbAgent->update(arg);
	}
	return true;
}

// ---------------------------------------------------------------------------
// ServerQueryOption
// ---------------------------------------------------------------------------
struct ServerQueryOption::PrivateContext {
	uint32_t targetServerId;

	PrivateContext(void)
	: targetServerId(ALL_SERVERS)
	{
	}
};

ServerQueryOption::ServerQueryOption(UserIdType userId)
: DataQueryOption(userId), m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ServerQueryOption::~ServerQueryOption()
{
	if (m_ctx)
		delete m_ctx;
}

static string serverIdCondition(uint32_t serverId)
{
	const char *columnName = COLUMN_DEF_SERVERS[IDX_SERVERS_ID].columnName;
	return StringUtils::sprintf("%s=%"PRIu32, columnName, serverId);
}

bool ServerQueryOption::hasPrivilegeCondition(string &condition) const
{
	UserIdType userId = getUserId();

	if (userId == USER_ID_SYSTEM || has(OPPRVLG_GET_ALL_SERVER)) {
		if (m_ctx->targetServerId != ALL_SERVERS)
			condition = serverIdCondition(m_ctx->targetServerId);
		return true;
	}

	if (userId == INVALID_USER_ID) {
		MLPL_DBG("INVALID_USER_ID\n");
		condition = DBClientHatohol::getAlwaysFalseCondition();
		return true;
	}

	return false;
}

void ServerQueryOption::setTargetServerId(uint32_t serverId)
{
	m_ctx->targetServerId = serverId;
}

string ServerQueryOption::getCondition(void) const
{
	string condition;
	uint32_t targetId = m_ctx->targetServerId;

	if (hasPrivilegeCondition(condition))
		return condition;

	// check allowed servers
	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	ServerHostGrpSetMap srvHostGrpSetMap;
	dbUser->getServerHostGrpSetMap(srvHostGrpSetMap, getUserId());

	size_t numServers = srvHostGrpSetMap.size();
	if (numServers == 0) {
		MLPL_DBG("No allowed server\n");
		return DBClientHatohol::getAlwaysFalseCondition();
	}

	if (targetId != ALL_SERVERS &&
	    srvHostGrpSetMap.find(targetId) != srvHostGrpSetMap.end())
	{
		return serverIdCondition(targetId);
	}

	numServers = 0;
	ServerHostGrpSetMapConstIterator it = srvHostGrpSetMap.begin();
	for (; it != srvHostGrpSetMap.end(); ++it) {
		const uint32_t serverId = it->first;

		if (serverId == ALL_SERVERS)
			return "";

		if (!condition.empty())
			condition += " OR ";
		condition += serverIdCondition(serverId);
		++numServers;
	}

	if (numServers == 1)
		return condition;
	return StringUtils::sprintf("(%s)", condition.c_str());
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void DBClientConfig::init(const CommandLineArg &cmdArg)
{
	HATOHOL_ASSERT(NUM_COLUMNS_SYSTEM == NUM_IDX_SYSTEM,
	  "NUM_COLUMNS_SYSTEM: %zd, NUM_IDX_SYSTEM: %d",
	  NUM_COLUMNS_SYSTEM, NUM_IDX_SYSTEM);

	HATOHOL_ASSERT(NUM_COLUMNS_SERVERS == NUM_IDX_SERVERS,
	  "NUM_COLUMNS_SERVERS: %zd, NUM_IDX_SERVERS: %d",
	  NUM_COLUMNS_SERVERS, NUM_IDX_SERVERS);

	//
	// set database info
	//
	static const DBSetupTableInfo DB_TABLE_INFO[] = {
	{
		TABLE_NAME_SYSTEM,
		NUM_COLUMNS_SYSTEM,
		COLUMN_DEF_SYSTEM,
		tableInitializerSystem,
	}, {
		TABLE_NAME_SERVERS,
		NUM_COLUMNS_SERVERS,
		COLUMN_DEF_SERVERS,
	}
	};
	static const size_t NUM_TABLE_INFO =
	  sizeof(DB_TABLE_INFO) / sizeof(DBSetupTableInfo);

	static const DBSetupFuncArg DB_SETUP_FUNC_ARG = {
		CONFIG_DB_VERSION,
		NUM_TABLE_INFO,
		DB_TABLE_INFO,
		&updateDB,
	};

	registerSetupInfo(
	  DB_DOMAIN_ID_CONFIG, DEFAULT_DB_NAME, &DB_SETUP_FUNC_ARG);

	if (!parseCommandLineArgument(cmdArg))
		THROW_HATOHOL_EXCEPTION("Failed to parse argument.");
}

void DBClientConfig::reset(void)
{
	DBConnectInfo &connInfo = PrivateContext::connInfo;
	connInfo.user = DEFAULT_USER_NAME;
	connInfo.password = DEFAULT_PASSWORD;
	connInfo.dbName = DEFAULT_DB_NAME;

	string portStr;
	if (connInfo.port == 0)
		portStr = "(default)";
	else
		portStr = StringUtils::sprintf("%zd", connInfo.port);
	bool usePassword = !connInfo.password.empty();
	MLPL_INFO("Configuration DB Server: %s, port: %s, "
	          "DB: %s, User: %s, use password: %s\n",
	          connInfo.host.c_str(), portStr.c_str(),
	          connInfo.dbName.c_str(),
	          connInfo.user.c_str(), usePassword ? "yes" : "no");
	setConnectInfo(DB_DOMAIN_ID_CONFIG, connInfo);
}

DBClientConfig::DBClientConfig(void)
: DBClient(DB_DOMAIN_ID_CONFIG),
  m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClientConfig::~DBClientConfig()
{
	if (m_ctx)
		delete m_ctx;
}

string DBClientConfig::getDatabaseDir(void)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_DATABASE_DIR);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	return ItemDataUtils::getString((*grpList.begin())->getItemAt(0));
}

void DBClientConfig::setDatabaseDir(const string &dir)
{
	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_DATABASE_DIR);
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(String, dir);
	arg.row = row;
	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientConfig::isFaceMySQLEnabled(void)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_ENABLE_FACE_MYSQL);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	return ItemDataUtils::getInt((*grpList.begin())->getItemAt(0));
}

int  DBClientConfig::getFaceRestPort(void)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_FACE_REST_PORT);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	return ItemDataUtils::getInt((*grpList.begin())->getItemAt(0));
}

void DBClientConfig::setFaceRestPort(int port)
{
	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_FACE_REST_PORT);
	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Int, port);
	arg.row = row;
	DBCLIENT_TRANSACTION_BEGIN() {
		update(arg);
	} DBCLIENT_TRANSACTION_END();
}

bool DBClientConfig::isCopyOnDemandEnabled(void)
{
	DBAgentSelectArg arg;
	arg.tableName = TABLE_NAME_SYSTEM;
	arg.columnDefs = COLUMN_DEF_SYSTEM;
	arg.columnIndexes.push_back(IDX_SYSTEM_ENABLE_COPY_ON_DEMAND);
	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	HATOHOL_ASSERT(!grpList.empty(), "Obtained Table: empty");
	return ItemDataUtils::getInt((*grpList.begin())->getItemAt(0));
}

HatoholError DBClientConfig::addOrUpdateTargetServer(
  MonitoringServerInfo *monitoringServerInfo,
  const OperationPrivilege &privilege)
{
	HatoholError err = HTERR_UNINITIALIZED;
	string condition = StringUtils::sprintf("id=%u",
	                                        monitoringServerInfo->id);
	DBCLIENT_TRANSACTION_BEGIN() {
		if (!isRecordExisting(TABLE_NAME_SERVERS, condition)) {
			err = _addTargetServer(monitoringServerInfo, privilege);
		} else {
			err = _updateTargetServer(monitoringServerInfo,
			                          privilege, condition);
		}
	} DBCLIENT_TRANSACTION_END();

	return err;
}

HatoholError DBClientConfig::deleteTargetServer(
  const ServerIdType serverId,
  const OperationPrivilege &privilege)
{
	if (!canDeleteTargetServer(serverId, privilege))
		return HatoholError(HTERR_NO_PRIVILEGE);

	DBAgentDeleteArg arg;
	arg.tableName = TABLE_NAME_SERVERS;
	const ColumnDef &colId = COLUMN_DEF_SERVERS[IDX_SERVERS_ID];
	arg.condition = StringUtils::sprintf("%s=%"FMT_SERVER_ID,
	                                     colId.columnName, serverId);
	DBCLIENT_TRANSACTION_BEGIN() {
		deleteRows(arg);
	} DBCLIENT_TRANSACTION_END();
	return HTERR_OK;
}

void DBClientConfig::getTargetServers
  (MonitoringServerInfoList &monitoringServers, ServerQueryOption &option)
{
	DBAgentSelectExArg arg;
	arg.tableName = TABLE_NAME_SERVERS;
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_ID]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_TYPE]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_HOSTNAME]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_IP_ADDRESS]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_NICKNAME]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_PORT]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_POLLING_INTERVAL_SEC]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_RETRY_INTERVAL_SEC]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_USER_NAME]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_PASSWORD]);
	arg.pushColumn(COLUMN_DEF_SERVERS[IDX_SERVERS_DB_NAME]);

	arg.condition = option.getCondition();

	DBCLIENT_TRANSACTION_BEGIN() {
		select(arg);
	} DBCLIENT_TRANSACTION_END();

	// check the result and copy
	const ItemGroupList &grpList = arg.dataTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		size_t idx = 0;
		const ItemGroup *itemGroup = *it;
		monitoringServers.push_back(MonitoringServerInfo());
		MonitoringServerInfo &svInfo = monitoringServers.back();

		svInfo.id        = GET_INT_FROM_GRP(itemGroup, idx++);
		int type         = GET_INT_FROM_GRP(itemGroup, idx++);
		svInfo.type      = static_cast<MonitoringSystemType>(type);
		svInfo.hostName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.ipAddress = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.nickname  = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.port      = GET_INT_FROM_GRP(itemGroup, idx++);
		svInfo.pollingIntervalSec
		                 = GET_INT_FROM_GRP(itemGroup, idx++);
		svInfo.retryIntervalSec
		                 = GET_INT_FROM_GRP(itemGroup, idx++);
		svInfo.userName  = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.password  = GET_STRING_FROM_GRP(itemGroup, idx++);
		svInfo.dbName    = GET_STRING_FROM_GRP(itemGroup, idx++);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool DBClientConfig::parseCommandLineArgument(const CommandLineArg &cmdArg)
{
	DBConnectInfo &connInfo = PrivateContext::connInfo;
	connInfo.reset();
	string dbServer;
	for (size_t i = 0; i < cmdArg.size(); i++) {
		const string &arg = cmdArg[i];
		if (arg == "--config-db-server") {
			if (i == cmdArg.size()-1) {
				MLPL_ERR(
				  "--config-db-server needs an argument.\n");
				return false;
			}
			i++;
			dbServer = cmdArg[i];
		}
	}

	if (!dbServer.empty()) {
		if (!parseDBServer(dbServer, connInfo.host, connInfo.port))
			return false;
	}

	return true;
}

void DBClientConfig::tableInitializerSystem(DBAgent *dbAgent, void *data)
{
	const ColumnDef &columnDefDatabaseDir =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_DATABASE_DIR];
	const ColumnDef &columnDefFaceRestPort =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_FACE_REST_PORT];
	const ColumnDef &columnDefEnableCopyOnDemand =
	  COLUMN_DEF_SYSTEM[IDX_SYSTEM_ENABLE_COPY_ON_DEMAND];

	// insert default value
	DBAgentInsertArg insArg;
	insArg.tableName = TABLE_NAME_SYSTEM;
	insArg.numColumns = NUM_COLUMNS_SYSTEM;
	insArg.columnDefs = COLUMN_DEF_SYSTEM;
	VariableItemGroupPtr row;

	// database_dir
	row->ADD_NEW_ITEM(String, columnDefDatabaseDir.defaultValue);

	row->ADD_NEW_ITEM(Int, 0); // enable_face_mysql

	// face_reset_port
	row->ADD_NEW_ITEM(Int, atoi(columnDefFaceRestPort.defaultValue));

	// enable_copy_on_demand
	row->ADD_NEW_ITEM(Int, atoi(columnDefEnableCopyOnDemand.defaultValue));

	insArg.row = row;
	dbAgent->insert(insArg);
}

bool DBClientConfig::parseDBServer(const string &dbServer,
                                   string &host, size_t &port)
{
	size_t posColon = dbServer.find(":");
	if (posColon == string::npos) {
		host = dbServer;
		return true;
	}
	if (posColon == dbServer.size() - 1) {
		MLPL_ERR("A column must not be the tail: %s\n",
		         dbServer.c_str());
		return false;
	}
	host = string(dbServer, 0, posColon);
	port = atoi(&dbServer.c_str()[posColon+1]);
	return true;
}

HatoholError DBClientConfig::_addTargetServer(
  MonitoringServerInfo *monitoringServerInfo,
  const OperationPrivilege &privilege)
{
	if (!privilege.has(OPPRVLG_CREATE_SERVER))
		return HatoholError(HTERR_NO_PRIVILEGE);

	// TODO: ADD this server to the liset this user can access to

	DBAgentInsertArg arg;
	arg.tableName = TABLE_NAME_SERVERS;
	arg.numColumns = NUM_COLUMNS_SERVERS;
	arg.columnDefs = COLUMN_DEF_SERVERS;

	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Int, 0);	// This is automatically set
					// (0 is dummy)
	row->ADD_NEW_ITEM(Int, monitoringServerInfo->type);
	row->ADD_NEW_ITEM(String,
	                  monitoringServerInfo->hostName);
	row->ADD_NEW_ITEM(String, monitoringServerInfo->ipAddress);
	row->ADD_NEW_ITEM(String, monitoringServerInfo->nickname);
	row->ADD_NEW_ITEM(Int, monitoringServerInfo->port);
	row->ADD_NEW_ITEM(Int, monitoringServerInfo->pollingIntervalSec);
	row->ADD_NEW_ITEM(Int, monitoringServerInfo->retryIntervalSec);
	row->ADD_NEW_ITEM(String, monitoringServerInfo->userName);
	row->ADD_NEW_ITEM(String, monitoringServerInfo->password);
	row->ADD_NEW_ITEM(String, monitoringServerInfo->dbName);
	arg.row = row;
	insert(arg);
	monitoringServerInfo->id = getLastInsertId();
	return HTERR_OK;
}

bool DBClientConfig::canUpdateTargetServer(
  MonitoringServerInfo *monitoringServerInfo,
  const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_UPDATE_ALL_SERVER))
		return true;

	if (!privilege.has(OPPRVLG_UPDATE_SERVER))
		return false;

	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->isAccessible(monitoringServerInfo->id, privilege, false);
}

HatoholError DBClientConfig::_updateTargetServer(
  MonitoringServerInfo *monitoringServerInfo,
  const OperationPrivilege &privilege,
  const string &condition)
{
	if (!canUpdateTargetServer(monitoringServerInfo, privilege))
		return HatoholError(HTERR_NO_PRIVILEGE);


	DBAgentUpdateArg arg;
	arg.tableName = TABLE_NAME_SERVERS;
	arg.columnDefs = COLUMN_DEF_SERVERS;
	arg.condition  = condition;

	VariableItemGroupPtr row;
	row->ADD_NEW_ITEM(Int, monitoringServerInfo->type);
	arg.columnIndexes.push_back(IDX_SERVERS_TYPE);

	row->ADD_NEW_ITEM(String, monitoringServerInfo->hostName);
	arg.columnIndexes.push_back(IDX_SERVERS_HOSTNAME);

	row->ADD_NEW_ITEM(String, monitoringServerInfo->ipAddress);
	arg.columnIndexes.push_back(IDX_SERVERS_IP_ADDRESS);

	row->ADD_NEW_ITEM(String, monitoringServerInfo->nickname);
	arg.columnIndexes.push_back(IDX_SERVERS_NICKNAME);

	row->ADD_NEW_ITEM(Int, monitoringServerInfo->port);
	arg.columnIndexes.push_back(IDX_SERVERS_PORT);

	row->ADD_NEW_ITEM(Int, monitoringServerInfo->pollingIntervalSec);
	arg.columnIndexes.push_back(IDX_SERVERS_POLLING_INTERVAL_SEC);
	row->ADD_NEW_ITEM(Int, monitoringServerInfo->retryIntervalSec);
	arg.columnIndexes.push_back(IDX_SERVERS_RETRY_INTERVAL_SEC);

	row->ADD_NEW_ITEM(String, monitoringServerInfo->userName);
	arg.columnIndexes.push_back(IDX_SERVERS_USER_NAME);
	row->ADD_NEW_ITEM(String, monitoringServerInfo->password);
	arg.columnIndexes.push_back(IDX_SERVERS_PASSWORD);
	row->ADD_NEW_ITEM(String, monitoringServerInfo->dbName);
	arg.columnIndexes.push_back(IDX_SERVERS_DB_NAME);

	arg.row = row;
	update(arg);
	return HTERR_OK;
}

bool DBClientConfig::canDeleteTargetServer(
  const ServerIdType serverId, const OperationPrivilege &privilege)
{
	if (privilege.has(OPPRVLG_DELETE_ALL_SERVER))
		return true;

	if (!privilege.has(OPPRVLG_DELETE_SERVER))
		return false;

	CacheServiceDBClient cache;
	DBClientUser *dbUser = cache.getUser();
	return dbUser->isAccessible(serverId, privilege);
}

