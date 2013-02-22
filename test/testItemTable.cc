#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include "ItemTable.h"

namespace testItemTable {

enum {
	DEFAULT_ITEM_ID,
	ITEM_ID_0,
	ITEM_ID_1,
	ITEM_ID_2,

	ITEM_ID_AGE,
	ITEM_ID_NAME,
	ITEM_ID_FAVORITE_COLOR,
	ITEM_ID_HEIGHT,
	ITEM_ID_NICKNAME,
};

struct TableStruct0 {
	int age;
	const char *name;
	const char *favoriteColor;
};

struct TableStruct1 {
	const char *name;
	int height;
	const char *nickname;
};

static TableStruct0 tableContent0[] = {
  {10, "anri", "red"},
  {20, "mai", "blue"},
};
static const size_t NUM_TABLE0 = sizeof(tableContent0) / sizeof(TableStruct0);

static TableStruct1 tableContent1[] = {
  {"anri",  150, "ann"},
  {"nobita",200, "snake"},
  {"maria", 170, "mai"},
  {"mai",   180, "maimai"},
  {"anri",  250, "tooower"},
};
static const size_t NUM_TABLE1 = sizeof(tableContent1) / sizeof(TableStruct1);

static void addItemTable0(ItemGroup *grp, TableStruct0 *table)
{
	grp->add(new ItemInt(ITEM_ID_AGE, table->age), false);
	grp->add(new ItemString(ITEM_ID_NAME, table->name), false);
	grp->add(new ItemString(ITEM_ID_FAVORITE_COLOR, table->favoriteColor), false);
}

static void addItemTable1(ItemGroup *grp, TableStruct1 *table)
{
	grp->add(new ItemString(ITEM_ID_NAME, table->name), false);
	grp->add(new ItemInt(ITEM_ID_HEIGHT, table->height), false);
	grp->add(new ItemString(ITEM_ID_NICKNAME, table->nickname), false);
}

template<typename T>
static ItemTable * addItems(T* srcTable, int numTable,
                            void (*addFunc)(ItemGroup *, T *))
{
	ItemTable *table = new ItemTable();
	for (int i = 0; i < numTable; i++) {
		ItemGroup *grp = table->addNewGroup();
		(*addFunc)(grp, &srcTable[i]);
	}
	return table;
}

template<typename T>
static void _assertItemData(const ItemGroup *itemGroup, T expected, int &idx)
{
	T val;
	ItemData *itemZ = itemGroup->getItemAt(idx);
	cut_assert_not_null(itemZ);
	idx++;
	itemZ->get(&val);
	cut_trace(cppcut_assert_equal(expected, val));
}
#define assertItemData(T, IGRP, E, IDX) \
cut_trace(_assertItemData<T>(IGRP, E, IDX))

template <typename RefDataType0, typename RefDataType1>
class AssertJoin
{
protected:
	typedef void (*AssertJoinRunner)
	               (const ItemGroup *itemGroup,
	                RefDataType0 *refData0, RefDataType1 *refData1,
	                size_t data0Index, size_t data1Index);

	RefDataType0 *m_refTable0;
	RefDataType1 *m_refTable1;
	ItemTable    *m_itemTable;
	size_t        m_numRowsRefTable0;
	size_t        m_numRowsRefTable1;
	size_t        m_refTable0Index;
	size_t        m_refTable1Index;
	void (*m_assertRunner)(const ItemGroup *itemGroup,
	                       RefDataType0 *refData0, RefDataType1 *refData1,
	                       size_t data0Index, size_t data1Index);
public:
	AssertJoin(ItemTable *itemTable,
	           RefDataType0 *refTable0, RefDataType1 *refTable1,
	           size_t numRowsRefTable0, size_t numRowsRefTable1)
	: m_itemTable(itemTable),
	  m_refTable0(refTable0),
	  m_refTable1(refTable1),
	  m_numRowsRefTable0(numRowsRefTable0),
	  m_numRowsRefTable1(numRowsRefTable1),
	  m_refTable0Index(0),
	  m_refTable1Index(0),
	  m_assertRunner(NULL)
	{
	}

	virtual ~AssertJoin()
	{
	}

	virtual void run(AssertJoinRunner runner)
	{
		m_assertRunner = runner;
		m_itemTable->foreach<AssertJoin *>(_assertForeach, this);
	}

	static bool _assertForeach(const ItemGroup *itemGroup, AssertJoin *obj)
	{
		return obj->assertForeach(itemGroup);
	}

	virtual void assertPreForeach(void)
	{
	}

	virtual void assertPostForeach(void)
	{
		m_refTable1Index++;
		if (m_refTable1Index == m_numRowsRefTable1) {
			m_refTable1Index = 0;
			m_refTable0Index++;
		}
	}

