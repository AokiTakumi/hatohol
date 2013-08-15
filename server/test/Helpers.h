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

#ifndef Helpers_h
#define Helpers_h

#include <cppcutter.h>
#include <StringUtils.h>
using namespace mlpl;

#include "ItemTable.h"
#include "DBAgent.h"
#include "DBClient.h"

#define DBCONTENT_MAGIC_CURR_DATETIME "#CURR_DATETIME#"
#define DBCONTENT_MAGIC_NULL          "#NULL#"

typedef pair<int,int>      IntIntPair;
typedef vector<IntIntPair> IntIntPairVector;

void _assertStringVector(StringVector &expected, StringVector &actual);
#define assertStringVector(E,A) cut_trace(_assertStringVector(E,A))

void _assertStringVectorVA(StringVector &actual, ...);
#define assertStringVectorVA(A,...) \
cut_trace(_assertStringVectorVA(A,##__VA_ARGS__))

template<typename T>
static ItemTable * addItems(T* srcTable, int numTable,
                            void (*addFunc)(ItemGroup *, T *),
                            void (*tableCreatedCb)(ItemTable *) = NULL)
{
	ItemTable *table = new ItemTable();
	if (tableCreatedCb)
		(*tableCreatedCb)(table);
	for (int i = 0; i < numTable; i++) {
		ItemGroup* grp = new ItemGroup();
		(*addFunc)(grp, &srcTable[i]);
		table->add(grp, false);
	}
	return table;
}

template<typename T>
static void _assertItemData(const ItemGroup *itemGroup, const T &expected, int &idx)
{
	const ItemData *itemZ = itemGroup->getItemAt(idx);
	cut_assert_not_null(itemZ);
	idx++;
	const T &val = ItemDataUtils::get<T>(itemZ);
	cut_trace(cppcut_assert_equal(expected, val));
}
#define assertItemData(T, IGRP, E, IDX) \
cut_trace(_assertItemData<T>(IGRP, E, IDX))

extern void _assertExist(const string &target, const string &words);
#define assertExist(T,W) cut_trace(_assertExist(T,W))

string executeCommand(const string &commandLine);
string getFixturesDir(void);
bool isVerboseMode(void);
string deleteDBClientDB(DBDomainId domainId);
string getDBClientDBPath(DBDomainId domainId);
string deleteDBClientZabbixDB(int serverId);
string execSqlite3ForDBClient(DBDomainId domainId, const string &statement);
string execSqlite3ForDBClientZabbix(int serverId, const string &statement);
string execMySQL(const string &dbName, const string &statement,
                 bool showHeader = false);

void _assertCurrDatetime(const string &datetime);
#define assertCurrDatetime(D) cut_trace(_assertCurrDatetime(D))

void _assertDBContent(DBAgent *dbAgent, const string &statement,
                      const string &expect);
#define assertDBContent(DB_AGENT, FMT, EXPECT) \
cut_trace(_assertDBContent(DB_AGENT, FMT, EXPECT))

void _assertCreateTable(DBAgent *dbAgent, const string &tableName);
#define assertCreateTable(DBAGENT,TBL_NAME) \
cut_trace(_assertCreateTable(DBAGENT,TBL_NAME))

template<typename T> void _assertAddToDB(T *arg, void (*func)(T *))
{
	bool gotException = false;
	try {
		(*func)(arg);
	} catch (const HatoholException &e) {
		gotException = true;
		cut_fail("%s", e.getFancyMessage().c_str());
	} catch (...) {
		gotException = true;
	}
	cppcut_assert_equal(false, gotException);
}

void makeTestMySQLDBIfNeeded(const string &dbName, bool recreate = false);
void setupTestDBServers(void);
void setupTestDBAction(bool dbRecreate = true);

#endif // Helpers_h
