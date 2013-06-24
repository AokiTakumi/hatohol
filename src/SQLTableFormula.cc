/* Hatohol
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

#include "SQLTableFormula.h"
#include "SQLProcessorException.h"
#include "HatoholException.h"

// ---------------------------------------------------------------------------
// SQLRowIterator
// ---------------------------------------------------------------------------
SQLTableRowIterator::~SQLTableRowIterator()
{
}

// ---------------------------------------------------------------------------
// SQLRowIteratorColumnsEqual
// ---------------------------------------------------------------------------
SQLTableRowIteratorColumnsEqual::SQLTableRowIteratorColumnsEqual
  (SQLTableProcessContext *otherTableCtx, int otherIndex, int myIndex)
: m_otherTableCtx(otherTableCtx),
  m_otherIndex(otherIndex),
  m_myIndex(myIndex)
{
}

bool SQLTableRowIteratorColumnsEqual::isIndexed
  (const ItemDataIndexVector &indexVector)
{
	if (indexVector.size() < m_myIndex) {
		THROW_HATOHOL_EXCEPTION(
		  "indexVector.size (%zd) < m_myIndex (%d)",
		  indexVector.size(), m_myIndex);
	}
	ItemDataIndex *itemIndex = indexVector[m_myIndex];
	if (itemIndex->getIndexType() == ITEM_DATA_INDEX_TYPE_NONE)
		return false;
	m_itemDataIndex = itemIndex;
	return true;
}

int SQLTableRowIteratorColumnsEqual::getNumberOfRows(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return -1;
}

bool SQLTableRowIteratorColumnsEqual::start(void)
{
	SQLTableElement *leftTable = m_otherTableCtx->tableElement;
	ItemGroupPtr leftRow = leftTable->getActiveRow();
	ItemDataPtr leftItem = leftRow->getItemAt(m_otherIndex);
	clearIndexingVariables();
	m_itemDataIndex->find(leftItem, m_indexMatchedItems);
	if (m_indexMatchedItems.empty())
		return false; // Not found
	return true;
}

ItemGroupPtr SQLTableRowIteratorColumnsEqual::getRow(void)
{
	size_t index = m_indexMatchedItemsIndex;
	ItemDataPtrForIndex &dataForIndex = m_indexMatchedItems[index];
	return dataForIndex.itemGroupPtr;
}

void SQLTableRowIteratorColumnsEqual::clearIndexingVariables(void)
{
	m_indexMatchedItems.clear();
	m_indexMatchedItemsIndex = 0;
}

bool SQLTableRowIteratorColumnsEqual::increment(void)
{
	size_t numMatchedItems = m_indexMatchedItems.size();
	m_indexMatchedItemsIndex++;
	if (m_indexMatchedItemsIndex >= numMatchedItems)
		return false;
	return true;
}

// ---------------------------------------------------------------------------
// SQLRowIteratorConstants
// ---------------------------------------------------------------------------
SQLTableRowIteratorConstants::SQLTableRowIteratorConstants
  (size_t columnIndex, const ItemGroupPtr &values)
: m_columnIndex(columnIndex),
  m_values(values)
{
}

bool SQLTableRowIteratorConstants::isIndexed
  (const ItemDataIndexVector &indexVector)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

int SQLTableRowIteratorConstants::getNumberOfRows(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return -1;
}

bool SQLTableRowIteratorConstants::start(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

ItemGroupPtr SQLTableRowIteratorConstants::getRow(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return ItemGroupPtr();
}

bool SQLTableRowIteratorConstants::increment(void)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

// ---------------------------------------------------------------------------
// SQLTableProcessContext
// ---------------------------------------------------------------------------
SQLTableProcessContext::SQLTableProcessContext(void)
: id(-1),
  tableElement(NULL)
{
}

SQLTableProcessContext::~SQLTableProcessContext()
{
	for (size_t i = 0; i < rowIteratorVector.size(); i++)
		delete rowIteratorVector[i];
}

// ---------------------------------------------------------------------------
// SQLTableProcessContextIndex
// ---------------------------------------------------------------------------
SQLTableProcessContextIndex::~SQLTableProcessContextIndex()
{
	clear();
}

void SQLTableProcessContextIndex::clear(void)
{
	for (size_t i = 0; i < tableCtxVector.size(); i++)
		delete tableCtxVector[i];
	tableCtxVector.clear();
	tableNameCtxMap.clear();
	tableCtxVector.clear();
}

SQLTableProcessContext *
SQLTableProcessContextIndex::getTableContext(const string &name,
                                             bool throwExceptionIfNotFound)
{
	map<string, SQLTableProcessContext *>::iterator it =
	  tableVarCtxMap.find(name);
	if (it != tableVarCtxMap.end())
		return it->second;
	it = tableNameCtxMap.find(name);
	if (it != tableNameCtxMap.end())
		return it->second;

	if (tableCtxVector.size() == 1 && name.empty())
		return tableCtxVector[0];

	if (throwExceptionIfNotFound) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found table: %s", name.c_str());
	}
	return NULL;
}

// ---------------------------------------------------------------------------
// SQLTableFormula
// ---------------------------------------------------------------------------
SQLTableFormula::~SQLTableFormula()
{
	TableSizeInfoVectorIterator it = m_tableSizeInfoVector.begin();
	for (; it != m_tableSizeInfoVector.end(); ++it)
		delete *it;
}

void SQLTableFormula::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
}

size_t SQLTableFormula::getColumnIndexOffset(const string &tableName)
{
	if (m_tableSizeInfoVector.empty())
		fixupTableSizeInfo();

	TableSizeInfoMapIterator it = m_tableVarSizeInfoMap.find(tableName);
	bool found = false;
	if (it != m_tableVarSizeInfoMap.end())
		found = true;
	if (!found) {
		it = m_tableSizeInfoMap.find(tableName);
		if (it != m_tableSizeInfoMap.end())
			found = true;
	}
	if (!found) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "Not found table: %s", tableName.c_str());
	}
	TableSizeInfo *tableSizeInfo = it->second;
	return tableSizeInfo->accumulatedColumnOffset;
}

const SQLTableFormula::TableSizeInfoVector &
  SQLTableFormula::getTableSizeInfoVector(void)
{
	if (m_tableSizeInfoVector.empty())
		fixupTableSizeInfo();
	return m_tableSizeInfoVector;
}

void SQLTableFormula::addTableSizeInfo(const string &tableName,
                                       const string &tableVar,
                                       size_t numColumns)
{
	size_t accumulatedColumnOffset = 0;
	if (!m_tableSizeInfoVector.empty()) {
		TableSizeInfo *prev = m_tableSizeInfoVector.back();
		accumulatedColumnOffset =
		  prev->accumulatedColumnOffset + prev->numColumns;
	}

	TableSizeInfo *tableSizeInfo = new TableSizeInfo();
	tableSizeInfo->name = tableName;
	tableSizeInfo->varName = tableVar;
	tableSizeInfo->numColumns = numColumns;
	tableSizeInfo->accumulatedColumnOffset = accumulatedColumnOffset;

	m_tableSizeInfoVector.push_back(tableSizeInfo);
	m_tableSizeInfoMap[tableName] = tableSizeInfo;
	if (!tableVar.empty())
		m_tableVarSizeInfoMap[tableVar] = tableSizeInfo;
}

void SQLTableFormula::makeTableSizeInfo(const TableSizeInfoVector &leftList,
                                        const TableSizeInfoVector &rightList)
{
	if (!m_tableSizeInfoVector.empty()) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "m_tableSizeInfoVector: Not empty.");
	}
	TableSizeInfoVectorConstIterator it = leftList.begin();
	for (; it != leftList.end(); ++it) {
		const TableSizeInfo *tableSizeInfo = *it;
		addTableSizeInfo(tableSizeInfo->name,
		                 tableSizeInfo->varName,
		                 tableSizeInfo->numColumns);
	}
	for (it = rightList.begin(); it != rightList.end(); ++it) {
		const TableSizeInfo *tableSizeInfo = *it;
		addTableSizeInfo(tableSizeInfo->name,
		                 tableSizeInfo->varName,
		                 tableSizeInfo->numColumns);
	}
}

// ---------------------------------------------------------------------------
// SQLTableElement
// ---------------------------------------------------------------------------
SQLTableElement::SQLTableElement(const string &name, const string &varName,
                                 SQLColumnIndexResoveler *resolver,
                                 SQLSubQueryMode subQueryMode)
: m_name(name),
  m_varName(varName),
  m_columnIndexResolver(resolver),
  m_tableProcessCtx(NULL),
  m_selectedRowIterator(NULL),
  m_subQueryMode(subQueryMode)
{
}

const string &SQLTableElement::getName(void) const
{
	return m_name;
}

const string &SQLTableElement::getVarName(void) const
{
	return m_varName;
}

// This function is called from SQLProcessorSelect::makeItemTables()
void SQLTableElement::setItemTable(ItemTablePtr itemTablePtr)
{
	m_itemTablePtr = itemTablePtr;
}

void SQLTableElement::selectRowIterator(SQLTableProcessContext *tableCtx)
{
	if (tableCtx->rowIteratorVector.empty())
		return;

	// This function is called from
	//   SQLProcessorSelect::pickupColumnComparisons()
	//     -> SQLFromParser::prepareJoin()
	// So m_itemTablePtr must have a valid value here.
	if (!m_itemTablePtr->hasIndex())
		return;
	const ItemDataIndexVector &indexVector =
	  m_itemTablePtr->getIndexVector();

	// we pickup SQLTableRowIterator instances that can use the indexes.
	vector<SQLTableRowIterator *> indexedRowIterators;
	for (size_t i = 0; i < tableCtx->rowIteratorVector.size(); i++) {
		SQLTableRowIterator *rowIterator =
		   tableCtx->rowIteratorVector[i];
		if (!rowIterator->isIndexed(indexVector))
			continue;
		indexedRowIterators.push_back(rowIterator);
	}
	if (indexedRowIterators.empty())
		return;

	// select the row iterator whose number of rows is minimum
	int minNumRows = indexedRowIterators[0]->getNumberOfRows();
	m_selectedRowIterator = indexedRowIterators[0];
	for (size_t i = 1; i < tableCtx->rowIteratorVector.size(); i++) {
		int numRows = indexedRowIterators[i]->getNumberOfRows();
		if (numRows < minNumRows) {
			minNumRows = numRows;
			m_selectedRowIterator = indexedRowIterators[i];
		}
	}
}

void SQLTableElement::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
}

ItemTablePtr SQLTableElement::getTable(void)
{
	return m_itemTablePtr;
}

ItemGroupPtr SQLTableElement::getActiveRow(void)
{
	// Non-indexing mode
	if (!isIndexingMode())
		return *m_currSelectedGroup;

	// Indexing mode
	return m_selectedRowIterator->getRow();
}

void SQLTableElement::startRowIterator(void)
{
	m_currSelectedGroup = m_itemTablePtr->getItemGroupList().begin();

	// Non-indexing mode
	if (!isIndexingMode())
		return;

	// Indexing mode
	if (!m_selectedRowIterator->start()) // not found
		m_currSelectedGroup = m_itemTablePtr->getItemGroupList().end();
}

bool SQLTableElement::rowIteratorEnd(void)
{
	return m_currSelectedGroup == m_itemTablePtr->getItemGroupList().end();
}

void SQLTableElement::rowIteratorInc(void)
{
	// Non-indexing mode
	if (!isIndexingMode()) {
		++m_currSelectedGroup;
		return;
	}

	// Indexing mode
	if (!m_selectedRowIterator->increment()) // reaches the end
		m_currSelectedGroup = m_itemTablePtr->getItemGroupList().end();
}

void SQLTableElement::fixupTableSizeInfo(void)
{
	string &name = m_varName.empty() ? m_name : m_varName;
	size_t numColumns = m_columnIndexResolver->getNumberOfColumns(name);
	addTableSizeInfo(m_name, m_varName, numColumns);
}

void SQLTableElement::setSQLTableProcessContext(SQLTableProcessContext *joinedTableCtx)
{
	m_tableProcessCtx = joinedTableCtx;
}

bool SQLTableElement::isIndexingMode(void)
{
	return m_selectedRowIterator;
}

void SQLTableElement::forceSetRowIterator(SQLTableRowIterator *rowIterator)
{
	// We set rowIterator even when the rowIterator uses indexes.
	const ItemDataIndexVector &indexVector =
	  m_itemTablePtr->getIndexVector();
	if (!rowIterator->isIndexed(indexVector))
		return;

	m_selectedRowIterator = rowIterator;
}

// ---------------------------------------------------------------------------
// SQLTableJoin
// ---------------------------------------------------------------------------
SQLTableJoin::SQLTableJoin(SQLJoinType type)
: m_type(type),
  m_leftFormula(NULL),
  m_rightFormula(NULL)
{
}

SQLTableFormula *SQLTableJoin::getLeftFormula(void) const
{
	return m_leftFormula;
}

SQLTableFormula *SQLTableJoin::getRightFormula(void) const
{
	return m_rightFormula;
}

void SQLTableJoin::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
	m_leftFormula->prepareJoin(ctxIndex);
	m_rightFormula->prepareJoin(ctxIndex);
}

void SQLTableJoin::setLeftFormula(SQLTableFormula *tableFormula)
{
	m_leftFormula = tableFormula;
}

void SQLTableJoin::setRightFormula(SQLTableFormula *tableFormula)
{
	m_rightFormula = tableFormula;
}

ItemGroupPtr SQLTableJoin::getActiveRow(void)
{
	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}

	VariableItemGroupPtr itemGroup;
	ItemGroupPtr leftRow = leftFormula->getActiveRow();
	for (size_t i = 0; i < leftRow->getNumberOfItems(); i++)
		itemGroup->add(leftRow->getItemAt(i));
	ItemGroupPtr rightRow = rightFormula->getActiveRow();
	for (size_t i = 0; i < rightRow->getNumberOfItems(); i++)
		itemGroup->add(rightRow->getItemAt(i));

	itemGroup->freeze();
	return ItemGroupPtr(itemGroup);
}

void SQLTableJoin::fixupTableSizeInfo(void)
{
	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}
	makeTableSizeInfo(leftFormula->getTableSizeInfoVector(),
	                  rightFormula->getTableSizeInfoVector());
}

// ---------------------------------------------------------------------------
// SQLTableCrossJoin
// ---------------------------------------------------------------------------
SQLTableCrossJoin::SQLTableCrossJoin(void)
: SQLTableJoin(SQL_JOIN_TYPE_CROSS)
{
}

ItemTablePtr SQLTableCrossJoin::getTable(void)
{
	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}
	return crossJoin(leftFormula->getTable(), rightFormula->getTable());
}

// ---------------------------------------------------------------------------
// SQLTableInnerJoin
// ---------------------------------------------------------------------------
SQLTableInnerJoin::SQLTableInnerJoin
  (const string &leftTableName, const string &leftColumnName,
   const string &rightTableName, const string &rightColumnName,
   SQLColumnIndexResoveler *resolver)
: SQLTableJoin(SQL_JOIN_TYPE_INNER),
  m_leftTableName(leftTableName),
  m_leftColumnName(leftColumnName),
  m_rightTableName(rightTableName),
  m_rightColumnName(rightColumnName),
  m_indexLeftJoinColumn(INDEX_NOT_SET),
  m_indexRightJoinColumn(INDEX_NOT_SET),
  m_columnIndexResolver(resolver),
  m_rightTableElement(NULL)
{
}

void SQLTableInnerJoin::prepareJoin(SQLTableProcessContextIndex *ctxIndex)
{
	// Current implementation of inner join overwrites selectedRowIterator.
	// However, the combination of them may improve performance.
	SQLTableProcessContext *leftTableCtx =
	  ctxIndex->getTableContext(m_leftTableName);
	SQLTableProcessContext *rightTableCtx =
	  ctxIndex->getTableContext(m_rightTableName);
	if (!leftTableCtx || !rightTableCtx) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftTableCtx (%s:%p) or lightTableCtx (%s:%p) is NULL.\n",
		  m_leftTableName.c_str(), leftTableCtx,
		  m_rightTableName.c_str(), rightTableCtx);
	}

	size_t leftColumnIdx =
	  m_columnIndexResolver->getIndex(m_leftTableName,
	                                  m_leftColumnName);
	size_t rightColumnIdx =
	  m_columnIndexResolver->getIndex(m_rightTableName,
	                                  m_rightColumnName);
	m_rightTableElement = rightTableCtx->tableElement;
	SQLTableRowIteratorColumnsEqual *rowIterator = 
	  new SQLTableRowIteratorColumnsEqual(leftTableCtx, leftColumnIdx,
	                                      rightColumnIdx);
	rightTableCtx->rowIteratorVector.push_back(rowIterator);
	m_rightTableElement->forceSetRowIterator(rowIterator);

	// call this function in chain
	SQLTableJoin::prepareJoin(ctxIndex);

	//
	// The following lines are for getActiveRows()
	//

	// The following code is copied from getTable(). We want to
	// extract same part as a function.
	if (!m_columnIndexResolver)
		THROW_HATOHOL_EXCEPTION("m_columnIndexResolver: NULL");

	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}

	if (m_indexLeftJoinColumn == INDEX_NOT_SET) {
		m_indexLeftJoinColumn =
		  m_columnIndexResolver->getIndex(m_leftTableName,
		                                  m_leftColumnName);
		m_indexLeftJoinColumn +=
		   leftFormula->getColumnIndexOffset(m_leftTableName);
	}
	if (m_indexRightJoinColumn == INDEX_NOT_SET) {
		m_indexRightJoinColumn =
		  m_columnIndexResolver->getIndex(m_rightTableName,
		                                  m_rightColumnName);
		m_indexRightJoinColumn +=
		  rightFormula->getColumnIndexOffset(m_rightTableName);
	}
}

ItemTablePtr SQLTableInnerJoin::getTable(void)
{
	if (!m_columnIndexResolver)
		THROW_HATOHOL_EXCEPTION("m_columnIndexResolver: NULL");

	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();
	if (!leftFormula || !rightFormula) {
		THROW_SQL_PROCESSOR_EXCEPTION(
		  "leftFormula (%p) or rightFormula (%p) is NULL.\n",
		  leftFormula, rightFormula);
	}

	if (m_indexLeftJoinColumn == INDEX_NOT_SET) {
		m_indexLeftJoinColumn =
		  m_columnIndexResolver->getIndex(m_leftTableName,
		                                  m_leftColumnName);
		m_indexLeftJoinColumn +=
		   leftFormula->getColumnIndexOffset(m_leftTableName);
	}
	if (m_indexRightJoinColumn == INDEX_NOT_SET) {
		m_indexRightJoinColumn =
		  m_columnIndexResolver->getIndex(m_rightTableName,
		                                  m_rightColumnName);
		m_indexRightJoinColumn +=
		  rightFormula->getColumnIndexOffset(m_rightTableName);
	}

	return innerJoin(leftFormula->getTable(), rightFormula->getTable(),
	                 m_indexLeftJoinColumn, m_indexRightJoinColumn);
}

ItemGroupPtr SQLTableInnerJoin::getActiveRow(void)
{
	// Right table is in an indexing mode, we can skip the condition check.
	if (m_rightTableElement->isIndexingMode())
		return  SQLTableJoin::getActiveRow();

	SQLTableFormula *leftFormula = getLeftFormula();
	SQLTableFormula *rightFormula = getRightFormula();

	ItemGroupPtr leftGrpPtr = leftFormula->getActiveRow();
	if (!leftGrpPtr.hasData())
		return leftGrpPtr;
	const ItemData *leftData = leftGrpPtr->getItemAt(m_indexLeftJoinColumn);
	ItemGroupPtr rightGrpPtr = rightFormula->getActiveRow();
	const ItemData *rightData =
	   rightGrpPtr->getItemAt(m_indexRightJoinColumn);
	if (!rightGrpPtr.hasData())
		return rightGrpPtr;

	if (*leftData != *rightData)
		return ItemGroupPtr(NULL);

	// TODO: The following function also gets active rows of the children.
	//       So it should be fixed more efficiently.
	return  SQLTableJoin::getActiveRow();
}

const string &SQLTableInnerJoin::getLeftTableName(void) const
{
	return m_leftTableName;
}

const string &SQLTableInnerJoin::getLeftColumnName(void) const
{
	return m_leftColumnName;
}

const string &SQLTableInnerJoin::getRightTableName(void) const
{
	return m_rightTableName;
}

const string &SQLTableInnerJoin::getRightColumnName(void) const
{
	return m_rightColumnName;
}
