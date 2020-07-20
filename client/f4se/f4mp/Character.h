#pragma once

#include "Entity.h"
#include "Inventory.h"

#include <array>

namespace f4mp
{
	class SPECIAL
	{
		enum Type
		{
			Strength,
			Perception,
			Endurance,
			Charisma,
			Intelligence,
			Agility,
			Luck
		};

	private:
		std::array<int, 7> special;
	};

	class Character : public Entity
	{
	public:
		Character(const Vector3& position, float hp, float ap, const SPECIAL& special, const Inventory& inventory);

	private:
		float hp;
		float ap;

		SPECIAL special;
		Inventory inventory;
	};
}