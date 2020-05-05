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
#include "TopicInfoIDs.h"
#include "f4se/NiNodes.h"
#include "f4se/PapyrusDelayFunctors.h"

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
	librg_network_add(&ctx, MessageType::Speak, OnSpeak);

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
	object = (F4SEObjectInterface*)f4se->QueryInterface(kInterface_Object);

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

			vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, TESForm*, TESObjectREFR*>("TopicInfoBegin", "F4MP",
				[](StaticFunctionTag* base, TESForm* topicInfo, TESObjectREFR* speaker)
				{
					/*static std::unordered_map<UInt32, UInt32> counts;
					counts[topicInfo->formID]++;

					for (auto& count : counts)
					{
						printf("%u : %u ", count.first, count.second);
					}
					printf("\n");

					switch (speaker->formID)
					{
					case 0x1D882B:
					case 0xE0B61:
					case 0x1E2300:
					case 0x1DE88E:
					case 0x1D882C:
					case 0x191F21:
					case 0x1E2301:
					case 0x1D882A:
					case 0x1D206D:
					case 0x1D1AED:
					case 0x1DE7F6:
					case 0x1AC2E5:
					case 0x1D206E:
					case 0x1D1AEC:
					case 0x193B43:
					case 0x2F27:
					case 0x4338C:
					case 0xED666:
					case 0xF480B:
					case 0x1D2864:
					case 0x193C39:
					case 0x248028:
					case 0x2A195:
					case 0x16939F:
						return;
					}*/

					std::unordered_multiset<UInt32>& linesToSpeak = GetInstance().linesToSpeak[speaker->formID];
					if (linesToSpeak.count(topicInfo->formID) > 0)
					{
						linesToSpeak.erase(topicInfo->formID);
						return;
					}

					BSFixedString name = [](TESForm* baseForm, ExtraDataList* extraDataList)
					{
						if (baseForm)
						{
							if (extraDataList)
							{
								BSExtraData* extraData = extraDataList->GetByType(ExtraDataType::kExtraData_TextDisplayData);
								if (extraData)
								{
									ExtraTextDisplayData* displayText = DYNAMIC_CAST(extraData, BSExtraData, ExtraTextDisplayData);
									if (displayText)
									{
										return *CALL_MEMBER_FN(displayText, GetReferenceName)(baseForm);
									}
								}
							}

							TESFullName* pFullName = DYNAMIC_CAST(baseForm, TESForm, TESFullName);
							if (pFullName)
								return pFullName->name;
						}

						return BSFixedString();
					}(speaker->baseForm, speaker->extraDataList);
					printf("topic info: %X / speaker: %X(%s)\n", topicInfo->formID, speaker->formID, name.c_str());

					F4MP& self = GetInstance();

					SpeakData data{ self.player->GetEntityID(), self.player->GetRefFormID() == speaker->formID ? 0x0 : speaker->formID, topicInfo->formID };
					librg_message_send_all(&self.ctx, MessageType::Speak, &data, sizeof(SpeakData));
				}, vm));

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

	messaging->RegisterListener(handle, "F4SE", [](F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			/*tArray<TESForm*>& sounds = (*g_dataHandler)->arrSOUN;
			tArray<TESForm*>& soundDescs = (*g_dataHandler)->arrSNDR;
			tArray<TESForm*>& topics = (*g_dataHandler)->arrDIAL;
			tArray<TESForm*>& scenes = (*g_dataHandler)->arrSCEN;

			printf("sounds(%u):\n", sounds.count);
			printf("sound descs(%u):\n", soundDescs.count);
			printf("topics(%u):\n", topics.count);
			printf("scenes(%u):\n", scenes.count);

			for (UInt32 i = 0; i < sounds.count; i++)
			{
				printf("%u: %s\n", i, sounds[i]->GetFullName());
			}

			for (UInt32 i = 0; i < soundDescs.count; i++)
			{
				printf("%u: %s\n", i, soundDescs[i]->GetFullName());
			}

			for (UInt32 i = 0; i < topics.count; i++)
			{
				printf("%u: %s\n", i, topics[i]->GetFullName());
			}

			for (UInt32 i = 0; i < scenes.count; i++)
			{
				printf("%u: %s\n", i, scenes[i]->GetFullName());
			}*/

			/*tArray<BGSKeyword*>& keywords = (*g_dataHandler)->arrKYWD;

			printf("keywords(%u):\n", keywords.count);

			for (UInt32 i = 0; i < keywords.count; i++)
			{
				BGSKeyword* keyword = keywords[i];
				if (Lower(keyword->keyword.c_str()).find("topic") == std::string::npos)
				{
					continue;
				}

				printf("%u: \"%s\" \"%s\"\n", i, keyword->GetFullName(), keyword->keyword.c_str());
			}*/

			/*tArray<TESForm*>& quests = (*g_dataHandler)->arrQUST;

			printf("quests(%u):\n", quests.count);

			for (UInt32 i = 0; i < quests.count; i++)
			{
				TESQuest* quest = dynamic_cast<TESQuest*>(quests[i]);
				if (!quest)
				{
					continue;
				}

				printf("%s\n", quest->GetFullName());

				for (UInt8* ptr = (UInt8*)quest->unk38; ptr != (UInt8*)&quest->unkF0; ptr++)
				{
					printf("%02x ", *ptr);
				}

				for (UInt8* ptr = (UInt8*)quest->unk38; ptr != (UInt8*)&quest->unkF0; ptr++)
				{
					printf("%c", *ptr);
				}

				printf("\n");
			}*/

			tArray<TESForm*>& topics = (*g_dataHandler)->arrDIAL;

			printf("topics(%u): %u\n", topics.count, kFormType_DIAL);

			/*for (UInt32 i = 0; i < topics.count; i++)
			{
				printf("%u(%u): %s / ", i, topics[i]->formType, topics[i]->GetFullName());
			}*/

			/*auto& eventSources = (*g_globalEvents)->eventSources;

			for (UInt32 i = 0; i < eventSources.count; i++)
			{
				if (i == 3)
				{
					continue;
				}

				class Sink : public BSTEventSink<void*>
				{
				public:
					UInt32 index;

					Sink(UInt32 index) : index(index)
					{

					}

					EventResult	ReceiveEvent(void** evn, void* dispatcher)
					{
						printf("%u: %p\n", index, evn);
						return kEvent_Continue;
					};
				};

				eventSources[i]->eventDispatcher.AddEventSink(new Sink(i));
			}*/

			//typedef BSTEventDispatcher<void*>* (*_GetEventDispatcher)();

			//RelocAddr<_GetEventDispatcher> GetDispatcher(0x004420F0);

			//UInt8* ptr = (UInt8*)GetDispatcher.GetUIntPtr();

			//static std::unordered_map<int, size_t> cnt;

			//for (int i = -23; i < 61; i++)
			//{
			//	switch (i)
			//	{
			//	case 20:
			//	//case 43:
			//	case -3:
			//	case 49:
			//	case 32:
			//	case 40:
			//	case 25:
			//	case 41:
			//	case 31:
			//	case 27:
			//		continue;
			//	}

			//	_GetEventDispatcher getEventDispatcher = (_GetEventDispatcher)(ptr + (i * 160));
			//	BSTEventDispatcher<void*>* eventDispatcher = getEventDispatcher();

			//	class Sink : public BSTEventSink<void*>
			//	{
			//	public:
			//		int index;

			//		Sink(int index) : index(index)
			//		{

			//		}

			//		EventResult	ReceiveEvent(void** evn, void* dispatcher)
			//		{
			//			cnt[index]++;
			//			printf("%d: %p\n", index, evn);

			//			return kEvent_Continue;
			//		};
			//	};

			//	eventDispatcher->AddEventSink(new Sink(i));
			//}

			/*tArray<TESForm*>& quests = (*g_dataHandler)->arrQUST;

			for (UInt32 i = 0; i < 2; i++)
			{
				TESQuest* quest = DYNAMIC_CAST(quests[i], TESForm, TESQuest);
				if (!quest)
				{
					continue;
				}

				for (UInt8* ptr = (UInt8*)&quest->unk38[((0xF0 - 0x38) >> 3) - 1]; ptr >= (UInt8*)&quest->unk38[0]; ptr--)
				{
					printf("%02X", *ptr);
				}
				printf("\n");

				for (UInt8* ptr = (UInt8*)&quest->unk0F8[((0x2F0 - 0xF8) >> 3) - 1]; ptr >= (UInt8*)&quest->unk0F8[0]; ptr--)
				{
					printf("%02X", *ptr);
				}
				printf("\n");
			}*/

			TESQuest* quest = DYNAMIC_CAST(LookupFormByID(0x0005DEE4), TESForm, TESQuest);
			if (quest)
			{
				/*for (UInt8* ptr = (UInt8*)&quest->unk38[((0xF0 - 0x38) >> 3) - 1]; ptr >= (UInt8*)&quest->unk38[0]; ptr--)
				{
					printf("%02X", *ptr);
				}
				printf("\n");

				for (UInt8* ptr = (UInt8*)&quest->unk0F8[((0x2F0 - 0xF8) >> 3) - 1]; ptr >= (UInt8*)&quest->unk0F8[0]; ptr--)
				{
					printf("%02X", *ptr);
				}
				printf("\n");*/

				//UInt8* ptr = *(UInt8**)((UInt8*)&quest->unk38[0] + 80);
				//printf("%p\n", ptr);
				///*for (int i = 199; i >= 0; i--)
				//{
				//	printf("%02X", ptr[i]);
				//}
				//printf("\n");*/
				//TESForm* form = (TESForm*)ptr;
				//printf("%u %x\n", form->formType, form->formID);

				/*UInt64* arr = (UInt64*)ptr;

				for (int j = 0; j < 10; j++)
				{
					ptr = (UInt8*)arr[j];

					for (int i = 199; i >= 0; i--)
					{
						printf("%02X", ptr[i]);
					}
					printf("\n");
				}*/

				/*tArray<TESForm*>& arr = *(tArray<TESForm*>*)((UInt8*)&quest->unk0F8[0] + 128);
				printf("%p\n", &arr);
				printf("%p %u %u\n", arr.entries, arr.capacity, arr.count);

				for (UInt32 i = 0; i < arr.count; i++)
				{
					printf("%p\n", arr[i]);
				}*/


				//UInt32 N1[]{ 291, 243, 195, 131, 99, 35, 19 };

				//for (UInt32 n : N1)
				//{
				//	UInt8* ptr = *(UInt8**)((UInt8*)&quest->unk38[0] + (355 - n) / 2);
				//	printf("%p\n", ptr);
				//	if (!ptr)
				//	{
				//		continue;
				//	}
				//	//TESForm* form = (TESForm*)ptr;
				//	//printf("%u %x\n", form->formType, form->formID);
				//	for (int i = 199; i >= 0; i--)
				//	{
				//		printf("%02X", ptr[i]);
				//	}
				//	printf("\n");
				//	for (int i = 0; i < 200; i++)
				//	{
				//		printf("%c", ptr[i]);
				//	}
				//	printf("\n");
				//}

				//printf("----------\n");

				//UInt32 N2[]{ 883, 867, 819, 803, 755, 739, 659, 595 };

				//for (UInt32 n : N2)
				//{
				//	UInt8* ptr = *(UInt8**)((UInt8*)&quest->unk0F8[0] + (995 - n) / 2);
				//	printf("%p\n", ptr);
				//	if (!ptr)
				//	{
				//		continue;
				//	}
				//	for (int i = 199; i >= 0; i--)
				//	{
				//		printf("%02X", ptr[i]);
				//	}
				//	printf("\n");
				//	for (int i = 0; i < 200; i++)
				//	{
				//		printf("%c", ptr[i]);
				//	}
				//	printf("\n");
				//}

				//UInt32 N1[]{ 195, 131 };

				//for (UInt32 n : N1)
				//{
				//	UInt8* ptr = *****(UInt8******)((UInt8*)&quest->unk38[0] + (355 - n) / 2);
				//	printf("%p\n", ptr);
				//	if (!ptr)
				//	{
				//		continue;
				//	}
				//	//TESForm* form = (TESForm*)ptr;
				//	//printf("%u %x\n", form->formType, form->formID);
				//	for (int i = 199; i >= 0; i--)
				//	{
				//		printf("%02X", ptr[i]);
				//	}
				//	printf("\n");
				//	for (int i = 0; i < 200; i++)
				//	{
				//		printf("%c", ptr[i]);
				//	}
				//	printf("\n");
				//}

				//printf("----------\n");

				//UInt32 N2[]{ 883, 819, 755, 659, 595 };

				//for (UInt32 n : N2)
				//{
				//	UInt8* ptr = *****(UInt8******)((UInt8*)&quest->unk0F8[0] + (995 - n) / 2);
				//	printf("%p\n", ptr);
				//	if (!ptr)
				//	{
				//		continue;
				//	}
				//	for (int i = 199; i >= 0; i--)
				//	{
				//		printf("%02X", ptr[i]);
				//	}
				//	printf("\n");
				//	for (int i = 0; i < 200; i++)
				//	{
				//		printf("%c", ptr[i]);
				//	}
				//	printf("\n");
				//}
			}
		}

		case F4SEMessagingInterface::kMessage_PostLoadGame:
		{
			struct OnTick : public IF4SEDelayFunctor
			{
				OnTick()
				{

				}

				const char* ClassName() const override
				{
					return "F4MP";
				}

				UInt32 ClassVersion() const override
				{
					return 1;
				}

				bool Save(const F4SESerializationInterface* intfc) override
				{
					return false;
				}

				bool Load(const F4SESerializationInterface* intfc, UInt32 version) override
				{
					return false;
				}

				bool Run(VMValue& resultOut) override
				{
					F4MP& f4mp = F4MP::GetInstance();

					librg_entity_iterate(&f4mp.ctx, LIBRG_ENTITY_ALIVE, [](librg_ctx* ctx, librg_entity* entity)
						{
							Entity::Get(entity)->OnTick();
						});

					return true;
				}

				bool ShouldReschedule(SInt32& delayMSOut) override
				{
					delayMSOut = 1;
					return true;
				}

				bool ShouldResumeStack(UInt32& stackIdOut) override
				{
					return false;
				}
			};

			GetInstance().object->GetDelayFunctorManager().Enqueue(new OnTick());
			break;
		}
		}
	});


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

	// HACK: 10 might be too much
	float speed = zpl_vec3_mag(position - (zpl_vec3&)ref->pos) * 10.f, rotSpeed = 500.f;
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

