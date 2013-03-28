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

#ifndef ConfigManager_h
#define ConfigManager_h

#include <glib.h>
#include <stdint.h>

#include "DBAgent.h"

class ConfigManager {
public:
	static ConfigManager *getInstance(void);

	void addTargetServer(MonitoringServerInfo *monitoringServerInfo);
	void getTargetServers(MonitoringServerInfoList &monitoringServers);

private:
	static GMutex         m_mutex;
	static ConfigManager *m_instance;

	// Constructor and destructor
	ConfigManager(void);
	virtual ~ConfigManager();
};

#endif // ConfigManager_h
