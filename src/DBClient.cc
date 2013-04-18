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

#include "DBClient.h"

struct DBClient::PrivateContext
{
	static GMutex mutex;
	DBAgent      *dbAgent;

	PrivateContext(void)
	: dbAgent(NULL)
	{
	}

	virtual ~PrivateContext()
	{
		if (dbAgent)
			delete dbAgent;
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
GMutex DBClient::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
DBClient::DBClient(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

DBClient::~DBClient()
{
	if (m_ctx)
		delete m_ctx;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// static methods
void DBClient::createTable
  (DBAgent *dbAgent, const string &tableName, size_t numColumns,
   const ColumnDef *columnDefs, CreateTableInitializer initializer, void *data)
{
	DBAgentTableCreationArg arg;
	arg.tableName  = tableName;
	arg.numColumns = numColumns;
	arg.columnDefs = columnDefs;
	dbAgent->createTable(arg);
	if (initializer)
		(*initializer)(dbAgent, data);
}

// non-static methods
void DBClient::setDBAgent(DBAgent *dbAgent)
{
	m_ctx->dbAgent = dbAgent;
}

DBAgent *DBClient::getDBAgent(void) const
{
	return m_ctx->dbAgent;
}

void DBClient::begin(void)
{
	getDBAgent()->begin();
}

void DBClient::rollback(void)
{
	getDBAgent()->rollback();
}

void DBClient::commit(void)
{
	getDBAgent()->commit();
}

void DBClient::insert(DBAgentInsertArg &insertArg)
{
	getDBAgent()->insert(insertArg);
}

void DBClient::update(DBAgentUpdateArg &updateArg)
{
	getDBAgent()->update(updateArg);
}

void DBClient::select(DBAgentSelectArg &selectArg)
{
	getDBAgent()->select(selectArg);
}

void DBClient::select(DBAgentSelectExArg &selectExArg)
{
	getDBAgent()->select(selectExArg);
}

void DBClient::deleteRows(DBAgentDeleteArg &deleteArg)
{
	getDBAgent()->deleteRows(deleteArg);
}
