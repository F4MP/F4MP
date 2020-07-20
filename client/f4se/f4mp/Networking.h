#pragma once

#include <string>
#include <list>
#include <unordered_map>
#include <functional>
#include <iterator>
#include <cstdint>

namespace f4mp
{
	namespace networking
	{
		class Networking;

		struct Vector3
		{
			float x, y, z;

			Vector3() : x(0.f), y(0.f), z(0.f) {}
		};

		struct Quaternion
		{
			float x, y, z, w;

			Quaternion() : x(0.f), y(0.f), z(0.f) {}
		};

		class Event
		{
			friend class Entity;

		public:
			using Type = uint32_t;

			virtual ~Event() {}

			virtual Type GetType() const = 0;

			template<class T>
			T Read()
			{
				T value;
				_Read(&value, sizeof(T));

				return value;
			}

			template<class T>
			void Write(const T& value)
			{
				_Write(&value, sizeof(T));
			}

		protected:
			virtual Networking& GetNetworking() = 0;

			virtual void _Read(void* value, size_t size) = 0;
			virtual void _Write(const void* value, size_t size) = 0;
		};

		using EventCallback = std::function<void(Event&)>;

		struct MessageOptions
		{
			bool reliable;

			Entity* target;
			Entity* except;

			MessageOptions(bool reliable = true, Entity* target = nullptr, Entity* except = nullptr);
		};

		class Entity
		{
			friend Networking;

		public:
			using Type = uint32_t;

			using ID = uint32_t;

			using InstantiationID = int32_t;

			enum : Type
			{
				ClientType = 0
			};

			enum : ID
			{
				InvalidID = (ID)-1
			};

		protected:
			struct _Interface
			{
				ID id;

				_Interface() : id(InvalidID) {}

				virtual void SendMessage(Event::Type messageType, const EventCallback& callback, const MessageOptions& options) = 0;
			};

		public:
			Vector3 position;
			Quaternion rotation;

			Entity() : _interface(nullptr) {}
			virtual ~Entity() {}

			virtual void OnCreate(Event& event) {}
			virtual void OnDestroy(Event& event) {}

			virtual void OnServerUpdate(Event& event) {}
			virtual void OnClientUpdate(Event& event) {}

			virtual void OnMessageReceive(Event& event) {}

			void SendMessage(Event::Type messageType, const EventCallback& callback, const MessageOptions& options = MessageOptions());

		protected:
			_Interface* _interface;
		};

		class Networking
		{
			friend Entity;

		public:
			std::function<Entity* (Entity::Type entityType)> instantiate;

			EventCallback onConnectionRequest;
			EventCallback onConnectionAccept;
			EventCallback onConnectionRefuse;

			virtual ~Networking() {}

			virtual void Start(const std::string& address, int32_t port) = 0;
			virtual void Stop() = 0;

			virtual void Tick() = 0;

			virtual bool Connected() const = 0;

			virtual void RegisterMessage(Event::Type messageType) = 0;
			virtual void UnregisterMessage(Event::Type messageType) = 0;

			virtual void Create(Entity* entity);

		protected:
			virtual Entity::_Interface* GetEntityInterface() = 0;

			Entity* Instantiate(Entity::InstantiationID instantiationID, Entity::ID entityID, Entity::Type entityType);

		private:
			std::unordered_map<Entity::InstantiationID, Entity*> entityInstantiationQueue;
		};
	}
}