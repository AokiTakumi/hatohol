#include <stdexcept>
#include "Utils.h"
#include "ItemGroup.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemGroup::ItemGroup(void)
{
}

void ItemGroup::add(ItemData *data, bool doRef)
{
	pair<ItemDataMapIterator, bool> result;
	ItemId itemId = data->getId();
	writeLock();
	result = m_itemMap.insert(pair<ItemId, ItemData *>(itemId, data));
	if (!result.second) {
		writeUnlock();
		string msg = AMSG("Failed: insert: itemId: %"PRIx_ITEM"\n",
		                  itemId);
		throw invalid_argument(msg);
	}
	m_itemVector.push_back(data);
	m_groupType.addType(data);
	writeUnlock();
	if (doRef)
		data->ref();
}

ItemData *ItemGroup::getItem(ItemId itemId) const
{
	ItemData *data = NULL;
	readLock();
	ItemDataMapConstIterator it = m_itemMap.find(itemId);
	if (it != m_itemMap.end())
		data = it->second;
	readUnlock();
	return data;
}

bool ItemGroup::compareType(const ItemGroup *itemGroup) const
{
	if (!itemGroup)
		return false;
	if (this == itemGroup)
		return true;
	readLock();
	itemGroup->readLock();
	bool ret = (m_groupType == itemGroup->m_groupType);
	itemGroup->readUnlock();
	readUnlock();

	return ret;
}

size_t ItemGroup::getNumberOfItems(void) const
{
	readLock();
	size_t ret = m_itemVector.size();
	readUnlock();
	return ret;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemGroup::~ItemGroup()
{
	// We don't need to take a lock, because this object is no longer used.
	ItemDataMapIterator it = m_itemMap.begin();
	for (; it != m_itemMap.end(); ++it) {
		ItemData *data = it->second;
		data->unref();
	}
}

