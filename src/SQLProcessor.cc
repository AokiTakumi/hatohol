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

#include <Logger.h>
#include <StringUtils.h>
using namespace mlpl;

#include <algorithm>
using namespace std;

#include <stdexcept>
#include <cstring>
#include "SQLProcessor.h"
#include "SQLProcessorException.h"
#include "ItemEnum.h"
#include "SQLUtils.h"
#include "Utils.h"

const SQLProcessor::SelectSubParser SQLProcessor::m_selectSubParsers[] = {
	&SQLProcessor::parseSelectedColumns,
	&SQLProcessor::parseGroupBy,
	&SQLProcessor::parseFrom,
	&SQLProcessor::parseWhere,
	&SQLProcessor::parseOrderBy,
	&SQLProcessor::parseLimit,
};

map<string, SQLProcessor::SelectSubParser>
  SQLProcessor::m_selectSectionParserMap;

enum BetweenParsingStep {
	BETWEEN_NONE,
	BETWEEN_EXPECT_FIRST,
	BETWEEN_EXPECT_AND,
	BETWEEN_EXPECT_SECOND,
};

struct SQLProcessor::SelectParserContext {
	SQLProcessor       *sqlProcessor;

	string              currWord;
	string              currWordLower;
	SelectParseSection  section;
	SQLSelectInfo      &selectInfo;

	// methods
	SelectParserContext(SQLProcessor *sqlProc,
	                    SelectParseSection _section,
	                    SQLSelectInfo &_selectInfo)
	: sqlProcessor(sqlProc),
	  section(_section),
	  selectInfo(_selectInfo)
	{
	}
};

class SQLFormulaColumnDataGetter : public FormulaVariableDataGetter {
public:
	SQLFormulaColumnDataGetter(string &name, SQLSelectInfo *selectInfo)
	: m_selectInfo(selectInfo),
	  m_columnInfo(NULL)
	{
		SQLColumnNameMapIterator it
		  = m_selectInfo->columnNameMap.find(name);
		if (it != m_selectInfo->columnNameMap.end()) {
			m_columnInfo = it->second;
			return;
		}

		// The owner of the SQLColumnInfo object created bellow
		// is *m_selectInfo. So its deletion is done
		// in the destructor of the SQLSelectInfo object.
		// No need to delete in this class.
		m_columnInfo = new SQLColumnInfo(name);
		m_selectInfo->columnNameMap[name] = m_columnInfo;
	}
	
	virtual ~SQLFormulaColumnDataGetter()
	{
	}

	virtual ItemDataPtr getData(void)
	{
		if (m_columnInfo->columnType !=
		    SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			string msg;
			TRMSG(msg,
			      "m_columnInfo->columnType(%d) != TYPE_NORMAL.",
			      m_columnInfo->columnType);
			throw logic_error(msg);
		}
		ItemId itemId = m_columnInfo->columnBaseDef->itemId;
		return ItemDataPtr
		         (m_selectInfo->evalTargetItemGroup->getItem(itemId));
	}

	SQLColumnInfo *getColumnInfo(void) const
	{
		return m_columnInfo;
	}

private:
	SQLSelectInfo *m_selectInfo;
	SQLColumnInfo *m_columnInfo;
};

// ---------------------------------------------------------------------------
// Public methods (SQLColumnDefinitino)
// ---------------------------------------------------------------------------
SQLOutputColumn::SQLOutputColumn(SQLFormulaInfo *_formulaInfo)
: formulaInfo(_formulaInfo),
  columnInfo(NULL),
  columnBaseDef(NULL),
  columnBaseDefDeleteFlag(false),
  tableInfo(NULL)
{
}

SQLOutputColumn::SQLOutputColumn(const SQLColumnInfo *_columnInfo)
: formulaInfo(NULL),
  columnInfo(_columnInfo),
  columnBaseDef(NULL),
  columnBaseDefDeleteFlag(false),
  tableInfo(NULL)
{
}

SQLOutputColumn::~SQLOutputColumn()
{
	if (columnBaseDefDeleteFlag)
		delete columnBaseDef;
}

ItemDataPtr SQLOutputColumn::getItem(const ItemGroup *itemGroup) const
{
	if (formulaInfo)
		return formulaInfo->formula->evaluate();
	if (columnInfo)
		return itemGroup->getItem(columnBaseDef->itemId);
	MLPL_BUG("formulaInfo and columnInfo are both NULL.");
	return ItemDataPtr();
}

