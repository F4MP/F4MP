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
	return _interface->id;
}

librg_data* f4mp::librg::Event::GetStorage()
{
	return _interface->data;
}

f4mp::librg::Event::Event(librg_event* _interface) : _interface(_interface)
{
}

f4mp::networking::Networking& f4mp::librg::Event::GetNetworking()
{
	return Librg::This(_interface->ctx);
}

f4mp::librg::Message::Type f4mp::librg::Message::GetType() const
{
	return _interface->id;
}

librg_data* f4mp::librg::Message::GetStorage()
{
	return _interface->data;
}

f4mp::networking::Networking& f4mp::librg::Message::GetNetworking()
{
	return Librg::This(_interface->ctx);
}

f4mp::librg::MessageData::Type f4mp::librg::MessageData::GetType() const
{
	return type;
}

f4mp::librg::MessageData::MessageData(Librg& librg, librg_message_id type, librg_data* _interface) : librg(librg), type(type), _interface(_interface)
{
}

librg_data* f4mp::librg::MessageData::GetStorage()
{
	return _interface;
}

f4mp::networking::Networking& f4mp::librg::MessageData::GetNetworking()
{
	return librg;
}

f4mp::librg::Entity::Entity(Librg& librg) : networking::Entity(new librg::Entity::_Interface(librg))
{
}

void f4mp::librg::Entity::_Interface::SendMessage(Event::Type messageType, const networking::EventCallback& callback, const networking::MessageOptions& options)
{
	librg_message_id type = static_cast<librg_message_id>(messageType);
	if (type != messageType)
	{
		throw std::overflow_error("librg_send");
	}

	librg_data data;
	librg_data_init(&data);

	MessageData eventObj(librg, type, &data);
	callback(eventObj);

	// TODO: make it work with librg_entity_control_get (only on the server side.)
	librg_peer* target = nullptr;
	librg_peer* except = nullptr;

	librg_message_sendex(librg.ctx, type, target, except, librg_option_get(LIBRG_NETWORK_MESSAGE_CHANNEL), options.reliable, data.rawptr, librg_data_get_wpos(&data));
	librg_data_free(&data);
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
	librg_message_id type = static_cast<librg_message_id>(messageType);
	if (type != messageType)
	{
		throw std::overflow_error("librg_network_add");
	}

	librg_network_add(ctx, type, OnMessageReceive);
}

void f4mp::librg::Librg::UnregisterMessage(Event::Type messageType)
{
	librg_message_id type = static_cast<librg_message_id>(messageType);
	if (type != messageType)
	{
		throw std::overflow_error("librg_network_remove");
	}

	librg_network_remove(ctx, type);
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
