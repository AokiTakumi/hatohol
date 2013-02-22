#include "Helpers.h"

template <typename RefDataType0, typename RefDataType1>
class AssertJoin
{
protected:
	typedef void (*AssertJoinRunner)
	               (const ItemGroup *itemGroup,
	                RefDataType0 *refData0, RefDataType1 *refData1,
	                size_t data0Index, size_t data1Index);

	RefDataType0 *m_refTable0;
	RefDataType1 *m_refTable1;
	ItemTable    *m_itemTable;
	size_t        m_numRowsRefTable0;
	size_t        m_numRowsRefTable1;
	size_t        m_refTable0Index;
	size_t        m_refTable1Index;
	void (*m_assertRunner)(const ItemGroup *itemGroup,
	                       RefDataType0 *refData0, RefDataType1 *refData1,
	                       size_t data0Index, size_t data1Index);
public:
	AssertJoin(ItemTable *itemTable,
	           RefDataType0 *refTable0, RefDataType1 *refTable1,
	           size_t numRowsRefTable0, size_t numRowsRefTable1)
	: m_itemTable(itemTable),
	  m_refTable0(refTable0),
	  m_refTable1(refTable1),
	  m_numRowsRefTable0(numRowsRefTable0),
	  m_numRowsRefTable1(numRowsRefTable1),
	  m_refTable0Index(0),
	  m_refTable1Index(0),
	  m_assertRunner(NULL)
	{
	}

	virtual ~AssertJoin()
	{
	}

	virtual void run(AssertJoinRunner runner)
	{
		m_assertRunner = runner;
		m_itemTable->foreach<AssertJoin *>(_assertForeach, this);
	}

	static bool _assertForeach(const ItemGroup *itemGroup, AssertJoin *obj)
	{
		return obj->assertForeach(itemGroup);
	}

	virtual void assertPreForeach(void)
	{
	}

	virtual void assertPostForeach(void)
	{
		m_refTable1Index++;
		if (m_refTable1Index == m_numRowsRefTable1) {
			m_refTable1Index = 0;
			m_refTable0Index++;
		}
	}

	virtual bool assertForeach(const ItemGroup *itemGroup)
	{
		assertPreForeach();
		(*m_assertRunner)(itemGroup,
		                  &m_refTable0[m_refTable0Index],
		                  &m_refTable1[m_refTable1Index],
		                  m_refTable0Index, m_refTable1Index);
		assertPostForeach();
		return true;
	}
};

template <typename RefDataType0, typename RefDataType1>
class AssertInnerJoin : public AssertJoin<RefDataType0, RefDataType1>
{
	IntIntPairVector &m_joinedRowsIndexVector;
	size_t            m_vectorIndex;
public:
	AssertInnerJoin(ItemTable *itemTable,
	                RefDataType0 *refTable0, RefDataType1 *refTable1,
	                size_t numRowsRefTable0, size_t numRowsRefTable1,
	                IntIntPairVector &joinedRowsIndexVector)
	: AssertJoin<RefDataType0, RefDataType1>
	    (itemTable, refTable0, refTable1,
	     numRowsRefTable0, numRowsRefTable1),
	  m_joinedRowsIndexVector(joinedRowsIndexVector),
	  m_vectorIndex(0)
	{
	}

	virtual void assertPreForeach(void)
	{
		IntIntPair &idxVector =
		  m_joinedRowsIndexVector[m_vectorIndex++];
		AssertJoin<RefDataType0, RefDataType1>::m_refTable0Index =
		  idxVector.first;
		AssertJoin<RefDataType0, RefDataType1>::m_refTable1Index =
		  idxVector.second;
	}

	virtual void assertPostForeach(void)
	{
	}
};

