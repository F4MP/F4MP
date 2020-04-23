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
#include "f4se/NiNodes.h"

#include <fstream>
#include <iterator>
#include <shlobj.h>				// CSIDL_MYCODUMENTS

std::vector<std::unique_ptr<f4mp::F4MP>> f4mp::F4MP::instances;
size_t f4mp::F4MP::activeInstance = 0, f4mp::F4MP::nextActiveInstance = 0;

f4mp::F4MP& f4mp::F4MP::GetInstance()
{
	if (instances.size() == 0)
	{
		instances.push_back(std::make_unique<F4MP>());
		instances.back()->player = std::make_unique<Player>();
	}

	return *instances[activeInstance];
}

f4mp::F4MP::F4MP() : ctx{}, port(0), handle(kPluginHandle_Invalid), messaging(nullptr), papyrus(nullptr), task(nullptr)
{
	ctx.tick_delay = 10.0;
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

	librg_network_add(&ctx, MessageType::Hit, OnHit);
	librg_network_add(&ctx, MessageType::FireWeapon, OnFireWeapon);
	librg_network_add(&ctx, MessageType::SyncEntity, OnSyncEntity);
	librg_network_add(&ctx, MessageType::SpawnBuilding, OnSpawnBuilding);
	librg_network_add(&ctx, MessageType::RemoveBuilding, OnRemoveBuilding);

	char	path[MAX_PATH];
	HRESULT err = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path);
	if (!SUCCEEDED(err))
	{
		_FATALERROR("SHGetFolderPath %08X failed (result = %08X lasterr = %08X)", CSIDL_MYDOCUMENTS, err, GetLastError());
	}
	ASSERT_CODE(SUCCEEDED(err), err);
	strcat_s(path, sizeof(path), "\\My Games\\Fallout4\\F4MP\\config.txt");

	std::ifstream configFile(path);
	if (!configFile)
	{
		std::ofstream newConfigFile(path);
		newConfigFile << "localhost" << std::endl;

		configFile = std::ifstream(path);
	}

	configFile >> config.hostAddress;
}

f4mp::F4MP::~F4MP()
{
	librg_free(&ctx);
}

bool f4mp::F4MP::Init(const F4SEInterface* f4se)
{
	FILE* tmp = nullptr;
	AllocConsole();
	freopen_s(&tmp, "CONOUT$", "w", stdout);

	printf("console opened\n");

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

			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("IsConnected", "F4MP", IsConnected, vm));
			vm->RegisterFunction(new NativeFunction4<StaticFunctionTag, bool, Actor*, TESNPC*, BSFixedString, SInt32>("Connect", "F4MP", Connect, vm));
			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("Disconnect", "F4MP", Disconnect, vm));
			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Tick", "F4MP", Tick, vm));
			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("SyncWorld", "F4MP", SyncWorld, vm));

			vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("GetPlayerEntityID", "F4MP", GetPlayerEntityID, vm));
			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, UInt32, TESObjectREFR*>("GetEntityID", "F4MP", GetEntityID, vm));
			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, UInt32, TESObjectREFR*>("SetEntityRef", "F4MP", SetEntityRef, vm));

			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, UInt32>("IsEntityValid", "F4MP", IsEntityValid, vm));

			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<Float32>, UInt32>("GetEntityPosition", "F4MP", GetEntityPosition, vm));
			vm->RegisterFunction(new NativeFunction4<StaticFunctionTag, void, UInt32, Float32, Float32, Float32>("SetEntityPosition", "F4MP", SetEntityPosition, vm));

			vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, UInt32, BSFixedString, Float32>("SetEntVarNum", "F4MP", SetEntVarNum, vm));
			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, UInt32, BSFixedString>("SetEntVarAnim", "F4MP", SetEntVarAnim, vm));
			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, Float32, UInt32, BSFixedString>("GetEntVarNum", "F4MP", GetEntVarNum, vm));
			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, UInt32>("GetEntVarAnim", "F4MP", GetEntVarAnim, vm));

			vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMArray<TESObjectREFR*>, TESObjectCELL*>("GetRefsInCell", "F4MP", GetRefsInCell, vm));
			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, Float32, Float32, Float32>("Atan2", "F4MP", Atan2, vm));

			vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, BSFixedString, Float32, Float32, Float32>("GetWalkDir", "F4MP", GetWalkDir, vm));

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

			vm->SetFunctionFlags("F4MP", "GetClientInstanceID", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "SetClient", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "IsConnected", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "Connect", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "Disconnect", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "Tick", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "SyncWorld", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetPlayerEntityID", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetEntityID", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "SetEntityRef", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "IsEntityValid", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetEntityPosition", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "SetEntityPosition", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "SetEntVarNum", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "SetEntVarAnim", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetEntVarNum", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetEntVarAnim", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetRefsInCell", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "Atan2", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetWalkDir", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "GetAction", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "Inspect", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "AnimLoops", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "CopyAppearance", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "CopyWornItems", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "PlayerHit", IFunction::kFunctionFlag_NoWait);
			vm->SetFunctionFlags("F4MP", "PlayerFireWeapon", IFunction::kFunctionFlag_NoWait);

			return true;
		}))
	{
		return false;
	}

	Animation::Init();

	return true;
}

