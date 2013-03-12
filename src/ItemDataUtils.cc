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

#include "ItemDataUtils.h"

// ---------------------------------------------------------------------------
// ItemDataUtils
// ---------------------------------------------------------------------------
ItemDataPtr ItemDataUtils::createAsNumber(const string &word)
{
	bool isFloat;
	if (!StringUtils::isNumber(word, &isFloat))
		return ItemDataPtr();

	if (!isFloat) {
		int number = atoi(word.c_str());
		return ItemDataPtr(new ItemInt(ITEM_ID_NOBODY, number), false);
	}

	double number = atof(word.c_str());
	MLPL_BUG("Not implemented: %s (%f)\n", __PRETTY_FUNCTION__, number);
	return ItemDataPtr();
}

ItemDataPtr ItemDataUtils::createAsNumberOrString(const string &word)
{
	bool isFloat;
	if (!StringUtils::isNumber(word, &isFloat))
		return ItemDataPtr(new ItemString(word), false);

	// TODO: avoid call isNumber() twice (here and in createAsNumber()).
	return createAsNumber(word);
}

// ---------------------------------------------------------------------------
// ItemDataIndex
// ---------------------------------------------------------------------------
ItemDataIndex::ItemDataIndex(ItemDataIndexType type)
: m_type(type),
  m_index(NULL),
  m_multiIndex(NULL)
{
}

ItemDataIndex::~ItemDataIndex()
{
	if (m_index)
		delete m_index;
	if (m_multiIndex)
		delete m_multiIndex;
}

bool ItemDataIndex::insert(const ItemData *itemData,
                           ItemGroup* itemGroup)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}

bool ItemDataIndex::find(const ItemData *itemData,
                         vector<ItemDataPtrForIndex> &foundItems)
{
	MLPL_BUG("Not implemented: %s\n", __PRETTY_FUNCTION__);
	return false;
}
