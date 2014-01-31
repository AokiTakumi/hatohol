/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */
#include <set>

template<typename T>
void ArmZabbixAPI::makeItemVector(std::vector<T> &idVector,
                                  const ItemTable *itemTable,
                                  const ItemId itemId)
{
	// First, make a set to remove duplication
	std::set<T> idSet;
	const ItemGroupList &grpList = itemTable->getItemGroupList();
	ItemGroupListConstIterator it = grpList.begin();
	for (; it != grpList.end(); ++it) {
		const ItemData *itemData = (*it)->getItem(itemId);
		if (itemData->isNull())
			continue;
		T id = *itemData;
		idSet.insert(id);
	}

	// Then, make a set to remove duplication
	typename std::set<T>::iterator jt = idSet.begin();
	for (; jt != idSet.end(); ++jt)
		idVector.push_back(*jt);
}


