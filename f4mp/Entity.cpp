#include "Entity.h"
#include "Player.h"
#include "NPC.h"
#include "f4mp.h"

#include "f4se/NiNodes.h"

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

f4mp::Entity::Entity() : entity(nullptr), refFormID((UInt32)-1)
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

	entity->entity = event->entity;

	event->entity->user_data = entity;

	entity->OnEntityCreate(event);

	return entity;
}

void f4mp::Entity::OnConnectRequest(librg_event* event)
{
}

void f4mp::Entity::OnConnectAccept(librg_event* event)
{
	// HACK: weird?
	entity = event->entity;
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
	OnEntityUpdate(event, true);
}

void f4mp::Entity::OnEntityRemove(librg_event* event)
{
	delete this;
	event->entity->user_data = nullptr;
}

void f4mp::Entity::OnClientUpdate(librg_event* event)
{
	TESObjectREFR* ref = GetRef();
	if (ref)
	{
		entity->position = (zpl_vec3&)ref->pos;

		SetNumber("angleX", ToDegrees(ref->rot.x));
		SetNumber("angleY", ToDegrees(ref->rot.y));
		SetNumber("angleZ", ToDegrees(ref->rot.z));
	}

	librg_data_wf32(event->data, GetNumber("angleX"));
	librg_data_wf32(event->data, GetNumber("angleY"));
	librg_data_wf32(event->data, GetNumber("angleZ"));
}

void f4mp::Entity::OnTick()
{
}

const librg_entity* f4mp::Entity::GetNetworkEntity() const
{
	return entity;
}

librg_entity* f4mp::Entity::GetNetworkEntity()
{
	return entity;
}

UInt32 f4mp::Entity::GetRefFormID() const
{
	return refFormID;
}

TESObjectREFR* f4mp::Entity::GetRef()
{
	//return ref;
	return dynamic_cast<TESObjectREFR*>(LookupFormByID(refFormID));
}

void f4mp::Entity::SetRef(TESObjectREFR* ref)
{
	refFormID = ref->formID;

	//this->ref = ref;

	/*NiNode* root = ref->GetActorRootNode(false);
	if (root)
	{
		printf("-----------\n");

		root->Visit([&](NiAVObject* obj)
			{
				printf("%s\n", obj->m_name.c_str());
				return false;
			});

		printf("-----------\n");
	}*/
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

void f4mp::Entity::OnEntityUpdate(librg_event* event, bool syncTransform)
{
	zpl_vec3 angles{ librg_data_rf32(event->data), librg_data_rf32(event->data), librg_data_rf32(event->data) };

	SetNumber("angleX", angles.x);
	SetNumber("angleY", angles.y);
	SetNumber("angleZ", angles.z);

	if (syncTransform)
	{
		TESObjectREFR* ref = GetRef();
		F4MP::SyncTransform(ref, entity->position, angles, true);
	}
}
