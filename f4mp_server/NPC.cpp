#include "NPC.h"

f4mp::NPC::NPC(u32 formID, u32 ownerEntityID) : formID(formID), ownerEntityID(ownerEntityID)
{
}

void f4mp::NPC::OnEntityCreate(librg_event* event)
{
	Entity::OnEntityCreate(event);

	librg_data_wf32(event->data, formID);
	librg_data_wf32(event->data, ownerEntityID);
}
