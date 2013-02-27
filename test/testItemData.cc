#include <string>
#include <vector>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <cutter.h>
#include <cppcutter.h>
#include "ItemData.h"

namespace testItemData {

class ItemIntTester : public ItemInt {
public:
	static bool destructorCalled;

	ItemIntTester(ItemId id, int data)
	: ItemInt(id, data)
	{
	}

	~ItemIntTester() {
		destructorCalled = true;
	}
};

static const int TEST_ITEM_ID = 1;

bool ItemIntTester::destructorCalled = false;

const static int NUM_ITEM_POOL = 10;
static ItemData *g_item[NUM_ITEM_POOL];
static ItemData *&x_item = g_item[0];
static ItemData *&y_item = g_item[1];
static ItemData *&z_item = g_item[2];
static ItemData *&w_item = g_item[3];

template<typename NativeType, class ItemDataType>
static void _assertGet(NativeType val)
{
	ItemData *item = new ItemDataType(val);
	NativeType readValue;
	item->get(&readValue);
	cppcut_assert_equal(val, readValue);
}
#define assertGet(NT,IDT,V) cut_trace((_assertGet<NT,IDT>(V)))

template <typename T, typename ItemDataType>
void assertOperatorPlus(T &v0, T &v1)
{
	x_item = new ItemDataType(v0);
	y_item = new ItemDataType(v1);
	z_item = *x_item + *y_item;
	cppcut_assert_not_null(z_item);
	ItemDataType *item = dynamic_cast<ItemDataType *>(z_item);
	cppcut_assert_not_null(item);
	cppcut_assert_equal(v0 + v1, item->get());
}

void teardown(void)
{
	for (int i = 0; i < NUM_ITEM_POOL; i++) {
		if (g_item[i]) {
			g_item[i]->unref();
			g_item[i] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructWithoutSpecificId(void)
{
	int val = 2080;
	ItemData *item = new ItemUint64(val);
	cppcut_assert_equal(SYSTEM_ITEM_ID_ANONYMOUS, item->getId());
}

// -------------------------------------------------------------------------
// get
// -------------------------------------------------------------------------
void test_getBool(void)
{
	assertGet(bool, ItemBool, true);
}

void test_getInt(void)
{
	assertGet(int, ItemInt, 12345);
}

void test_getUint64(void)
{
	assertGet(uint64_t, ItemUint64, 0xfedcba9876543210);
}

void test_getDouble(void)
{
	assertGet(double, ItemDouble, 0.98);
}

void test_getString(void)
{
	assertGet(string, ItemString, "test String");
}

// -------------------------------------------------------------------------
// set
// -------------------------------------------------------------------------
void test_setUint64(void)
{
	uint64_t val = 0xfedcba9876543210;
	ItemData *item = new ItemUint64(1, val);
	val = 0x89abcdef01234567;
	uint64_t readValue;
	item->set(&val);
	item->get(&readValue);
	cut_assert_equal_int_least64(val, readValue);
}

void test_setInt(void)
{
	int val = 12345;
	ItemData *item = new ItemInt(1, val);
	val = 88;
	int readValue;
	item->set(&val);
	item->get(&readValue);
	cut_assert_equal_int(val, readValue);
}

void test_setDouble(void)
{
	double val = -5.2;
	ItemData *item = new ItemDouble(val);
	val = -4.8;
	double readValue;
	item->set(&val);
	item->get(&readValue);
	cppcut_assert_equal(val, readValue);
}

void test_setString(void)
{
	string val = "test String";
	ItemData *item = new ItemString(1, val);
	val = "FOO";
	string readValue;
	item->set(&val);
	item->get(&readValue);
	cut_assert_equal_string(val.c_str(), readValue.c_str());
}

// -------------------------------------------------------------------------
// getValue
// -------------------------------------------------------------------------
void test_getValueInt(void)
{
	int val = 2345;
	ItemData *item = new ItemInt(TEST_ITEM_ID, val);
	ItemInt *itemInt = dynamic_cast<ItemInt *>(item);
	cppcut_assert_not_null(itemInt);
	cppcut_assert_equal(val, itemInt->get());
}

void test_getValueDouble(void)
{
	double val = 5.8213e3;
	ItemData *item = new ItemDouble(val);
	ItemDouble *itemDouble = dynamic_cast<ItemDouble *>(item);
	cppcut_assert_not_null(itemDouble);
	cppcut_assert_equal(val, itemDouble->get());
}

void test_getValueString(void)
{
	string val = "dog cat bird";
	ItemData *item = new ItemString(TEST_ITEM_ID, val);
	ItemString *itemString = dynamic_cast<ItemString *>(item);
	cppcut_assert_not_null(itemString);
	cppcut_assert_equal(val, itemString->get());
}

void test_getStringBool(void)
{
	bool val = false;
	ItemData *item = new ItemBool(TEST_ITEM_ID, val);
	stringstream ss;
	ss << val;
	cppcut_assert_equal(ss.str(), item->getString());
}

void test_getStringInt(void)
{
	int val = -50;
	ItemData *item = new ItemInt(TEST_ITEM_ID, val);
	stringstream ss;
	ss << val;
	cppcut_assert_equal(ss.str(), item->getString());
}

void test_getStringUint64(void)
{
	uint64_t val = 0x1234567890abcdef;
	ItemData *item = new ItemUint64(TEST_ITEM_ID, val);
	stringstream ss;
	ss << val;
	cppcut_assert_equal(ss.str(), item->getString());
}

void test_getStringString(void)
{
	string val = "music";
	ItemData *item = new ItemString(TEST_ITEM_ID, val);
	cppcut_assert_equal(val, item->getString());
}

void test_getId(void)
{
	int id = 5;
	ItemData *item = new ItemInt(id, -5);
	cut_assert_equal_int(id, item->getId());
}

void testRefUnref(void)
{
	ItemIntTester *tester = new ItemIntTester(1, 10);
	tester->destructorCalled = false;
	cppcut_assert_equal(1, tester->getUsedCount());
	tester->ref();
	cppcut_assert_equal(2, tester->getUsedCount());
	cppcut_assert_equal(false, tester->destructorCalled);
	tester->unref();
	cppcut_assert_equal(1, tester->getUsedCount());
	cppcut_assert_equal(false, tester->destructorCalled);
	tester->unref();
	cppcut_assert_equal(true, tester->destructorCalled);
}

void test_operatorGtIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorGtIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorLtIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorLtIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorGeIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operatorGeIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operatorLeIntPositive(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operatorLeIntNegative(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, -15);
	y_item = new ItemInt(TEST_ITEM_ID, -20);
	z_item = new ItemInt(TEST_ITEM_ID, -15);
	w_item = new ItemInt(TEST_ITEM_ID, -10);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operatorEqInt(void)
{
	x_item = new ItemInt(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 20);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 10);
	cppcut_assert_not_equal(*x_item, *y_item);
	cppcut_assert_equal    (*x_item, *z_item);
	cppcut_assert_not_equal(*x_item, *w_item);
}

void test_operatorGtUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorLtUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorGeUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operatorLeUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 10);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operatorEqUint64(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemUint64(TEST_ITEM_ID, 20);
	z_item = new ItemUint64(TEST_ITEM_ID, 15);
	w_item = new ItemUint64(TEST_ITEM_ID, 10);
	cppcut_assert_not_equal(*x_item, *y_item);
	cppcut_assert_equal    (*x_item, *z_item);
	cppcut_assert_not_equal(*x_item, *w_item);
}

void test_operatorGtUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item > *y_item);
	cppcut_assert_equal(false, *x_item > *z_item);
	cppcut_assert_equal(false, *x_item > *w_item);
}

void test_operatorLtUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item < *y_item);
	cppcut_assert_equal(false, *x_item < *z_item);
	cppcut_assert_equal(true,  *x_item < *w_item);
}

