#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PluginAPI.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"

#include "f4se/GameData.h"

#include <librg.h>

#include <memory>
#include <vector>
#include <unordered_map>

struct PlayerData
{
	std::unordered_map<std::string, Float32> numbers;
	std::unordered_map<std::string, SInt32> integers;

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

	F4MP() : ctx{}, currentEvent(nullptr), port(0), playerEntityID((UInt32)-1), papyrus(nullptr),
		animStates
		{
			"None",
			
			"JogForward",
			"JogBackward",
			"JogLeft",
			"JogRight",

			"JumpUp",
			"JumpFall",
			"JumpLand"
		}
	{
		for (SInt32 i = 0; i < static_cast<SInt32>(animStates.size()); i++)
		{
			animStateIDs[animStates[i]] = i;
		}

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

	std::unordered_map<std::string, BGSAction*> actions;
	
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

				vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, UInt32, BSFixedString, Float32>("SetEntVarNum", "F4MP", SetEntVarNum, vm));
				vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, UInt32, BSFixedString>("SetEntVarAnim", "F4MP", SetEntVarAnim, vm));
				vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, Float32, UInt32, BSFixedString>("GetEntVarNum", "F4MP", GetEntVarNum, vm));
				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, UInt32>("GetEntVarAnim", "F4MP", GetEntVarAnim, vm));

				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, Actor*>("GetTransforms", "F4MP", GetTransforms, vm));
				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, Actor*>("SetTransforms", "F4MP", SetTransforms, vm));
				
				vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BGSAction*, BSFixedString>("GetAction", "F4MP",
					[](StaticFunctionTag* base, BSFixedString name)
					{
						F4MP& self = GetInstance();
						return self.actions[name.c_str()];
					}, vm));

				vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, Float32, Float32, Float32>("Atan2", "F4MP", Atan2, vm));
				
				vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, BSFixedString, Float32, Float32, Float32>("GetWalkDir", "F4MP", _GetWalkDir, vm));

				return true;
			}))
		{
			return false;
		}

		return true;
	}

	static int GetWalkDir(const zpl_vec2& displacement, float lookAngle)
	{
		const float walkThreshold = 10.f;

		float speed = zpl_vec2_mag(displacement);
		if (speed < walkThreshold)
		{
			return -1;
		}

		const float pi = 3.14159265359f;
		const float rad2deg = 180.f / pi, deg2rad = pi / 180.f;
		lookAngle *= deg2rad;
		zpl_vec2 lookDir{ sin(lookAngle), cos(lookAngle) };
		zpl_vec2 moveDir = displacement / speed;

		float dot = zpl_vec2_dot(lookDir, moveDir);
		zpl_f32 cross;
		zpl_vec2_cross(&cross, lookDir, moveDir);

		if (fabsf(dot) > cosf(45.f * deg2rad))
		{
			if (dot > 0.f)
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			if (cross > 0.f)
			{
				return 2;
			}
			else
			{
				return 3;
			}
		}
	}