void f4mp::F4MP::MoveTo(TESObjectREFR* ref, zpl_vec3 position, zpl_vec3 angles)
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
		MoveTo(ref, data.position, data.angles);
	}
}

void f4mp::F4MP::OnSpawnBuilding(librg_message* msg)
{
	F4MP& self = GetInstance();

	SpawnBuildingData data;
	librg_data_rptr(msg->data, &data, sizeof(SpawnBuildingData));

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

	if (data.baseFormID == 0)
	{
		return;
	}

	TESObjectREFR* building = PlaceAtMe_Native((*g_gameVM)->m_virtualMachine, 0, (TESObjectREFR**)g_player.GetPtr(), LookupFormByID(data.baseFormID), 1, true, false, false);
	MoveTo(building, data.position, data.angles);

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

void f4mp::F4MP::OnSpeak(librg_message* msg)
{
	F4MP& self = GetInstance();

	class Topic : public TESForm
	{
	public:
		enum { kTypeID = kFormType_DIAL };
	};

	SpeakData data;
	librg_data_rptr(msg->data, &data, sizeof(SpeakData));

	static Topic* createdTopic = nullptr;
	if (!createdTopic)
	{
		createdTopic = static_cast<Topic*>(Runtime_DynamicCast(IFormFactory::GetFactoryForType(kFormType_DIAL)->Create(), RTTI_TESForm, RTTI_TESTopic));
		Topic* templateTopic = static_cast<Topic*>(Runtime_DynamicCast(LookupFormByID(0x0022FA98), RTTI_TESForm, RTTI_TESTopic));

		memcpy((UInt8*)createdTopic + sizeof(TESForm), (UInt8*)templateTopic + sizeof(TESForm), 88);

		UInt8* ptr = (UInt8*)createdTopic;
		TESForm**& infos = *(TESForm***)(ptr + sizeof(TESForm) + 48);
		UInt32& count1 = *(UInt32*)(ptr + sizeof(TESForm) + 64);
		UInt32& count2 = *(UInt32*)(ptr + sizeof(TESForm) + 68);
		infos = new TESForm * [1];
		count1 = count2 = 1;
	}

	TESObjectREFR* speaker = nullptr;
	if (data.speakerFormID == 0x0)
	{
		speaker = Entity::GetAs<Player>(self.FetchEntity(data.clientEntityID))->GetRef();
	}
	else
	{
		speaker = DYNAMIC_CAST(LookupFormByID(data.speakerFormID), TESForm, TESObjectREFR);
	}

	if (!speaker)
	{
		return;
	}

	TESForm* topicInfo = LookupFormByID(data.topicInfoFormID);
	if (!topicInfo)
	{
		return;
	}

	Topic* topic = createdTopic;
	Actor* actor = nullptr;
	bool inHead = false;
	TESObjectREFR* target = nullptr;

	UInt8* ptr = (UInt8*)createdTopic;
	TESForm**& infos = *(TESForm***)(ptr + sizeof(TESForm) + 48);
	infos[0] = topicInfo;

	VMVariable topicVar, actorVar, inHeadVar, targetVar;
	topicVar.Set(&topic);
	actorVar.Set(&actor);
	inHeadVar.Set(&inHead);
	targetVar.Set(&target);

	VMArray<VMVariable> args;
	args.Push(&topicVar);
	args.Push(&actorVar);
	args.Push(&inHeadVar);
	args.Push(&targetVar);

	CallFunctionNoWait(speaker, "Say", args);

	self.linesToSpeak[speaker->formID].insert(topicInfo->formID);

	printf("topic info: %X / speaker: %X / client: %u\n", topicInfo->formID, speaker->formID, data.clientEntityID);
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

	static std::vector<TESForm*> topicInfos;
	if (topicInfos.size() == 0)
	{
		for (UInt32& topicInfoID : topicInfoIDs)
		{
			switch (topicInfoID)
			{
			case 1749611: // 536
			case 1749606: // 176
			case 1731234: // 89
			case 1748309: // 63
			case 1753049: // 59
			case 1703050: // 50
			case 1632496: // 49
			case 1731236: // 49
			case 1748306: // 48
			case 1632495: // 48
			case 1748307: // 47
			case 1748305: // 46
			case 1707292: // 46
			case 1748311: // 45
			case 1753796: // 45
			case 1731235: // 44
			case 247304: // 44
			case 1707291: // 44
			case 1731233: // 44
			case 1748308: // 41
			case 275332: // 22
			case 719093: // 21
			case 1653468: // 20
			case 275333: // 20
			case 920523: // 20
			case 1653469: // 20
			case 920519: // 19
			case 920525: // 19
			case 275331: // 19
			case 719091: // 19
			case 920521: // 19
			case 1653466: // 19
			case 920522: // 19
			case 920520: // 18
			case 275328: // 18
			case 920518: // 18
			case 1653467: // 18
			case 920517: // 18
			case 920524: // 18
			case 719092: // 17
			case 275329: // 15
			case 1910882: // 14
			case 1910881: // 13
			case 275327: // 13
			case 275330: // 12
			case 1001471: // 8
			case 172434: // 8
			case 172433: // 8
			case 1001468: // 7
			case 1001469: // 7
			case 1001470: // 7
			case 1001467: // 7
			case 0x1651EB:
			case 0xE7513:
			case 0x1651EF:
			case 0x144F58:
			case 0x1651F5:
			case 0xE7517:
			case 0xE6F65:
			case 0x1651EE:
			case 0x1651F1:
			case 0x144F38:
				continue;
			}

			TESForm* topicInfo = (TESForm*)Runtime_DynamicCast(LookupFormByID(topicInfoID), RTTI_TESForm, RTTI_TESTopicInfo);
			if (!topicInfo)
			{
				continue;
			}

			topicInfos.push_back(topicInfo);
		}

		printf("%llu %llu\n", sizeof(topicInfoIDs) / sizeof(UInt32), topicInfos.size());
	}

	VMArray<TESForm*> topicInfoArray(topicInfos);

	GetInstance().papyrus->GetExternalEventRegistrations("OnTopicInfoRegister", &topicInfoArray, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			VMArray<TESForm*>* topicInfos = (VMArray<TESForm*>*)dataPtr;
			SendPapyrusEvent1(handle, scriptName, callbackName, *topicInfos);
		});

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
