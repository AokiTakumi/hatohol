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
#include "AsuraException.h"
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

class SQLProcessorColumnIndexResolver : public SQLColumnIndexResoveler {
public:
	SQLProcessorColumnIndexResolver(TableNameStaticInfoMap &nameInfoMap)
	: m_tableNameStaticInfoMap(nameInfoMap)
	{
	}

	virtual int getIndex(const string &tableName,
	                     const string &columnName) const
	{
		TableNameStaticInfoMapIterator it;
		it = m_tableNameStaticInfoMap.find(tableName);
		if (it == m_tableNameStaticInfoMap.end()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Not found table: %s\n", tableName.c_str());
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		return SQLUtils::getColumnIndex(columnName, staticInfo);
	}

private:
	TableNameStaticInfoMap &m_tableNameStaticInfoMap;
};

struct SQLProcessor::PrivateContext {
	SQLProcessor       *sqlProcessor;
	SQLSelectInfo      *selectInfo;

	SelectParseSection  section;
	string              currWord;
	string              currWordLower;

	SQLProcessorColumnIndexResolver columnIndexResolver;

	// currently processed Item Group used in selectMatchingRows()
	ItemGroupPtr        evalTargetItemGroup;

	// The number of times to be masked for generating output lines.
	size_t              makeTextRowsWriteMaskCount;

	// methods
	PrivateContext(SQLProcessor *sqlProc,
	               TableNameStaticInfoMap &nameInfoMap)
	: sqlProcessor(sqlProc),
	  selectInfo(NULL),
	  section(SELECT_PARSING_SECTION_COLUMN),
	  columnIndexResolver(nameInfoMap),
	  evalTargetItemGroup(NULL),
	  makeTextRowsWriteMaskCount(0)
	{
	}
};

class SQLFormulaColumnDataGetter : public FormulaVariableDataGetter {
public:
	SQLFormulaColumnDataGetter(string &name,
	                           SQLSelectInfo *selectInfo,
	                           ItemGroupPtr &evalTargetItemGroup)
	: m_evalTargetItemGroup(evalTargetItemGroup),
	  m_columnInfo(NULL)
	{
		SQLColumnNameMapIterator it
		  = selectInfo->columnNameMap.find(name);
		if (it != selectInfo->columnNameMap.end()) {
			m_columnInfo = it->second;
			return;
		}

		// The owner of the SQLColumnInfo object created below
		// is *selectInfo. So its deletion is done
		// in the destructor of the SQLSelectInfo object.
		// No need to delete in this class.
		m_columnInfo = new SQLColumnInfo(name);
		selectInfo->columnNameMap[name] = m_columnInfo;
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
		return ItemDataPtr(m_evalTargetItemGroup->getItem(itemId));
	}

	SQLColumnInfo *getColumnInfo(void) const
	{
		return m_columnInfo;
	}

private:
	ItemGroupPtr  &m_evalTargetItemGroup;
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
		checkParsedResult();

		// set members in SQLFormulaColumnDataGetter
		fixupColumnNameMap();

		// associate each column with the table
		associateColumnWithTable();

		// associate each table with static table information
		associateTableWithStaticInfo();

		// make ItemTable objects for all specified tables
		setColumnTypeAndBaseDefInColumnInfo();
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
		MLPL_DBG("Got SQLProcessorException: <%s:%d> %s\n",
		         e.getSourceFileName().c_str(),
		         e.getLineNumber(), message);
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
: m_ctx(NULL),
  m_separatorSpaceComma(" ,"),
  m_tableNameStaticInfoMap(tableNameStaticInfoMap),
  m_processorInsert(tableNameStaticInfoMap),
  m_processorUpdate(tableNameStaticInfoMap)
{
	m_ctx = new PrivateContext(this, tableNameStaticInfoMap);

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
	if (m_ctx)
		delete m_ctx;
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
	m_ctx->selectInfo = &selectInfo;

	// set ColumnDataGetterFactory
	selectInfo.columnParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, m_ctx);
	selectInfo.whereParser.setColumnDataGetterFactory
	  (formulaColumnDataGetterFactory, m_ctx);

	// callback function for column and where section
	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_COLUMN]
	  = selectInfo.columnParser.getSeparatorChecker();

	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_FROM]
	  = selectInfo.fromParser.getSeparatorChecker();

	m_selectSeprators[SQLProcessor::SELECT_PARSING_SECTION_WHERE]
	  = selectInfo.whereParser.getSeparatorChecker();

	while (!selectInfo.statement.finished()) {
		m_ctx->currWord = readNextWord();
		if (m_ctx->currWord.empty())
			continue;

		// check if this is a keyword.
		m_ctx->currWordLower = StringUtils::toLower(m_ctx->currWord);
		it = m_selectSectionParserMap.find(m_ctx->currWordLower);
		if (it != m_selectSectionParserMap.end()) {
			// When the function returns 'true', it means
			// the current word is section keyword and
			subParser = it->second;
			if ((this->*subParser)())
				continue;
		}

		// parse each component
		if (m_ctx->section >= NUM_SELECT_PARSING_SECTION) {
			THROW_ASURA_EXCEPTION(
			  "section(%d) >= NUM_SELECT_PARSING_SECTION\n",
			  m_ctx->section);
		}
		subParser = m_selectSubParsers[m_ctx->section];
		if (!(this->*subParser)())
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

void SQLProcessor::checkParsedResult(void) const
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	if (selectInfo->columnNameMap.empty())
		THROW_SQL_PROCESSOR_EXCEPTION("Not found: columns.");

	if (selectInfo->tables.empty())
		THROW_SQL_PROCESSOR_EXCEPTION("Not found: tables.");
}

