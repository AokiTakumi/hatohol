/*
 * Copyright (C) 2014 Project Hatohol
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

#ifndef ItemGroupStream_h 
#define ItemGroupStream_h

#include <string>
#include "ItemGroup.h"

class ItemGroupStream {
	friend int &operator<<(int &lhs, ItemGroupStream &igStream);
	friend uint64_t &operator<<(uint64_t &lhs, ItemGroupStream &igStream);
	friend std::string &operator<<(std::string &lhs, ItemGroupStream &igStream);
public:
	ItemGroupStream(const ItemGroup *itemGroup);
	const ItemData *getItem(void) const;

protected:
	template <typename T>
	static T &
	substitute(T &lhs, ItemGroupStream &igStream)
	{
		const ItemData *itemData = igStream.getItem();
		igStream.m_index++;
		lhs = static_cast<T>(*itemData);
		return lhs;
	}

private:
	// To keep peformance, we don't use private context.
	const ItemGroup *m_itemGroup;
	size_t           m_index;
};

inline int &operator<<(int &lhs, ItemGroupStream &igStream)
{
	return ItemGroupStream::substitute<int>(lhs, igStream);
}

inline uint64_t &operator<<(uint64_t &lhs, ItemGroupStream &igStream)
{
	return ItemGroupStream::substitute<uint64_t>(lhs, igStream);
}

inline std::string &operator<<(std::string &lhs, ItemGroupStream &igStream)
{
	return ItemGroupStream::substitute<std::string>(lhs, igStream);
}

#endif // ItemGroupStream_h

