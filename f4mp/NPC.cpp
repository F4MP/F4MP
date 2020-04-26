#include "NPC.h"
#include "f4mp.h"

f4mp::NPC::NPC() : formID(0), ownerEntityID((UInt32)-1)
{
}

void f4mp::NPC::OnEntityCreate(librg_event* event)
{
	Character::OnEntityCreate(event);

	UInt32 entityID = GetNetworkEntity()->id;

	formID = librg_data_ri32(event->data);
	ownerEntityID = librg_data_ri32(event->data);

	F4MP& f4mp = F4MP::GetInstance();

	f4mp.entityIDs[formID] = entityID;

	_MESSAGE("OnSpawnEntity: %u(%x)", entityID, formID);

	TESObjectREFR* gameEntity = DYNAMIC_CAST(LookupFormByID(formID), TESForm, TESObjectREFR);
	if (!gameEntity)
	{
		return;
	}

	SetRef(gameEntity);

	printf("%u %x\n", entityID, formID);
}

void f4mp::NPC::OnEntityUpdate(librg_event* event)
{
	Character::OnEntityUpdate(event);
}

void f4mp::NPC::OnClientUpdate(librg_event* event)
{
	Character::OnClientUpdate(event);
}
