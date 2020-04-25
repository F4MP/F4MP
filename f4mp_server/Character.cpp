#include "Character.h"

f4mp::Character::Character()
{

}

void f4mp::Character::OnEntityUpdate(librg_event* event)
{
	Entity::OnEntityUpdate(event);

	Utils::Write(event->data, nodeTransforms);
	Utils::Write(event->data, transformDeltaTime);
}

void f4mp::Character::OnClientUpdate(librg_event* event)
{
	Entity::OnClientUpdate(event);

	Utils::Read(event->data, nodeTransforms);
	Utils::Read(event->data, transformDeltaTime);
}