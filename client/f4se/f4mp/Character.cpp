#include "Character.h"

// TODO: utilize position.
f4mp::Character::Character(networking::Networking& net, networking::Entity::ID id, const Vector3& position, float hp, float ap, const SPECIAL& special, const Inventory& inventory) :
	Entity(net), id(id), hp(hp), ap(ap), special(special), inventory(inventory)
{
}
