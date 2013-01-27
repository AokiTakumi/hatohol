#include "Logger.h"
using namespace mlpl;

#include "FormulaElement.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FormulaElement::FormulaElement(void)
: m_leftHand(NULL),
  m_rightHand(NULL),
  m_operator(NULL),
  m_parent(NULL)
{
}

FormulaElement::~FormulaElement()
{
	if (m_leftHand)
		delete m_leftHand;
	if (m_rightHand)
		delete m_rightHand;
	if (m_operator)
		delete m_operator;
}

void FormulaElement::setLeftHand(FormulaElement *elem)
{
	m_leftHand = elem;
	m_leftHand->m_parent = this;
}

void FormulaElement::setRightHand(FormulaElement *elem)
{
	m_rightHand = elem;
	m_rightHand->m_parent = this;
}

void FormulaElement::setOperator(FormulaOperator *op)
{
	m_operator = op;
}

FormulaElement *FormulaElement::getLeftHand(void) const
{
	return m_leftHand;
}

FormulaElement *FormulaElement::getRightHand(void) const
{
	return m_rightHand;
}

ItemDataPtr FormulaElement::evaluate(void)
{
	if (!m_operator) {
		MLPL_DBG("m_operator: NULL.\n");
		return ItemDataPtr();
	}

	return m_operator->evaluate(m_leftHand, m_rightHand);
}

// ---------------------------------------------------------------------------
// class: FormulaColumn
// ---------------------------------------------------------------------------
FormulaColumn::FormulaColumn(string &name,
                             FormulaColumnDataGetter *columnDataGetter)
: m_name(name),
  m_columnGetter(columnDataGetter)
{
}

FormulaColumn::~FormulaColumn()
{
	if (m_columnGetter)
		delete m_columnGetter;
}

ItemDataPtr FormulaColumn::evaluate(void)
{
	return m_columnGetter->getData(this);
}

const string &FormulaColumn::getName(void) const
{
	return m_name;
}

FormulaColumnDataGetter *FormulaColumn::getFormulaColumnGetter(void) const
{
	return m_columnGetter;
}
