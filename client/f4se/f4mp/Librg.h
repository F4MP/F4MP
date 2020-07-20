#pragma once

#include <librg.h>

#include "Networking.h"

namespace f4mp
{
	namespace librg
	{
		class Librg;
		class Entity;

		namespace details
		{
			class _Event : public networking::Event
			{
			protected:
				virtual librg_data* GetStorage() = 0;

			private:
				void _Read(void* value, size_t size) override;
				void _Write(const void* value, size_t size) override;
			};
		}

		class Event : public details::_Event
		{
			friend Librg;

		public:
			Type GetType() const override;

		private:
			librg_event* _interface;

			Event(librg_event* _interface);

			librg_data* GetStorage() override;

			networking::Networking& GetNetworking() override;
		};

		class Message : public details::_Event
		{
		public:
			Type GetType() const override;

		private:
			librg_message* _interface;

			librg_data* GetStorage() override;

			networking::Networking& GetNetworking() override;
		};

		class MessageData : public details::_Event
		{
			friend Entity;

		public:
			Type GetType() const override;

		private:
			Librg& librg;

			librg_message_id type;
			librg_data* _interface;

			MessageData(Librg& librg, librg_message_id type, librg_data* _interface);

			librg_data* GetStorage() override;

			networking::Networking& GetNetworking() override;
		};

		class Entity : public networking::Entity
		{
		public:
			Entity(Librg& librg);

		private:
			struct _Interface : public networking::Entity::_Interface
			{
				_Interface(Librg& librg) : librg(librg) {}

				void SendMessage(Event::Type messageType, const networking::EventCallback& callback, const networking::MessageOptions& options) override;

				Librg& librg;
			};
		};

		class Librg : public networking::Networking
		{
			friend Event;
			friend Message;
			friend Entity;

		public:
			Librg();
			~Librg();

			void Start(const std::string& address, int32_t port) override;
			void Stop() override;

			void Tick() override;

			bool Connected() const override;

			void RegisterMessage(Event::Type messageType) override;
			void UnregisterMessage(Event::Type messageType) override;

		private:
			librg_ctx* ctx;

			static Librg& This(librg_ctx* ctx);

			static void OnConnectionRequest(librg_event* event);
			static void OnConnectionAccept(librg_event* event);
			static void OnConnectionRefuse(librg_event* event);

			static void OnEntityCreate(librg_event* event);
			static void OnEntityUpdate(librg_event* event);
			static void OnEntityRemove(librg_event* event);

			static void OnClientStreamerUpdate(librg_event* event);

			static void OnMessageReceive(librg_message* message);
		};
	}
}