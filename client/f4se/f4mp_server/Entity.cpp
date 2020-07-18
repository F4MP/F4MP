#include "Entity.h"
#include "Player.h"
#include "NPC.h"

f4mp::Entity* f4mp::Entity::Get(librg_entity* entity)
{
	if (entity == nullptr)
	{
		return nullptr;
	}

	return (Entity*)entity->user_data;
}

f4mp::Entity* f4mp::Entity::Get(librg_event* event)
{
	return Get(event->entity);
}

f4mp::Entity* f4mp::Entity::Get(librg_peer* peer)
{
	return (Entity*)peer->data;
}

f4mp::Entity* f4mp::Entity::Create(librg_entity* entity)
{
	Entity* instance = nullptr;

	switch (entity->type)
	{
	case EntityType::Player:
		instance = new Player();
		break;

	case EntityType::NPC:
		instance = new NPC();
		break;
	}

	return Create(entity, instance);
}

f4mp::Entity* f4mp::Entity::Create(librg_event* event)
{
	return Create(event->entity);
}

f4mp::Entity::Entity() : entityID((u32)-1)
{

}

f4mp::Entity::~Entity()
{

}

void f4mp::Entity::OnConnectRequest(librg_event* event)
{
}

void f4mp::Entity::OnConnectAccept(librg_event* event)
{
}

void f4mp::Entity::OnConnectRefuse(librg_event* event)
{
}

void f4mp::Entity::OnDisonnect(librg_event* event)
{
	delete this;
	event->entity->user_data = nullptr;
}

void f4mp::Entity::OnEntityCreate(librg_event* event)
{
}

void f4mp::Entity::OnEntityUpdate(librg_event* event)
{
	librg_data_wf32(event->data, angles[0]);
	librg_data_wf32(event->data, angles[1]);
	librg_data_wf32(event->data, angles[2]);
}

void f4mp::Entity::OnEntityRemove(librg_event* event)
{
}

void f4mp::Entity::OnClientUpdate(librg_event* event)
{
	angles[0] = librg_data_rf32(event->data);
	angles[1] = librg_data_rf32(event->data);
	angles[2] = librg_data_rf32(event->data);
}

u32 f4mp::Entity::GetEntityID() const
{
	return entityID;
}

void f4mp::Entity::SetEntityID(u32 id)
{
	entityID = id;
}
