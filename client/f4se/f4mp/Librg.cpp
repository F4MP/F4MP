#define LIBRG_IMPLEMENTATION

#include "Librg.h"

#include <stdexcept>

void f4mp::librg::details::_Event::_Read(void* value, size_t size)
{
	librg_data_rptr(GetStorage(), value, size);
}

void f4mp::librg::details::_Event::_Write(const void* value, size_t size)
{
	librg_data_wptr(GetStorage(), const_cast<void*>(value), size);
}

f4mp::librg::Event::Type f4mp::librg::Event::GetType() const
{
	return info->id;
}

librg_data* f4mp::librg::Event::GetStorage()
{
	return info->data;
}

f4mp::librg::Event::Event(librg_event* info) : info(info)
{
}

f4mp::networking::Networking& f4mp::librg::Event::GetNetworking()
{
	return Librg::This(info->ctx);
}

f4mp::librg::Message::Type f4mp::librg::Message::GetType() const
{
	return info->id;
}

librg_data* f4mp::librg::Message::GetStorage()
{
	return info->data;
}

f4mp::networking::Networking& f4mp::librg::Message::GetNetworking()
{
	return Librg::This(info->ctx);
}

f4mp::librg::MessageData::Type f4mp::librg::MessageData::GetType() const
{
	return type;
}

f4mp::librg::MessageData::MessageData(Librg& librg, u16 type, librg_data* info) : librg(librg), type(type), info(info)
{
}

librg_data* f4mp::librg::MessageData::GetStorage()
{
	return info;
}

f4mp::networking::Networking& f4mp::librg::MessageData::GetNetworking()
{
	return librg;
}

f4mp::librg::Librg::Librg() : ctx(nullptr)
{
	ctx = new librg_ctx();
	librg_init(ctx);

	ctx->user_data = this;

	librg_event_add(ctx, LIBRG_CONNECTION_REQUEST, OnConnectionRequest);
	librg_event_add(ctx, LIBRG_CONNECTION_ACCEPT, OnConnectionAccept);
	librg_event_add(ctx, LIBRG_CONNECTION_REFUSE, OnConnectionRefuse);

	LIBRG_ENTITY_CREATE;
	
}

f4mp::librg::Librg::~Librg()
{
	Stop();

	librg_free(ctx);
	delete ctx;
	ctx = nullptr;
}

void f4mp::librg::Librg::Start(const std::string& address, int32_t port)
{
	Stop();

	std::string hostAddress = address;

	switch (librg_network_start(ctx, { port, &hostAddress[0] }))
	{
	case 0: break;

	default:
		throw std::runtime_error("librg_network_start");
	}
}

void f4mp::librg::Librg::Stop()
{
	librg_network_stop(ctx);
}

void f4mp::librg::Librg::Tick()
{
	librg_tick(ctx);
}

bool f4mp::librg::Librg::Connected() const
{
	return !!librg_is_connected(ctx);
}

void f4mp::librg::Librg::RegisterMessage(Event::Type messageType)
{
	u16 type = static_cast<u16>(messageType);
	if (type != messageType)
	{
		throw std::overflow_error("librg_network_add");
	}

	librg_network_add(ctx, type, OnMessageReceive);
}

void f4mp::librg::Librg::UnregisterMessage(Event::Type messageType)
{
	u16 type = static_cast<u16>(messageType);
	if (type != messageType)
	{
		throw std::overflow_error("librg_network_remove");
	}

	librg_network_remove(ctx, type);
}

void f4mp::librg::Librg::SendMessage(networking::Entity& sender, Event::Type messageType, const networking::EventCallback& callback)
{
	u16 type = static_cast<u16>(messageType);
	if (type != messageType)
	{
		throw std::overflow_error("librg_send");
	}

	librg_send(ctx, type, data,
		{
			MessageData eventObj(*this, type, &data);
			callback(eventObj);
		});
}

f4mp::librg::Librg& f4mp::librg::Librg::This(librg_ctx* ctx)
{
	return *static_cast<Librg*>(ctx->user_data);
}

void f4mp::librg::Librg::OnConnectionRequest(librg_event* event)
{
	Event eventObj(event);
	This(event->ctx).onConnectionRequest(eventObj);
}

void f4mp::librg::Librg::OnConnectionAccept(librg_event* event)
{
	Event eventObj(event);
	This(event->ctx).onConnectionAccept(eventObj);
}

void f4mp::librg::Librg::OnConnectionRefuse(librg_event* event)
{
	Event eventObj(event);
	This(event->ctx).onConnectionRefuse(eventObj);
}

void f4mp::librg::Librg::OnEntityCreate(librg_event* event)
{
}

void f4mp::librg::Librg::OnEntityUpdate(librg_event* event)
{
}

void f4mp::librg::Librg::OnEntityRemove(librg_event* event)
{
}

void f4mp::librg::Librg::OnClientStreamerUpdate(librg_event* event)
{
}

void f4mp::librg::Librg::OnMessageReceive(librg_message* message)
{
}
