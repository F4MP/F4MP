#include "api.h"

#define GEN_DATA_READWRITE(TYPE) \
    EXPORT TYPE ZPL_JOIN2(data_r,TYPE)(struct librg_data *data){ return ZPL_JOIN2(librg_data_r,TYPE)(data); } \
    EXPORT void ZPL_JOIN2(data_w,TYPE)(struct librg_data *data, TYPE value){ ZPL_JOIN2(librg_data_w,TYPE)(data, value); } \
    EXPORT TYPE ZPL_JOIN3(data_r,TYPE,_at)(struct librg_data *data, isize position){ return ZPL_JOIN3(librg_data_r,TYPE,_at)(data, position); } \
    EXPORT void ZPL_JOIN3(data_w,TYPE,_at)(struct librg_data *data, TYPE value, isize position){ ZPL_JOIN3(librg_data_w,TYPE,_at)(data, value, position); }

EXPORT f4mp::Server* server_create(const char* address, i32 port)
{
	try
	{
		return new f4mp::Server(address, port);
	}
	catch (const std::exception&)
	{
		return nullptr;
	}
}

EXPORT void server_destroy(f4mp::Server* server)
{
	delete server;
}

EXPORT void server_start(f4mp::Server* server)
{
	server->Start();
}

EXPORT librg_ctx* server_get_ctx(f4mp::Server* server)
{
	return server->GetContext();
}

EXPORT void server_tick(f4mp::Server* server)
{
	server->Tick();
}

GEN_DATA_READWRITE(i8);
GEN_DATA_READWRITE(u8);
GEN_DATA_READWRITE(i16);
GEN_DATA_READWRITE(u16);
GEN_DATA_READWRITE(i32);
GEN_DATA_READWRITE(u32);
GEN_DATA_READWRITE(i64);
GEN_DATA_READWRITE(u64);
GEN_DATA_READWRITE(f32);
GEN_DATA_READWRITE(f64);
GEN_DATA_READWRITE(b8);
GEN_DATA_READWRITE(b16);
GEN_DATA_READWRITE(b32);

EXPORT void event_add(f4mp::Server* server, u64 id, librg_event_cb* callback)
{
	librg_event_add(server->GetContext(), id, callback);
}

EXPORT void network_add(f4mp::Server* server, u16 id, librg_message_cb* callback)
{
	librg_network_add(server->GetContext(), id, callback);
}