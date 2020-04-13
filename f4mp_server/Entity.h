#pragma once

#include "server_common.h"

namespace f4mp
{
	class Entity
	{
	public:
		static Entity* Get(librg_entity* entity);
		static Entity* Get(librg_event* event);

		template<class T> static T* GetAs(librg_entity* entity);
		template<class T> static T* GetAs(librg_event* event);

		static Entity* Create(librg_event* event);

		virtual ~Entity();

		virtual void OnConnectRequest(librg_event* event);
		virtual void OnConnectAccept(librg_event* event);
		virtual void OnConnectRefuse(librg_event* event);
		virtual void OnDisonnect(librg_event* event);

		virtual void OnEntityCreate(librg_event* event);
		virtual void OnEntityUpdate(librg_event* event);
		virtual void OnEntityRemove(librg_event* event);

		virtual void OnClientUpdate(librg_event* event);
	};

	template<class T>
	T* Entity::GetAs(librg_entity* entity)
	{
		return dynamic_cast<T*>(Get(entity));
	}

	template<class T>
	T* Entity::GetAs(librg_event* event)
	{
		return dynamic_cast<T*>(Get(event));
	}
}