#define LIBRG_IMPLEMENTATION
#define LIBRG_DEBUG
#define librg_log _DMESSAGE
//#define ZPL_ASSERT_MSG _ERROR

#define ZPL_ASSERT_MSG(cond, msg, ...)\
    do {\
        if (!(cond)) {\
            _ERROR(msg, ##__VA_ARGS__);\
        }\
    } while (0)

#include "f4mp.h"

std::vector<std::unique_ptr<f4mp::F4MP>> f4mp::F4MP::instances;
size_t f4mp::F4MP::activeInstance = 0;

f4mp::F4MP& f4mp::F4MP::GetInstance()
{
	while (instances.size() <= activeInstance)
	{
		instances.push_back(std::make_unique<F4MP>());
		if (instances.size() > 1)
		{
			F4MP& mainInstance = *instances.front();
			F4MP& newInstance = *instances.back();
			newInstance.messaging = mainInstance.messaging;
			newInstance.papyrus = mainInstance.papyrus;
			newInstance.task = mainInstance.task;
		}
	}

	return *instances[activeInstance];
}

f4mp::F4MP::F4MP() : ctx{}, port(0), handle(kPluginHandle_Invalid), messaging(nullptr), papyrus(nullptr), task(nullptr)
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

	librg_network_add(&ctx, Message::Hit, OnHit);
	librg_network_add(&ctx, Message::FireWeapon, OnFireWeapon);

	player = std::make_unique<Player>();
}

f4mp::F4MP::~F4MP()
{
	librg_free(&ctx);
}

bool f4mp::F4MP::Init(const F4SEInterface* f4se)
{
	handle = f4se->GetPluginHandle();

	messaging = (F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging);
	papyrus = (F4SEPapyrusInterface*)f4se->QueryInterface(kInterface_Papyrus);
	task = (F4SETaskInterface*)f4se->QueryInterface(kInterface_Task);

	/*messaging->RegisterListener(handle, "F4SE", [](F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			auto arr = &(*g_dataHandler)->arrNONE;

			for (int i = 0; i < kFormType_Max; i++)
			{
				for (UInt32 j = 0; j < arr[i].count; j++)
				{
					if (arr[i][j]->GetFormType() == kFormType_IDLE)
					{
						_MESSAGE("%d: %s", i, arr[i][j]->GetFullName());
					}
				}
			}
		}
		}
	});*/

	if (!papyrus->Register([](VirtualMachine* vm)
		{
			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("GetClientInstanceID", "F4MP", GetClientInstanceID, vm));
			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, UInt32>("SetClient", "F4MP", SetClient, vm));

			vm->RegisterFunction(new NativeFunction4<StaticFunctionTag, bool, Actor*, TESNPC*, BSFixedString, SInt32>("Connect", "F4MP", Connect, vm));
			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("Disconnect", "F4MP", Disconnect, vm));
			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Tick", "F4MP", Tick, vm));
			//vm->SetFunctionFlags("F4MP", "Tick", IFunction::kFunctionFlag_NoWait);

			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("GetPlayerEntityID", "F4MP", GetPlayerEntityID, vm));

			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, UInt32>("IsEntityValid", "F4MP", IsEntityValid, vm));
			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<Float32>, UInt32>("GetEntityPosition", "F4MP", GetEntityPosition, vm));
			vm->RegisterFunction(new NativeFunction4<StaticFunctionTag, void, UInt32, Float32, Float32, Float32>("SetEntityPosition", "F4MP", SetEntityPosition, vm));
			vm->SetFunctionFlags("F4MP", "SetEntityPosition", IFunction::kFunctionFlag_NoWait);

			vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, UInt32, BSFixedString, Float32>("SetEntVarNum", "F4MP", SetEntVarNum, vm));
			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, UInt32, BSFixedString>("SetEntVarAnim", "F4MP", SetEntVarAnim, vm));
			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, Float32, UInt32, BSFixedString>("GetEntVarNum", "F4MP", GetEntVarNum, vm));
			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, UInt32>("GetEntVarAnim", "F4MP", GetEntVarAnim, vm));

			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, Float32, Float32, Float32>("Atan2", "F4MP", Atan2, vm));

			vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, BSFixedString, Float32, Float32, Float32>("GetWalkDir", "F4MP", _GetWalkDir, vm));

			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BGSAction*, BSFixedString>("GetAction", "F4MP",
				[](StaticFunctionTag* base, BSFixedString name) -> BGSAction*
				{
					auto& actions = (*g_dataHandler)->arrAACT;
					for (UInt32 i = 0; i < actions.count; i++)
					{
						if (std::string(actions[i]->GetFullName()).compare(name.c_str()))
						{
							return actions[i];
						}
					}

					return nullptr;
				}, vm));

			vm->RegisterFunction(new NativeFunction1 < StaticFunctionTag, void, VMArray<TESForm*>>("Inspect", "F4MP",
				[](StaticFunctionTag* base, VMArray<TESForm*> forms)
				{
					_MESSAGE("size: %u", forms.Length());

					for (UInt32 i = 0; i < forms.Length(); i++)
					{
						TESForm* form;
						forms.Get(&form, i);
						_MESSAGE("%p %u %x %s", form - RelocationManager::s_baseAddr, form->GetFormType(), form->formID, form->GetFullName());
					}
				}, vm));

			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>("AnimLoops", "F4MP", AnimLoops, vm));

			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, TESNPC*, TESNPC*>("CopyAppearance", "F4MP", CopyAppearance, vm));
			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, Actor*, Actor*>("CopyWornItems", "F4MP", CopyWornItems, vm));

			vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, UInt32, UInt32, Float32>("PlayerHit", "F4MP", PlayerHit, vm));
			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("PlayerFireWeapon", "F4MP", PlayerFireWeapon, vm));

			return true;
		}))
	{
		return false;
	}

	Animation::Init();

	return true;
}

