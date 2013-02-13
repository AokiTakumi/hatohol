/* Asura
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

#ifndef FormulaOperator_h 
#define FormulaOperator_h

#include "FormulaElement.h"
#include "ItemDataPtr.h"

// ---------------------------------------------------------------------------
// FormulaOperator
// ---------------------------------------------------------------------------
class FormulaOperator : public FormulaElement {
public:
	FormulaOperator(FormulaElementPriority prio, bool unary = false);
	virtual ~FormulaOperator();
};

// ---------------------------------------------------------------------------
// class: FormulaParenthesis
// ---------------------------------------------------------------------------
class FormulaParenthesis : public FormulaOperator {
public:
	FormulaParenthesis(void);
	virtual ~FormulaParenthesis();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// class: FormulaComparatorEqual
// ---------------------------------------------------------------------------
class FormulaComparatorEqual : public FormulaOperator {
public:
	FormulaComparatorEqual(void);
	virtual ~FormulaComparatorEqual();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// FormulaAnd
// ---------------------------------------------------------------------------
class FormulaOperatorAnd : public FormulaOperator {
public:
	FormulaOperatorAnd(void);
	virtual ~FormulaOperatorAnd();
	virtual ItemDataPtr evaluate(void);
};

// ---------------------------------------------------------------------------
// FormulaBetween
// ---------------------------------------------------------------------------
class FormulaBetween : public FormulaOperator {
public:
	FormulaBetween(ItemDataPtr v0, ItemDataPtr v1);
	virtual ~FormulaBetween();
	virtual ItemDataPtr evaluate(void);
	ItemDataPtr getV0(void) const;
	ItemDataPtr getV1(void) const;

private:
	ItemDataPtr      m_v0;
	ItemDataPtr      m_v1;
};

#endif // FormulaOperator_h
