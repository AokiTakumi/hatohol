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

#ifndef SQLFromParser_h
#define SQLFromParser_h

#include <string>
#include <ParsableString.h>
#include "SQLTableFormula.h"
#include "FormulaElement.h"
#include "PrimaryCondition.h"

class SQLFromParser
{
public:
	static void init(void);
	SQLFromParser(void);
	virtual ~SQLFromParser();
	SQLTableFormula *getTableFormula(void) const;
	SQLTableElementList &getTableElementList(void) const;
	mlpl::SeparatorCheckerWithCallback *getSeparatorChecker(void);
	void setColumnIndexResolver(SQLColumnIndexResoveler *resolver);
	void setSubQueryMode(SQLSubQueryMode subQueryMode);
	void prepareJoin
	  (const PrimaryConditionList &primaryConditionList,
	   SQLTableProcessContextIndex *ctxIndex);
	ItemTablePtr doJoin(FormulaElement *whereFormula);

	virtual void add(const std::string &word, const std::string &wordLower);
	virtual void flush(void);
	virtual void close(void);

protected:
	typedef void (SQLFromParser::*SubParser)
	  (const std::string &word, const std::string &wordLower);
	enum ParsingState {
		PARSING_STAT_EXPECT_TABLE_NAME,
		PARSING_STAT_POST_TABLE_NAME,
		PARSING_STAT_CREATED_TABLE,
		PARSING_STAT_GOT_INNER,
		PARSING_STAT_EXPECT_ON,
		PARSING_STAT_EXPECT_INNER_JOIN_LEFT_FIELD,
		PARSING_STAT_EXPECT_INNER_JOIN_EQUAL,
		PARSING_STAT_EXPECT_INNER_JOIN_RIGHT_FIELD,
		NUM_PARSING_STAT,
	};

	//
	// Sub parsers
	//
	void subParserExpectTableName(const std::string &word,
	                              const std::string &wordLower);
	void subParserPostTableName(const std::string &word,
	                            const std::string &wordLower);
	void subParserCreatedTable(const std::string &word,
	                           const std::string &wordLower);
	void subParserGotInner(const std::string &word,
	                       const std::string &wordLower);
	void subParserExpectOn(const std::string &word,
	                       const std::string &wordLower);
	void subParserExpectLeftField(const std::string &word,
	                              const std::string &wordLower);
	void subParserExpectJoinEqual(const std::string &word,
	                              const std::string &wordLower);
	void subParserExpectRightField(const std::string &word,
	                               const std::string &wordLower);

	//
	// general sub routines
	//
	void goNextStateIfWordIsExpected(const std::string &expectedWord,
	                                 const std::string &actualWord,
	                                 ParsingState nextState);
	void insertTableFormula(SQLTableFormula *tableFormula);
	void makeTableElement(
	  const std::string &tableName,
	  const std::string &varName = mlpl::StringUtils::EMPTY_STRING);
	void associatePrimaryConditionsWithTableProcessorContext
	       (const PrimaryConditionList &primaryConditionList,
	        SQLTableProcessContextIndex *ctxIndex);
	void associatePrimaryConditionColumnsEqual
	       (const PrimaryConditionColumnsEqual *condColumnsEqual,
	        SQLTableProcessContextIndex *ctxIndex);
	void associatePrimaryConditionConstants
	       (const PrimaryConditionConstants *condConstants,
	        SQLTableProcessContextIndex *ctxIndex);
	void selectTableRowIteratorEachTable(SQLTableProcessContext *tableCtx);
	void makeCrossJoin(void);
	void makeInnerJoin(void);
	void parseInnerJoinLeftField(const std::string &fieldName);
	void parseInnerJoinRightField(const std::string &fieldName);
	void doJoineOneRow(FormulaElement *whereFormula);
	void IterateTableRowForJoin(SQLTableElementListIterator tableItr,
	                            FormulaElement *whereFormula);

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbEqual(const char separator,
	                              SQLFromParser *fromParsesr);
	void separatorCbEqual(const char separator);

	static void _separatorCbComma(const char separator,
	                              SQLFromParser *fromParsesr);
	void separatorCbComma(const char separator);

private:
	struct PrivateContext;
	static SubParser             m_subParsers[];
	static size_t                m_numSubParsers;
	PrivateContext              *m_ctx;
	mlpl::SeparatorCheckerWithCallback m_separator;
};

#endif // SQLFromParser_h