// ---------------------------------------------------------------------------
// Public methods (SQLTableInfo)
// ---------------------------------------------------------------------------
SQLTableInfo::SQLTableInfo(void)
: staticInfo(NULL)
{
}

// ---------------------------------------------------------------------------
// Public methods (SQLColumnInfo)
// ---------------------------------------------------------------------------
SQLColumnInfo::SQLColumnInfo(string &_name)
: name(_name),
  tableInfo(NULL),
  columnBaseDef(NULL),
  columnType(COLUMN_TYPE_UNKNOWN)
{
}

void SQLColumnInfo::associate(SQLTableInfo *_tableInfo)
{
	tableInfo = _tableInfo;
	_tableInfo->columnList.push_back(this);
}

void SQLColumnInfo::setColumnType(void)
{
	if (name == "*")
		columnType = SQLColumnInfo::COLUMN_TYPE_ALL;
	else if (baseName == "*")
		columnType = SQLColumnInfo::COLUMN_TYPE_ALL_OF_TABLE;
	else
		columnType = SQLColumnInfo::COLUMN_TYPE_NORMAL;
}

// ---------------------------------------------------------------------------
// Public methods (SQLSelectInfo)
// ---------------------------------------------------------------------------
SQLSelectInfo::SQLSelectInfo(ParsableString &_statement)
: SQLProcessorInfo(_statement),
  useIndex(false),
  makeTextRowsWriteMaskCount(0),
  evalTargetItemGroup(NULL),
  itemFalsePtr(new ItemBool(false), false)
{
}

SQLSelectInfo::~SQLSelectInfo()
{
	SQLColumnNameMapIterator columnIt = columnNameMap.begin();
	for (; columnIt != columnNameMap.end(); ++columnIt)
		delete columnIt->second;

	SQLTableInfoListIterator tableIt = tables.begin();
	for (; tableIt != tables.end(); ++tableIt)
		delete *tableIt;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
void SQLProcessor::init(void)
{
	m_selectSectionParserMap["select"] = &SQLProcessor::parseSectionColumn;
	m_selectSectionParserMap["from"]   = &SQLProcessor::parseSectionFrom;
	m_selectSectionParserMap["where"]  = &SQLProcessor::parseSectionWhere;
	m_selectSectionParserMap["order"]  = &SQLProcessor::parseSectionOrder;
	m_selectSectionParserMap["group"]  = &SQLProcessor::parseSectionGroup;
	m_selectSectionParserMap["limit"]  = &SQLProcessor::parseSectionLimit;

	// check the size of m_selectSubParsers
	size_t size = sizeof(SQLProcessor::m_selectSubParsers) / 
	                sizeof(SelectSubParser);
	if (size != NUM_SELECT_PARSING_SECTION) {
		string msg;
		TRMSG(msg, "sizeof(m_selectSubParsers) is invalid: "
		           "(expcect/actual: %d/%d).",
		      NUM_SELECT_PARSING_SECTION, size);
		throw logic_error(msg);
	}
}

bool SQLProcessor::select(SQLSelectInfo &selectInfo)
{
	try {
		// disassemble the query statement
		if (!parseSelectStatement(selectInfo))
			return false;
		makeTableInfo(selectInfo);
		checkParsedResult(selectInfo);

		// set members in SQLFormulaColumnDataGetter
		if (!fixupColumnNameMap(selectInfo))
			return false;

		// associate each column with the table
		if (!associateColumnWithTable(selectInfo))
			return false;

		// associate each table with static table information
		if (!associateTableWithStaticInfo(selectInfo))
			return false;

		// make ItemTable objects for all specified tables
		if (!setColumnTypeAndBaseDefInColumnInfo(selectInfo))
			return false;
		if (!makeColumnDefs(selectInfo))
			return false;
		if (!makeItemTables(selectInfo))
			return false;

		// join tables
		doJoin(selectInfo);

		// pickup matching rows
		if (!selectMatchingRows(selectInfo))
			return false;

		// convert data to string
		if (!makeTextOutput(selectInfo))
			return false;
	} catch (const SQLProcessorException &e) {
		const char *message = e.what();
		selectInfo.errorMessage = message;
		MLPL_DBG("Got SQLProcessorException: %s\n", message);
		return false;
	}

	return true;
}

bool SQLProcessor::insert(SQLInsertInfo &insertInfo)
{
	return m_processorInsert.insert(insertInfo);
}

bool SQLProcessor::update(SQLUpdateInfo &updateInfo)
{
	return m_processorUpdate.update(updateInfo);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
SQLProcessor::SQLProcessor(TableNameStaticInfoMap &tableNameStaticInfoMap)
: m_separatorSpaceComma(" ,"),
  m_tableNameStaticInfoMap(tableNameStaticInfoMap),
  m_processorInsert(tableNameStaticInfoMap),
  m_processorUpdate(tableNameStaticInfoMap)
{
	// Other elements are set in parseSelectStatement().
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_GROUP_BY] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_ORDER_BY] = 
	  &m_separatorSpaceComma;
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_LIMIT] = 
	  &m_separatorSpaceComma;
}