void SQLProcessor::fixupColumnNameMap(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLColumnNameMapIterator it = selectInfo->columnNameMap.begin();
	for (; it != selectInfo->columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;
		parseColumnName(columnInfo->name, columnInfo->baseName,
		                columnInfo->tableVar);
		columnInfo->setColumnType();
	}
}

void SQLProcessor::associateColumnWithTable(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLColumnNameMapIterator it = selectInfo->columnNameMap.begin();
	for (; it != selectInfo->columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;
		if (columnInfo->columnType == SQLColumnInfo::COLUMN_TYPE_ALL)
			continue;

		// set SQLColumnInfo::tableInfo and SQLTableInfo::columnList.
		if (selectInfo->tables.size() == 1) {
			SQLTableInfo *tableInfo = *selectInfo->tables.begin();
			if (columnInfo->tableVar.empty())
				columnInfo->associate(tableInfo);
			else if (columnInfo->tableVar == tableInfo->varName)
				columnInfo->associate(tableInfo);
			else {
				THROW_SQL_PROCESSOR_EXCEPTION(
				  "columnInfo.tableVar (%s) != "
				  "tableInfo.varName (%s)\n",
				  columnInfo->tableVar.c_str(),
				  tableInfo->varName.c_str());
			}
			continue;
		}

		const SQLTableInfo *tableInfo
		  = getTableInfoFromVarName(*selectInfo, columnInfo->tableVar);
		columnInfo->associate(const_cast<SQLTableInfo *>(tableInfo));
	}
}

void SQLProcessor::associateTableWithStaticInfo(void)
{
	SQLTableInfoListIterator tblInfoIt = m_ctx->selectInfo->tables.begin();
	for (; tblInfoIt != m_ctx->selectInfo->tables.end(); ++tblInfoIt) {
		SQLTableInfo *tableInfo = *tblInfoIt;
		TableNameStaticInfoMapIterator it;
		it = m_tableNameStaticInfoMap.find(tableInfo->name);
		if (it == m_tableNameStaticInfoMap.end()) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "Not found table: %s\n", tableInfo->name.c_str());
		}
		const SQLTableStaticInfo *staticInfo = it->second;
		tableInfo->staticInfo = staticInfo;
	}
}

void SQLProcessor::setColumnTypeAndBaseDefInColumnInfo(void)
{
	SQLSelectInfo *selectInfo = m_ctx->selectInfo;
	SQLColumnNameMapIterator it = selectInfo->columnNameMap.begin();
	for (; it != selectInfo->columnNameMap.end(); ++it) {
		SQLColumnInfo *columnInfo = it->second;

		// baseDef
		if (columnInfo->columnType != SQLColumnInfo::COLUMN_TYPE_NORMAL)
			continue;

		if (!columnInfo->tableInfo) {
			THROW_SQL_PROCESSOR_EXCEPTION(
			  "columnInfo->stableInfo is NULL\n");
		}

		const SQLTableStaticInfo *staticInfo =
		  columnInfo->tableInfo->staticInfo;
		if (!staticInfo)
			THROW_SQL_PROCESSOR_EXCEPTION("staticInfo is NULL\n");

		columnInfo->columnBaseDef =
		  SQLUtils::getColumnBaseDefinition(columnInfo->baseName,
		                                    staticInfo);
	}
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
	return selectInfo.joinedTable->foreach<PrivateContext *>
	                                    (pickupMatchingRows, m_ctx);
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
		m_ctx->makeTextRowsWriteMaskCount =
		  selectInfo.selectedTable->getNumberOfRows() - 1;
	}
	bool ret;
	ret = selectInfo.selectedTable->foreach<PrivateContext *>
	                                       (makeTextRows, m_ctx);
	return ret;
}

bool SQLProcessor::pickupMatchingRows(const ItemGroup *itemGroup,
                                      PrivateContext *ctx)
{
	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	ctx->evalTargetItemGroup = nonConstItemGroup;
	FormulaElement *formula = ctx->selectInfo->whereParser.getFormula();
	ItemDataPtr result = formula->evaluate();
	if (!result.hasData()) {
		MLPL_DBG("result has no data.\n");
		return false;
	}
	if (*result == *ctx->selectInfo->itemFalsePtr)
		return true;
	ctx->selectInfo->selectedTable->add(nonConstItemGroup);
	return true;
}

