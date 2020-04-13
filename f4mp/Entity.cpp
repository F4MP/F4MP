#include "Entity.h"
#include "Player.h"
#include "NPC.h"

f4mp::Entity* f4mp::Entity::Get(librg_event* event)
{
	return Get(event->entity);
}

f4mp::Entity* f4mp::Entity::Get(librg_entity* entity)
{
	if (entity == nullptr)
	{
		return nullptr;
	}

	return (Entity*)entity->user_data;
}

f4mp::Entity::~Entity()
{
}

f4mp::Entity* f4mp::Entity::Create(librg_event* event)
{
	Entity* entity = nullptr;

	switch (event->entity->type)
	{
	case EntityType::Player:
		entity = new Player();
		break;

	case EntityType::NPC:
		entity = new NPC();
		break;
	}

	event->entity->user_data = entity;

	entity->OnEntityCreate(event);

	return entity;
}

void f4mp::Entity::OnConnectRequest(librg_event* event)
{
}

void f4mp::Entity::OnConnectAccept(librg_event* event)
{
}

void f4mp::Entity::OnDisonnect(librg_event* event)
{
	event->entity->user_data = nullptr;
}

void f4mp::Entity::OnEntityCreate(librg_event* event)
{
}

void f4mp::Entity::OnEntityUpdate(librg_event* event)
{
}

void f4mp::Entity::OnEntityRemove(librg_event* event)
{
	delete this;
	event->entity->user_data = nullptr;
}

void f4mp::Entity::OnClientUpdate(librg_event* event)
{
}
