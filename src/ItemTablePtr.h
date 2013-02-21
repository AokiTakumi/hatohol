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

#ifndef ItemTablePtr_h
#define ItemTablePtr_h

#include <list>
using namespace std;

#include "ItemPtr.h"
#include "ItemTable.h"

typedef ItemPtr<ItemTable> ItemTablePtr;

typedef list<ItemTablePtr>                 ItemTablePtrList;
typedef list<ItemTablePtr>::iterator       ItemTablePtrListIterator;
typedef list<ItemTablePtr>::const_iterator ItemTablePtrListConstIterator;

ItemTablePtr
innerJoin     (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1,
               size_t indexLeftJoinColumn, size_t indexRightJoinColumn);

ItemTablePtr
leftOuterJoin (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

ItemTablePtr
rightOuterJoin(const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

ItemTablePtr
fullOuterJoin (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

ItemTablePtr
crossJoin     (const ItemTablePtr &tablePtr0, const ItemTablePtr &tablePtr1);

template<>
ItemPtr<ItemTable>::ItemPtr(void);

#endif // #define ItemTablePtr_h

