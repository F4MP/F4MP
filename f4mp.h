#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PluginAPI.h"
#include "f4se/PapyrusEvents.h"

#include <librg.h>

#include <memory>
#include <vector>
#include <unordered_map>

struct PlayerData
{
	std::unordered_map<std::string, Float32> numbers;
	
	PlayerData()
	{

	}
};

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

	F4MP() : ctx{}, currentEvent(nullptr), port(0), playerEntityID((UInt32)-1), papyrus(nullptr)
	{
		ctx.mode = LIBRG_MODE_CLIENT;

		librg_init(&ctx);

		librg_event_add(&ctx, LIBRG_CONNECTION_REQUEST, OnConnectRequest);
		librg_event_add(&ctx, LIBRG_CONNECTION_ACCEPT, OnConnectAccept);
		librg_event_add(&ctx, LIBRG_CONNECTION_REFUSE, OnConnectRefuse);
		librg_event_add(&ctx, LIBRG_CONNECTION_DISCONNECT, OnDisonnect);

		librg_event_add(&ctx, LIBRG_ENTITY_CREATE, OnEntityCreate);
		librg_event_add(&ctx, LIBRG_ENTITY_UPDATE, OnEntityUpdate);
		librg_event_add(&ctx, LIBRG_ENTITY_REMOVE, OnEntityRemove);

		librg_event_add(&ctx, LIBRG_CLIENT_STREAMER_UPDATE, OnClientUpdate);
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
				//vm->SetFunctionFlags("F4MP", "Tick", IFunction::kFunctionFlag_NoWait);

				vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("GetPlayerEntityID", "F4MP", GetPlayerEntityID, vm));

				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, UInt32>("IsEntityValid", "F4MP", IsEntityValid, vm));
				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<Float32>, UInt32>("GetEntityPosition", "F4MP", GetEntityPosition, vm));
				vm->RegisterFunction(new NativeFunction4<StaticFunctionTag, void, UInt32, Float32, Float32, Float32>("SetEntityPosition", "F4MP", SetEntityPosition, vm));
				vm->SetFunctionFlags("F4MP", "SetEntityPosition", IFunction::kFunctionFlag_NoWait);

				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, Float32>("WriteNumber", "F4MP", WriteNumber, vm));
				vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, Float32>("ReadNumber", "F4MP", ReadNumber, vm));
				//vm->SetFunctionFlags("F4MP", "WriteNumber", IFunction::kFunctionFlag_NoWait);

				vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, UInt32, BSFixedString, Float32>("SetEntityNumber", "F4MP", SetEntityNumber, vm));
				vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, Float32, UInt32, BSFixedString>("GetEntityNumber", "F4MP", GetEntityNumber, vm));

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
	librg_event* currentEvent;

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

		event->entity->user_data = new PlayerData();
	}

	static void OnConnectRefuse(librg_event* event)
	{
		F4MP& self = GetInstance();
		_ERROR("failed to connect to %s:%d!", self.address.c_str(), self.port);
	}

	static void OnDisonnect(librg_event* event)
	{
		PlayerData* data = (PlayerData*)event->entity->user_data;
		if (data)
		{
			delete data;
			event->entity->user_data = nullptr;
		}
	}

	//TODO: abstract event handlers as they're basically the same

	static void OnEntityCreate(librg_event* event)
	{
		event->entity->user_data = new PlayerData();

		F4MP& self = GetInstance();
		_MESSAGE("entity with ID '%d' has created", event->entity->id);

		self.currentEvent = event;

		self.papyrus->GetExternalEventRegistrations("OnEntityCreate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				librg_event* event = static_cast<librg_event*>(dataPtr);

				UInt32 id = event->entity->id;
				SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
			});

		self.currentEvent = nullptr;
	}

	static void OnEntityUpdate(librg_event* event)
	{
		F4MP& self = GetInstance();

		self.currentEvent = event;

		self.papyrus->GetExternalEventRegistrations("OnEntityUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				librg_event* event = static_cast<librg_event*>(dataPtr);

				UInt32 id = event->entity->id;
				SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
			});

		self.currentEvent = nullptr;

		PlayerData* data = (PlayerData*)event->entity->user_data;
		if (!data)
		{
			return;
		}

		data->numbers["angleX"] = librg_data_rf32(event->data);
		data->numbers["angleY"] = librg_data_rf32(event->data);
		data->numbers["angleZ"] = librg_data_rf32(event->data);
	}

	static void OnEntityRemove(librg_event* event)
	{
		PlayerData* data = (PlayerData*)event->entity->user_data;
		if (data)
		{
			delete data;
			event->entity->user_data = nullptr;
		}

		F4MP& self = GetInstance();
		_MESSAGE("entity with ID '%d' has removed", event->entity->id);

		self.papyrus->GetExternalEventRegistrations("OnEntityRemove", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				librg_event* event = static_cast<librg_event*>(dataPtr);

				UInt32 id = event->entity->id;
				SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
			});
	}

	static void OnClientUpdate(librg_event* event)
	{
		F4MP& self = GetInstance();

		self.currentEvent = event;

		self.papyrus->GetExternalEventRegistrations("OnClientUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				librg_event* event = static_cast<librg_event*>(dataPtr);

				UInt32 id = event->entity->id;
				SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
			});

		self.currentEvent = nullptr;

		PlayerData* data = (PlayerData*)event->entity->user_data;
		if (!data)
		{
			return;
		}

		librg_data_wf32(event->data, data->numbers["angleX"]);
		librg_data_wf32(event->data, data->numbers["angleY"]);
		librg_data_wf32(event->data, data->numbers["angleZ"]);
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

	//TODO: exception for the event not being the proper one

	static void WriteNumber(StaticFunctionTag* base, Float32 number)
	{
		F4MP& self = GetInstance();
		if (!self.currentEvent || self.currentEvent->id != LIBRG_CLIENT_STREAMER_UPDATE)
		{
			_ERROR("id of %p is not LIBRG_CLIENT_STREAMER_UPDATE", self.currentEvent);
			return;
		}

		librg_data_wf32(self.currentEvent->data, number);
	}

	static void WriteString(StaticFunctionTag* base, BSFixedString string)
	{
		
	}

	static Float32 ReadNumber(StaticFunctionTag* base)
	{
		F4MP& self = GetInstance();
		if (!self.currentEvent || self.currentEvent->id != LIBRG_ENTITY_UPDATE)
		{
			_ERROR("id of %p is not LIBRG_ENTITY_UPDATE", self.currentEvent);
			return 0.f;
		}

		return librg_data_rf32(self.currentEvent->data);
	}

	static BSFixedString ReadString(StaticFunctionTag* base)
	{
		return BSFixedString();
	}

	static void SetEntityNumber(StaticFunctionTag* base, UInt32 entityID, BSFixedString name, Float32 value)
	{
		F4MP& self = GetInstance();

		librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
		if (!entity)
		{
			_ERROR("no entity with ID '%d'!", entityID);
			return;
		}

		PlayerData* data = (PlayerData*)entity->user_data;
		data->numbers[name.c_str()] = value;
	}

	static Float32 GetEntityNumber(StaticFunctionTag* base, UInt32 entityID, BSFixedString name)
	{
		F4MP& self = GetInstance();

		librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
		if (!entity)
		{
			_ERROR("no entity with ID '%d'!", entityID);
			return 0.f;
		}

		PlayerData* data = (PlayerData*)entity->user_data;
		return data->numbers[name.c_str()];
	}
};
