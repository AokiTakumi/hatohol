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

#ifndef Params_h
#define Params_h

#include <cstdio>
#include <stdint.h>
#include <set>
#include <map>
#include "config.h"

#ifndef USE_CPP11
#define override
#define unique_ptr auto_ptr
#endif

typedef int DBTablesId;

static const DBTablesId DB_TABLES_ID_CONFIG     = 0x0010;
static const DBTablesId DB_TABLES_ID_ACTION     = 0x0018;
static const DBTablesId DB_TABLES_ID_MONITORING = 0x0020;
static const DBTablesId DB_TABLES_ID_USER       = 0x0030;
static const DBTablesId DB_TABLES_ID_HOST       = 0x0040;

typedef int ServerIdType;
#define FMT_SERVER_ID "d"

typedef uint64_t HostIdType;
#define FMT_HOST_ID PRIu64

typedef int ActionIdType;
#define FMT_ACTION_ID "d"

typedef int UserIdType;
#define FMT_USER_ID "d"

typedef int AccessInfoIdType;
#define FMT_ACCESS_INFO_ID "d"

typedef int UserRoleIdType;
#define FMT_USER_ROLE_ID "d"

typedef uint64_t EventIdType;
#define FMT_EVENT_ID PRIu64

typedef uint64_t ItemIdType;
#define FMT_ITEM_ID PRIu64

typedef uint64_t TriggerIdType;
#define FMT_TRIGGER_ID PRIu64

typedef uint64_t HostgroupIdType;
#define FMT_HOST_GROUP_ID PRIu64

typedef uint64_t HostIdType;
#define FMT_HOST_ID PRIu64

typedef int IncidentTrackerIdType;
#define FMT_INCIDENT_TRACKER_ID "d"

static const ServerIdType    INVALID_SERVER_ID = -2;
static const ServerIdType    ALL_SERVERS       = -1;

static const HostIdType      ALL_HOSTS   = -1;
static const HostgroupIdType ALL_HOST_GROUPS = -1;
static const IncidentTrackerIdType ALL_INCIDENT_TRACKERS = -1;

// Define a special ID By using the -2.
// Because, it does not overlap with the hostID set by the Zabbix.
// There is a possibility that depends on Zabbix version of this.
static const HostIdType MONITORING_SERVER_SELF_ID = -2;

static const TriggerIdType FAILED_CONNECT_ZABBIX_TRIGGERID = -1;
static const TriggerIdType FAILED_CONNECT_MYSQL_TRIGGERID  = -2;
static const TriggerIdType FAILED_INTERNAL_ERROR_TRIGGERID = -3;
static const TriggerIdType FAILED_PARSER_ERROR_TRIGGERID   = -4;

static const UserIdType INVALID_USER_ID = -1;
static const UserIdType USER_ID_ANY     = -2;

// This ID is not used for actual users.
// This program and the tests use it internally.
static const UserIdType USER_ID_SYSTEM  = 0;

static const UserRoleIdType INVALID_USER_ROLE_ID = -1;

static const size_t INVALID_COLUMN_IDX = -1;

static const EventIdType EVENT_NOT_FOUND = -1;

static const int INVALID_ARM_PLUGIN_INFO_ID = -1;

typedef std::set<UserIdType>      UserIdSet;
typedef UserIdSet::iterator       UserIdSetIterator;
typedef UserIdSet::const_iterator UserIdSetIterator;
extern const UserIdSet EMPTY_USER_ID_SET;

typedef std::set<AccessInfoIdType>      AccessInfoIdSet;
typedef AccessInfoIdSet::iterator       AccessInfoIdSetIterator;
typedef AccessInfoIdSet::const_iterator AccessInfoIdSetIterator;
extern const AccessInfoIdSet EMPTY_ACCESS_INFO_ID_SET;

typedef std::set<UserRoleIdType>      UserRoleIdSet;
typedef UserRoleIdSet::iterator       UserRoleIdSetIterator;
typedef UserRoleIdSet::const_iterator UserRoleIdSetIterator;
extern const UserRoleIdSet EMPTY_USER_ROLE_ID_SET;

typedef std::set<ServerIdType>      ServerIdSet;
typedef ServerIdSet::iterator       ServerIdSetIterator;
typedef ServerIdSet::const_iterator ServerIdSetConstIterator;
extern const ServerIdSet EMPTY_SERVER_ID_SET;

typedef std::set<IncidentTrackerIdType>      IncidentTrackerIdSet;
typedef IncidentTrackerIdSet::iterator       IncidentTrackerIdSetIterator;
typedef IncidentTrackerIdSet::const_iterator IncidentTrackerIdSetConstIterator;
extern const IncidentTrackerIdSet EMPTY_INCIDENT_TRACKER_ID_SET;

typedef std::set<HostgroupIdType>           HostgroupIdSet;
typedef HostgroupIdSet::iterator            HostgroupIdSetIterator;
typedef HostgroupIdSet::const_iterator      HostgroupIdSetConstIterator;

typedef std::map<ServerIdType, HostgroupIdSet> ServerHostGrpSetMap;
typedef ServerHostGrpSetMap::iterator       ServerHostGrpSetMapIterator;
typedef ServerHostGrpSetMap::const_iterator ServerHostGrpSetMapConstIterator;

enum SyncType {
	SYNC,
	ASYNC,
};
#endif // Params_h