SQLProcessor::~SQLProcessor()
{
}

bool
SQLProcessor::checkSelectedAllColumns(const SQLSelectInfo &selectInfo,
                                      const SQLColumnInfo &columnInfo) const
{
	if (columnInfo.name == "*")
		return true;
	if (!columnInfo.tableInfo)
		return false;
	if (columnInfo.baseName == "*")
		return true;
	return false;
}

bool SQLProcessor::parseSelectStatement(SQLSelectInfo &selectInfo)
{
	MLPL_DBG("<%s> %s\n", __func__, selectInfo.statement.getString());
	map<string, SelectSubParser>::iterator it;
	SelectSubParser subParser = NULL;
	SelectParserContext ctx(this, SELECT_PARSING_SECTION_COLUMN,
	                        selectInfo);

	// set ColumnDataGetterFactory
	selectInfo.columnParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, &selectInfo);
	selectInfo.whereParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, &selectInfo);

	// callback function for column and where section
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_COLUMN]
	  = selectInfo.columnParser.getSeparatorChecker();

	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_FROM]
	  = selectInfo.fromParser.getSeparatorChecker();

	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_WHERE]
	  = selectInfo.whereParser.getSeparatorChecker();

	while (!selectInfo.statement.finished()) {
		ctx.currWord = readNextWord(ctx);
		if (ctx.currWord.empty())
			continue;

		// check if this is a keyword.
		ctx.currWordLower = StringUtils::toLower(ctx.currWord);
		it = m_selectSectionParserMap.find(ctx.currWordLower);
		if (it != m_selectSectionParserMap.end()) {
			// When the function returns 'true', it means
			// the current word is section keyword and
			subParser = it->second;
			if ((this->*subParser)(ctx))
				continue;
		}

		// parse each component
		if (ctx.section >= NUM_SELECT_PARSING_SECTION) {
			MLPL_BUG("section(%d) >= NUM_SELECT_PARSING_SECTION\n",
			         ctx.section);
			return false;
		}
		subParser = m_selectSubParsers[ctx.section];
		if (!(this->*subParser)(ctx))
			return false;
	}
	if (!selectInfo.columnParser.close())
		return false;
	if (!selectInfo.whereParser.close())
		return false;
	selectInfo.fromParser.close();

	return true;
}

void SQLProcessor::makeTableInfo(SQLSelectInfo &selectInfo)
{
	SQLTableElementListConstIterator it =
	  selectInfo.fromParser.getTableElementList().begin();
	for (; it != selectInfo.fromParser.getTableElementList().end(); ++it) {
		// make selectInfo.tables
		const SQLTableElement *tableElem = *it;
		SQLTableInfo *tableInfo = new SQLTableInfo();
		tableInfo->name    = tableElem->getName();
		tableInfo->varName = tableElem->getVarName();
		selectInfo.tables.push_back(tableInfo);

		// make selectInfo.tableVarInfoMap
		string &varName = tableInfo->varName;
		if (varName.empty())
			continue;
		pair<SQLTableVarNameInfoMapIterator, bool> ret =
		  selectInfo.tableVarInfoMap.insert
		    (pair<string, SQLTableInfo *>(varName, tableInfo));
		if (!ret.second) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Failed to insert: table name: %s, %s.",
			  varName.c_str(), selectInfo.statement.getString());
		}
	}
}

void SQLProcessor::checkParsedResult(const SQLSelectInfo &selectInfo) const
{
	if (selectInfo.columnNameMap.empty())
		THROW_SQL_PROCESSOR_EXCEPTION("Not found: columns.");

	if (selectInfo.tables.empty())
		THROW_SQL_PROCESSOR_EXCEPTION("Not found: tables.");
}

