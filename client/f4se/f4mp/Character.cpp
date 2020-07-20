#include "Character.h"

// TODO: utilize position.
f4mp::Character::Character(const Vector3& position, float hp, float ap, const SPECIAL& special, const Inventory& inventory) :
	Entity(position), hp(hp), ap(ap), special(special), inventory(inventory)
{
}
