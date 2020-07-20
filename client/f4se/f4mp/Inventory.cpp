#include "Inventory.h"

f4mp::Item::Item(FormID id, const std::string& name, Type type) : id(id), name(name), type(type)
{
}

f4mp::Inventory::Inventory(const std::vector<Item::Ref>& items, const std::vector<Item::Ref>& equipped) : items(items), equipped(equipped)
{
}
