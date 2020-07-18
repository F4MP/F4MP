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
		class Event
		{
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
			virtual void _Read(void* value, size_t size) = 0;
			virtual void _Write(const void* value, size_t size) = 0;
		};

		class Entity
		{
		public:
			virtual ~Entity() {}

			virtual void OnCreate(Event& event) {}
			virtual void OnDestroy(Event& event) {}

			virtual void OnServerUpdate(Event& event) {}
			virtual void OnClientUpdate(Event& event) {}

			virtual void OnMessageReceive(Event& event) {}
		};

		class Networking
		{
		public:
			std::function<void(Event&)> onConnectionRequest;
			std::function<void(Event&)> onConnectionAccept;
			std::function<void(Event&)> onConnectionRefuse;

			virtual ~Networking() {}

			virtual void Start(const std::string& address, int32_t port) = 0;
			virtual void Stop() = 0;

			virtual void Tick() = 0;

			virtual bool Connected() const = 0;
		};
	}
}