librg_entity* f4mp::F4MP::FetchEntity(UInt32 id, const std::string& errorMsg)
{
	librg_entity* entity = librg_entity_fetch(&ctx, id);
	if (!entity)
	{
		_ERROR(errorMsg.c_str(), id);
	}

	return entity;
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

void f4mp::F4MP::SyncTransform(TESObjectREFR* ref, zpl_vec3 position, zpl_vec3 angles, bool ignoreAngleXY)
{
	// HACK: somehow calling F4MP::TranslateTo causes a crash.

	if (!ref)
	{
		return;
	}

	if (ignoreAngleXY)
	{
		angles.x = angles.y = 0.f;
	}

	VMVariable x, y, z, angleX, angleY, angleZ, speedVar, rotSpeedVar;
	x.Set<Float32>(&position.x); y.Set<Float32>(&position.y); z.Set<Float32>(&position.z);
	angleX.Set<Float32>(&angles.x); angleY.Set<Float32>(&angles.y); angleZ.Set<Float32>(&angles.z);

	float speed = zpl_vec3_mag(position - (zpl_vec3&)ref->pos) * 3.f, rotSpeed = 500.f;
	speedVar.Set<Float32>(&speed); rotSpeedVar.Set<Float32>(&rotSpeed);

	VMArray<VMVariable> args;
	args.Push(&x); args.Push(&y); args.Push(&z);
	args.Push(&angleX); args.Push(&angleY); args.Push(&angleZ);
	args.Push(&speedVar); args.Push(&rotSpeedVar);

	CallFunctionNoWait(ref, "TranslateTo", args);
}

void f4mp::F4MP::TranslateTo(TESObjectREFR* ref, zpl_vec3 position, zpl_vec3 angles, Float32 speed, Float32 rotSpeed)
{
	if (!ref)
	{
		return;
	}

	VMVariable x, y, z, angleX, angleY, angleZ, speedVar, rotSpeedVar;
	x.Set<Float32>(&position.x); y.Set<Float32>(&position.y); z.Set<Float32>(&position.z);
	angleX.Set<Float32>(&angles.x); angleY.Set<Float32>(&angles.y); angleZ.Set<Float32>(&angles.z);

	float _speed = 500.f, _rotSpeed = 500.f;
	speedVar.Set<Float32>(&_speed); rotSpeedVar.Set<Float32>(&_rotSpeed);

	VMArray<VMVariable> args;
	args.Push(&x); args.Push(&y); args.Push(&z);
	args.Push(&angleX); args.Push(&angleY); args.Push(&angleZ);
	args.Push(&speedVar); args.Push(&rotSpeedVar);

	CallFunctionNoWait(ref, "TranslateTo", args);
}

void f4mp::F4MP::SetTransform(TESObjectREFR* ref, zpl_vec3 position, zpl_vec3 angles)
{
	if (!ref)
	{
		return;
	}

	VMVariable x, y, z, angleX, angleY, angleZ, speed, rotSpeed;
	x.Set<Float32>(&position.x); y.Set<Float32>(&position.y); z.Set<Float32>(&position.z);
	angleX.Set<Float32>(&angles.x); angleY.Set<Float32>(&angles.y); angleZ.Set<Float32>(&angles.z);

	VMArray<VMVariable> args1, args2;
	args1.Push(&x); args1.Push(&y); args1.Push(&z);
	args2.Push(&angleX); args2.Push(&angleY); args2.Push(&angleZ);

	CallFunctionNoWait(ref, "SetPosition", args1);
	CallFunctionNoWait(ref, "SetAngle", args2);
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

	Entity::Create(event);

	// The Papyrus event "OnEntityCreate" is proccessed by the Player class.
}

void f4mp::F4MP::OnEntityUpdate(librg_event* event)
{
	//F4MP& self = GetInstance();
	//
	//self.papyrus->GetExternalEventRegistrations("OnEntityUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
	//	{
	//		librg_event* event = static_cast<librg_event*>(dataPtr);
	//		UInt32 id = event->entity->id;
	//		SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
	//	});

	Entity* entity = Entity::Get(event);
	if (!entity)
	{
		return;
	}

	entity->OnEntityUpdate(event);
}

void f4mp::F4MP::OnEntityRemove(librg_event* event)
{
	Entity* entity = Entity::Get(event);
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
	//F4MP& self = GetInstance();
	//
	//self.papyrus->GetExternalEventRegistrations("OnClientUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
	//	{
	//		librg_event* event = static_cast<librg_event*>(dataPtr);
	//		UInt32 id = event->entity->id;
	//		SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
	//	});

	Entity* entity = Entity::Get(event);
	if (!entity)
	{
		return;
	}

	entity->OnClientUpdate(event);
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

	for (auto& instance : instances)
	{
		Player* player = Entity::GetAs<Player>(instance->FetchEntity(entity));
		if (!player || !player->GetRef())
		{
			continue;
		}

		VMArray<VMVariable> args;
		CallFunctionNoWait(player->GetRef(), "FireWeapon", args);
	}
}

void f4mp::F4MP::OnSyncEntity(librg_message* msg)
{
	F4MP& self = GetInstance();

	SyncEntityData data;
	librg_data_rptr(msg->data, &data, sizeof(SyncEntityData));

	TESObjectREFR* ref = DYNAMIC_CAST(LookupFormByID(data.formID), TESForm, TESObjectREFR);
	if (ref != nullptr)
	{
		SetTransform(ref, data.position, data.angles);
	}
}

void f4mp::F4MP::OnSpawnBuilding(librg_message* msg)
{
	F4MP& self = GetInstance();

	SpawnBuildingData data;
	librg_data_rptr(msg->data, &data, sizeof(SpawnBuildingData));

	if (data.baseFormID == 0)
	{
		return;
	}

	UInt64 uniqueID = GetUniqueFormID(data.ownerEntityID, data.formID);

	auto othersBuilding = self.buildings.find(uniqueID);
	if (othersBuilding != self.buildings.end())
	{
		TESObjectREFR* ref = DYNAMIC_CAST(LookupFormByID(othersBuilding->second.formID), TESForm, TESObjectREFR);
		if (ref)
		{
			TranslateTo(ref, data.position, data.angles, 500.f, 500.f);
			return;
		}
	}

	TESObjectREFR* building = PlaceAtMe_Native((*g_gameVM)->m_virtualMachine, 0, (TESObjectREFR**)g_player.GetPtr(), LookupFormByID(data.baseFormID), 1, true, false, false);
	SetTransform(building, data.position, data.angles);

	printf("building spawned: %llx %f %f %f\n", uniqueID, data.position.x, data.position.y, data.position.z);

	for (auto& instance : instances)
	{
		instance->buildings[uniqueID] = { building->formID, data.position, data.angles };
		instance->knownBuildings.insert(building->formID);
	}
}

void f4mp::F4MP::OnRemoveBuilding(librg_message* msg)
{
	F4MP& self = GetInstance();

	RemoveBuildingData data;
	librg_data_rptr(msg->data, &data, sizeof(RemoveBuildingData));

	auto building = self.buildings.find(data.uniqueFormID);
	if (building != self.buildings.end())
	{
		TESObjectREFR* ref = DYNAMIC_CAST(LookupFormByID(building->second.formID), TESForm, TESObjectREFR);
		if (ref)
		{
			VMArray<VMVariable> args;
			CallFunctionNoWait(ref, "Delete", args);
			ref->flags |= TESObjectREFR::kFlag_IsDeleted;
		}

		printf("building removed: %llx %p\n", data.uniqueFormID, ref);

		for (auto& instance : instances)
		{
			instance->buildings.erase(building->first);
			instance->knownBuildings.erase(building->second.formID);
		}
	}
}

UInt32 f4mp::F4MP::GetClientInstanceID(StaticFunctionTag* base)
{
	return activeInstance;
}

void f4mp::F4MP::SetClient(StaticFunctionTag* base, UInt32 instance)
{
	nextActiveInstance = instance;
}

bool f4mp::F4MP::IsConnected(StaticFunctionTag* base)
{
	return !!librg_is_connected(&GetInstance().ctx);
}

bool f4mp::F4MP::Connect(StaticFunctionTag* base, Actor* player, TESNPC* playerActorBase, BSFixedString address, SInt32 port)
{
	F4MP& self = GetInstance();
	librg_network_stop(&self.ctx);

	self.player = std::make_unique<Player>();
	self.player->OnConnect(player, playerActorBase);

	self.address = strlen(address.c_str()) > 0 ? address.c_str() : self.config.hostAddress;
	self.port = port;

	if (librg_network_start(&self.ctx, { self.port, const_cast<char*>(self.address.c_str()) }))
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
		// this part is essential
		if (librg_is_connected(&instance->ctx))
		{
			SyncWorld(base);
		}

		librg_tick(&instance->ctx);
	}

	activeInstance = nextActiveInstance;

	while (instances.size() <= activeInstance)
	{
		instances.push_back(std::make_unique<F4MP>());
		instances.back()->player = std::make_unique<Player>();

		F4MP& mainInstance = *instances.front();
		F4MP& newInstance = *instances.back();
		newInstance.messaging = mainInstance.messaging;
		newInstance.papyrus = mainInstance.papyrus;
		newInstance.task = mainInstance.task;
	}
}

