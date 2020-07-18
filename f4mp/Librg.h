#pragma once

#include "Networking.h"

#include <librg.h>

namespace f4mp
{
	namespace librg
	{
		namespace details
		{
			class _Event : public networking::Event
			{
			protected:
				virtual librg_data* GetStorage() = 0;

				void _Read(void* value, size_t size) override;
				void _Write(const void* value, size_t size) override;
			};
		}

		class Event : public details::_Event
		{
		public:
			Type GetType() const override;

		protected:
			librg_data* GetStorage() override;

		private:
			friend class Librg;

			Event(librg_event* info);

			librg_event* info;
		};

		class Message : public details::_Event
		{
		public:
			Type GetType() const override;

		protected:
			librg_data* GetStorage() override;

		private:
			librg_message* info;
		};

		class Librg : public networking::Networking
		{
		public:
			Librg();
			~Librg();

			void Start(const std::string& address, int32_t port) override;
			void Stop() override;

			void Tick() override;

			bool Connected() const override;

		private:
			librg_ctx* ctx;

			static Librg& This(librg_ctx* ctx);

			static void OnConnectionRequest(librg_event* event);
			static void OnConnectionAccept(librg_event* event);
			static void OnConnectionRefuse(librg_event* event);
		};
	}
}