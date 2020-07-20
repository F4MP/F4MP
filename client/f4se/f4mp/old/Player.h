#pragma once

#include "Character.h"

#include <memory>

namespace f4mp
{
	class Player : public Character
	{
	private:
		UInt32 entityID;

		zpl_vec3 prevPosition;
		zpl_vec3 displacement;

		client::AppearanceData appearance;
		client::WornItemsData wornItems;

		// TODO: might wanna move them to the Entity class.
		// NOTE: in that case, fix the ~EntVar~ functions to also work with the Entity class.
		std::unordered_map<std::string, SInt32> integers;

	public:
		static int GetWalkDir(const zpl_vec2& displacement, float lookAngle);
		
		static void SetAppearance(TESNPC* actorBase, const AppearanceData& appearance);
		static void SetWornItems(Actor* actor, const WornItemsData& wornItems);

		Player();

		void OnConnect(Actor* player, TESNPC* playerActorBase);
		void OnConnectRequest(librg_event* event) override;
		void OnConnectAccept(librg_event* event) override;
		void OnDisonnect(librg_event* event) override;

		void OnEntityCreate(librg_event* event) override;
		void OnEntityUpdate(librg_event* event) override;
		void OnEntityRemove(librg_event* event) override;

		void OnClientUpdate(librg_event* event) override;

		void OnTick() override;

		SInt32 GetInteger(const std::string& name) const;
		void SetInteger(const std::string& name, SInt32 integer);

		SInt32 GetAnimStateID() const;
		const std::string& GetAnimState() const;
		void SetAnimStateID(SInt32 id);
		void SetAnimState(const std::string& name);

		UInt32 GetEntityID() const;
		const client::AppearanceData& GetAppearance() const;
		const client::WornItemsData& GetWornItems() const;
		
		const zpl_vec3& GetDisplacement() const;

		float GetLookAngle() const;
	};
}