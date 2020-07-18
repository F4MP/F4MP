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

f4mp::librg::Message::Type f4mp::librg::Message::GetType() const
{
	return info->id;
}

librg_data* f4mp::librg::Message::GetStorage()
{
	return info->data;
}

f4mp::librg::Librg::Librg() : ctx(nullptr)
{
	ctx = new librg_ctx();
	librg_init(ctx);

	ctx->user_data = this;

	librg_event_add(ctx, LIBRG_CONNECTION_REQUEST, OnConnectionRequest);
	librg_event_add(ctx, LIBRG_CONNECTION_ACCEPT, OnConnectionAccept);
	librg_event_add(ctx, LIBRG_CONNECTION_REFUSE, OnConnectionRefuse);
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
		// TODO: proper exception
		throw std::runtime_error("");
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