void f4mp::F4MP::SyncWorld(StaticFunctionTag* base)
{
	F4MP& self = GetInstance();
	if (!librg_is_connected(&self.ctx))
	{
		return;
	}

	const float syncRadius = 500.f;
	const float syncRadiusSqr = syncRadius * syncRadius;

	PlayerCharacter* player = *g_player;
	auto& refs = player->parentCell->objectList;
	const NiPoint3& playerPos = player->pos;

	std::list<UInt32> newBuildings;
	
	for (UInt32 i = 0; i < refs.count; i++)
	{
		TESObjectREFR* ref = refs[i];
		Actor* actor = DYNAMIC_CAST(ref, TESForm, Actor);
		bool isActor = actor != nullptr;

		if ((ref->formID & 0xff000000) == 0xff000000)
		{
			if (ref->baseForm->formType == 36 && !(ref->flags & TESObjectREFR::kFlag_IsDeleted))
			{
				// 36 means static objects, right?
				if (ref->baseForm && ref->baseForm->formType == 36 && self.knownBuildings.count(ref->formID) == 0)
				{
					newBuildings.push_back(ref->formID);
				}
			}
		}
		else
		{
			if (isActor)
			{
				if (ref == player || ref->baseForm == nullptr)
				{
					continue;
				}

				if (self.entityIDs.find(ref->formID) == self.entityIDs.end())
				{
					self.entityIDs[ref->formID] = (UInt32)-1;

					TESFullName* fullName = DYNAMIC_CAST(ref->baseForm, TESForm, TESFullName);
					if (fullName == nullptr || strstr(fullName->name.c_str(), "F4MP") != nullptr)
					{
						continue;
					}

					SpawnEntityData data{ ref->formID, (zpl_vec3&)ref->pos, ToDegrees((zpl_vec3&)ref->rot) };
					librg_message_send_all(&self.ctx, MessageType::SpawnEntity, &data, sizeof(SpawnEntityData));

					printf("%s(%x)\n", fullName->name.c_str(), data.formID);
				}
			}
			else
			{
				NiPoint3 dist = playerPos - ref->pos;
				float distSqr = zpl_vec3_mag2({ dist.x, dist.y, dist.z });

				if (distSqr < syncRadiusSqr)
				{
					SyncEntityData data{ ref->formID, (zpl_vec3&)ref->pos, ToDegrees((zpl_vec3&)ref->rot), librg_time_now(&self.ctx) };
					librg_message_send_all(&self.ctx, MessageType::SyncEntity, &data, sizeof(SyncEntityData));
				}
			}
		}

		// TODO: optimize to work by events

		const float epsilon = 1e-8f;

		if (newBuildings.size() > 0)
		{
			for (auto it = newBuildings.begin(); it != newBuildings.end();)
			{
				TESObjectREFR* ref = DYNAMIC_CAST(LookupFormByID(*it), TESForm, TESObjectREFR);
				if (!ref || (ref->flags & TESObjectREFR::kFlag_IsDeleted))
				{
					it = newBuildings.erase(it);
					continue;
				}

				// HACK: find the real solution, not the way around it!
				/*if (std::find_if(self.buildings.begin(), self.buildings.end(), [&](const std::pair<UInt64, TransformData>& building)
					{
						TESObjectREFR* known = DYNAMIC_CAST(LookupFormByID(building.second.formID), TESForm, TESObjectREFR);
						if (!known)
						{
							return false;
						}

						return zpl_vec3_mag2((zpl_vec3&)known->pos - (zpl_vec3&)ref->pos) < epsilon;
					}) != self.buildings.end())
				{
					it = newBuildings.erase(it);
					continue;
				}*/

				SpawnBuildingData data{ self.player->GetEntityID(), ref->formID, ref->baseForm->formID, (zpl_vec3&)ref->pos, ToDegrees((zpl_vec3&)ref->rot) };
				librg_message_send_all(&self.ctx, MessageType::SpawnBuilding, &data, sizeof(SpawnBuildingData));

				for (auto& instance : instances)
				{
					instance->buildings[GetUniqueFormID(data.ownerEntityID, data.formID)] = { data.formID, data.position, data.angles };
					instance->knownBuildings.insert(data.formID);
				}

				it++;
			}
		}

		for (auto it = self.buildings.begin(); it != self.buildings.end();)
		{
			UInt64 uniqueID = it->first;
			UInt32 formID = it->second.formID;
			TESObjectREFR* ref = DYNAMIC_CAST(LookupFormByID(formID), TESForm, TESObjectREFR);

			if (ref)
			{
				if (zpl_vec3_mag2((zpl_vec3&)ref->pos - it->second.position) > epsilon || zpl_vec3_mag2(ToDegrees((zpl_vec3&)ref->rot) - it->second.angles) > epsilon)
				{
					// HACK: 0 for just a transform update.
					SpawnBuildingData data{ self.player->GetEntityID(), ref->formID, 0/*ref->baseForm->formID*/, (zpl_vec3&)ref->pos, ToDegrees((zpl_vec3&)ref->rot) };
					librg_message_send_all(&self.ctx, MessageType::SpawnBuilding, &data, sizeof(SpawnBuildingData));

					it->second.position = data.position;
					it->second.angles = data.angles;
				}
				it++;
			}
			else
			{
				RemoveBuildingData data{ uniqueID };
				librg_message_send_all(&self.ctx, MessageType::RemoveBuilding, &data, sizeof(RemoveBuildingData));

				it = self.buildings.erase(it);

				for (auto& instance : instances)
				{
					instance->buildings.erase(uniqueID);
					instance->knownBuildings.erase(formID);
				}
			}
		}
	}
}

