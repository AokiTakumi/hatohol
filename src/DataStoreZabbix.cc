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
#include "DataStoreZabbix.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DataStoreZabbix::DataStoreZabbix(int dataStoreId)
: m_armApi(dataStoreId)
{
	m_armApi.start();
}

DataStoreZabbix::~DataStoreZabbix(void)
{
}

ItemTablePtr DataStoreZabbix::getTriggers(void)
{
	return m_armApi.getTrigger();
}

ItemTablePtr DataStoreZabbix::getFunctions(void)
{
	return m_armApi.getFunctions();
}

ItemTablePtr DataStoreZabbix::getItems(void)
{
	return m_armApi.getItems();
}

ItemTablePtr DataStoreZabbix::getHosts(void)
{
	return m_armApi.getHosts();
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
