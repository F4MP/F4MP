#include "NPC.h"
#include "f4mp.h"

f4mp::NPC::NPC() : formID(0), ownerEntityID((UInt32)-1)
{
}

void f4mp::NPC::OnEntityCreate(librg_event* event)
{
	Entity::OnEntityCreate(event);

	UInt32 entityID = GetNetworkEntity()->id;

	formID = librg_data_rf32(event->data);
	ownerEntityID = librg_data_rf32(event->data);

	F4MP& f4mp = F4MP::GetInstance();

	f4mp.entityIDs[formID] = entityID;

	_MESSAGE("OnSpawnEntity: %u(%x)", entityID, formID);

	TESObjectREFR* gameEntity = dynamic_cast<TESObjectREFR*>(LookupFormByID(formID));
	if (!gameEntity)
	{
		return;
	}

	SetRef(gameEntity);

	printf("%u %x\n", entityID, formID);
}

void f4mp::NPC::OnEntityUpdate(librg_event* event)
{
	if (ownerEntityID != F4MP::GetInstance().player->GetEntityID())
	{
		Entity::OnEntityUpdate(event);
	}
}

void f4mp::NPC::OnClientUpdate(librg_event* event)
{
	if (ownerEntityID == F4MP::GetInstance().player->GetEntityID())
	{
		Entity::OnClientUpdate(event);
	}
}
