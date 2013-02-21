#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cppcutter.h>
#include "SQLFromParser.h"
#include "Asura.h"
#include "AsuraException.h"

namespace testSQLFromParser {

template<typename T>
static void assertTableFormulaType(SQLTableFormula *obj)
{
	cut_assert_not_null(obj);
	cppcut_assert_equal
	  (true, typeid(T) == typeid(*obj),
	   cut_message("actual type: %s (%s)",
	               DEMANGLED_TYPE_NAME(*obj), TYPE_NAME(*obj)));
}

#define assertCrossJoin(X) \
cut_trace(assertTableFormulaType<SQLTableCrossJoin>(X))

static void _assertTableElement
  (SQLTableFormula *tableFormula,
   const string &expectedTableName,
   const string &expectedTableVarName = StringUtils::EMPTY_STRING)
{
	cut_trace(assertTableFormulaType<SQLTableElement>(tableFormula));
	SQLTableElement *tableElem =
	  dynamic_cast<SQLTableElement *>(tableFormula);
	cppcut_assert_not_null(tableElem);
	cppcut_assert_equal(expectedTableName, tableElem->getName());
	cppcut_assert_equal(expectedTableVarName, tableElem->getVarName());
}

#define assertTableElement(X, N, ...) \
cut_trace(_assertTableElement(X, N, ##__VA_ARGS__))

static void _assertInnerJoin
  (SQLTableFormula *tableFormula,
   const string &expectedTableName0, const string &expectedColumnName0,
   const string &expectedTableName1, const string &expectedColumnName1)
{
	cut_trace(assertTableFormulaType<SQLTableInnerJoin>(tableFormula));
	SQLTableInnerJoin *innerJoin =
	  dynamic_cast<SQLTableInnerJoin *>(tableFormula);
	cppcut_assert_not_null(innerJoin);
	cppcut_assert_equal(expectedTableName0, innerJoin->getLeftTableName());
	cppcut_assert_equal(expectedColumnName0, innerJoin->getLeftColumnName());
	cppcut_assert_equal(expectedTableName1, innerJoin->getRightTableName());
	cppcut_assert_equal(expectedColumnName1, innerJoin->getRightColumnName());
}
#define assertInnerJoin(X, N0, F0, N1, F1) \
cut_trace(_assertInnerJoin(X, N0, F0, N1, F1))

static void _assertInputStatement(SQLFromParser &fromParser,
                                  ParsableString &statement)
{
	SeparatorCheckerWithCallback *separator =
	  fromParser.getSeparatorChecker();

	try {
		while (!statement.finished()) {
			string word = statement.readWord(*separator);
			string lower = StringUtils::toLower(word);
			fromParser.add(word, lower);
		}
		fromParser.close();
	} catch (const AsuraException &e) {
		cut_fail("Exception: <%s:%d> %s\n",
		         e.getSourceFileName().c_str(), e.getLineNumber(),
		         e.what());
	}
}
#define assertInputStatement(P,S) cut_trace(_assertInputStatement(P,S))

#define DEFINE_PARSER_AND_RUN(PARSER, TABLE_FORMULA, STATEMENT) \
ParsableString _statement(STATEMENT); \
SQLFromParser PARSER; \
assertInputStatement(PARSER, _statement); \
SQLTableFormula *TABLE_FORMULA = PARSER.getTableFormula(); \
cppcut_assert_not_null(TABLE_FORMULA);

void setup(void)
{
	asuraInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_oneTable(void)
{
	const char *tableName = "tab";
	string statement = StringUtils::sprintf("from %s", tableName);
	DEFINE_PARSER_AND_RUN(fromParser, tableFormula, statement);
	assertTableElement(tableFormula, tableName);
}

void test_oneTableWithVar(void)
{
	const char *tableName = "tab";
	const char *varName = "t";
	string statement = StringUtils::sprintf("from %s %s", 
	                                        tableName, varName);
	DEFINE_PARSER_AND_RUN(fromParser, tableFormula, statement);
	assertTableElement(tableFormula, tableName, varName);
}

void test_twoTables(void)
{
	const char *tableName0 = "tab0";
	const char *tableName1 = "tab1";
	string statement = StringUtils::sprintf("from %s,%s",
	                                        tableName0, tableName1);
	DEFINE_PARSER_AND_RUN(fromParser, tableFormula, statement);
	assertCrossJoin(tableFormula);
	SQLTableCrossJoin *crossJoin =
	  dynamic_cast<SQLTableCrossJoin *>(tableFormula);
	cppcut_assert_not_null(crossJoin);
	assertTableElement(crossJoin->getLeftFormula(), tableName0);
	assertTableElement(crossJoin->getRightFormula(), tableName1);
}

void test_twoTablesWithVars(void)
{
	const char *tableName0 = "tab0";
	const char *varName0 = "t0";
	const char *tableName1 = "tab1";
	const char *varName1 = "t1";
	string statement = StringUtils::sprintf("from %s %s, %s %s",
	                                        tableName0, varName0,
	                                        tableName1, varName1);
	DEFINE_PARSER_AND_RUN(fromParser, tableFormula, statement);
	assertCrossJoin(tableFormula);
	SQLTableCrossJoin *crossJoin =
	  dynamic_cast<SQLTableCrossJoin *>(tableFormula);
	cppcut_assert_not_null(crossJoin);
	assertTableElement(crossJoin->getLeftFormula(), tableName0, varName0);
	assertTableElement(crossJoin->getRightFormula(), tableName1, varName1);
}

void test_twoTablesInnerJoin(void)
{
	const char *tableName0 = "tab0";
	const char *tableName1 = "tab1";
	const char *field0 = "field0";
	const char *field1 = "field1";
	string statement =
	  StringUtils::sprintf("from %s inner join %s on %s.%s=%s.%s",
	                       tableName0, tableName1,
	                       tableName0, field0, tableName1, field1);
	DEFINE_PARSER_AND_RUN(fromParser, tableFormula, statement);
	assertInnerJoin(tableFormula, tableName0, field0, tableName1, field1);
	SQLTableInnerJoin *innerJoin =
	  dynamic_cast<SQLTableInnerJoin *>(tableFormula);
	cppcut_assert_not_null(innerJoin);
	assertTableElement(innerJoin->getLeftFormula(), tableName0);
	assertTableElement(innerJoin->getRightFormula(), tableName1);
}

} // namespace testSQLFromParser
