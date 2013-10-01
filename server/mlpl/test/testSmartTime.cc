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
#include <sys/time.h>
#include <errno.h>
#include <cmath>
#include "SmartTime.h"

using namespace mlpl;

namespace testSmartTime {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_getCurrTime(void)
{
	static const double ALLOWED_ERROR = 1e-3;
	timeval tv;
	cppcut_assert_equal(0, gettimeofday(&tv, NULL),
	                    cut_message("errno: %d", errno)); 
	SmartTime smtime = SmartTime::getCurrTime();
	double diff = tv.tv_sec + tv.tv_usec/1e6;
	diff -= smtime.getAsSec();
	cppcut_assert_equal(true, fabs(diff) < ALLOWED_ERROR,
	                    cut_message("smtime: %e, diff: %e",
	                                smtime.getAsSec(), diff)); 
}

void test_constructorTimespec(void)
{
	timespec ts;
	ts.tv_sec = 1379641056;
	ts.tv_nsec = 987654321;
	SmartTime smtime(ts);

	double actual = smtime.getAsSec();
	int actualInt = (int)actual;
	cppcut_assert_equal((int)ts.tv_sec, actualInt);
	int  actualDecimalPartUsec = (actual - actualInt) * 1e6;
	cppcut_assert_equal((int)(ts.tv_nsec/1e3), actualDecimalPartUsec);
}

void test_constructorNone(void)
{
	SmartTime smtime;
	cppcut_assert_equal(0.0, smtime.getAsSec());
}

} // namespace testSmartTime