bool SQLProcessor::fixupColumnNameMap(SQLSelectInfo &selectInfo)
{
	SQLColumnNameMapIterator it = selectInfo.columnNameMap.begin();
	for (; it != selectInfo.columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;
		if (!parseColumnName(columnInfo->name, columnInfo->baseName,
	                              columnInfo->tableVar)) {
			return false;
		}
		columnInfo->setColumnType();
	}
	return true;
}

bool SQLProcessor::associateColumnWithTable(SQLSelectInfo &selectInfo)
{
	SQLColumnNameMapIterator it = selectInfo.columnNameMap.begin();
	for (; it != selectInfo.columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;
		if (columnInfo->columnType == SQLColumnInfo::COLUMN_TYPE_ALL)
			continue;

		// set SQLColumnInfo::tableInfo and SQLTableInfo::columnList.
		if (selectInfo.tables.size() == 1) {
			SQLTableInfo *tableInfo = *selectInfo.tables.begin();
			if (columnInfo->tableVar.empty())
				columnInfo->associate(tableInfo);
			else if (columnInfo->tableVar == tableInfo->varName)
				columnInfo->associate(tableInfo);
			else {
				MLPL_DBG("columnInfo.tableVar (%s) != "
				         "tableInfo.varName (%s)\n",
				         columnInfo->tableVar.c_str(),
				         tableInfo->varName.c_str());
				return false;
			}
			continue;
		}

		const SQLTableInfo *tableInfo
		  = getTableInfoFromVarName(selectInfo, columnInfo->tableVar);
		if (!tableInfo)
			return false;
		columnInfo->associate(const_cast<SQLTableInfo *>(tableInfo));

		// set SQLColumnInfo::baseDef.
	}
	return true;
}

