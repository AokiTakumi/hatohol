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
#include "SmartBuffer.h"
using namespace mlpl;

namespace testSmartBuffer {

static SmartBuffer *g_sbuf = NULL;

void cut_teardown(void)
{
	if (g_sbuf) {
		delete g_sbuf;
		g_sbuf = NULL;
	}
}

// ----------------------------------------------------------------------------
// test cases
// ----------------------------------------------------------------------------
void test_watermarkInit(void)
{
	SmartBuffer sbuf;
	cppcut_assert_equal((size_t)0, sbuf.watermark());
}

void test_watermarkAdd(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	sbuf.add32(3);
	cppcut_assert_equal((size_t)5, sbuf.watermark());
}

void test_watermarkIncIndex(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	sbuf.incIndex(5);
	cppcut_assert_equal((size_t)6, sbuf.watermark());
}

void test_watermarkResetIndex(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	// resetIndex() doesn't reset the watermark
	sbuf.resetIndex();
	cppcut_assert_equal((size_t)1, sbuf.watermark());
	sbuf.add32(3);
	cppcut_assert_equal((size_t)4, sbuf.watermark());
}

void test_watermarkResetIndexDeep(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	sbuf.resetIndexDeep();
	cppcut_assert_equal((size_t)0, sbuf.watermark());
	sbuf.add32(3);
	cppcut_assert_equal((size_t)4, sbuf.watermark());
}

void test_watermarkAlloc(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	cppcut_assert_equal((size_t)1, sbuf.watermark());
	sbuf.alloc(20, true);
	cppcut_assert_equal((size_t)0, sbuf.watermark());
}

void test_watermarkAllocNotDeep(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add8(3);
	cppcut_assert_equal((size_t)1, sbuf.watermark());
	sbuf.alloc(20, false);
	cppcut_assert_equal((size_t)1, sbuf.watermark());
}

void test_watermarkSetAt(void)
{
	SmartBuffer sbuf;
	sbuf.alloc(100);
	sbuf.add32(3);
	sbuf.setAt(50, 3);
	cppcut_assert_equal((size_t)4, sbuf.watermark());
}

void test_getPointer(void)
{
	SmartBuffer sbuf(10);
	for (size_t i = 0; i < 10; i++)
		sbuf.add8(2*i);
	cppcut_assert_equal((char)10, *sbuf.getPointer<char>(5));
}

void test_getPointerDefaultParam(void)
{
	SmartBuffer sbuf(10);
	for (size_t i = 0; i < 10; i++)
		sbuf.add8(2*i);
	sbuf.resetIndex();
	sbuf.incIndex(3);
	cppcut_assert_equal((char)6, *sbuf.getPointer<char>());
}

void test_getPointerWithType(void)
{
	SmartBuffer sbuf(10);
	for (size_t i = 0; i < 10; i++)
		sbuf.add8(2*i);
	sbuf.resetIndex();
	cppcut_assert_equal((uint32_t)0x06040200, *sbuf.getPointer<uint32_t>());
}

void test_takeOver(void)
{
	static const size_t buflen = 5;
	SmartBuffer sbuf(buflen);
	for (size_t i = 0; i < buflen; i++)
		sbuf.add8(i);
	const char *ptr = sbuf;
	size_t size  = sbuf.size();
	size_t index = sbuf.index();
	size_t watermark = sbuf.watermark();

	g_sbuf = sbuf.takeOver();
	cppcut_assert_equal(ptr, (const char *)(*g_sbuf));
	cppcut_assert_equal(size, g_sbuf->size());
	cppcut_assert_equal(index, g_sbuf->index());
	cppcut_assert_equal(watermark, g_sbuf->watermark());

	cppcut_assert_equal(NULL, (const char *)sbuf);
	cppcut_assert_equal((size_t)0, sbuf.size());
	cppcut_assert_equal((size_t)0, sbuf.index());
	cppcut_assert_equal((size_t)0, sbuf.watermark());
}

} // namespace testSmartBuffer
