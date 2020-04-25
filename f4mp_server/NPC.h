#pragma once

#include "Character.h"

namespace f4mp
{
	class NPC : public Character
	{
	public:
		NPC(u32 formID = 0, u32 ownerEntityID = (u32)-1);

		void OnEntityCreate(librg_event* event) override;

	private:
		u32 formID;
		
		u32 ownerEntityID;
	};
}