bool SQLProcessor::associateTableWithStaticInfo(SQLSelectInfo &selectInfo)
{
	SQLTableInfoListIterator tblInfoIt = selectInfo.tables.begin();
	for (; tblInfoIt != selectInfo.tables.end(); ++tblInfoIt) {
		SQLTableInfo *tableInfo = *tblInfoIt;
		TableNameStaticInfoMapIterator it;
		it = m_tableNameStaticInfoMap.find(tableInfo->name);
		if (it == m_tableNameStaticInfoMap.end()) {
			MLPL_DBG("Not found table: %s\n",
			         tableInfo->name.c_str());
			return false;
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		tableInfo->staticInfo = staticInfo;
	}
	return true;
}

bool
SQLProcessor::setColumnTypeAndBaseDefInColumnInfo(SQLSelectInfo &selectInfo)
{
	SQLColumnNameMapIterator it = selectInfo.columnNameMap.begin();
	for (; it != selectInfo.columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;

		// baseDef
		if (columnInfo->columnType != SQLColumnInfo::COLUMN_TYPE_NORMAL)
			continue;

		if (!columnInfo->tableInfo) {
			MLPL_BUG("columnInfo->stableInfo is NULL\n");
			return false;
		}

		const SQLTableStaticInfo *staticInfo =
		  columnInfo->tableInfo->staticInfo;
		if (!staticInfo) {
			MLPL_BUG("staticInfo is NULL\n");
			return false;
		}

		columnInfo->columnBaseDef =
		  SQLUtils::getColumnBaseDefinition(columnInfo->baseName,
		                                    staticInfo);
		if (!columnInfo->columnBaseDef)
			return false;
	}
	return true;
}

void SQLProcessor::addOutputColumn(SQLSelectInfo &selectInfo,
                                   const SQLColumnInfo *columnInfo,
                                   const ColumnBaseDefinition *columnBaseDef,
                                   const SQLFormulaInfo *formulaInfo)
{
	const SQLTableInfo *tableInfo = columnInfo->tableInfo;
	selectInfo.outputColumnVector.push_back(SQLOutputColumn(columnInfo));
	SQLOutputColumn &outCol = selectInfo.outputColumnVector.back();
	outCol.columnBaseDef = columnBaseDef;
	outCol.tableInfo     = tableInfo;
	outCol.schema        = getDBName();
	outCol.table         = tableInfo->name;
	outCol.tableVar      = tableInfo->varName;
	outCol.column        = columnBaseDef->columnName;
	if (!formulaInfo) // when 'select * or n.*'
		outCol.columnVar = columnBaseDef->columnName;
	else if (formulaInfo->alias.empty())
		outCol.columnVar = columnBaseDef->columnName;
	else
		outCol.columnVar = formulaInfo->alias;
}

void SQLProcessor::addOutputColumn(SQLSelectInfo &selectInfo,
                                   SQLFormulaInfo *formulaInfo)
{
	selectInfo.outputColumnVector.push_back(SQLOutputColumn(formulaInfo));
	SQLOutputColumn &outCol = selectInfo.outputColumnVector.back();
	outCol.columnBaseDef = makeColumnBaseDefForFormula(formulaInfo);
	//outCol.tableInfo     =
	outCol.schema        = getDBName();
	//outCol.table         =
	//outCol.tableVar      =
	outCol.column        = formulaInfo->expression;
	if (formulaInfo->alias.empty())
		outCol.columnVar = formulaInfo->expression;
	else
		outCol.columnVar = formulaInfo->alias;
}

bool SQLProcessor::addOutputColumnsOfAllTables(SQLSelectInfo &selectInfo,
                                               const SQLColumnInfo *_columnInfo)
{
	// Note: We can just use the argument type w/o 'const'.
	// Or change the algorithm more elegant.
	SQLColumnInfo *columnInfo = const_cast<SQLColumnInfo *>(_columnInfo);

	SQLTableInfoListIterator it = selectInfo.tables.begin();
	for (; it != selectInfo.tables.end(); ++it) {
		columnInfo->tableInfo = *it;
		if (!addOutputColumnsOfOneTable(selectInfo, columnInfo))
			return false;
	}
	return true;
}

bool SQLProcessor::addOutputColumnsOfOneTable(SQLSelectInfo &selectInfo,
                                              const SQLColumnInfo *columnInfo)
{
	const SQLTableInfo *tableInfo = columnInfo->tableInfo;
	if (!tableInfo->staticInfo) {
		MLPL_BUG("tableInfo->staticInfo is NULL\n");
		return false;
	}

	ColumnBaseDefListConstIterator it;
	it = tableInfo->staticInfo->columnBaseDefList.begin();
	for (; it != tableInfo->staticInfo->columnBaseDefList.end(); ++it) {
		const ColumnBaseDefinition *columnBaseDef = &(*it);
		addOutputColumn(selectInfo, columnInfo, columnBaseDef);
	}
	return true;
}

const ColumnBaseDefinition *
SQLProcessor::makeColumnBaseDefForFormula(SQLFormulaInfo *formulaInfo)
{
	ColumnBaseDefinition *columnBaseDef = new ColumnBaseDefinition();
	columnBaseDef->itemId = SYSTEM_ITEM_ID_ANONYMOUS;
	columnBaseDef->tableName = "";
	columnBaseDef->columnName = "";
	columnBaseDef->type = SQL_COLUMN_TYPE_BIGUINT;
	columnBaseDef->columnLength = 20;
	MLPL_BUG("Tentative implementation: type and columnLength must be "
	         "calculated by analyzing used columns.\n");
	columnBaseDef->flags = 0; // TODO: substitute the appropriate value.
	return columnBaseDef;
}

bool SQLProcessor::makeColumnDefs(SQLSelectInfo &selectInfo)
{
	const SQLFormulaInfoVector &formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	for (size_t i = 0; i < formulaInfoVector.size(); i++) {
		SQLFormulaInfo *formulaInfo = formulaInfoVector[i];
		FormulaVariable *formulaVariable =
		  dynamic_cast<FormulaVariable *>(formulaInfo->formula);
		if (!formulaVariable) {
			// When the column formula is not single column.
			addOutputColumn(selectInfo, formulaInfo);
			continue;
		}

		// search the ColumnInfo instance
		FormulaVariableDataGetter *dataGetter =
			formulaVariable->getFormulaVariableGetter();
		SQLFormulaColumnDataGetter *sqlDataGetter =
		  dynamic_cast<SQLFormulaColumnDataGetter *>(dataGetter);
		SQLColumnInfo *columnInfo = sqlDataGetter->getColumnInfo();

		int columnType = columnInfo->columnType;
		if (columnType == SQLColumnInfo::COLUMN_TYPE_ALL) {
			if (!addOutputColumnsOfAllTables(selectInfo,
			                                 columnInfo))
				return false;
			continue;
		} else if (!columnInfo->tableInfo) {
			MLPL_BUG("columnInfo->tableInfo is NULL\n");
			return false;
		}

		if (columnType == SQLColumnInfo::COLUMN_TYPE_ALL_OF_TABLE) {
			if (!addOutputColumnsOfOneTable(selectInfo, columnInfo))
				return false;
		} else if (columnType == SQLColumnInfo::COLUMN_TYPE_NORMAL) {
			if (!columnInfo->columnBaseDef) {
				MLPL_BUG("columnInfo.columnBaseDef is NULL\n");
				return false;
			}
			addOutputColumn(selectInfo, columnInfo,
			                columnInfo->columnBaseDef,
			                formulaInfo);
		} else {
			MLPL_BUG("Invalid columnType: %d\n", columnType);
			return false;
		}
	}
	return true;
}

bool SQLProcessor::makeItemTables(SQLSelectInfo &selectInfo)
{
	SQLTableElementList &tableElemList =
	  selectInfo.fromParser.getTableElementList();
	if (tableElemList.size() != selectInfo.tables.size()) {
		THROW_ASURA_EXCEPTION(
		  "tableElemList.size() != selectInfo.tables.size() (%zd, %zd)",
		  tableElemList.size(), selectInfo.tables.size());
	}
	SQLTableElementListIterator tblElemIt = tableElemList.begin();

	SQLTableInfoListIterator tblInfoIt = selectInfo.tables.begin();
	for (; tblInfoIt != selectInfo.tables.end(); ++tblInfoIt, ++tblElemIt) {
		const SQLTableInfo *tableInfo = *tblInfoIt;
		SQLTableMakeFunc func = tableInfo->staticInfo->tableMakeFunc;
		ItemTablePtr tablePtr = (this->*func)(selectInfo, *tableInfo);
		if (!tablePtr.hasData()) {
			MLPL_DBG("ItemTable: table has no data. "
			         "name: %s, var: %s\n",
			         (*tblInfoIt)->name.c_str(),
			         (*tblInfoIt)->varName.c_str());
			return false;
		}
		(*tblElemIt)->setItemTable(tablePtr);
	}
	return true;
}

void SQLProcessor::doJoin(SQLSelectInfo &selectInfo)
{
	selectInfo.joinedTable =
	  selectInfo.fromParser.getTableFormula()->getTable();
}

bool SQLProcessor::selectMatchingRows(SQLSelectInfo &selectInfo)
{
	FormulaElement *formula = selectInfo.whereParser.getFormula();
	if (!formula) {
		selectInfo.selectedTable = selectInfo.joinedTable;
		return true;
	}
	return selectInfo.joinedTable->foreach<SQLSelectInfo&>
	                                    (pickupMatchingRows, selectInfo);
}

bool SQLProcessor::makeTextOutput(SQLSelectInfo &selectInfo)
{
	// check if statistical function is included
	bool hasStatisticalFunc = false;
	const SQLFormulaInfoVector &formulaInfoVector
	  = selectInfo.columnParser.getFormulaInfoVector();
	for (size_t i = 0; i < formulaInfoVector.size(); i++) {
		if (formulaInfoVector[i]->hasStatisticalFunc) {
			hasStatisticalFunc = true;
			break;
		}
	}

	if (hasStatisticalFunc) {
		selectInfo.makeTextRowsWriteMaskCount =
		  selectInfo.selectedTable->getNumberOfRows() - 1;
	}
	bool ret;
	ret = selectInfo.selectedTable->foreach<SQLSelectInfo&>
	                                       (makeTextRows, selectInfo);
	return ret;
}

bool SQLProcessor::pickupMatchingRows(const ItemGroup *itemGroup,
                                      SQLSelectInfo &selectInfo)
{
	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	selectInfo.evalTargetItemGroup = nonConstItemGroup;
	FormulaElement *formula = selectInfo.whereParser.getFormula();
	ItemDataPtr result = formula->evaluate();
	if (!result.hasData()) {
		MLPL_DBG("result has no data.\n");
		return false;
	}
	if (*result == *selectInfo.itemFalsePtr)
		return true;
	selectInfo.selectedTable->add(nonConstItemGroup);
	return true;
}

bool SQLProcessor::makeTextRows(const ItemGroup *itemGroup,
                                SQLSelectInfo &selectInfo)
{
	bool doOutput = false;
	if (selectInfo.makeTextRowsWriteMaskCount == 0)
		doOutput = true;
	else
		selectInfo.makeTextRowsWriteMaskCount--;

	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	selectInfo.evalTargetItemGroup = nonConstItemGroup;

	StringVector textVector;
	for (size_t i = 0; i < selectInfo.outputColumnVector.size(); i++) {
		const SQLOutputColumn &outputColumn =
		  selectInfo.outputColumnVector[i];
		const ItemDataPtr itemPtr = outputColumn.getItem(itemGroup);
		if (!itemPtr.hasData()) {
			MLPL_BUG("Failed to get item data.\n");
			return false;
		}
		if (!doOutput)
			continue;
		textVector.push_back(itemPtr->getString());
	}
	if (!textVector.empty())
		selectInfo.textRows.push_back(textVector);
	return true;
}

//
// Select status parsers
//
bool SQLProcessor::parseSectionColumn(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_COLUMN;
	return true;
}

bool SQLProcessor::parseSectionFrom(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_FROM;
	return true;
}

bool SQLProcessor::parseSectionWhere(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_WHERE;
	return true;
}

bool SQLProcessor::parseSectionOrder(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.statement.setParsingPosition(currPos);
		return false;
	}

	ctx.section = SELECT_PARSING_SECTION_ORDER_BY;
	return true;
}

