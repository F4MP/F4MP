#pragma once

#include "client.h"

namespace f4mp
{
	class Entity
	{
	public:
		static Entity* Get(librg_event* event);
		static Entity* Get(librg_entity* entity);

		template<class T>
		static T* GetAs(librg_event* event);

		template<class T>
		static T* GetAs(librg_entity* entity);

	public:
		Entity();
		virtual ~Entity();

		static Entity* Create(librg_event* event);

		virtual void OnConnectRequest(librg_event* event);
		virtual void OnConnectAccept(librg_event* event);
		virtual void OnDisonnect(librg_event* event);

		virtual void OnEntityCreate(librg_event* event);
		virtual void OnEntityUpdate(librg_event* event);
		virtual void OnEntityRemove(librg_event* event);

		virtual void OnClientUpdate(librg_event* event);

		virtual void OnTick();

		const librg_entity* GetNetworkEntity() const;
		librg_entity* GetNetworkEntity();
		UInt32 GetRefFormID() const;
		TESObjectREFR* GetRef();
		void SetRef(TESObjectREFR* ref);

		Float32 GetNumber(const std::string& name) const;
		void SetNumber(const std::string& name, Float32 number);

	protected:
		virtual void OnEntityUpdate(librg_event* event, bool syncTransform);

	private:
		librg_entity* entity;
		//TESObjectREFR* ref;
		UInt32 refFormID;

		std::unordered_map<std::string, Float32> numbers;
	};

	template<class T>
	inline T* Entity::GetAs(librg_event* event)
	{
		return dynamic_cast<T*>(Get(event));
	}

	template<class T>
	inline T* Entity::GetAs(librg_entity* entity)
	{
		return dynamic_cast<T*>(Get(entity));
	}
}