private:
	static std::unique_ptr<F4MP> instance;

	std::string address;
	SInt32 port;

	UInt32 playerEntityID;

	zpl_vec3 prevPosition;

	librg_ctx ctx;
	librg_event* currentEvent;

	F4SEPapyrusInterface* papyrus;

	std::vector<std::string> animStates;
	std::unordered_map<std::string, SInt32> animStateIDs;

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

		data->integers["animState"] = librg_data_ri32(event->data);
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

		SInt32 animState = data->integers["animState"];
		if (animState != self.animStateIDs["JumpUp"] || animState != self.animStateIDs["JumpFall"])
		{
			zpl_vec2 displacement{ event->entity->position.x - self.prevPosition.x, event->entity->position.y - self.prevPosition.y };
			SInt32 newAnimState;

			switch (GetWalkDir(displacement, data->numbers["angleZ"]))
			{
			case 0:
				newAnimState = self.animStateIDs["JogForward"];
				break;
			case 1:
				newAnimState = self.animStateIDs["JogBackward"];
				break;
			case 2:
				newAnimState = self.animStateIDs["JogLeft"];
				break;
			case 3:
				newAnimState = self.animStateIDs["JogRight"];
				break;
			default:
				newAnimState = 0;
			}

			data->integers["animState"] = newAnimState;
		}

		self.prevPosition = event->entity->position;

		librg_data_wf32(event->data, data->numbers["angleX"]);
		librg_data_wf32(event->data, data->numbers["angleY"]);
		librg_data_wf32(event->data, data->numbers["angleZ"]);

		librg_data_wi32(event->data, data->integers["animState"]);
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

		/*tArray<TESForm*> idles = (*g_dataHandler)->arrIDLE;

		_MESSAGE("%d", *g_isGameDataReady);

		_MESSAGE("%u", (*g_dataHandler)->arrIDLE.count);

		for (UInt32 i = 0; i < idles.count; i++)
		{
			TESForm* idle = idles[i];
			_MESSAGE(idle->GetFullName());
		}*/


		auto actions = (*g_dataHandler)->arrAACT;

		_MESSAGE("%u", actions.count);

		for (UInt32 i = 0; i < actions.count; i++)
		{
			_MESSAGE("%u: %s", i, actions[i]->keyword.c_str());

			self.actions[actions[i]->keyword.c_str()] = actions[i];
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

	//abstract ...EntVar...

	static void SetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name, Float32 value)
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

	static void SetEntVarAnim(StaticFunctionTag* base, UInt32 entityID, BSFixedString animState)
	{
		F4MP& self = GetInstance();

		librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
		if (!entity)
		{
			_ERROR("no entity with ID '%d'!", entityID);
			return;
		}

		PlayerData* data = (PlayerData*)entity->user_data;
		data->integers["animState"] = self.animStateIDs[animState.c_str()];
	}

	static Float32 GetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name)
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

	static BSFixedString GetEntVarAnim(StaticFunctionTag* base, UInt32 entityID)
	{
		F4MP& self = GetInstance();

		librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
		if (!entity)
		{
			_ERROR("no entity with ID '%d'!", entityID);
			return self.animStates[0].c_str();
		}

		PlayerData* data = (PlayerData*)entity->user_data;
		return self.animStates[data->integers["animState"]].c_str();
	}

	std::unordered_map<std::string, NiTransform> transforms;

	static void GetTransforms(StaticFunctionTag* base, Actor* actor)
	{
		/*for (UInt32 i = 0; i < actor->actorValueData.count; i++)
		{
			Actor::ActorValueData data = actor->actorValueData[i];
			_MESSAGE("%u %f", data.avFormID, data.value);
		}*/

		F4MP& self = GetInstance();

		actor->GetObjectRootNode()->Visit([&](NiAVObject* obj)
			{
				self.transforms[obj->m_name.c_str()] = obj->m_localTransform;
				return false;
			});
	}

	static void SetTransforms(StaticFunctionTag* base, Actor* actor)
	{
		if (!actor)
		{
			_ERROR("actor is null");
			return;
		}

		NiNode* root = actor->GetObjectRootNode();
		if (!root)
		{
			_ERROR("the root node of %p is null", actor);
			return;
		}

		F4MP& self = GetInstance();

		root->Visit([&](NiAVObject* obj)
			{
				auto found = self.transforms.find(obj->m_name.c_str());
				if (found == self.transforms.end())
				{
					return false;
				}

				obj->m_localTransform = found->second;
				//obj->UpdateTransformAndBounds();

				return false;
			});
	}

	static Float32 Atan2(StaticFunctionTag* base, Float32 y, Float32 x)
	{
		const Float32 pi = 3.14159265359f;
		return fmod(2.f * pi - atan2(y, x), 2.f * pi);
	}

	static BSFixedString _GetWalkDir(StaticFunctionTag* base, Float32 dX, Float32 dY, Float32 angleZ)
	{
		switch (GetWalkDir(zpl_vec2{ dX, dY }, angleZ))
		{
		case 0:
			return "Forward";
		case 1:
			return "Backward";
		case 2:
			return "Left";
		case 3:
			return "Right";
		default:
			return "None";
		}
	}
};
