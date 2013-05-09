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

#ifndef DBClientConfig_h
#define DBClientConfig_h

#include <list>
#include "DBClient.h"

enum MonitoringSystemType {
	MONITORING_SYSTEM_ZABBIX,
};

struct MonitoringServerInfo {
	int                  id;
	MonitoringSystemType type;
	string               hostName;
	string               ipAddress;
	string               nickname;
	int                  port;
	int                  pollingIntervalSec;
	int                  retryIntervalSec;
};

typedef list<MonitoringServerInfo>         MonitoringServerInfoList;
typedef MonitoringServerInfoList::iterator MonitoringServerInfoListIterator;

class DBClientConfig : public DBClient {
public:
	static int CONFIG_DB_VERSION;
	static const char *DEFAULT_DB_NAME;
	static void reset(void);
	static void parseCommandLineArgument(CommandLineArg &cmdArg);

	DBClientConfig(void);
	virtual ~DBClientConfig();

	string  getDatabaseDir(void);
	void setDatabaseDir(const string &dir);
	bool isFaceMySQLEnabled(void);
	int  getFaceRestPort(void);
	void setFaceRestPort(int port);
	void addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	void getTargetServers(MonitoringServerInfoList &monitoringServers);

protected:
	static void resetDBInitializedFlags(void);
	static void tableInitializerSystem(DBAgent *dbAgent, void *data);
	void prepareSetupFunction(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBClientConfig_h
