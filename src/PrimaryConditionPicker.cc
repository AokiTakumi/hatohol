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

#include "PrimaryConditionPicker.h"
#include "SQLUtils.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
PrimaryConditionPicker::~PrimaryConditionPicker()
{
	PrimaryConditionListIterator it = m_primaryConditionList.begin();
	for (; it != m_primaryConditionList.end(); ++it)
		delete *it;
}

void PrimaryConditionPicker::pickupPrimary
  (FormulaElement *formula)
{
	picupPrimaryRecursive(m_primaryConditionList, formula);
}

const PrimaryConditionList &
PrimaryConditionPicker::getPrimaryConditionList(void) const
{
	return m_primaryConditionList;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
bool PrimaryConditionPicker::picupPrimaryRecursive
  (PrimaryConditionList &primaryConditionList, FormulaElement *formula)
{
	FormulaComparatorEqual *compEq =
	   dynamic_cast<FormulaComparatorEqual *>(formula);
	if (compEq)
		return pickupComparatorEqual(primaryConditionList, compEq);

	FormulaOperatorAnd *operatorAnd =
	   dynamic_cast<FormulaOperatorAnd *>(formula);
	if (operatorAnd)
		return pickupOperatorAnd(primaryConditionList, operatorAnd);

	FormulaIn *formulaIn = dynamic_cast<FormulaIn *>(formula);
	if (formulaIn)
		return pickupFormulaIn(primaryConditionList, formulaIn);

	return false;
}

bool PrimaryConditionPicker::pickupComparatorEqual
  (PrimaryConditionList &primaryConditionList, FormulaComparatorEqual *compEq)
{
	FormulaVariable *leftVariable =
	   dynamic_cast<FormulaVariable *>(compEq->getLeftHand());
	if (!leftVariable)
		return false;

	FormulaVariable *rightVariable =
	   dynamic_cast<FormulaVariable *>(compEq->getRightHand());
	if (!rightVariable)
		return false;

	PrimaryCondition *primaryCondition = new PrimaryCondition();
	SQLUtils::decomposeTableAndColumn(leftVariable->getName(),
	                                  primaryCondition->leftTableName,
	                                  primaryCondition->leftColumnName);
	SQLUtils::decomposeTableAndColumn(rightVariable->getName(),
	                                  primaryCondition->rightTableName,
	                                  primaryCondition->rightColumnName);
	primaryConditionList.push_back(primaryCondition);
	return true;
}

bool PrimaryConditionPicker::pickupOperatorAnd
  (PrimaryConditionList &primaryConditionList, FormulaOperatorAnd *operatorAnd)
{
	FormulaElement *leftElem = operatorAnd->getLeftHand();
	picupPrimaryRecursive(primaryConditionList, leftElem);

	FormulaElement *rightElem = operatorAnd->getRightHand();
	picupPrimaryRecursive(primaryConditionList, rightElem);
	return true;
}

bool PrimaryConditionPicker::pickupFormulaIn
  (PrimaryConditionList &primaryConditionList, FormulaIn *formulaIn)
{
	FormulaVariable *leftVariable =
	   dynamic_cast<FormulaVariable *>(formulaIn->getLeftHand());
	if (!leftVariable)
		return false;

	// TODO: make a PrimaryCondition instance and push it to the list.

	return true;
}
