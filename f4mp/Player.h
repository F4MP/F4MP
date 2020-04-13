#pragma once

#include "client.h"
#include "Animation.h"

#include <memory>

namespace f4mp
{
	class Player
	{
	private:
		std::unique_ptr<Animation> animation;

		UInt32 entityID;

		zpl_vec3 prevPosition;

		client::AppearanceData appearance;
		client::WornItemsData wornItems;

		std::unordered_map<std::string, Float32> numbers;
		std::unordered_map<std::string, SInt32> integers;

	public:
		static Player* Get(librg_event* event);
		static Player* Get(librg_entity* entity);

		static int GetWalkDir(const zpl_vec2& displacement, float lookAngle);
		
		static void SetAppearance(TESNPC* actorBase, const AppearanceData& appearance);
		static void SetWornItems(Actor* actor, const WornItemsData& wornItems);

		Player();

		void OnConnect(Actor* player, TESNPC* playerActorBase);
		void OnConnectRequest(librg_event* event);
		void OnConnectAccept(librg_event* event);
		void OnDisonnect(librg_event* event);

		void OnEntityCreate(librg_event* event);
		void OnEntityUpdate(librg_event* event);
		void OnEntityRemove(librg_event* event);

		void OnClientUpdate(librg_event* event);

		Float32 GetNumber(const std::string& name) const;
		SInt32 GetInteger(const std::string& name) const;
		void SetNumber(const std::string& name, Float32 number);
		void SetInteger(const std::string& name, SInt32 integer);

		SInt32 GetAnimStateID() const;
		const std::string& GetAnimState() const;
		void SetAnimStateID(SInt32 id);
		void SetAnimState(const std::string& name);

		UInt32 GetEntityID() const;
		const client::AppearanceData& GetAppearance() const;
		const client::WornItemsData& GetWornItems() const;

		float GetLookAngle() const;
	};
}