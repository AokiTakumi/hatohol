#include <cppcutter.h>
#include "SQLProcessorException.h"
#include "ExceptionTestUtils.h"

namespace testSQLProcessorException {

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_throw(void)
{
	assertThrow(SQLProcessorException, SQLProcessorException);
}

void test_throwAsHatoholException(void)
{
	assertThrow(SQLProcessorException, HatoholException);
}

void test_throwAsException(void)
{
	assertThrow(SQLProcessorException, exception);
}

} // namespace testSQLProcessorException
