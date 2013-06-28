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

#ifndef SQLProcessorUpdate_h
#define SQLProcessorUpdate_h

#include "SQLProcessorTypes.h"
#include "SQLWhereParser.h"
#include "ItemDataPtr.h"

struct SQLUpdateInfo : public SQLProcessorInfo {
	// parsed matter
	string           table;
	StringVector     columnVector;
	StringVector     valueVector;
	SQLWhereParser   whereParser;

	// table to be updated
	const SQLTableStaticInfo *tableStaticInfo;
	ItemTablePtr             tablePtr;

	// TODO: replace this member with the global static object.
	// convinient variable
	ItemDataPtr      itemFalsePtr;

	//
	// constructor and destructor
	//
	SQLUpdateInfo(ParsableString &_statment);
	virtual ~SQLUpdateInfo();
};

class SQLProcessorUpdate
{
private:
	enum UpdateParseSection {
		UPDATE_PARSING_SECTION_UPDATE,
		UPDATE_PARSING_SECTION_TABLE,
		UPDATE_PARSING_SECTION_SET_KEYWORD,
		UPDATE_PARSING_SECTION_COLUMN,
		UPDATE_PARSING_SECTION_EQUAL,
		UPDATE_PARSING_SECTION_VALUE,
		UPDATE_PARSING_SECTION_POST_ONE_SET,
		UPDATE_PARSING_SECTION_WHERE,
		NUM_UPDATE_PARSING_SECTION,
	};

public:
	static void init(void);
	SQLProcessorUpdate(TableNameStaticInfoMap &tableNameStaticInfoMap);
	virtual ~SQLProcessorUpdate();
	virtual bool update(SQLUpdateInfo &updateInfo);

protected:
	struct PrivateContext;
	typedef void
	  (SQLProcessorUpdate::*UpdateSubParser)(void);

	void parseUpdateStatement(SQLUpdateInfo &updateInfo);
	void getStaticTableInfo(SQLUpdateInfo &updateInfo);
	void getTable(SQLUpdateInfo &updateInfo);
	void doUpdate(SQLUpdateInfo &updateInfo);

	//
	// Sub parsers
	//
	void parseUpdate(void);
	void parseTable(void);
	void parseSetKeyword(void);
	void parseColumn(void);
	void parseEqual(void);
	void parseValue(void);
	void parsePostOneSet(void);
	void parseWhere(void);

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbComma(const char separator,
	                              SQLProcessorUpdate *obj);
	void separatorCbComma(const char separator);

	static void _separatorCbQuot(const char separator,
	                             SQLProcessorUpdate *obj);
	void separatorCbQuot(const char separator);

	static void _separatorCbEqual(const char separator,
	                              SQLProcessorUpdate *obj);
	void separatorCbEqual(const char separator);

	//
	// General sub routines
	//
	string readCurrWord(void); 
	void checkCurrWord(string expected, UpdateParseSection nextSection);
	static FormulaVariableDataGetter *
	  formulaColumnDataGetterFactory(const string &name, void *priv);
	static bool updateMatchingRows(const ItemGroup *itemGroup,
	                               PrivateContext *ctx);
	static bool updateMatchingCell(const ItemGroup *itemGroup,
	                               PrivateContext *ctx,
	                               string &columnName, string &value);

private:
	static const UpdateSubParser m_updateSubParsers[];
	PrivateContext              *m_ctx;
};

#endif // SQLProcessorUpdate_h

