#pragma once

#include "Character.h"

namespace f4mp
{
	class NPC : public Character
	{
	public:
		NPC();

		void OnEntityCreate(librg_event* event) override;
		void OnEntityUpdate(librg_event* event) override;

		void OnClientUpdate(librg_event* event) override;

	private:
		UInt32 formID, ownerEntityID;
	};
}