bool SQLProcessor::parseSectionGroup(SelectParserContext &ctx)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(ctx, &currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		ctx.selectInfo.statement.setParsingPosition(currPos);
		return false;
	}

	ctx.section = SELECT_PARSING_SECTION_GROUP_BY;
	return true;
}

bool SQLProcessor::parseSectionLimit(SelectParserContext &ctx)
{
	ctx.section = SELECT_PARSING_SECTION_LIMIT;
	return true;
}

//
// Select statment parsers
//
bool SQLProcessor::parseSelectedColumns(SelectParserContext &ctx)
{
	return ctx.selectInfo.columnParser.add(ctx.currWord, ctx.currWordLower);
}

bool SQLProcessor::parseGroupBy(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: GROUP_BY\n");
	return false;
}

bool SQLProcessor::parseFrom(SelectParserContext &ctx)
{
	ctx.selectInfo.fromParser.add(ctx.currWord, ctx.currWordLower);
	return true;
}

bool SQLProcessor::parseWhere(SelectParserContext &ctx)
{
	return ctx.selectInfo.whereParser.add(ctx.currWord, ctx.currWordLower);
}

bool SQLProcessor::parseOrderBy(SelectParserContext &ctx)
{
	ctx.selectInfo.orderedColumns.push_back(ctx.currWord);
	return true;
}

