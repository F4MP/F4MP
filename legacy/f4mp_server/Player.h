#pragma once

#include "Character.h"

namespace f4mp
{
	class Player : public Character
	{
	public:
		void OnConnectRequest(librg_event* event) override;
		void OnConnectRefuse(librg_event* event) override;

		void OnEntityCreate(librg_event* event) override;
		void OnEntityUpdate(librg_event* event) override;

		void OnClientUpdate(librg_event* event) override;

		const AppearanceData& GetAppearance() const;
		const WornItemsData& GetWornItems() const;

	private:
		float health;

		int animState;

		AppearanceData appearance;
		WornItemsData wornItems;
	};
}