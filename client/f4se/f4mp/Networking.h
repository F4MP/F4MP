#pragma once

#include <string>
#include <list>
#include <functional>
#include <iterator>
#include <cstdint>

namespace f4mp
{
	namespace networking
	{
		class Networking;

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
			using ID = uint32_t;

		protected:
			struct _Interface
			{
				ID id;

				virtual void SendMessage(Event::Type messageType, const EventCallback& callback, const MessageOptions& options) = 0;
			};

		public:
			Entity(Networking& networking);
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

		protected:
			virtual Entity::_Interface* GetEntityInterface() = 0;
		};
	}
}