std::vector<TESForm*> f4mp::F4MP::DecodeWornItems(const WornItemsData& wornItems)
{
	std::vector<TESForm*> items;

	//for (auto& item : wornItems.data)
	for (size_t k = 0; k < wornItems.data1.size(); k++)
	{
		switch (wornItems.data1[k])
		{
		case kFormType_ARMO:
		{
			auto& armos = (*g_dataHandler)->arrARMO;
			for (UInt32 i = 0; i < armos.count; i++)
			{
				if (wornItems.data2[k].compare(armos[i]->fullName.name.c_str()) == 0)
				{
					items.push_back(armos[i]);
					break;
				}
			}
			break;
		}
		case kFormType_WEAP:
		{
			auto& weaps = (*g_dataHandler)->arrWEAP;
			for (UInt32 i = 0; i < weaps.count; i++)
			{
				if (wornItems.data2[k].compare(weaps[i]->fullName.name.c_str()) == 0)
				{
					items.push_back(weaps[i]);
					break;
				}
			}
			break;
		}
		}
	}

	//for (auto item : items)
	//{
	//	_MESSAGE(item->GetFullName());
	//}

	return items;
}

void f4mp::F4MP::OnConnectRequest(librg_event* event)
{
	F4MP& self = GetInstance();
	_MESSAGE("trying to connect to %s:%d..", self.address.c_str(), self.port);

	self.player->OnConnectRequest(event);
}

void f4mp::F4MP::OnConnectAccept(librg_event* event)
{
	F4MP& self = GetInstance();
	_MESSAGE("successfully connected to %s:%d", self.address.c_str(), self.port);

	self.player->OnConnectAccept(event);
}

void f4mp::F4MP::OnConnectRefuse(librg_event* event)
{
	F4MP& self = GetInstance();
	_ERROR("failed to connect to %s:%d!", self.address.c_str(), self.port);
}

void f4mp::F4MP::OnDisonnect(librg_event* event)
{
	F4MP& self = GetInstance();

	self.player->OnDisonnect(event);
}

void f4mp::F4MP::OnEntityCreate(librg_event* event)
{
	_MESSAGE("entity with ID '%d' has created", event->entity->id);

	Player* player = new Player();
	player->OnEntityCreate(event);

	struct Tmp
	{
		UInt32 id;
		const WornItemsData& wornItems;
	} tmp{ event->entity->id, player->GetWornItems() };

	F4MP& self = F4MP::GetInstance();

	self.papyrus->GetExternalEventRegistrations("OnEntityCreate", &tmp, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			Tmp* tmp = static_cast<Tmp*>(dataPtr);
			VMArray<TESForm*> wornItems(F4MP::DecodeWornItems(tmp->wornItems));

			SendPapyrusEvent2<UInt32, VMArray<TESForm*>>(handle, scriptName, callbackName, tmp->id, wornItems);
		});
}

