#ifndef ItemGroup_h
#define ItemGroup_h

#include <map>
using namespace std;

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "ItemData.h"

typedef uint64_t ItemGroupId;
#define PRIx_ITEM_GROUP PRIx64
#define PRIu_ITEM_GROUP PRIu64

typedef map<ItemId, ItemData *> ItemDataMap;
typedef ItemDataMap::iterator   ItemDataMapIterator;

class ItemGroup;
typedef map<ItemGroupId, ItemGroup *> ItemGroupMap;
typedef ItemGroupMap::iterator        ItemGroupMapIterator;

class ItemGroup {
public:
	ItemGroup(ItemGroupId id);
	virtual ~ItemGroup();
	void add(ItemData *data);

private:
	ItemGroupId m_groupId;
	ItemDataMap m_itemMap;
};

#endif  // ItemGroup_h
