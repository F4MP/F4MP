#include "f4se/PapyrusNativeFunctions.h"

#include <librg.h>

#include <memory>

class F4MP
{
public:
	static F4MP& GetInstance()
	{
		if (!instance)
		{
			instance = std::make_unique<F4MP>();
		}

		return *instance;
	}

	static bool RegisterFuncs(VirtualMachine* vm)
	{
		vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, BSFixedString, SInt32>("Connect", "F4MP", Connect, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("Disconnect", "F4MP", Disconnect, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Tick", "F4MP", Tick, vm));
		vm->SetFunctionFlags("F4MP", "Tick", IFunction::kFunctionFlag_NoWait);

		return true;
	}

	F4MP() : ctx{}, port(0)
	{
		ctx.mode = LIBRG_MODE_CLIENT;
	}

	virtual ~F4MP()
	{
		librg_network_stop(&ctx);
		librg_free(&ctx);
	}

private:
	static std::unique_ptr<F4MP> instance;

	std::string address;
	SInt32 port;

	librg_ctx ctx;

	static void OnConnectRequest(librg_event* event)
	{
		F4MP& self = GetInstance();
		_MESSAGE("trying to connect to %s:%d..", self.address.c_str(), self.port);
	}

	static void OnConnectAccept(librg_event* event)
	{
		F4MP& self = GetInstance();
		_MESSAGE("successfully connected to %s:%d", self.address.c_str(), self.port);
	}

	static void OnConnectRefuse(librg_event* event)
	{
		F4MP& self = GetInstance();
		_ERROR("failed to connect to %s:%d!", self.address.c_str(), self.port);
	}

	static bool Connect(StaticFunctionTag* base, BSFixedString address, SInt32 port)
	{
		F4MP& self = GetInstance();

		self.address = address;
		self.port = port;

		if (librg_init(&self.ctx))
		{
			_ERROR("network initialization failed!");
			return false;
		}

		librg_event_add(&self.ctx, LIBRG_CONNECTION_REQUEST, OnConnectRequest);
		librg_event_add(&self.ctx, LIBRG_CONNECTION_ACCEPT, OnConnectAccept);
		librg_event_add(&self.ctx, LIBRG_CONNECTION_REFUSE, OnConnectRefuse);

		if (librg_network_start(&self.ctx, { port, const_cast<char*>(address.c_str()) }))
		{
			_ERROR("failed to connect to the server!");
			return false;
		}

		return true;
	}

	static bool Disconnect(StaticFunctionTag* base)
	{
		if (!instance)
		{
			_ERROR("no network connection to end!");
			return false;
		}

		instance.release();

		_MESSAGE("successfully ended the network connection");
		return true;
	}

	static void Tick(StaticFunctionTag* base)
	{
		F4MP& self = GetInstance();
		librg_tick(&self.ctx);
	}
};
