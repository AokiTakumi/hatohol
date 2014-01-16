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

#ifndef SQLColumnParser_h
#define SQLColumnParser_h

#include <string>
#include <list>
#include <deque>
#include "ParsableString.h"
#include "SQLFormulaParser.h"
#include "FormulaElement.h"
#include "FormulaFunction.h"

struct SQLFormulaInfo {
	FormulaElement *formula;
	std::string     expression;
	std::string     alias;
	bool            hasStatisticalFunc;

	// construct and destructor
	SQLFormulaInfo(void);
	~SQLFormulaInfo();
};

typedef vector<SQLFormulaInfo *>       SQLFormulaInfoVector;
typedef SQLFormulaInfoVector::iterator SQLFormulaInfoVectorIterator;

class SQLColumnParser : public SQLFormulaParser
{
public:
	static void init(void);

	SQLColumnParser(void);
	virtual ~SQLColumnParser();
	virtual void add(const string& word, const string &wordLower);
	virtual void close(void);
	const SQLFormulaInfoVector &getFormulaInfoVector(void) const;
	bool getDistinctFlag(void) const;

protected:
	//
	// general sub routines
	//
	void appendFormulaString(const char character);
	void appendFormulaString(const string &str);
	void closeCurrFormulaInfo(void);
	void closeCurrentFormulaString(void);
	void closeCurrentFormula(void);

	//
	// SeparatorChecker callbacks
	//
	static void separatorCbSpace
	  (const char separator, SQLColumnParser *columnParser);
	static void separatorCbComma
	  (const char separator, SQLColumnParser *columnParser);
	virtual void separatorCbParenthesisOpen(const char separator);
	virtual void separatorCbParenthesisClose(const char separator);

	//
	// Keyword handlers
	//
	void kwHandlerAs(void);
	void kwHandlerDistinct(void);

	//
	// functino parsers
	//
	void funcParserMax(void);
	void funcParserCount(void);
	void funcParserSum(void);

private:
	// Type definition
	struct PrivateContext;

	// static function
	static KeywordHandlerMap          m_keywordHandlerMap;
	static FunctionParserMap          m_functionParserMap;

	// General variables
	SQLFormulaInfoVector              m_formulaInfoVector;
	PrivateContext                   *m_ctx;
};

#endif // SQLColumnParser_h