void test_operatorGeUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(true,  *x_item >= *y_item);
	cppcut_assert_equal(true,  *x_item >= *z_item);
	cppcut_assert_equal(false, *x_item >= *w_item);
}

void test_operatorLeUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 10);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 20);
	cppcut_assert_equal(false, *x_item <= *y_item);
	cppcut_assert_equal(true,  *x_item <= *z_item);
	cppcut_assert_equal(true,  *x_item <= *w_item);
}

void test_operatorEqUint64Int(void)
{
	x_item = new ItemUint64(TEST_ITEM_ID, 15);
	y_item = new ItemInt(TEST_ITEM_ID, 20);
	z_item = new ItemInt(TEST_ITEM_ID, 15);
	w_item = new ItemInt(TEST_ITEM_ID, 10);
	cppcut_assert_not_equal(*x_item, *y_item);
	cppcut_assert_equal    (*x_item, *z_item);
	cppcut_assert_not_equal(*x_item, *w_item);
}


void test_operatorEqBool(void)
{
	x_item = new ItemBool(true);
	y_item = new ItemBool(true);
	cppcut_assert_equal(*x_item, *y_item);
}

void test_operatorNotEqBool(void)
{
	x_item = new ItemBool(true);
	y_item = new ItemBool(false);
	cppcut_assert_not_equal(*x_item, *y_item);
}

void test_operatorPlusInt(void)
{
	int v0 = 20;
	int v1 = -50;
	assertOperatorPlus<int, ItemInt>(v0, v1);
}

void test_operatorPlusUint64(void)
{
	uint64_t v0 = 58320;
	uint64_t v1 = 250;
	assertOperatorPlus<uint64_t, ItemUint64>(v0, v1);
}

} // namespace testItemData
