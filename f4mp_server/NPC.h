#pragma once

#include "Entity.h"

namespace f4mp
{
	class NPC : public Entity
	{
	public:
		NPC(u32 formID = 0, u32 ownerEntityID = (u32)-1);

		void OnEntityCreate(librg_event* event) override;

	private:
		u32 formID;
		
		u32 ownerEntityID;
	};
}