void f4mp::F4MP::OnEntityUpdate(librg_event* event)
{
	F4MP& self = GetInstance();

	self.papyrus->GetExternalEventRegistrations("OnEntityUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			librg_event* event = static_cast<librg_event*>(dataPtr);
			UInt32 id = event->entity->id;
			SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
		});

	Player* entity = Player::Get(event);
	if (!entity)
	{
		return;
	}

	entity->OnEntityUpdate(event);
}

void f4mp::F4MP::OnEntityRemove(librg_event* event)
{
	Player* entity = Player::Get(event);
	if (entity)
	{
		entity->OnEntityRemove(event);
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

void f4mp::F4MP::OnClientUpdate(librg_event* event)
{
	F4MP& self = GetInstance();

	self.papyrus->GetExternalEventRegistrations("OnClientUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			librg_event* event = static_cast<librg_event*>(dataPtr);
			UInt32 id = event->entity->id;
			SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
		});

	Player* player = Player::Get(event);
	if (!player)
	{
		return;
	}

	player->OnClientUpdate(event);
}

void f4mp::F4MP::OnHit(librg_message* msg)
{
	HitData data;
	librg_data_rptr(msg->data, &data, sizeof(HitData));

	_MESSAGE("OnHit: %u -> %u", data.hitter, data.hittee);

	F4MP& self = GetInstance();
	self.papyrus->GetExternalEventRegistrations("OnPlayerHit", &data, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			F4MP& self = GetInstance();

			HitData* data = static_cast<HitData*>(dataPtr);
			if (data->hittee == self.player->GetEntityID())
			{
				SendPapyrusEvent1<Float32>(handle, scriptName, callbackName, data->damage);
			}
		});
}

void f4mp::F4MP::OnFireWeapon(librg_message* msg)
{
	UInt32 entity;
	librg_data_rptr(msg->data, &entity, sizeof(UInt32));

	_MESSAGE("OnFireWeapon: %u", entity);

	F4MP& self = GetInstance();
	self.papyrus->GetExternalEventRegistrations("OnFireWeapon", &entity, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			F4MP& self = GetInstance();

			UInt32* data = static_cast<UInt32*>(dataPtr);
			SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, *data);
		});
}

UInt32 f4mp::F4MP::GetClientInstanceID(StaticFunctionTag* base)
{
	return activeInstance;
}

void f4mp::F4MP::SetClient(StaticFunctionTag* base, UInt32 instance)
{
	activeInstance = instance;
}

bool f4mp::F4MP::Connect(StaticFunctionTag* base, Actor* player, TESNPC* playerActorBase, BSFixedString address, SInt32 port)
{
	F4MP& self = GetInstance();
	librg_network_stop(&self.ctx);

	self.player = std::make_unique<Player>();
	self.player->OnConnect(player, playerActorBase);

	self.address = address;
	self.port = port;

	if (librg_network_start(&self.ctx, { port, const_cast<char*>(address.c_str()) }))
	{
		_ERROR("failed to connect to the server!");
		return false;
	}

	return true;
}

bool f4mp::F4MP::Disconnect(StaticFunctionTag* base)
{
	F4MP& self = GetInstance();
	librg_network_stop(&self.ctx);

	_MESSAGE("successfully ended the network connection");
	return true;
}

void f4mp::F4MP::Tick(StaticFunctionTag* base)
{
	//F4MP& self = GetInstance();
	//librg_tick(&self.ctx);

	for (auto& instance : instances)
	{
		librg_tick(&instance->ctx);
	}
}

UInt32 f4mp::F4MP::GetPlayerEntityID(StaticFunctionTag* base)
{
	F4MP& self = GetInstance();
	return self.player->GetEntityID();
}

