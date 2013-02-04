#include <cstdio>
#include "ItemData.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ItemId ItemData::getId(void) const
{
	return m_itemId;
}

const ItemDataType &ItemData::getItemType(void) const
{
	return m_itemType;
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
ItemData::ItemData(ItemId id, ItemDataType type)
: m_itemId(id),
  m_itemType(type)
{
}

ItemData::~ItemData()
{
}

// ---------------------------------------------------------------------------
// Public methods (ItemGeneric)
// ---------------------------------------------------------------------------
template<> string ItemInt::getString(void) const
{
	return StringUtils::sprintf("%d", m_data);
};

template<> string ItemUint64::getString(void) const
{
	return StringUtils::sprintf("%"PRIu64, m_data);
};

//
// ItemInt
//
template<> bool ItemInt::operator >(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data > data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemInt::operator <(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data < data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemInt::operator >=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data >= data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemInt::operator <=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data <= data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemInt::operator ==(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		return (m_data == data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

//
// ItemUint64
//
template<> bool ItemUint64::operator >(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data > data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return true;
		}
		return (m_data > (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemUint64::operator <(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data < data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return true;
		}
		return (m_data < (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemUint64::operator >=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data >= data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return true;
		}
		return (m_data >= (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}

	return false;
}

template<> bool ItemUint64::operator <=(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data <= data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return false;
		}
		return (m_data <= (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

template<> bool ItemUint64::operator ==(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_UINT64) {
		uint64_t data;
		itemData.get(&data);
		return (m_data == data);
	} else if (itemData.getItemType() == ITEM_TYPE_INT) {
		int data;
		itemData.get(&data);
		if (data < 0) {
			MLPL_WARN("'data' is negative. "
			          "The result may not be wrong.");
			return false;
		}
		return (m_data == (uint64_t)data);
	} else {
		MLPL_BUG("Not implemented: %s type: %d\n",
		         __PRETTY_FUNCTION__, itemData.getItemType());
	}
	return false;
}

//
// ItemString
//
template<> bool ItemString::operator ==(ItemData &itemData) const
{
	if (itemData.getItemType() == ITEM_TYPE_STRING) {
		string data;
		itemData.get(&data);
		return (m_data == data);
	}
	else {
		MLPL_DBG("type is not String: %d\n", itemData.getItemType());
		return false;
	}
	return false;
}
