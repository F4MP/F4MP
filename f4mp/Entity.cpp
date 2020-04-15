#include "Entity.h"
#include "Player.h"
#include "NPC.h"
#include "f4mp.h"

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

f4mp::Entity::Entity()
{
	SetNumber("angleX", 0.f);
	SetNumber("angleY", 0.f);
	SetNumber("angleZ", 0.f);
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
	SetNumber("angleX", librg_data_rf32(event->data));
	SetNumber("angleY", librg_data_rf32(event->data));
	SetNumber("angleZ", librg_data_rf32(event->data));
}

void f4mp::Entity::OnEntityRemove(librg_event* event)
{
	delete this;
	event->entity->user_data = nullptr;
}

void f4mp::Entity::OnClientUpdate(librg_event* event)
{
	librg_data_wf32(event->data, GetNumber("angleX"));
	librg_data_wf32(event->data, GetNumber("angleY"));
	librg_data_wf32(event->data, GetNumber("angleZ"));
}

Float32 f4mp::Entity::GetNumber(const std::string& name) const
{
	// HACK: horrible
	return numbers.find(name)->second;
}

void f4mp::Entity::SetNumber(const std::string& name, Float32 number)
{
	if (F4MP::GetInstance().player.get() == this)
	{
		for (auto& instance : F4MP::instances)
		{
			instance->player->numbers[name] = number;
		}

		return;
	}

	numbers[name] = number;
}
