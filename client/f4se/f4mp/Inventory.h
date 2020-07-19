#pragma once

#include "F4MP.h"

#include <vector>
#include <memory>

namespace f4mp
{
	class Item
	{
	public:
		using Ref = std::shared_ptr<Item>;
		using Type = UInt32;

		Item(FormID id, const std::string& name, Type type);

	private:
		FormID id;

		std::string name;
		
		Type type;
	};

	class Inventory
	{
	public:
		Inventory(const std::vector<Item::Ref>& items, const std::vector<Item::Ref>& equipped);

	private:
		std::vector<Item::Ref> items;
		std::vector<Item::Ref> equipped;
	};
}