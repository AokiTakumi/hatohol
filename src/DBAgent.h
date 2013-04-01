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

#ifndef DBAgent_h
#define DBAgent_h

#include <string>
#include <list>
using namespace std;

#include <glib.h>
#include <stdint.h>

enum MonitoringSystemType {
	MONITORING_SYSTEM_ZABBIX,
};

struct MonitoringServerInfo {
	uint32_t             id;
	MonitoringSystemType type;
	string               hostName;
	string               ipAddress;
	string               nickname;
};

typedef list<MonitoringServerInfo>         MonitoringServerInfoList;
typedef MonitoringServerInfoList::iterator MonitoringServerInfoListIterator;

enum TriggerStatusType {
	TRIGER_STATUS_OK,
	TRIGER_STATUS_PROBLEM,
};

enum TriggerSeverityType {
	TRIGGER_SEVERITY_INFO,
	TRIGGER_SEVERITY_WARN,
};

struct TriggerInfo {
	TriggerStatusType   status;
	TriggerSeverityType severity;
	timespec            lastChangedTime;
	uint32_t            serverId;
	string              hostId;
	string              hostName;
	string              brief;
};

typedef list<TriggerInfo>         TriggerInfoList;
typedef TriggerInfoList::iterator TriggerInfoListIterator;

class DBAgent {
public:
	DBAgent(void);
	virtual ~DBAgent();

	// virtual methods
	virtual bool isTableExisting(const string &tableName) = 0;
	virtual bool isRecordExisting(const string &tableName,
	                              const string &condition) = 0;
	virtual void
	   addTargetServer(MonitoringServerInfo *monitoringServerInfo) = 0;
	virtual void
	   getTargetServers(MonitoringServerInfoList &monitoringServers) = 0;
private:
};

#endif // DBAgent_h
