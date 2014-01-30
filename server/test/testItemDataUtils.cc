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

#include <cppcutter.h>
#include "ItemDataUtils.h"
#include "StringUtils.h"
using namespace std;
using namespace mlpl;

namespace testItemDataUtils {

static ItemData *g_item = NULL;
void cut_teardown(void)
{
	if (g_item) {
		g_item->unref();
		g_item = NULL;
	}
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_createAsNumberInt(void)
{
	int number = 50;
	string numberStr = StringUtils::sprintf("%d", number);
	ItemDataPtr dataPtr = ItemDataUtils::createAsNumber(numberStr);
	cppcut_assert_equal(true, dataPtr.hasData());
	const ItemInt *itemInt = dynamic_cast<const ItemInt *>(&*dataPtr);
	cppcut_assert_not_null(itemInt);
	cppcut_assert_equal(number, itemInt->get());
}

void test_createAsNumberInvalid(void)
{
	string word = "ABC";
	ItemDataPtr dataPtr = ItemDataUtils::createAsNumber(word);
	cppcut_assert_equal(false, dataPtr.hasData());
}

//
// Convenient operators
//
void test_operatorShiftFromItemIntToInt(void)
{
	const int expect = -8;
	ItemDataPtr itemPtr(new ItemInt(expect), false);
	int actual;
	actual << itemPtr;
	cppcut_assert_equal(expect, actual);
}

void test_operatorShiftFromItemStringToString(void)
{
	const string expect = "Test string";
	g_item = new ItemString(expect);
	string actual;
	actual << g_item;
	cppcut_assert_equal(expect, actual);
}

} // namespace testItemDataUtils