UInt32 f4mp::F4MP::GetPlayerEntityID(StaticFunctionTag* base)
{
	F4MP& self = GetInstance();
	return self.player->GetEntityID();
}

UInt32 f4mp::F4MP::GetEntityID(StaticFunctionTag* base, TESObjectREFR* ref)
{
	F4MP& self = GetInstance();

	auto entityID = self.entityIDs.find(ref->formID);
	if (entityID == self.entityIDs.end())
	{
		return (UInt32)-1;
	}
	
	return entityID->second;
}

void f4mp::F4MP::SetEntityRef(StaticFunctionTag* base, UInt32 entityID, TESObjectREFR* ref)
{
	Entity::Get(GetInstance().FetchEntity(entityID))->SetRef(ref);
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

	librg_entity* entity = self.FetchEntity(entityID);
	if (!entity)
	{
		return VMArray<Float32>(result);
	}

	result = { entity->position.x, entity->position.y, entity->position.z };
	return VMArray<Float32>(result);
}

void f4mp::F4MP::SetEntityPosition(StaticFunctionTag* base, UInt32 entityID, float x, float y, float z)
{
	F4MP& self = GetInstance();

	librg_entity* entity = self.FetchEntity(entityID);
	if (!entity)
	{
		return;
	}

	// HACK: temporary

	if (entityID == self.player->GetEntityID())
	{
		for (auto& instance : instances)
		{
			if (!librg_is_connected(&instance->ctx))
			{
				continue;
			}

			entity = instance->FetchEntity(instance->player->GetEntityID());
			entity->position.x = x;
			entity->position.y = y;
			entity->position.z = z;
		}

		return;
	}

	entity->position.x = x;
	entity->position.y = y;
	entity->position.z = z;
}

