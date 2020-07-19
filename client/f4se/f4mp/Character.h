#pragma once

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

	class Character : public networking::Entity
	{
	public:
		Character(networking::Networking& net, networking::Entity::ID id, const Vector3& position, float hp, float ap, const SPECIAL& special, const Inventory& inventory);

	private:
		networking::Entity::ID id;

		float hp;
		float ap;

		SPECIAL special;
		Inventory inventory;
	};
}