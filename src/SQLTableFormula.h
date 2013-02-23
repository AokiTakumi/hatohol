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

#ifndef SQLTableFormula_h
#define SQLTableFormula_h

#include <list>
#include <string>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include "SQLProcessorTypes.h"
#include "ItemTablePtr.h"

// ---------------------------------------------------------------------------
// SQLTableFormula
// ---------------------------------------------------------------------------
class SQLTableFormula
{
public:
	virtual ~SQLTableFormula();
	virtual ItemTablePtr getTable(void) = 0;
};

// ---------------------------------------------------------------------------
// SQLTableElement
// ---------------------------------------------------------------------------
class SQLTableElement : public SQLTableFormula
{
public:
	SQLTableElement(const string &name,
	                const string &varName = StringUtils::EMPTY_STRING);
	const string &getName(void) const;
	const string &getVarName(void) const;
	void setItemTable(ItemTablePtr itemTablePtr);
	virtual ItemTablePtr getTable(void);

private:
	string m_name;
	string m_varName;
	ItemTablePtr m_itemTablePtr;
};

typedef list<SQLTableElement *>             SQLTableElementList;
typedef SQLTableElementList::iterator       SQLTableElementListIterator;
typedef SQLTableElementList::const_iterator SQLTableElementListConstIterator;

// ---------------------------------------------------------------------------
// SQLTableJoin
// ---------------------------------------------------------------------------
class SQLTableJoin : public SQLTableFormula
{
public:
	SQLTableJoin(SQLJoinType type);
	SQLTableFormula *getLeftFormula(void) const;
	SQLTableFormula *getRightFormula(void) const;
	void setLeftFormula(SQLTableFormula *tableFormula);
	void setRightFormula(SQLTableFormula *tableFormula);

private:
	SQLJoinType      m_type;
	SQLTableFormula *m_leftFormula;
	SQLTableFormula *m_rightFormula;
};

// ---------------------------------------------------------------------------
// SQLTableCorssJoin
// ---------------------------------------------------------------------------
class SQLTableCrossJoin : public SQLTableJoin
{
public:
	SQLTableCrossJoin(void);
	virtual ItemTablePtr getTable(void);

private:
};


// ---------------------------------------------------------------------------
// SQLTableInnerJoin
// ---------------------------------------------------------------------------
class SQLTableInnerJoin : public SQLTableJoin
{
public:
	SQLTableInnerJoin(const string &leftTableName,
	                  const string &leftColumnName,
	                  const string &rightTableName,
	                  const string &rightColumnName);
	virtual ItemTablePtr getTable(void);

	const string &getLeftTableName(void) const;
	const string &getLeftColumnName(void) const;
	const string &getRightTableName(void) const;
	const string &getRightColumnName(void) const;

private:
	static const size_t INDEX_NOT_SET = -1;
	string m_leftTableName;
	string m_leftColumnName;
	string m_rightTableName;
	string m_rightColumnName;
	size_t m_indexLeftJoinColumn;
	size_t m_indexRightJoinColumn;
};

#endif // SQLTableFormula_h


