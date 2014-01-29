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

#ifndef SQLProcessorInsert_h
#define SQLProcessorInsert_h

#include "ParsableString.h"
#include "SQLProcessorTypes.h"
#include "ItemDataPtr.h"

struct SQLInsertInfo : public SQLProcessorInfo {
	// parsed matter
	std::string        table;
	mlpl::StringVector columnVector;
	mlpl::StringVector valueVector;

	//
	// constructor and destructor
	//
	SQLInsertInfo(const mlpl::ParsableString &_statment);
	virtual ~SQLInsertInfo();
};

class SQLProcessorInsert
{
private:
	enum InsertParseSection {
		INSERT_PARSING_SECTION_INSERT,
		INSERT_PARSING_SECTION_INTO,
		INSERT_PARSING_SECTION_TABLE,
		INSERT_PARSING_SECTION_COLUMN,
		INSERT_PARSING_SECTION_VALUES_KEYWORD,
		INSERT_PARSING_SECTION_VALUE,
		INSERT_PARSING_SECTION_END,
		NUM_INSERT_PARSING_SECTION,
	};

public:
	static void init(void);
	SQLProcessorInsert(TableNameStaticInfoMap &tableNameStaticInfoMap);
	virtual ~SQLProcessorInsert();
	virtual bool insert(SQLInsertInfo &insertInfo);

protected:
	struct PrivateContext;
	typedef bool
	  (SQLProcessorInsert::*InsertSubParser)(void);

	bool parseInsertStatement(SQLInsertInfo &insertInfo);
	void checkTableAndColumns(SQLInsertInfo &insertInfo);
	void makeColumnDefValueMap(SQLInsertInfo &insertInfo);
	void doInsertToTable(SQLInsertInfo &insertInfo);

	//
	// Sub parsers
	//
	bool parseInsert(void);
	bool parseInto(void);
	bool parseTable(void);
	bool parseColumn(void);
	bool parseValuesKeyword(void);
	bool parseValue(void);
	bool parseEnd(void);

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbParenthesisOpen(const char separator,
	                                        SQLProcessorInsert *obj);
	void separatorCbParenthesisOpen(const char separator);

	static void _separatorCbParenthesisClose(const char separator,
	                                         SQLProcessorInsert *obj);
	void separatorCbParenthesisClose(const char separator);

	static void _separatorCbComma(const char separator,
	                              SQLProcessorInsert *obj);
	void separatorCbComma(const char separator);

	static void _separatorCbQuot(const char separator,
	                             SQLProcessorInsert *obj);
	void separatorCbQuot(const char separator);

	//
	// General sub routines
	//
	bool checkCurrWord(const std::string &expected,
	                   InsertParseSection nextSection);
	bool pushColumn(void);
	bool pushValue(void);

private:
	static const InsertSubParser m_insertSubParsers[];
	PrivateContext              *m_ctx;
};

#endif // SQLProcessorInsert_h

