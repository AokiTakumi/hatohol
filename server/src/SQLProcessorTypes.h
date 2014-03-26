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

#ifndef SQLProcessorTypes_h
#define SQLProcessorTypes_h

#include <list>
#include "ParsableString.h"
#include "ItemTablePtr.h"

enum SQLColumnType {
	SQL_COLUMN_TYPE_INT,
	SQL_COLUMN_TYPE_BIGUINT,
	SQL_COLUMN_TYPE_VARCHAR,
	SQL_COLUMN_TYPE_CHAR,
	SQL_COLUMN_TYPE_TEXT,
	SQL_COLUMN_TYPE_DOUBLE,
	SQL_COLUMN_TYPE_DATETIME,
	NUM_SQL_COLUMN_TYPES,
};

enum SQLJoinType {
	SQL_JOIN_TYPE_UNKNOWN,
	SQL_JOIN_TYPE_INNER,
	SQL_JOIN_TYPE_LEFT_OUTER,
	SQL_JOIN_TYPE_RIGHT_OUTER,
	SQL_JOIN_TYPE_FULL_OUTER,
	SQL_JOIN_TYPE_CROSS,
};

enum SQLKeyType {
	SQL_KEY_NONE,
	SQL_KEY_PRI,
	SQL_KEY_UNI,
	SQL_KEY_IDX, // an index only for the column
};

enum SQLSubQueryMode {
	SQL_SUB_QUERY_NONE,
	SQL_SUB_QUERY_EXISTS,
	SQL_SUB_QUERY_NOT_EXISTS,
};

enum SQLColumnFlags {
	SQL_COLUMN_FLAG_AUTO_INC = (1 << 0),
};

struct SQLProcessorInfo {
	// input statement
	mlpl::ParsableString  statement;

	// error information
	std::string           errorMessage;

	//
	// constructor and destructor
	//
	SQLProcessorInfo(const mlpl::ParsableString &_statement);
	virtual ~SQLProcessorInfo();
};

struct ColumnDef {
	ItemId         itemId;
	const char    *tableName;
	const char    *columnName;
	SQLColumnType  type;
	size_t         columnLength;
	size_t         decFracLength;
	bool           canBeNull;
	SQLKeyType     keyType;
	uint16_t       flags;

	// If there's no default value, 'defalutValue' is "".
	// If the default is NULL, 'defalutValue' shoud be NULL.
	const char    *defaultValue;
};

typedef std::list<ColumnDef>          ColumnDefList;
typedef ColumnDefList::iterator       ColumnDefListIterator;
typedef ColumnDefList::const_iterator ColumnDefListConstIterator;

struct ColumnAccessInfo {
	int         index;
	ColumnDef *columnDef;
};
typedef std::map<std::string, ColumnAccessInfo> ColumnNameAccessInfoMap;
typedef ColumnNameAccessInfoMap::iterator
  ColumnNameAccessInfoMapIterator;
typedef ColumnNameAccessInfoMap::const_iterator
  ColumnNameAccessInfoMapConstIterator;

typedef ItemTablePtr (*SQLTableGetFunc)(void);

struct SQLTableStaticInfo {
	int                     tableId;
	const char             *tableName;
	SQLTableGetFunc         tableGetFunc;
	const ColumnDefList     columnDefList;
	std::vector<ItemDataIndexType> indexTypeVector;

	// The value (ColumnDefinition *) points an instance in
	// 'columnDefList' in this struct.
	// So we must not explicitly free it.
	ColumnNameAccessInfoMap columnAccessInfoMap;
};

typedef std::map<std::string, const SQLTableStaticInfo *>
  TableNameStaticInfoMap;
typedef TableNameStaticInfoMap::iterator TableNameStaticInfoMapIterator;

typedef std::map<ItemGroupId, const SQLTableStaticInfo *>
  ItemGroupIdStaticInfoMap;
typedef ItemGroupIdStaticInfoMap::iterator ItemGroupIdStaticInfoMapIterator;

class SQLColumnIndexResoveler {
public:
	virtual int getIndex(const std::string &tableName,
	                     const std::string &columnName) const = 0;
	virtual size_t getNumberOfColumns(const std::string &tableName) const = 0;
};

class SQLProcessorSelect;
class SQLProcessorSelectFactory {
public:
	virtual SQLProcessorSelect *create(SQLSubQueryMode subQueryMode) = 0;
};

struct SQLProcessorSelectShareInfo {
	SQLProcessorSelectFactory  &processorSelectFactory;
	const mlpl::ParsableString *statement;
	bool                        allowSectionParserChange;

	// methods
	SQLProcessorSelectShareInfo(SQLProcessorSelectFactory &selectFactory);
	void clear(void);
};

class SQLFoundRowOnJoinException {
};

#endif // SQLProcessorTypes_h