bool SQLProcessor::parseLimit(SelectParserContext &ctx)
{
	MLPL_BUG("Not implemented: %s: %s\n", ctx.currWord.c_str(), __func__);
	return true;
}

//
// General sub routines
//
string SQLProcessor::readNextWord(SelectParserContext &ctx,
                                  ParsingPosition *position)
{
	SeparatorChecker *separator = m_selectSeprators[ctx.section];
	if (position)
		*position = ctx.selectInfo.statement.getParsingPosition();
	return ctx.selectInfo.statement.readWord(*separator);
}

bool SQLProcessor::parseColumnName(const string &name,
                                   string &baseName, string &tableVar)
{
	size_t dotPos = name.find('.');
	if (dotPos == 0) {
		MLPL_DBG("Column name begins from dot. : %s", name.c_str());
		return false;
	}
	if (dotPos == (name.size() - 1)) {
		MLPL_DBG("Column name ends with dot. : %s", name.c_str());
		return false;
	}

	if (dotPos != string::npos) {
		tableVar = string(name, 0, dotPos);
		baseName = string(name, dotPos + 1);
	} else {
		baseName = name;
	}
	return true;
}

const SQLTableInfo *
SQLProcessor::getTableInfoFromVarName(SQLSelectInfo &selectInfo,
                                      string &tableVar)
{
	map<string, const SQLTableInfo *>::iterator it;
	it = selectInfo.tableVarInfoMap.find(tableVar);
	if (it == selectInfo.tableVarInfoMap.end()) {
		MLPL_DBG("Failed to find: %s\n", tableVar.c_str());
		return NULL;
	}
	return it->second;
}

FormulaVariableDataGetter *
SQLProcessor::formulaColumnDataGetterFactory(string &name, void *priv)
{
	SQLSelectInfo *selectInfo = static_cast<SQLSelectInfo *>(priv);
	return new SQLFormulaColumnDataGetter(name, selectInfo);
}
