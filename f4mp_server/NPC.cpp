#include "NPC.h"

f4mp::NPC::NPC(u32 formID, u32 ownerEntityID) : formID(formID), ownerEntityID(ownerEntityID)
{
}

void f4mp::NPC::OnEntityCreate(librg_event* event)
{
	Character::OnEntityCreate(event);

	librg_data_wi32(event->data, formID);
	librg_data_wi32(event->data, ownerEntityID);
}