	virtual bool assertForeach(const ItemGroup *itemGroup)
	{
		assertPreForeach();
		(*m_assertRunner)(itemGroup,
		                  &m_refTable0[m_refTable0Index],
		                  &m_refTable1[m_refTable1Index],
		                  m_refTable0Index, m_refTable1Index);
		assertPostForeach();
		return true;
	}
};

typedef pair<int,int>      IntIntPair;
typedef vector<IntIntPair> IntIntPairVector;

template <typename RefDataType0, typename RefDataType1>
class AssertInnerJoin : public AssertJoin<RefDataType0, RefDataType1>
{
	IntIntPairVector &m_joinedRowsIndexVector;
	size_t            m_vectorIndex;
public:
	AssertInnerJoin(ItemTable *itemTable,
	                RefDataType0 *refTable0, RefDataType1 *refTable1,
	                size_t numRowsRefTable0, size_t numRowsRefTable1,
	                IntIntPairVector &joinedRowsIndexVector)
	: AssertJoin<RefDataType0, RefDataType1>
	    (itemTable, refTable0, refTable1,
	     numRowsRefTable0, numRowsRefTable1),
	  m_joinedRowsIndexVector(joinedRowsIndexVector),
	  m_vectorIndex(0)
	{
	}

	virtual void assertPreForeach(void)
	{
		IntIntPair &idxVector =
		  m_joinedRowsIndexVector[m_vectorIndex++];
		AssertJoin<RefDataType0, RefDataType1>::m_refTable0Index =
		  idxVector.first;
		AssertJoin<RefDataType0, RefDataType1>::m_refTable1Index =
		  idxVector.second;
	}

	virtual void assertPostForeach(void)
	{
	}
};

static void assertJoinRunner(const ItemGroup *itemGroup,
                                  TableStruct0 *refData0,
                                  TableStruct1 *refData1,
                                  size_t data0Index, size_t data1Index)
{
	int idx = 0;
	assertItemData(int,    itemGroup, refData0->age, idx);
	assertItemData(string, itemGroup, refData0->name, idx);
	assertItemData(string, itemGroup, refData0->favoriteColor, idx);
	assertItemData(string, itemGroup, refData1->name, idx);
	assertItemData(int,    itemGroup, refData1->height, idx);
	assertItemData(string, itemGroup, refData1->nickname, idx);
}

static void assertEmptyTable(ItemTable *table)
{
	cut_trace(cut_assert_not_null(table));
	cut_trace(cppcut_assert_equal((size_t)0, table->getNumberOfRows()));
	cut_trace(cppcut_assert_equal((size_t)0, table->getNumberOfColumns()));
}

static const int NUM_TABLE_POOL = 10;
static ItemTable *g_table[NUM_TABLE_POOL];
static ItemTable *&x_table = g_table[0];
static ItemTable *&y_table = g_table[1];
static ItemTable *&z_table = g_table[2];

void teardown(void)
{
	for (int i = 0; i < NUM_TABLE_POOL; i++) {
		if (g_table[i]) {
			g_table[i]->unref();
			g_table[i] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	x_table = new ItemTable();
	cppcut_assert_equal(1, x_table->getUsedCount());
}

void test_addNewGroup(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = x_table->addNewGroup();
	cut_assert_not_null(grp);
	cppcut_assert_equal(1, grp->getUsedCount());
}

void test_addNoRef(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = new ItemGroup();
	x_table->add(grp, false);
	cppcut_assert_equal(1, grp->getUsedCount());
}

void test_addRef(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = new ItemGroup();
	x_table->add(grp, true);
	cppcut_assert_equal(2, grp->getUsedCount());
	grp->unref();
}

void test_getNumberOfRows(void)
{
	x_table = new ItemTable();
	cut_assert_equal_int(0, x_table->getNumberOfRows());

	ItemGroup *grp = x_table->addNewGroup();
	cut_assert_equal_int(1, x_table->getNumberOfRows());

	grp = x_table->addNewGroup();
	cut_assert_equal_int(2, x_table->getNumberOfRows());
}

void test_getNumberOfColumns(void)
{
	x_table = new ItemTable();
	cut_assert_equal_int(0, x_table->getNumberOfColumns());

	ItemGroup *grp = x_table->addNewGroup();
	cut_assert_equal_int(0, x_table->getNumberOfColumns());
}

void test_addWhenHasMoreThanOneGroup(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = x_table->addNewGroup();
	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);

	// add second group
	grp = x_table->addNewGroup();
	const ItemGroupType *itemGroupType = grp->getItemGroupType();
	cut_assert_not_null(itemGroupType);
	cppcut_assert_equal(ITEM_TYPE_INT, itemGroupType->getType(0));
	cppcut_assert_equal(ITEM_TYPE_STRING, itemGroupType->getType(1));

	// add items
	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);

	// add thrid group
	grp = x_table->addNewGroup();
	grp->add(new ItemInt(ITEM_ID_0, -20), false);
	grp->add(new ItemString(ITEM_ID_1, "dog"), false);
	cppcut_assert_equal((size_t)3, x_table->getNumberOfRows());
}

void test_addInvalidItemsWhenHasMoreThanOneGroup(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = x_table->addNewGroup();
	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);