bool SQLProcessor::makeTextRows(const ItemGroup *itemGroup,
                                PrivateContext *ctx)
{
	SQLSelectInfo *selectInfo = ctx->selectInfo;
	bool doOutput = false;
	if (ctx->makeTextRowsWriteMaskCount == 0)
		doOutput = true;
	else
		ctx->makeTextRowsWriteMaskCount--;

	ItemGroup *nonConstItemGroup = const_cast<ItemGroup *>(itemGroup);
	ctx->evalTargetItemGroup = nonConstItemGroup;

	StringVector textVector;
	for (size_t i = 0; i < selectInfo->outputColumnVector.size(); i++) {
		const SQLOutputColumn &outputColumn =
		  selectInfo->outputColumnVector[i];
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
		selectInfo->textRows.push_back(textVector);
	return true;
}

//
// Select status parsers
//
bool SQLProcessor::parseSectionColumn(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_COLUMN;
	return true;
}

bool SQLProcessor::parseSectionFrom(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_FROM;
	m_ctx->selectInfo->fromParser.setColumnIndexResolver
	  (&m_ctx->columnIndexResolver);
	return true;
}

bool SQLProcessor::parseSectionWhere(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_WHERE;
	return true;
}

bool SQLProcessor::parseSectionOrder(void)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(&currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		m_ctx->selectInfo->statement.setParsingPosition(currPos);
		return false;
	}

	m_ctx->section = SELECT_PARSING_SECTION_ORDER_BY;
	return true;
}

bool SQLProcessor::parseSectionGroup(void)
{
	ParsingPosition currPos;
	string nextWord = readNextWord(&currPos);
	if (nextWord.empty())
		return false;

	if (!StringUtils::casecmp(nextWord, "by")) {
		m_ctx->selectInfo->statement.setParsingPosition(currPos);
		return false;
	}

	m_ctx->section = SELECT_PARSING_SECTION_GROUP_BY;
	return true;
}

bool SQLProcessor::parseSectionLimit(void)
{
	m_ctx->section = SELECT_PARSING_SECTION_LIMIT;
	return true;
}

//
// Select statment parsers
//
bool SQLProcessor::parseSelectedColumns(void)
{
	return m_ctx->selectInfo->columnParser.add(m_ctx->currWord,
	                                           m_ctx->currWordLower);
}

bool SQLProcessor::parseGroupBy(void)
{
	MLPL_BUG("Not implemented: GROUP_BY\n");
	return false;
}

bool SQLProcessor::parseFrom(void)
{
	m_ctx->selectInfo->fromParser.add(m_ctx->currWord,
	                                  m_ctx->currWordLower);
	return true;
}

bool SQLProcessor::parseWhere(void)
{
	return m_ctx->selectInfo->whereParser.add(m_ctx->currWord,
	                                          m_ctx->currWordLower);
}

bool SQLProcessor::parseOrderBy(void)
{
	m_ctx->selectInfo->orderedColumns.push_back(m_ctx->currWord);
	return true;
}

bool SQLProcessor::parseLimit(void)
{
	MLPL_BUG("Not implemented: %s: %s\n",
	         m_ctx->currWord.c_str(), __func__);
	return true;
}

//
// General sub routines
//
string SQLProcessor::readNextWord(ParsingPosition *position)
{
	SeparatorChecker *separator = m_selectSeprators[m_ctx->section];
	if (position)
		*position = m_ctx->selectInfo->statement.getParsingPosition();
	return m_ctx->selectInfo->statement.readWord(*separator);
}

void SQLProcessor::parseColumnName(const string &name,
                                   string &baseName, string &tableVar)
{
	size_t dotPos = name.find('.');
	if (dotPos == 0) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Column name begins from dot. : %s", name.c_str());
	}
	if (dotPos == (name.size() - 1)) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Column name ends with dot. : %s", name.c_str());
	}

	if (dotPos != string::npos) {
		tableVar = string(name, 0, dotPos);
		baseName = string(name, dotPos + 1);
	} else {
		baseName = name;
	}
}

const SQLTableInfo *
SQLProcessor::getTableInfoFromVarName(const SQLSelectInfo &selectInfo,
                                      const string &tableVar)
{
	SQLTableVarNameInfoMapConstIterator it
	  = selectInfo.tableVarInfoMap.find(tableVar);
	if (it == selectInfo.tableVarInfoMap.end()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Failed to find: %s\n", tableVar.c_str());
	}
	return it->second;
}

FormulaVariableDataGetter *
SQLProcessor::formulaColumnDataGetterFactory(string &name, void *priv)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(priv);
	SQLSelectInfo *selectInfo = ctx->selectInfo;
	return new SQLFormulaColumnDataGetter(name, selectInfo,
	                                      ctx->evalTargetItemGroup);
}
