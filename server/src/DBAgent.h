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

#ifndef DBAgent_h
#define DBAgent_h

#include <string>
#include <glib.h>
#include <stdint.h>
#include "Params.h"
#include "SQLProcessorTypes.h"

static const int      AUTO_INCREMENT_VALUE = 0;
static const uint64_t AUTO_INCREMENT_VALUE_U64 = 0;
static const int CURR_DATETIME = -1;

struct DBAgentTableCreationArg {
	std::string         tableName;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
};

struct DBAgentInsertArg {
	std::string         tableName;
	size_t              numColumns;
	const ColumnDef    *columnDefs;
	ItemGroupPtr        row;
};

struct DBAgentUpdateArg {
	std::string         tableName;
	const ColumnDef    *columnDefs;
	std::vector<size_t> columnIndexes;
	ItemGroupPtr        row;
	std::string         condition;
};

struct DBAgentSelectArg {
	std::string         tableName;
	const ColumnDef    *columnDefs;
	std::vector<size_t> columnIndexes;

	// output
	ItemTablePtr        dataTable;
};

struct DBAgentSelectExArg {
	std::string                tableName;
	std::vector<std::string>   statements;
	std::vector<SQLColumnType> columnTypes;
	std::string condition;
	std::string orderBy;
	size_t limit;
	size_t offset;

	// output
	ItemTablePtr        dataTable;

	// constructor and methods
	DBAgentSelectExArg(void);
	void pushColumn(const ColumnDef &columnDef,
	                const std::string &varName = "");
};

struct DBAgentDeleteArg {
	std::string tableName;
	std::string condition;
};

struct DBAgentAddColumnsArg {
	std::string         tableName;
	const ColumnDef    *columnDefs;
	std::vector<size_t> columnIndexes;
};

struct DBConnectInfo {
	std::string host;
	size_t      port;
	std::string user;
	std::string password;
	std::string dbName;

	DBConnectInfo(void);
	virtual ~DBConnectInfo();
	void reset(void);
	const char *getHost(void) const;
	const char *getUser(void) const;
	const char *getPassword(void) const;
};

typedef void (*DBSetupFunc)(DBDomainId domainId, void *data);
static const DBDomainId DEFAULT_DB_DOMAIN_ID = 0;

class DBAgent {
public:
	struct TableProfile {
		std::string         tableName;
		const ColumnDef    *columnDefs;
		const size_t        numColumns;
	};

	struct UpdateRow {
		size_t      columnIndex;
		ItemDataPtr dataPtr;

		UpdateRow(const size_t &index, ItemData *itemData);
	};
	
	struct UpdateArg {
		const TableProfile            &tableProfile;
		std::string                    condition;
		std::vector<const UpdateRow *> rows;

		UpdateArg(const TableProfile &tableProfile);
		virtual ~UpdateArg();
		void add(const size_t &columnIndex, const int         &val);
		void add(const size_t &columnIndex, const uint64_t    &val);
		void add(const size_t &columnIndex, const double      &val);
		void add(const size_t &columnIndex, const std::string &val);
	};

	static void addSetupFunction(DBDomainId domainId,
	                             DBSetupFunc setupFunc, void *data = NULL);

	DBAgent(DBDomainId = DEFAULT_DB_DOMAIN_ID, bool skipSetup = false);
	virtual ~DBAgent();
	DBDomainId getDBDomainId(void) const;

	// virtual methods
	virtual bool isTableExisting(const std::string &tableName) = 0;
	virtual bool isRecordExisting(const std::string &tableName,
	                              const std::string &condition) = 0;
	virtual void begin(void) = 0;
	virtual void commit(void) = 0;
	virtual void rollback(void) = 0;
	virtual void createTable(DBAgentTableCreationArg &tableCreationArg) = 0;
	virtual void insert(DBAgentInsertArg &insertArg) = 0;
	virtual void update(DBAgentUpdateArg &updateArg) = 0;
	virtual void select(DBAgentSelectArg &selectArg) = 0;
	virtual void select(DBAgentSelectExArg &selectExArg) = 0;
	virtual void deleteRows(DBAgentDeleteArg &deleteArg) = 0;
	virtual void addColumns(DBAgentAddColumnsArg &addColumnsArg) = 0;
	virtual uint64_t getLastInsertId(void) = 0;
	virtual uint64_t getNumberOfAffectedRows(void) = 0;

	/**
	 * Update a record if there is the record with the same value in the
	 * specified column. Or this function executes an insert operation.
	 * NOTE: the value that belongs to the column with PRIMARY KEY is not
	 * updated.
	 *
	 * @param itemGroup
	 * An ItemGroup instance that has values for the record to be
	 * updated or inserted.
	 *
	 * @param tableName
	 * The target table name.
	 *
	 * @param numColumns
	 * The number of columns of the table.
	 *
	 * @param targetIndex
	 * A column index used for the comparison.
	 *
	 * @return true if updated, otherwise false.
	 */
	virtual bool updateIfExistElseInsert(
	  const ItemGroup *itemGroup, const std::string &tableName,
	  size_t numColumns, const ColumnDef *columnDefs, size_t targetIndex);

protected:
	static std::string makeSelectStatement(DBAgentSelectArg &selectArg);
	static std::string makeSelectStatement(DBAgentSelectExArg &selectExArg);
	static std::string getColumnValueString(const ColumnDef *columnDef,
	                                        const ItemData *itemData);
	static std::string makeUpdateStatement(DBAgentUpdateArg &updateArg);
	static std::string makeDeleteStatement(DBAgentDeleteArg &deleteArg);
	static std::string makeDatetimeString(int datetime);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // DBAgent_h