void f4mp::F4MP::SetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name, Float32 value)
{
	F4MP& self = GetInstance();

	Player* player = Entity::GetAs<Player>(self.FetchEntity(entityID));
	if (!player)
	{
		return;
	}

	player->SetNumber(name.c_str(), value);
}

void f4mp::F4MP::SetEntVarAnim(StaticFunctionTag* base, UInt32 entityID, BSFixedString animState)
{
	F4MP& self = GetInstance();

	Player* player = Entity::GetAs<Player>(self.FetchEntity(entityID));
	if (!player)
	{
		return;
	}

	player->SetAnimState(animState.c_str());
}

Float32 f4mp::F4MP::GetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name)
{
	F4MP& self = GetInstance();

	Player* player = Entity::GetAs<Player>(self.FetchEntity(entityID));
	if (!player)
	{
		return 0.f;
	}

	return player->GetNumber(name.c_str());
}

BSFixedString f4mp::F4MP::GetEntVarAnim(StaticFunctionTag* base, UInt32 entityID)
{
	F4MP& self = GetInstance();

	Player* player = Entity::GetAs<Player>(self.FetchEntity(entityID));
	if (!player)
	{
		return Animation::GetStateName(0).c_str();
	}

	return player->GetAnimState().c_str();
}

VMArray<TESObjectREFR*> f4mp::F4MP::GetRefsInCell(StaticFunctionTag* base, TESObjectCELL* cell)
{
	return std::vector<TESObjectREFR*>(&cell->objectList.entries[0], &cell->objectList.entries[cell->objectList.count]);
}