	// add second group
	grp = x_table->addNewGroup();
	ItemData *item = new ItemString(ITEM_ID_1, "foo");
	bool gotException = false;
	try {
		grp->add(item, false);
	} catch (invalid_argument e) {
		item->unref();
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_addItemsWhenPreviousGroupIncompletion(void)
{
	x_table = new ItemTable();
	ItemGroup *grp = x_table->addNewGroup();
	grp->add(new ItemInt(ITEM_ID_0, 500), false);
	grp->add(new ItemString(ITEM_ID_1, "foo"), false);

	// add second group
	grp = x_table->addNewGroup();
	grp->add(new ItemInt(ITEM_ID_0, 200), false);

	// add thrid group
	bool gotException = false;
	ItemData *item = new ItemInt(ITEM_ID_0, -5);
	try {
		grp->add(item, false);
	} catch (invalid_argument e) {
		item->unref();
		gotException = true;
	}
	cppcut_assert_equal(true, gotException);
}

void test_crossJoin(void)
{
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);

	y_table = addItems<TableStruct1>(tableContent1, NUM_TABLE1,
	                                 addItemTable1);

	z_table = x_table->crossJoin(y_table);
	cut_assert_not_null(z_table);

	// check the result
	size_t numColumns = x_table->getNumberOfColumns() +
	                    y_table->getNumberOfColumns();
	size_t numRows = NUM_TABLE0 * NUM_TABLE1;
	cppcut_assert_equal(numColumns, z_table->getNumberOfColumns());
	cppcut_assert_equal(numRows, z_table->getNumberOfRows());

	AssertJoin<TableStruct0, TableStruct1>
	  assertJoin(z_table, tableContent0, tableContent1,
	             NUM_TABLE0, NUM_TABLE1);
	assertJoin.run(assertJoinRunner);
}

void test_crossJoinBothEmpty(void)
{
	x_table = new ItemTable();
	y_table = new ItemTable();
	z_table = x_table->crossJoin(y_table);
	assertEmptyTable(z_table);
}

void test_crossJoinRightEmpty(void)
{
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);
	y_table = new ItemTable();
	z_table = x_table->crossJoin(y_table);
	assertEmptyTable(z_table);
}

void test_crossJoinLeftEmpty(void)
{
	x_table = new ItemTable();
	y_table = addItems<TableStruct1>(tableContent1, NUM_TABLE1,
	                                 addItemTable1);
	z_table = x_table->crossJoin(y_table);
	assertEmptyTable(z_table);
}

void test_innerJoin(void)
{
	x_table = addItems<TableStruct0>(tableContent0, NUM_TABLE0,
	                                 addItemTable0);

	y_table = addItems<TableStruct1>(tableContent1, NUM_TABLE1,
	                                 addItemTable1);

	const size_t indexLeftColumn = 1; // name
	const size_t indexRightColumn = 0; // name
	z_table = x_table->innerJoin(y_table,
	                             indexLeftColumn, indexRightColumn);
	cut_assert_not_null(z_table);

	// check the result
	IntIntPairVector joinedRowsIndexVector;
	for (size_t i = 0; i < NUM_TABLE0; i++) {
		TableStruct0 *tbl0 = &tableContent0[i];
		for (size_t j = 0; j < NUM_TABLE1; j++) {
			TableStruct1 *tbl1 = &tableContent1[j];
			if (strcmp(tbl0->name, tbl1->name) != 0)
				continue;
			joinedRowsIndexVector.push_back(IntIntPair(i,j));
		}
	}

	size_t numColumns = x_table->getNumberOfColumns() +
	                    y_table->getNumberOfColumns();
	size_t numRows = joinedRowsIndexVector.size();
	cppcut_assert_equal(numColumns, z_table->getNumberOfColumns());
	cppcut_assert_equal(numRows, z_table->getNumberOfRows());

	AssertInnerJoin<TableStruct0, TableStruct1>
	  assertJoin(z_table, tableContent0, tableContent1,
	             NUM_TABLE0, NUM_TABLE1, joinedRowsIndexVector);
	assertJoin.run(assertJoinRunner);
}

} // namespace testItemTable
