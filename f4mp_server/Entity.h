#pragma once

#include "server_common.h"

namespace f4mp
{
	class Entity
	{
	public:
		static Entity* Get(librg_entity* entity);
		static Entity* Get(librg_event* event);
		static Entity* Get(librg_peer* peer);

		template<class T, class U> static T* GetAs(U entity);

		static Entity* Create(librg_entity* entity);
		static Entity* Create(librg_event* event);

		template<class T>
		static T* Create(librg_entity* entity, T* instance);

		Entity();
		virtual ~Entity();

		virtual void OnConnectRequest(librg_event* event);
		virtual void OnConnectAccept(librg_event* event);
		virtual void OnConnectRefuse(librg_event* event);
		virtual void OnDisonnect(librg_event* event);

		virtual void OnEntityCreate(librg_event* event);
		virtual void OnEntityUpdate(librg_event* event);
		virtual void OnEntityRemove(librg_event* event);

		virtual void OnClientUpdate(librg_event* event);

		u32 GetEntityID() const;
		void SetEntityID(u32 id);

	private:
		u32 entityID;

		float angles[3];
	};

	template<class T, class U>
	T* Entity::GetAs(U entity)
	{
		return dynamic_cast<T*>(Get(entity));
	}
	template<class T>
	inline T* Entity::Create(librg_entity* entity, T* instance)
	{
		instance->SetEntityID(entity->id);
		entity->user_data = instance;

		return instance;
	}
}