Float32 f4mp::F4MP::Atan2(StaticFunctionTag* base, Float32 y, Float32 x)
{
	const Float32 pi = 3.14159265359f;
	return fmod(atan2(y, x) + 2.f * pi, 2.f * pi);
}

BSFixedString f4mp::F4MP::GetWalkDir(StaticFunctionTag* base, Float32 dX, Float32 dY, Float32 angleZ)
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

/*static void CopyHeardPart(BGSHeadPart* src, BGSHeadPart*& dest)
{
	dest = (BGSHeadPart*)Heap_Allocate(sizeof(BGSHeadPart));
	memset(dest, 0, sizeof(BGSHeadPart));

	*dest = *src;

	return;

	dest->fullName = src->fullName;
	dest->partFlags = src->partFlags;
	dest->type = src->type;

	for (UInt32 i = 0; i < src->extraParts.count; i++)
	{
		BGSHeadPart* extra;
		CopyHeardPart(src->extraParts[i], extra);
		dest->extraParts.Push(extra);
	}

	return;

	src->textureSet = dest->textureSet;

	//dest->model = src->model;
	//std::copy(&src->morphs[0], &src->morphs[3], &dest->morphs[0]);

	dest->unk158 = src->unk158;
	dest->partName = src->partName;
}*/

void f4mp::F4MP::PlayerHit(StaticFunctionTag* base, UInt32 hitter, UInt32 hittee, Float32 damage)
{
	F4MP& self = GetInstance();

	HitData data{ hitter, hittee, damage };

	librg_message_send_all(&self.ctx, MessageType::Hit, &data, sizeof(HitData));
}

void f4mp::F4MP::PlayerFireWeapon(StaticFunctionTag* base)
{
	F4MP& self = GetInstance();

	UInt32 playerEntityID = self.player->GetEntityID();
	librg_message_send_all(&self.ctx, MessageType::FireWeapon, &playerEntityID, sizeof(UInt32));
}
