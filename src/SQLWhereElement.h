#ifndef SQLWhereElement_h
#define SQLWhereElement_h

#include <PolytypeNumber.h>
using namespace mlpl;

#include "ItemDataPtr.h"

enum SQLWhereOperatorType {
	SQL_WHERE_OP_EQ,
	SQL_WHERE_OP_GT,
	SQL_WHERE_OP_GE,
	SQL_WHERE_OP_LT,
	SQL_WHERE_OP_LE,
	SQL_WHERE_OP_BETWEEN,
	SQL_WHERE_OP_AND,
	SQL_WHERE_OP_OR,
};

enum SQLWhereElementType {
	SQL_WHERE_ELEM_ELEMENT,
	SQL_WHERE_ELEM_COLUMN,
	SQL_WHERE_ELEM_NUMBER,
	SQL_WHERE_ELEM_STRING,
	SQL_WHERE_ELEM_PAIRED_NUMBER,
};

// ---------------------------------------------------------------------------
// class: SQLWhereOperator
// ---------------------------------------------------------------------------
class SQLWhereElement;
class SQLWhereOperator {
public:
	const SQLWhereOperatorType getType(void) const;
	virtual ~SQLWhereOperator();
	virtual bool evaluate(SQLWhereElement *leftHand,
	                      SQLWhereElement *rightHand) = 0;

protected:
	SQLWhereOperator(SQLWhereOperatorType type);
	bool checkType(SQLWhereElement *elem, SQLWhereElementType type) const;

private:
	const SQLWhereOperatorType m_type;
};

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorEqual
// ---------------------------------------------------------------------------
class SQLWhereOperatorEqual : public SQLWhereOperator {
public:
	SQLWhereOperatorEqual();
	virtual ~SQLWhereOperatorEqual();
	virtual bool evaluate(SQLWhereElement *leftHand,
	                      SQLWhereElement *rightHand);
};

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorAnd
// ---------------------------------------------------------------------------
class SQLWhereOperatorAnd : public SQLWhereOperator {
public:
	SQLWhereOperatorAnd();
	virtual ~SQLWhereOperatorAnd();
	virtual bool evaluate(SQLWhereElement *leftHand,
	                      SQLWhereElement *rightHand);
};

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorBetween
// ---------------------------------------------------------------------------
class SQLWhereOperatorBetween : public SQLWhereOperator {
public:
	SQLWhereOperatorBetween();
	virtual ~SQLWhereOperatorBetween();
	virtual bool evaluate(SQLWhereElement *leftHand,
	                      SQLWhereElement *rightHand);
};

// ---------------------------------------------------------------------------
// class: SQLWhereELement
// ---------------------------------------------------------------------------
class SQLWhereElement {
public:
	SQLWhereElement(SQLWhereElementType elemType = SQL_WHERE_ELEM_ELEMENT);
	virtual ~SQLWhereElement();

	SQLWhereElementType getType(void) const;
	SQLWhereElement  *getLeftHand(void) const;
	SQLWhereElement  *getRightHand(void) const;
	SQLWhereOperator *getOperator(void) const;
	SQLWhereElement  *getParent(void) const;
	void setLeftHand(SQLWhereElement *whereElem);
	void setRightHand(SQLWhereElement *whereElem);
	void setOperator(SQLWhereOperator *whereOp);
	void setParent(SQLWhereElement *whereElem);
	bool isFull(void);
	virtual bool evaluate(void);
	virtual ItemDataPtr getItemData(int index = 0);

private:
	SQLWhereElementType m_type;
	SQLWhereElement  *m_leftHand;
	SQLWhereOperator *m_operator;
	SQLWhereElement  *m_rightHand;

	SQLWhereElement  *m_parent;
};

// ---------------------------------------------------------------------------
// class: SQLWhereColumn
// ---------------------------------------------------------------------------
class SQLWhereColumn;
typedef ItemDataPtr
  (*SQLWhereColumnDataGetter)(SQLWhereColumn *whereColumn, void *priv);
typedef void
  (*SQLWhereColumnPrivDataDestructor)(SQLWhereColumn *whereColumn, void *priv);

class SQLWhereColumn : public SQLWhereElement {
public:
	SQLWhereColumn
	  (string &columnName, SQLWhereColumnDataGetter dataGetter, void *priv,
	   SQLWhereColumnPrivDataDestructor privDataDestructor = NULL);
	virtual ~SQLWhereColumn();
	const string &getColumnName(void) const;
	void *getPrivateData(void) const;
	virtual ItemDataPtr getItemData(int index = 0);

private:
	string                   m_columnName;
	SQLWhereColumnDataGetter m_valueGetter;
	void                    *m_priv;
	SQLWhereColumnPrivDataDestructor m_privDataDestructor;
};

// ---------------------------------------------------------------------------
// class: SQLWhereNumber
// ---------------------------------------------------------------------------
class SQLWhereNumber : public SQLWhereElement {
public:
	SQLWhereNumber(const PolytypeNumber &value);
	SQLWhereNumber(int value);
	SQLWhereNumber(double value);
	virtual ~SQLWhereNumber();
	const PolytypeNumber &getValue(void) const;
private:
	PolytypeNumber m_value;
};

// ---------------------------------------------------------------------------
// class: SQLWhereString
// ---------------------------------------------------------------------------
class SQLWhereString : public SQLWhereElement {
public:
	SQLWhereString(string &str);
	virtual ~SQLWhereString();
	const string &getValue(void) const;
private:
	string m_str;
};

// ---------------------------------------------------------------------------
// class: SQLWherePairedNumber
// ---------------------------------------------------------------------------
class SQLWherePairedNumber : public SQLWhereElement {
public:
	SQLWherePairedNumber(const PolytypeNumber &v0,
	                     const PolytypeNumber &v1);
	virtual ~SQLWherePairedNumber();
	const PolytypeNumber &getFirstValue(void) const;
	const PolytypeNumber &getSecondValue(void) const;
private:
	PolytypeNumber m_value0;
	PolytypeNumber m_value1;
};

#endif // SQLWhereElement_h
