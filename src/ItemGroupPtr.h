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

#ifndef ItemGroupPtr_h
#define ItemGroupPtr_h

#include <Logger.h>
using namespace mlpl;

#include "ItemGroup.h"
#include "ItemPtr.h"

typedef ItemPtr<ItemGroup>       VariableItemGroupPtr;
typedef ItemPtr<const ItemGroup> ItemGroupPtr;

template<>
ItemPtr<ItemGroup>::ItemPtr(void);

#endif // #define ItemGroupPtr_h

