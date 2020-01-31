#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PluginAPI.h"
#include "f4se/PapyrusEvents.h"

#include <librg.h>

#include <memory>
#include <vector>

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

	F4MP() : ctx{}, port(0), playerEntityID((UInt32)-1), papyrus(nullptr)
	{
		ctx.mode = LIBRG_MODE_CLIENT;

		librg_init(&ctx);

		librg_event_add(&ctx, LIBRG_CONNECTION_REQUEST, OnConnectRequest);
		librg_event_add(&ctx, LIBRG_CONNECTION_ACCEPT, OnConnectAccept);
		librg_event_add(&ctx, LIBRG_CONNECTION_REFUSE, OnConnectRefuse);

		librg_event_add(&ctx, LIBRG_ENTITY_CREATE, OnEntityCreate);
	}

	virtual ~F4MP()
	{
		librg_free(&ctx);
	}

	bool Register(F4SEPapyrusInterface* _papyrus)
	{
		papyrus = _papyrus;

		if (!papyrus->Register([](VirtualMachine* vm)
			{
				vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, BSFixedString, SInt32>("Connect", "F4MP", Connect, vm));
				vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("Disconnect", "F4MP", Disconnect, vm));
				vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Tick", "F4MP", Tick, vm));
				vm->SetFunctionFlags("F4MP", "Tick", IFunction::kFunctionFlag_NoWait);

				vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("GetPlayerEntityID", "F4MP", GetPlayerEntityID, vm));

				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, UInt32>("IsEntityValid", "F4MP", IsEntityValid, vm));
				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<Float32>, UInt32>("GetEntityPosition", "F4MP", GetEntityPosition, vm));
				vm->RegisterFunction(new NativeFunction4<StaticFunctionTag, void, UInt32, Float32, Float32, Float32>("SetEntityPosition", "F4MP", SetEntityPosition, vm));
				vm->SetFunctionFlags("F4MP", "SetEntityPosition", IFunction::kFunctionFlag_NoWait);

				return true;
			}))
		{
			return false;
		}

		return true;
	}

private:
	static std::unique_ptr<F4MP> instance;

	std::string address;
	SInt32 port;

	UInt32 playerEntityID;

	librg_ctx ctx;

	F4SEPapyrusInterface* papyrus;

	static void OnConnectRequest(librg_event* event)
	{
		F4MP& self = GetInstance();
		_MESSAGE("trying to connect to %s:%d..", self.address.c_str(), self.port);
	}

	static void OnConnectAccept(librg_event* event)
	{
		F4MP& self = GetInstance();
		_MESSAGE("successfully connected to %s:%d", self.address.c_str(), self.port);

		self.playerEntityID = event->entity->id;
	}

	static void OnConnectRefuse(librg_event* event)
	{
		F4MP& self = GetInstance();
		_ERROR("failed to connect to %s:%d!", self.address.c_str(), self.port);
	}

	static void OnEntityCreate(librg_event* event)
	{
		F4MP& self = GetInstance();
		_MESSAGE("entity with ID '%d' has created", event->entity->id);

		self.papyrus->GetExternalEventRegistrations("OnEntityCreate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				librg_event* event = static_cast<librg_event*>(dataPtr);

				UInt32 id = event->entity->id;
				SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
			});
	}

	static bool Connect(StaticFunctionTag* base, BSFixedString address, SInt32 port)
	{
		F4MP& self = GetInstance();
		librg_network_stop(&self.ctx);

		self.address = address;
		self.port = port;

		if (librg_network_start(&self.ctx, { port, const_cast<char*>(address.c_str()) }))
		{
			_ERROR("failed to connect to the server!");
			return false;
		}

		return true;
	}

	static bool Disconnect(StaticFunctionTag* base)
	{
		F4MP& self = GetInstance();
		librg_network_stop(&self.ctx);

		_MESSAGE("successfully ended the network connection");
		return true;
	}

	static void Tick(StaticFunctionTag* base)
	{
		F4MP& self = GetInstance();
		librg_tick(&self.ctx);
	}

	static UInt32 GetPlayerEntityID(StaticFunctionTag* base)
	{
		F4MP& self = GetInstance();
		return self.playerEntityID;
	}

	static bool IsEntityValid(StaticFunctionTag* base, UInt32 entityID)
	{
		F4MP& self = GetInstance();
		return !!librg_entity_valid(&self.ctx, entityID);
	}

	static VMArray<Float32> GetEntityPosition(StaticFunctionTag* base, UInt32 entityID)
	{
		F4MP& self = GetInstance();
		std::vector<Float32> result{ -1, -1, -1 };

		librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
		if (!entity)
		{
			_ERROR("no entity with ID '%d'!", entityID);
			return VMArray<Float32>(result);
		}

		result = { entity->position.x, entity->position.y, entity->position.z };
		return VMArray<Float32>(result);
	}

	static void SetEntityPosition(StaticFunctionTag* base, UInt32 entityID, float x, float y, float z)
	{
		F4MP& self = GetInstance();

		librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
		if (!entity)
		{
			_ERROR("no entity with ID '%d'!", entityID);
			return;
		}

		entity->position.x = x;
		entity->position.y = y;
		entity->position.z = z;
	}
};
