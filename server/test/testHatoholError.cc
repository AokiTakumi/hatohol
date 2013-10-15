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
#include "HatoholError.h"
using namespace std;

namespace testHatoholError {

void cut_setup(void)
{
}

void cut_teardown(void)
{
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_defaultValue(void)
{
	HatoholError err;
	cppcut_assert_equal(HTERR_UNINITIALIZED, err.getErrorCode());
	cppcut_assert_equal(true, err.getOptMessage().empty());
}

void test_getOptMessage(void)
{
	string optMsg = "Option message.";
	HatoholError err(HTERR_OK, optMsg);
	cppcut_assert_equal(optMsg, err.getOptMessage());
}

void test_operatorEqual(void)
{
	HatoholError err(HTERR_OK);
	cppcut_assert_equal(true, err == HTERR_OK);
}

void test_operatorNotEqual(void)
{
	HatoholError err;
	cppcut_assert_equal(true, err != HTERR_OK);
}


} // namespace testHatoholError