bool f4mp::F4MP::IsEntityValid(StaticFunctionTag* base, UInt32 entityID)
{
	F4MP& self = GetInstance();
	return !!librg_entity_valid(&self.ctx, entityID);
}

VMArray<Float32> f4mp::F4MP::GetEntityPosition(StaticFunctionTag* base, UInt32 entityID)
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

void f4mp::F4MP::SetEntityPosition(StaticFunctionTag* base, UInt32 entityID, float x, float y, float z)
{
	F4MP& self = GetInstance();

	librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
	if (!entity)
	{
		_ERROR("no entity with ID '%d'!", entityID);
		return;
	}

	// HACK: temporary

	if (entityID == self.player->GetEntityID())
	{
		for (auto& instance : instances)
		{
			entity = librg_entity_fetch(&instance->ctx, instance->player->GetEntityID());
			entity->position.x = x;
			entity->position.y = y;
			entity->position.z = z;
		}
	}

	entity->position.x = x;
	entity->position.y = y;
	entity->position.z = z;
}

void f4mp::F4MP::SetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name, Float32 value)
{
	F4MP& self = GetInstance();

	librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
	if (!entity)
	{
		_ERROR("no entity with ID '%d'!", entityID);
		return;
	}

	Player::Get(entity)->SetNumber(name.c_str(), value);
}

void f4mp::F4MP::SetEntVarAnim(StaticFunctionTag* base, UInt32 entityID, BSFixedString animState)
{
	F4MP& self = GetInstance();

	librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
	if (!entity)
	{
		_ERROR("no entity with ID '%d'!", entityID);
		return;
	}

	Player::Get(entity)->SetAnimState(animState.c_str());
}

Float32 f4mp::F4MP::GetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name)
{
	F4MP& self = GetInstance();

	librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
	if (!entity)
	{
		_ERROR("no entity with ID '%d'!", entityID);
		return 0.f;
	}

	return Player::Get(entity)->GetNumber(name.c_str());
}

BSFixedString f4mp::F4MP::GetEntVarAnim(StaticFunctionTag* base, UInt32 entityID)
{
	F4MP& self = GetInstance();

	librg_entity* entity = librg_entity_fetch(&self.ctx, entityID);
	if (!entity)
	{
		_ERROR("no entity with ID '%d'!", entityID);
		return Animation::GetStateName(0).c_str();
	}

	return Player::Get(entity)->GetAnimState().c_str();
}

Float32 f4mp::F4MP::Atan2(StaticFunctionTag* base, Float32 y, Float32 x)
{
	const Float32 pi = 3.14159265359f;
	return fmod(atan2(y, x) + 2.f * pi, 2.f * pi);
}

BSFixedString f4mp::F4MP::_GetWalkDir(StaticFunctionTag* base, Float32 dX, Float32 dY, Float32 angleZ)
{
	switch (Player::GetWalkDir(zpl_vec2{ dX, dY }, angleZ))
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

bool f4mp::F4MP::AnimLoops(StaticFunctionTag* base, BSFixedString animState)
{
	return Animation::Loops(Animation::GetStateID(animState.c_str()));
}

void f4mp::F4MP::CopyAppearance(StaticFunctionTag* base, TESNPC* src, TESNPC* dest)
{
	client::AppearanceData appearance;
	appearance.Fill(src);
	Player::SetAppearance(dest, appearance);
}

void f4mp::F4MP::CopyWornItems(StaticFunctionTag* base, Actor* src, Actor* dest)
{
	client::WornItemsData wornItems;
	wornItems.Fill(src);
	Player::SetWornItems(dest, wornItems);
}

void f4mp::F4MP::PlayerHit(StaticFunctionTag* base, UInt32 hitter, UInt32 hittee, Float32 damage)
{
	F4MP& self = GetInstance();

	HitData data{ hitter, hittee, damage };

	librg_message_send_all(&self.ctx, Message::Hit, &data, sizeof(HitData));
}

void f4mp::F4MP::PlayerFireWeapon(StaticFunctionTag* base)
{
	F4MP& self = GetInstance();

	UInt32 playerEntityID = self.player->GetEntityID();
	librg_message_send_all(&self.ctx, Message::FireWeapon, &playerEntityID, sizeof(UInt32));
}
