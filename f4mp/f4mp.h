#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PluginAPI.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/GameThreads.h"
#include "f4se/GameObjects.h"
#include "f4se/GameForms.h"
#include "f4se/GameFormComponents.h"
#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameRTTI.h"

#include "common.h"

#include <librg.h>

#include <memory>
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <algorithm>

namespace f4mp
{
	namespace client
	{
		struct AppearanceData : public f4mp::AppearanceData
		{
			void Fill(TESNPC* actorBase)
			{
				Clear();

				female = CALL_MEMBER_FN(actorBase, GetSex)() == 1;
				weights = { actorBase->weightThin, actorBase->weightMuscular, actorBase->weightLarge };
				hairColor = actorBase->headData->hairColor->fullName.name;

				for (UInt8 i = 0; i < actorBase->numHeadParts; i++)
				{
					headParts.push_back(actorBase->headParts[i]->partName.c_str());
				}

				for (UInt32 i = 0; i < actorBase->morphSetValue->count; i++)
				{
					morphSetValue.push_back((*actorBase->morphSetValue)[i]);
				}

				actorBase->morphRegionData->ForEach([=](TESNPC::FaceMorphRegion* region)
					{
						//morphRegionData.push_back(std::make_tuple(region->index, std::vector<f32>(&region->value[0], &region->value[8])));
						morphRegionData1.push_back(region->index);
						morphRegionData2.push_back(std::vector<f32>(&region->value[0], &region->value[8]));
						return true;
					});

				actorBase->morphSetData->ForEach([&](TESNPC::MorphSetData* data)
					{
						//morphSetData.push_back(std::make_tuple(data->key, data->value));
						morphSetData1.push_back(data->key);
						morphSetData2.push_back(data->value);
						return true;
					});
			}
		};

		struct WornItemsData : public f4mp::WornItemsData
		{
			void Fill(Actor* actor)
			{
				Clear();

				for (UInt32 i = 0; i < ActorEquipData::kMaxSlots; i++)
				{
					TESForm* item = actor->equipData->slots[i].item;
					if (item)
					{
						/*if (std::find_if(data.begin(), data.end(), [&](const std::tuple<u8, std::string>& wornItem)
							{
								return std::get<0>(wornItem) == item->formType && std::get<1>(wornItem).compare(item->GetFullName()) == 0;
							}) == data.end())
						{
							data.push_back(std::make_pair(item->formType, item->GetFullName()));
						}*/

						size_t j;
						for (j = 0; j < data1.size(); j++)
						{
							if (data1[j] == item->formType && data2[j].compare(item->GetFullName()) == 0)
							{
								break;
							}
						}

						if (j == data1.size())
						{
							data1.push_back(item->formType);
							data2.push_back(item->GetFullName());
						}
					}
				}
			}
		};
	}

	struct FrameData
	{
		std::unordered_map<std::string, Float32> numbers;
		std::unordered_map<std::string, SInt32> integers;

		std::vector<HitData> events;
		int weaponFires = 0; // TODO: temporary
	};

	struct PlayerData
	{
		std::unordered_map<std::string, Float32> numbers;
		std::unordered_map<std::string, SInt32> integers;

		client::AppearanceData appearance;
		client::WornItemsData wornItems;

		PlayerData()
		{

		}
	};

	class Task : public ITaskDelegate
	{
	public:
		Task(const std::function<void()>& callback) : callback(callback)
		{

		}

		void Run() override
		{
			callback();
		}

		std::function<void()> callback;
	};

	class F4MP
	{
	public:
		static F4MP& GetInstance()
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

		F4MP() : ctx{}, port(0), playerEntityID((UInt32)-1), handle(kPluginHandle_Invalid), messaging(nullptr), papyrus(nullptr), task(nullptr),
			animStates
		{
			//TODO: animation priority
			"None",

			"JogForward",
			"JogBackward",
			"JogLeft",
			"JogRight",

			"JumpUp",
			"JumpFall",
			"JumpLand",

			"FireWeapon"
		}
		{
			for (SInt32 i = 0; i < static_cast<SInt32>(animStates.size()); i++)
			{
				const std::string& state = animStates[i];
				std::string lowerState(state.length(), '\0');

				for (size_t i = 0; i < state.length(); i++)
				{
					lowerState[i] = tolower(state[i]);
				}

				animStateIDs[lowerState] = i;
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

			librg_network_add(&ctx, Message::Hit, OnHit);
		}

		virtual ~F4MP()
		{
			librg_free(&ctx);
		}

		bool Init(const F4SEInterface* f4se)
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

					vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, TESNPC*, TESNPC*>("CopyAppearance", "F4MP", CopyAppearance, vm));
					vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, Actor*, Actor*>("CopyWornItems", "F4MP", CopyWornItems, vm));

					vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, void, UInt32, UInt32, Float32>("PlayerHit", "F4MP", PlayerHit, vm));
					vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("PlayerFireWeapon", "F4MP", PlayerFireWeapon, vm));
					
					return true;
				}))
			{
				return false;
			}

			return true;
		}

		SInt32 GetAnimStateID(PlayerData* data)
		{
			return data->integers["animState"];
		}

		const std::string& GetAnimState(PlayerData* data)
		{
			return animStates[GetAnimStateID(data)];
		}

		void SetAnimStateID(PlayerData* data, SInt32 stateID)
		{
			data->integers["animState"] = stateID;
		}

		void SetAnimState(PlayerData* data, const std::string& state)
		{
			std::string lowerState(state.length(), '\0');

			for (size_t i = 0; i < state.length(); i++)
			{
				lowerState[i] = tolower(state[i]);
			}

			SetAnimStateID(data, animStateIDs[lowerState]);
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

		static void SetAppearance(TESNPC* actorBase, const AppearanceData& appearance)
		{
			//sex
			//HACK: i don't know if it's legit or not
			if (appearance.female)
			{
				actorBase->actorData.flags |= TESActorBaseData::kFlagFemale;
			}
			else
			{
				actorBase->actorData.flags &= ~TESActorBaseData::kFlagFemale;
			}

			//HACK: there might be some memory leaks going around that might cause horrible problems. i'm sorry i don't know better..

			if (actorBase->headData)
			{
				Heap_Free(actorBase->headData);
				actorBase->headData = nullptr;
			}

			if (actorBase->headParts)
			{
				Heap_Free(actorBase->headParts);
				actorBase->headParts = nullptr;
			}

			if (actorBase->morphSetValue)
			{
				Heap_Free(actorBase->morphSetValue);
				actorBase->morphSetValue = nullptr;
			}

			if (actorBase->morphRegionData)
			{
				Heap_Free(actorBase->morphRegionData);
				actorBase->morphRegionData = nullptr;
			}

			if (actorBase->morphSetData)
			{
				Heap_Free(actorBase->morphSetData);
				actorBase->morphSetData = nullptr;
			}

			if (actorBase->tints)
			{
				Heap_Free(actorBase->tints);
				actorBase->tints = nullptr;
			}

			actorBase->headData = new TESNPC::HeadData();
			tArray<BGSColorForm*>& colors = (*g_dataHandler)->arrCLFM;

			UInt32 j;

			for (j = 0; j < colors.count; j++)
			{
				if (appearance.hairColor.compare(colors[j]->fullName.name.c_str()) == 0)
				{
					actorBase->headData->hairColor = colors[j];
					break;
				}
			}

			if (j == colors.count)
			{
				_MESSAGE("hairColor: %s", appearance.hairColor.c_str());
			}

			//TODO: texture sync
			//dest->headData->faceTextures = src->headData->faceTextures;

			actorBase->weightThin = appearance.weights[0];
			actorBase->weightMuscular = appearance.weights[1];
			actorBase->weightLarge = appearance.weights[2];

			actorBase->numHeadParts = appearance.headParts.size();
			actorBase->headParts = (BGSHeadPart**)Heap_Allocate(sizeof(BGSHeadPart*) * actorBase->numHeadParts);

			for (UInt8 i = 0; i < actorBase->numHeadParts; i++)
			{
				tArray<BGSHeadPart*>& headParts = (*g_dataHandler)->arrHDPT;

				for (j = 0; j < headParts.count; j++)
				{
					if (appearance.headParts[i].compare(headParts[j]->partName.c_str()) == 0)
					{
						actorBase->headParts[i] = headParts[j];
						break;
					}
				}

				if (j == headParts.count)
				{
					_MESSAGE("hairPart: %s", appearance.headParts[i].c_str());
				}
			}

			actorBase->morphSetValue = new tArray<float>();
			for (UInt32 i = 0; i < appearance.morphSetValue.size(); i++)
			{
				actorBase->morphSetValue->Push(appearance.morphSetValue[i]);
			}

			actorBase->morphRegionData = new tHashSet<TESNPC::FaceMorphRegion, UInt32>();
			/*for(const auto& region : appearance.morphRegionData)
			{
				TESNPC::FaceMorphRegion tmp;
				tmp.index = std::get<0>(region);
				std::copy(std::get<1>(region).begin(), std::get<1>(region).end(), tmp.value);
				actorBase->morphRegionData->Add(&tmp);
			}*/

			for (size_t i = 0; i < appearance.morphRegionData1.size(); i++)
			{
				TESNPC::FaceMorphRegion tmp;
				tmp.index = appearance.morphRegionData1[i];
				std::copy(appearance.morphRegionData2[i].begin(), appearance.morphRegionData2[i].end(), tmp.value);
				actorBase->morphRegionData->Add(&tmp);
			}

			actorBase->morphSetData = new tHashSet<TESNPC::MorphSetData, UInt32>();
			for (size_t i = 0; i < appearance.morphSetData1.size(); i++)
			{
				TESNPC::MorphSetData tmp;
				tmp.key = appearance.morphSetData1[i];
				tmp.value = appearance.morphSetData2[i];
				actorBase->morphSetData->Add(&tmp);
			}
			/*for (const auto& data : appearance.morphSetData)
			{
				TESNPC::MorphSetData tmp;
				tmp.key = std::get<0>(data);
				tmp.value = std::get<1>(data);
				actorBase->morphSetData->Add(&tmp);
			}*/

			//TODO: tint sync
			//dest->tints = src->tints;
		}

		static std::vector<TESForm*> DecodeWornItems(const WornItemsData& wornItems)
		{
			std::vector<TESForm*> items;

			//for (auto& item : wornItems.data)
			for(size_t k = 0; k < wornItems.data1.size(); k++)
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

		static void SetWornItems(Actor* actor, const WornItemsData& wornItems)
		{
			F4MP& self = GetInstance();

			struct Args
			{
				Actor* actor;
				const WornItemsData& wornItems;
			} args{ actor, wornItems };

			self.papyrus->GetExternalEventRegistrations("OnCopyWornItems", &args, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				Args* args = static_cast<Args*>(dataPtr);
				VMArray<TESForm*> itemsToWear(DecodeWornItems(args->wornItems));

				SendPapyrusEvent2<Actor*, VMArray<TESForm*>>(handle, scriptName, callbackName, args->actor, itemsToWear);
			});
		}

	private:
		static std::vector<std::unique_ptr<F4MP>> instances;
		static size_t activeInstance;

		std::string address;
		SInt32 port;

		UInt32 playerEntityID;

		zpl_vec3 prevPosition;

		client::AppearanceData playerAppearance;
		client::WornItemsData playerWornItems;

		librg_ctx ctx;

		PluginHandle handle;

		F4SEMessagingInterface* messaging;
		F4SEPapyrusInterface* papyrus;
		F4SETaskInterface* task;

		std::vector<std::string> animStates;
		std::unordered_map<std::string, SInt32> animStateIDs;

		std::queue<FrameData> records;

		static void OnConnectRequest(librg_event* event)
		{
			F4MP& self = GetInstance();
			_MESSAGE("trying to connect to %s:%d..", self.address.c_str(), self.port);

			Utils::Write(event->data, self.playerAppearance.female);
			Utils::Write(event->data, self.playerAppearance.weights);
			Utils::Write(event->data, self.playerAppearance.hairColor);
			Utils::Write(event->data, self.playerAppearance.headParts);
			Utils::Write(event->data, self.playerAppearance.morphSetValue);
			Utils::Write(event->data, self.playerAppearance.morphRegionData1);
			Utils::Write(event->data, self.playerAppearance.morphRegionData2);
			Utils::Write(event->data, self.playerAppearance.morphSetData1);
			Utils::Write(event->data, self.playerAppearance.morphSetData2);

			Utils::Write(event->data, self.playerWornItems.data1);
			Utils::Write(event->data, self.playerWornItems.data2);
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
			_MESSAGE("entity with ID '%d' has created", event->entity->id);

			PlayerData* data = new PlayerData();
			event->entity->user_data = data;

			Utils::Read(event->data, data->appearance.female);
			Utils::Read(event->data, data->appearance.weights);
			Utils::Read(event->data, data->appearance.hairColor);
			Utils::Read(event->data, data->appearance.headParts);
			Utils::Read(event->data, data->appearance.morphSetValue);
			Utils::Read(event->data, data->appearance.morphRegionData1);
			Utils::Read(event->data, data->appearance.morphRegionData2);
			Utils::Read(event->data, data->appearance.morphSetData1);
			Utils::Read(event->data, data->appearance.morphSetData2);

			Utils::Read(event->data, data->wornItems.data1);
			Utils::Read(event->data, data->wornItems.data2);

			for (const auto& item : data->wornItems.data2)
			{
				_MESSAGE("%s", item.c_str());
			}

			F4MP& self = GetInstance();
			
			auto& npcs = (*g_dataHandler)->arrNPC_;
			for (UInt32 i = 0; i < npcs.count; i++)
			{
				if (strcmp(npcs[i]->fullName.name.c_str(), "F4MP Player") == 0)
				{
					SetAppearance(npcs[i], data->appearance);
					break;
				}
			}

			struct Tmp
			{
				UInt32 id;
				const WornItemsData& wornItems;
			} tmp{ event->entity->id, data->wornItems };

			self.papyrus->GetExternalEventRegistrations("OnEntityCreate", &tmp, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				Tmp* tmp = static_cast<Tmp*>(dataPtr);
				VMArray<TESForm*> wornItems(DecodeWornItems(tmp->wornItems));

				SendPapyrusEvent2<UInt32, VMArray<TESForm*>>(handle, scriptName, callbackName, tmp->id, wornItems);
			});
		}

		static void OnEntityUpdate(librg_event* event)
		{
			F4MP& self = GetInstance();

			self.papyrus->GetExternalEventRegistrations("OnEntityUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				librg_event* event = static_cast<librg_event*>(dataPtr);
				UInt32 id = event->entity->id;
				SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
			});

			PlayerData* data = (PlayerData*)event->entity->user_data;
			if (!data)
			{
				return;
			}

			data->numbers["angleX"] = librg_data_rf32(event->data);
			data->numbers["angleY"] = librg_data_rf32(event->data);
			data->numbers["angleZ"] = librg_data_rf32(event->data);

			data->numbers["health"] = librg_data_rf32(event->data);

			self.SetAnimStateID(data, librg_data_ri32(event->data));
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

			self.papyrus->GetExternalEventRegistrations("OnClientUpdate", event, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
				{
					librg_event* event = static_cast<librg_event*>(dataPtr);
					UInt32 id = event->entity->id;
					SendPapyrusEvent1<UInt32>(handle, scriptName, callbackName, id);
				});

			PlayerData* data = (PlayerData*)event->entity->user_data;
			if (!data)
			{
				return;
			}

			const std::string& animState = self.GetAnimState(data);
			if (animState != "JumpUp" && animState != "JumpFall" && animState != "FireWeapon")
			{
				zpl_vec2 displacement{ event->entity->position.x - self.prevPosition.x, event->entity->position.y - self.prevPosition.y };
				const char* newAnimState;

				switch (GetWalkDir(displacement, data->numbers["angleZ"]))
				{
				case 0:
					newAnimState = "JogForward";
					break;
				case 1:
					newAnimState = "JogBackward";
					break;
				case 2:
					newAnimState = "JogLeft";
					break;
				case 3:
					newAnimState = "JogRight";
					break;
				default:
					newAnimState = "None";
				}

				self.SetAnimState(data, newAnimState);
			}

			self.prevPosition = event->entity->position;

			if (activeInstance == 0 && self.records.size() > 0)
			{
				FrameData replay = self.records.front();
				self.records.pop();

				librg_data_wf32(event->data, replay.numbers["angleX"]);
				librg_data_wf32(event->data, replay.numbers["angleY"]);
				librg_data_wf32(event->data, replay.numbers["angleZ"]);

				librg_data_wf32(event->data, replay.numbers["health"]);

				librg_data_wi32(event->data, replay.integers["animState"]);

				// TODO: temporary
				
				for (HitData& hitData : replay.events)
				{
					librg_message_send_all(&self.ctx, Message::Hit, &hitData, sizeof(HitData));
				}

				for (int i = 0; i < replay.weaponFires; i++)
				{
					librg_message_send_all(&self.ctx, Message::FireWeapon, nullptr, 0);
				}
			}
			else
			{
				librg_data_wf32(event->data, data->numbers["angleX"]);
				librg_data_wf32(event->data, data->numbers["angleY"]);
				librg_data_wf32(event->data, data->numbers["angleZ"]);

				librg_data_wf32(event->data, data->numbers["health"]);

				librg_data_wi32(event->data, self.GetAnimStateID(data));

				if (activeInstance > 0)
				{
					FrameData& record = self.records.back();

					for (const auto& number : data->numbers)
					{
						record.numbers[number.first] = number.second;
					}

					for (const auto& integer : data->integers)
					{
						record.integers[integer.first] = integer.second;
					}

					self.records.push(FrameData());
				}
			}

			if (self.GetAnimState(data) == "FireWeapon")
			{
				self.SetAnimState(data, "None");
			}
		}

		static void OnHit(librg_message* msg)
		{
			_MESSAGE("OnHit");

			HitData data;
			librg_data_rptr(msg->data, &data, sizeof(HitData));

			F4MP& self = GetInstance();
			self.papyrus->GetExternalEventRegistrations("OnPlayerHit", &data, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
				{
					F4MP& self = GetInstance();
					
					HitData* data = static_cast<HitData*>(dataPtr);
					if (data->hittee == self.playerEntityID)
					{
						SendPapyrusEvent1<Float32>(handle, scriptName, callbackName, data->damage);
					}
				});
		}
		
		static UInt32 GetClientInstanceID(StaticFunctionTag* base)
		{
			return activeInstance;
		}

		static void SetClient(StaticFunctionTag* base, UInt32 instance)
		{
			activeInstance = instance;
		}

		static bool Connect(StaticFunctionTag* base, Actor* player, TESNPC* playerActorBase, BSFixedString address, SInt32 port)
		{
			F4MP& self = GetInstance();
			librg_network_stop(&self.ctx);

			self.playerAppearance.Fill(playerActorBase);
			self.playerWornItems.Fill(player);

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
			//F4MP& self = GetInstance();
			//librg_tick(&self.ctx);

			for (auto& instance : instances)
			{
				librg_tick(&instance->ctx);
			}
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

		//TODO: abstract ...EntVar...

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
			self.SetAnimState(data, animState.c_str());
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
			return self.GetAnimState(data).c_str();
		}

		static Float32 Atan2(StaticFunctionTag* base, Float32 y, Float32 x)
		{
			const Float32 pi = 3.14159265359f;
			return fmod(atan2(y, x) + 2.f * pi, 2.f * pi);
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

		//static void CopyHeardPart(BGSHeadPart* src, BGSHeadPart*& dest)
		//{
		//	dest = (BGSHeadPart*)Heap_Allocate(sizeof(BGSHeadPart));
		//	memset(dest, 0, sizeof(BGSHeadPart));

		//	*dest = *src;

		//	return;

		//	dest->fullName = src->fullName;
		//	dest->partFlags = src->partFlags;
		//	dest->type = src->type;

		//	for (UInt32 i = 0; i < src->extraParts.count; i++)
		//	{
		//		BGSHeadPart* extra;
		//		CopyHeardPart(src->extraParts[i], extra);
		//		dest->extraParts.Push(extra);
		//	}

		//	return;

		//	src->textureSet = dest->textureSet;

		//	//dest->model = src->model;
		//	//std::copy(&src->morphs[0], &src->morphs[3], &dest->morphs[0]);

		//	dest->unk158 = src->unk158;
		//	dest->partName = src->partName;
		//}
		
		static void CopyAppearance(StaticFunctionTag* base, TESNPC* src, TESNPC* dest)
		{
			client::AppearanceData appearance;
			appearance.Fill(src);
			SetAppearance(dest, appearance);
		}

		static void CopyWornItems(StaticFunctionTag* base, Actor* src, Actor* dest)
		{
			client::WornItemsData wornItems;
			wornItems.Fill(src);
			SetWornItems(dest, wornItems);
		}
		
		static void PlayerHit(StaticFunctionTag* base, UInt32 hitter, UInt32 hittee, Float32 damage)
		{
			F4MP& self = GetInstance();

			HitData data{ hitter, hittee, damage };

			// TODO: temporary
			if (activeInstance == 0)
			{
				librg_message_send_all(&self.ctx, Message::Hit, &data, sizeof(HitData));
			}
			else
			{
				self.records.back().events.push_back(data);
			}
		}

		static void PlayerFireWeapon(StaticFunctionTag* base)
		{
			F4MP& self = GetInstance();

			// TODO: temporary
			if (activeInstance == 0)
			{
				librg_message_send_all(&self.ctx, Message::FireWeapon, nullptr, 0);
			}
			else
			{
				self.records.back().weaponFires++;
			}
		}
	};
}
