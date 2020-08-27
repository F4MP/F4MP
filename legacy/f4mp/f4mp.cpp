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

std::string f4mp::F4MP::GetPath()
{
	std::string path(MAX_PATH, '\0');

	HRESULT err = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
	if (!SUCCEEDED(err))
	{
		_FATALERROR("SHGetFolderPath %08X failed (result = %08X lasterr = %08X)", CSIDL_MYDOCUMENTS, err, GetLastError());
	}
	ASSERT_CODE(SUCCEEDED(err), err);

	return path.substr(0, path.find('\0')) + "\\My Games\\Fallout4\\F4MP\\";
}

f4mp::F4MP::F4MP() : ctx{}, port(0), handle(kPluginHandle_Invalid), messaging(nullptr), papyrus(nullptr), task(nullptr), topicInstance(nullptr)
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

	const std::string path = GetPath() + "config.txt";

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

	Animator::Init();

	messaging->RegisterListener(handle, "F4SE", [](F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		case F4SEMessagingInterface::kMessage_GameDataReady:
		{
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
					static Animator animator(Animator::Human);
					static Animator::Animation animation;

					static bool enabled = false;
					if (!enabled)
					{
						if (GetAsyncKeyState(VK_F3))
						{
							enabled = true;

							animation = Animator::Animation();
							
							for (UInt32 node = 1; node < animator.GetAnimatedNodeCount(); node++)
							{
								animation.nodes.push_back(animator.GetNodeName(node));
							}
						}
					}
					else
					{
						if (GetAsyncKeyState(VK_F4))
						{
							enabled = false;

							animation.frames.pop_back();

							auto getDifference = [&](size_t referenceFrame, size_t frame)
							{
								double difference = 0.f;

								for (size_t j = 0; j < animation.frames[frame].transforms.size(); j++)
								{
									const Animator::Transform& transform = animation.frames[frame].transforms[j];
									const Animator::Transform& initialTransform = animation.frames[referenceFrame].transforms[j];
									difference += zpl_vec3_mag(transform.position - initialTransform.position) - (zpl_quat_dot(transform.rotation, initialTransform.rotation) - 1.f) + abs(transform.scale - initialTransform.scale);
								}

								return difference;
							};

							std::vector<double> differences(animation.frames.size());

							for (size_t i = 0; i < animation.frames.size(); i++)
							{
								differences[i] = getDifference(i, 0);
							}

							for (size_t i = 0; i < animation.frames.size(); i++)
							{
								if (0 < i && i < animation.frames.size() - 1)
								{
									if ((differences[i] - differences[i - 1]) * (differences[i + 1] - differences[i]) < 0)
									{
										const double threshold = 1.0;

										if (differences[i] < threshold)
										{
											printf("> ");
										}
									}
								}

								printf("%llu: %f %f\n", i, animation.frames[i].duration, differences[i]);
							}

							int name = 0;
							std::ifstream tmp;

							do
							{
								tmp.close();
								tmp.open((GetPath() + std::to_string(++name) + ".txt").c_str());
							} while (tmp.good());

							animator.Save(GetPath() + std::to_string(name) + ".txt", animation);
						}

						TESObjectREFR* ref = *g_player;
						if (ref)
						{
							if (animation.frames.size() > 0)
							{
								float& duration = animation.frames.back().duration;
								duration = zpl_time_now() - duration;
							}

							Animator::Frame frame;
							frame.duration = zpl_time_now();
							frame.transforms.resize(animation.nodes.size());

							if (!animator.ForEachNode(ref, [&](NiNode* node, UInt32 nodeIndex)
								{
									if (nodeIndex == 0)
									{
										return false;
									}

									const NiMatrix43 rot = node->m_localTransform.rot;
									zpl_mat4 mat
									{
										rot.data[0][0], rot.data[0][1], rot.data[0][2], rot.data[0][3],
										rot.data[1][0], rot.data[1][1], rot.data[1][2], rot.data[1][3],
										rot.data[2][0], rot.data[2][1], rot.data[2][2], rot.data[2][3],
									};
									zpl_quat quat;
									zpl_quat_from_mat4(&quat, &mat);

									quat *= zpl_sign(quat.w);

									frame.transforms[nodeIndex - 1] = { (zpl_vec3&)node->m_localTransform.pos, quat, node->m_localTransform.scale };

									return false;
								}))
							{
								animation.frames.push_back(frame);
							}
						}
					}

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

	SpeakData data;
	librg_data_rptr(msg->data, &data, sizeof(SpeakData));

	if (!self.topicInstance)
	{
		self.topicInstance = static_cast<Topic*>(Runtime_DynamicCast(IFormFactory::GetFactoryForType(kFormType_DIAL)->Create(), RTTI_TESForm, RTTI_TESTopic));
		Topic* templateTopic = static_cast<Topic*>(Runtime_DynamicCast(LookupFormByID(0x0022FA98), RTTI_TESForm, RTTI_TESTopic));

		memcpy((UInt8*)self.topicInstance + sizeof(TESForm), (UInt8*)templateTopic + sizeof(TESForm), 88);

		UInt8* ptr = (UInt8*)self.topicInstance;
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

	Topic* topic = self.topicInstance;
	Actor* actor = nullptr;
	bool inHead = false;
	TESObjectREFR* target = nullptr;

	UInt8* ptr = (UInt8*)self.topicInstance;
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

	self.topicInfoRemainders.clear();

	std::vector<TESForm*> topicInfos;

	std::unordered_set<UInt32> existingTopicInfoIDs
	{
		0x0018E8EE, 0x001768FF, 0x00047258, 0x0017C9BE, 0x000EC830, 0x0017978C, 0x001A765A, 0x000A8226, 0x00056A76, 0x00140532, 0x0017973F, 0x00087754, 0x00177E35, 0x00185031, 0x000D3E99, 0x00086365, 0x00166AC2, 0x000A8237, 0x00167F53, 0x00036FCF, 0x00179746, 0x00179F73, 0x00225312, 0x0017C9DF, 0x00177E20, 0x0017647C, 0x0022530F, 0x001AAD51, 0x00062BCC, 0x000D3EA4, 0x0001FA52, 0x000EF21E, 0x00112DDC, 0x0017C9BF, 0x000304E6, 0x00140535, 0x001797C3, 0x0004724C, 0x0023C65D, 0x001768FD, 0x00166ABF, 0x0017B46E, 0x0017C9F1, 0x00166AC3, 0x001797BC, 0x00178EDA, 0x0017978D, 0x0003710F, 0x00144F4F, 0x000C4515, 0x001797C2, 0x0017643F, 0x001797A0, 0x0017C9CC, 0x00135759, 0x00061541, 0x000304CF, 0x000EC82F, 0x0023C668, 0x00176498, 0x00179775, 0x001764AC, 0x001651F1, 0x000A02BC, 0x000A822C, 0x0019B4D3, 0x000FFC4D, 0x001AADBA, 0x0019B4EB, 0x00177E18, 0x0017FBA7, 0x000304D4, 0x00225310, 0x0014FC9C, 0x000304D9, 0x00225321, 0x0015B87C, 0x00179734, 0x0007814E, 0x00144F59, 0x000304E7, 0x00149E7A, 0x001768F6, 0x00086361, 0x00179F6B, 0x00179788, 0x00087761, 0x00140526, 0x000E7515, 0x00225333, 0x00176446, 0x0003059A, 0x0017C9DA, 0x000AEFA1, 0x00135762, 0x0017B44D, 0x0019B4DC, 0x00176491, 0x001A6AA4, 0x001AB266, 0x0017977C, 0x00166AC0, 0x000ED746, 0x0023C656, 0x00178EEA, 0x00144F38, 0x00179737, 0x000E6201, 0x00179750, 0x001684A7, 0x000304CD, 0x00185030, 0x000E0BC7, 0x000E6F64, 0x00179769, 0x000C450E, 0x000E0BC8, 0x0017FBAB, 0x000A8220, 0x0017B467, 0x000370FE, 0x0014051B, 0x0007814A, 0x00177E1F, 0x00178EE8, 0x0006566E, 0x001768F8, 0x00037104, 0x000304C7, 0x000E4786, 0x0017B479, 0x0017C9CB, 0x000A600D, 0x000C2B72, 0x00179797, 0x0014052A, 0x00176486, 0x001651EF, 0x00176453, 0x0011FBC2, 0x000A823B, 0x0015FA42, 0x0018502C, 0x0019B4E3, 0x00225319, 0x0013576D, 0x0019B4D6, 0x001764A8, 0x00147713, 0x00087753, 0x0017B43D, 0x0017648E, 0x00135758, 0x00140519, 0x00047267, 0x0017C9D4, 0x001A6AA2, 0x00140538, 0x001797C9, 0x001A7666, 0x00140537, 0x0017B477, 0x001797AA, 0x000CF6C1, 0x0017B46C, 0x000304E3, 0x0017C9AA, 0x0017973A, 0x00144F4C, 0x000304CC, 0x0001FDBB, 0x00176470, 0x00176460, 0x001578A2, 0x00176454, 0x000304DD, 0x00061542, 0x00193ADA, 0x0003D180, 0x0012C0B2, 0x0019B4E5, 0x000C370E, 0x001FDE74, 0x0008774C, 0x0017B44F, 0x000E61B2, 0x000F47FF, 0x00179F6D, 0x000EDF49, 0x000304EC, 0x00176449, 0x001768D3, 0x00144F3A, 0x001768E9, 0x00176479, 0x0023B89A, 0x00086354, 0x00061549, 0x00178EF6, 0x00178EF0, 0x0003D181, 0x0023C666, 0x001651EE, 0x0023C655, 0x00036FDE, 0x0016042F, 0x000FF3EA, 0x00179751, 0x001764B0, 0x001A0D1C, 0x00166AB5, 0x00087750, 0x00144F49, 0x00112D1A, 0x00182EAA, 0x00061543, 0x001768EA, 0x0023C660, 0x001797BE, 0x0023C66B, 0x000A8239, 0x00061554, 0x0017C9FC, 0x0017975F, 0x001813B3, 0x00036FCD, 0x000ED741, 0x00037101, 0x00178ED7, 0x000AA789, 0x000A8223, 0x00037116, 0x000E7513, 0x000FF3E9, 0x00176490, 0x000D3E9F, 0x00174F85, 0x00144F42, 0x000E2128, 0x00166ABB, 0x001768FA, 0x00036FD7, 0x00179762, 0x00144A53, 0x0014DC52, 0x000FF3E0, 0x0018501D, 0x0023C659, 0x000304DA, 0x001D2862, 0x000E61F8, 0x00179782, 0x00176477, 0x00107B0B, 0x00036FD5, 0x0017C99F, 0x00178EF1, 0x000304C5, 0x0017977F, 0x00186052, 0x00174F87, 0x000304C8, 0x000FFF5C, 0x00185033, 0x0017FBB0, 0x0023C653, 0x0017978E, 0x001797C1, 0x001D2861, 0x0014771A, 0x000ED4E6, 0x00179773, 0x000780CD, 0x00178EE4, 0x00147726, 0x0017C995, 0x0008774E, 0x00178EDE, 0x001797A6, 0x001ABC01, 0x00178EF2, 0x0008775D, 0x0003710C, 0x0004724F, 0x0018E384, 0x001768F0, 0x00036FDB, 0x000553BE, 0x001832A6, 0x0008635D, 0x00149E6D, 0x00140530, 0x0017646C, 0x0004725E, 0x001768DF, 0x0017B440, 0x000D3E92, 0x000A3759, 0x000E0BC5, 0x00087765, 0x001797C7, 0x00177E39, 0x0022531F, 0x000304ED, 0x00186050, 0x000304E5, 0x0017973D, 0x0017FB9D, 0x00217A8C, 0x0017C9B5, 0x000E37C4, 0x0023C65B, 0x00062BCE, 0x00179765, 0x0012A210, 0x0014771C, 0x000304CA, 0x0015F078, 0x001A6AA3, 0x0017B476, 0x00179768, 0x0017C9A4, 0x00087766, 0x00179F70, 0x00176464, 0x00185014, 0x00179799, 0x00176478, 0x00144F43, 0x00176473, 0x0017C9B1, 0x00225320, 0x000E7514, 0x00177E3E, 0x0017FB95, 0x000AA788, 0x000D3EA2, 0x0017972A, 0x000A600E, 0x00037114, 0x000A8242, 0x00166AC1, 0x0022532C, 0x0014052D, 0x000304C6, 0x0022532E, 0x00087757, 0x0023C65E, 0x00061545, 0x0017974F, 0x0017C9D5, 0x00178EDB, 0x00135760, 0x000FF3D8, 0x00177E14, 0x001ABBFE, 0x00147719, 0x000A8238, 0x001768ED, 0x000A02BD, 0x0006155B, 0x0014771E, 0x0017B457, 0x00179F6C, 0x00177E25, 0x001768E8, 0x00087763, 0x00174F8A, 0x0017C9DB, 0x000E0BCC, 0x0017647E, 0x000A8227, 0x000AEFA0, 0x001AAD55, 0x000304DF, 0x00179760, 0x001768E1, 0x00144F64, 0x00176900, 0x0008775A, 0x000E697A, 0x0017C9B3, 0x0003D172, 0x0017645A, 0x001768E3, 0x0017C992, 0x0017B462, 0x0017648B, 0x00179770, 0x0015FA4C, 0x000E61F9, 0x00179789, 0x00225335, 0x0017FBA9, 0x0014DC51, 0x001768DD, 0x0018E8EF, 0x0017977E, 0x0017645D, 0x0006566A, 0x0013575E, 0x0017C9B7, 0x000EDF48, 0x0017C9A2, 0x00169619, 0x00179781, 0x00176475, 0x00174F88, 0x0006155E, 0x00086364, 0x0017976E, 0x00086360, 0x00179739, 0x00019343, 0x0017979B, 0x0017C9DE, 0x00140517, 0x0017FBA8, 0x00135763, 0x00106F2A, 0x000304D1, 0x00179763, 0x0003D186, 0x0023C663, 0x00036FD3, 0x0022531A, 0x0015FA4D, 0x000D3E94, 0x0006566B, 0x001764A1, 0x00178EF4, 0x0003C608, 0x000EDF41, 0x000E2127, 0x00144F44, 0x0003D185, 0x000C4512, 0x0017974C, 0x00174F86, 0x0017B47E, 0x0017645F, 0x000304DE, 0x00179796, 0x001832A5, 0x000780C6, 0x0017CA00, 0x00177E2C, 0x0019B4E8, 0x0017B47C, 0x00177E1C, 0x00135767, 0x00179724, 0x00177E15, 0x00179F6F, 0x001797A7, 0x001CB28D, 0x00182EAB, 0x000A822E, 0x000E37BC, 0x0017B45C, 0x00179728, 0x00056A77, 0x0015D335, 0x001797B5, 0x00144A60, 0x000C4517, 0x0017C9A8, 0x000E0BCA, 0x00176444, 0x00178EEF, 0x000E37BB, 0x000C450F, 0x00185028, 0x001ABBFC, 0x00135764, 0x000C451A, 0x00047261, 0x0017C9F4, 0x0017C9E5, 0x000780C5, 0x000E0BCB, 0x000BA774, 0x00225318, 0x000ED744, 0x001CB28C, 0x00037110, 0x0017C99C, 0x0013576A, 0x00036FCE, 0x0017B448, 0x0018501F, 0x00179749, 0x0017C9D1, 0x00179F68, 0x00176448, 0x00178EDD, 0x00179785, 0x001CB28E, 0x00047268, 0x00144A54, 0x00115318, 0x00144A56, 0x0017645B, 0x00037108, 0x000C4524, 0x000AE80F, 0x000EF21D, 0x001AADBC, 0x000A821E, 0x00176492, 0x00062BCB, 0x000EC82D, 0x001764AB, 0x00179778, 0x00166ABE, 0x0017649A, 0x00177E37, 0x0019B4D4, 0x001768F9, 0x00148B6F, 0x0019B4ED, 0x0017C9C4, 0x0017972E, 0x00047266, 0x00177E33, 0x00179727, 0x0017C9FB, 0x001832A8, 0x00179740, 0x0023C665, 0x00178EDF, 0x00036FD0, 0x00036FCC, 0x0018E37E, 0x00087760, 0x000FF3EE, 0x00154FF0, 0x000A8228, 0x000E0BCD, 0x000EEF75, 0x0017C9D9, 0x001768E7, 0x00061561, 0x00036FD1, 0x0017978F, 0x001A7663, 0x000E6979, 0x00144F5C, 0x00186057, 0x001AADBB, 0x0018E8F0, 0x0021636D, 0x0017C9EE, 0x0017C9DD, 0x000780C4, 0x0014DC53, 0x000C4513, 0x00147721, 0x001ABBFB, 0x000304E4, 0x0017B466, 0x00179784, 0x0017979D, 0x00177E2E, 0x0023C648, 0x0017FBA2, 0x00036FC9, 0x00179756, 0x00037100, 0x000C451D, 0x00179755, 0x0013C6CD, 0x0014052F, 0x001768F7, 0x00065DE7, 0x0017C9C0, 0x000C451F, 0x00037112, 0x00144F63, 0x000FF3E5, 0x00166ABA, 0x00140524, 0x000C450B, 0x00176487, 0x0023C65F, 0x0017B473, 0x00061565, 0x0019B4E9, 0x0001934B, 0x0021636E, 0x0017979A, 0x001533E6, 0x0019B4E4, 0x0014051C, 0x0017C9B9, 0x0017972F, 0x00176485, 0x00144F56, 0x00087764, 0x00225334, 0x001533E7, 0x0017C9CE, 0x0003710E, 0x0017B47A, 0x0017FBB7, 0x000A375A, 0x00087751, 0x0017644D, 0x0022532A, 0x00177E34, 0x0012AFF5, 0x0008775B, 0x00178EF5, 0x000C450D, 0x0004725B, 0x000EDF44, 0x0018E37C, 0x0017C9BC, 0x0008775E, 0x000EDF4C, 0x00178EE2, 0x0017979F, 0x00144F4E, 0x0018500F, 0x000304D2, 0x00177E2A, 0x000A8233, 0x00176441, 0x000AE80E, 0x00225330, 0x000304D3, 0x0023C64C, 0x001797AD, 0x0019B4D9, 0x00186058, 0x00177E28, 0x0004724B, 0x001651F2, 0x00179776, 0x00144F40, 0x001832AB, 0x00078159, 0x00225313, 0x0017C9D7, 0x00179725, 0x001768FE, 0x00037106, 0x0017979C, 0x001768E0, 0x0014771B, 0x000ECA11, 0x000F47FB, 0x0008635F, 0x0021636A, 0x0017B442, 0x00177E32, 0x00086356, 0x00185016, 0x00179744, 0x00178EDC, 0x0017C9F7, 0x001768F1, 0x0017B44A, 0x0018E381, 0x00185035, 0x0017C9E1, 0x000648FE, 0x00147718, 0x00177E12, 0x000E6F63, 0x00061547, 0x0019B4DE, 0x001797BF, 0x0017646B, 0x0003D188, 0x00140514, 0x00062BCA, 0x00144F46, 0x0017C9F0, 0x00147727, 0x0004725F, 0x0018500C, 0x00179F72, 0x000304CB, 0x00185009, 0x00166AB7, 0x0015083B, 0x000A8240, 0x0017C99B, 0x00047257, 0x000ED740, 0x000A8234, 0x00061550, 0x00087767, 0x0015D336, 0x000304E9, 0x00056A74, 0x001832A4, 0x001797B6, 0x00179753, 0x0017644E, 0x0017B458, 0x0023C651, 0x001832A9, 0x00185025, 0x00179741, 0x000304D7, 0x000C4531, 0x000C453A, 0x0023C64D, 0x0019B4EA, 0x0003710D, 0x0017B447, 0x0008635C, 0x00086351, 0x0015FA3E, 0x00019348, 0x00046C9A, 0x0016961A, 0x00179720, 0x001832AA, 0x00176497, 0x00037109, 0x0017976A, 0x001832A3, 0x000A8229, 0x000E2126, 0x00179731, 0x0017C99D, 0x000ED73F, 0x00176440, 0x00179732, 0x0021BDEB, 0x00144F4D, 0x00178EE1, 0x000EEF6F, 0x001797C0, 0x0022531E, 0x001797AE, 0x00036FD6, 0x000F47FE, 0x0017C9D0, 0x0003710A, 0x00144A5B, 0x00086358, 0x0017C9F2, 0x0014771F, 0x0022530E, 0x0022533C, 0x000304E0, 0x00177E3B, 0x000A822A, 0x001768D8, 0x00179759, 0x00178EE3, 0x0022533B, 0x00087756, 0x000C452D, 0x000E6F65, 0x0017C9AE, 0x000E61FD, 0x001160B4, 0x0006155C, 0x00185018, 0x00019331, 0x0023C654, 0x0003D177, 0x0017BEEB, 0x00176472, 0x0017C9A7, 0x000E61F6, 0x00225323, 0x0017C9F9, 0x0017646A, 0x00037107, 0x0017C999, 0x001797A9, 0x000EEF62, 0x00144A5C, 0x000304E1, 0x00177E24, 0x0017C9B0, 0x00176461, 0x0017C9C1, 0x0017647D, 0x0018501E, 0x00140522, 0x000C4516, 0x00147714, 0x00186695, 0x00061555, 0x0017C9BB, 0x000ECA10, 0x001A765B, 0x00179729, 0x0023C657, 0x0014052C, 0x0023C66C, 0x0018502B, 0x0017C8D4, 0x0017C9A1, 0x00061560, 0x0014053B, 0x00087762, 0x00086352, 0x000304EE, 0x0013AB30, 0x00129A37, 0x001ABFD9, 0x00144F3B, 0x0006153C, 0x0017C997, 0x0021BDE1, 0x0018501C, 0x0006F10B, 0x0023C66E, 0x0022530D, 0x00087758, 0x0003D174, 0x00177E29, 0x0017973B, 0x0017975B, 0x000370FF, 0x00179767, 0x00176455, 0x000E37C3, 0x00112DD9, 0x00225315, 0x0017FBAC, 0x00179722, 0x0018502E, 0x00047251, 0x0017C9AF, 0x000E37BD, 0x001A7665, 0x0017B441, 0x001797B9, 0x0017C998, 0x0017C9F6, 0x00176481, 0x0018E37D, 0x0017B468, 0x00056A75, 0x00036FC8, 0x000E6978, 0x0017B445, 0x001797A5, 0x00225336, 0x001768D2, 0x0004724D, 0x000ECA0E, 0x0004725D, 0x00176458, 0x00144F52, 0x0018500E, 0x00249D5E, 0x0006153F, 0x000ECA14, 0x00225324, 0x00178EF3, 0x000AEF9F, 0x000E61FA, 0x0017644B, 0x000A8235, 0x0023C66D, 0x001797AF, 0x000E6200, 0x0017C9A6, 0x0006153E, 0x0013C6CE, 0x0017B43C, 0x0001934C, 0x00144F41, 0x0017C9CA, 0x00176450, 0x0023C65C, 0x0013575B, 0x000304DB, 0x00185029, 0x00179791, 0x00177E3C, 0x0017B43A, 0x0017C9E4, 0x0017B459, 0x00061564, 0x000C451E, 0x001768D5, 0x00193ADD, 0x0003D187, 0x000B95B2, 0x0001934A, 0x0009902A, 0x0017976F, 0x00178EF7, 0x00061536, 0x000ED745, 0x00179745, 0x000E37C7, 0x00140539, 0x0017C9E2, 0x00179787, 0x0017FB9B, 0x000E61F7, 0x00179795, 0x001764A2, 0x00140528, 0x00176471, 0x00176901, 0x0023C64E, 0x0012BAA9, 0x0022531C, 0x00144F37, 0x00061544, 0x0014FEEF, 0x001651EC, 0x00185012, 0x00037111, 0x00176463, 0x00177E16, 0x00061546, 0x001AAD54, 0x001768D4, 0x0022532D, 0x00185034, 0x001797B2, 0x0023B89D, 0x0017644C, 0x001797B7, 0x0017973E, 0x00179757, 0x000EEF77, 0x00179738, 0x00178EED, 0x0017974D, 0x000EEF5E, 0x000304D6, 0x000FFC4C, 0x0017C9B4, 0x00186054, 0x000E37C6, 0x0017B45D, 0x000A821F, 0x00036FDC, 0x00148B70, 0x0013575A, 0x0003710B, 0x0018EFDB, 0x0017C9AB, 0x001768D6, 0x00177E23, 0x001B9C99, 0x0017FBAA, 0x0021636B, 0x0017FBB4, 0x0006154E, 0x000E61FB, 0x0017978B, 0x00174F8B, 0x001764B1, 0x00179733, 0x0023B899, 0x00176451, 0x000304C4, 0x0017976D, 0x0006154B, 0x001768D9, 0x00115317, 0x000C450C, 0x00144F35, 0x000C4519, 0x0019B4DB, 0x0018502F, 0x00144F51, 0x000EDF43, 0x00037105, 0x0006155A, 0x0017FB97, 0x00185010, 0x000ECA15, 0x0023C662, 0x0017C9C2, 0x000780D4, 0x0008775F, 0x001764B2, 0x00037103, 0x00217A8D, 0x00179772, 0x0019B4E1, 0x001ABCA7, 0x00144A57, 0x000C4510, 0x000780D5, 0x00144F36, 0x00086357, 0x000648FF, 0x00176447, 0x001768DB, 0x00177E1E, 0x00144A58, 0x00193ADB, 0x000780D6, 0x00061557, 0x00177E36, 0x00176457, 0x000EC82E, 0x0017973C, 0x00135768, 0x00185032, 0x00063B58, 0x00166ABC, 0x00144F3F, 0x0017C9C3, 0x00179779, 0x00037115, 0x001768FC, 0x00166AB6, 0x0022533D, 0x0018500B, 0x000553BF, 0x00179780, 0x000EDF47, 0x0017C9C8, 0x001ABBFD, 0x001768E5, 0x00225311, 0x00134D20, 0x00179748, 0x00217A95, 0x00186A3D, 0x00179793, 0x001A8B6B, 0x0014051E, 0x0017C9F8, 0x0017C9B6, 0x0017C9FD, 0x001ABC00, 0x00176480, 0x000EDF46, 0x00061556, 0x0023C64B, 0x0013575F, 0x0008775C, 0x0017C9E3, 0x00179790, 0x000EDF4A, 0x0023C649, 0x0013575C, 0x0017FB9F, 0x00179786, 0x00225339, 0x00177E17, 0x0003D18F, 0x000AE809, 0x0017C9BD, 0x0003D176, 0x00144F69, 0x000B32DB, 0x001768EC, 0x00086353, 0x0017C9A0, 0x00178EE7, 0x0017B446, 0x00112D19, 0x0017C993, 0x0017976C, 0x000A8221, 0x0017977D, 0x00144F4A, 0x000E37C0, 0x000EEF70, 0x000C451B, 0x00178EE0, 0x00179764, 0x001768EF, 0x0004724E, 0x00144F39, 0x00176452, 0x00061558, 0x0003D179, 0x00225328, 0x0018502A, 0x001651F0, 0x000553C0, 0x0003D17D, 0x00140518, 0x0017B44B, 0x0006566D, 0x00036FDD, 0x0017644A, 0x001A6AA1, 0x0019B4D2, 0x000AE80B, 0x001A765F, 0x0017975E, 0x001797C6, 0x00179735, 0x00037117, 0x0017FBB5, 0x0017B461, 0x0018AF9D, 0x00176462, 0x001768F2, 0x00047252, 0x000C4672, 0x000A823E, 0x0017C9B8, 0x000A823A, 0x000EEF63, 0x000FF3EC, 0x0017C9C7, 0x000EDF42, 0x0019B4E6, 0x001FDE71, 0x000AF13E, 0x00174F83, 0x00061551, 0x0017644F, 0x000A8225, 0x00177E19, 0x001797AB, 0x00177E3A, 0x00144F45, 0x0023C652, 0x0017975A, 0x0017979E, 0x0018501B, 0x00135765, 0x0014772A, 0x000304D5, 0x000E61FF, 0x00176459, 0x001797A8, 0x0017C9FE, 0x000E61F5, 0x000F47FC, 0x00061559, 0x0022532B, 0x0017B456, 0x0017974E, 0x0013575D, 0x00225316, 0x0017B449, 0x00179766, 0x00185027, 0x000A822F, 0x0017C9A9, 0x0021BDE0, 0x000780CE, 0x000304E2, 0x00185021, 0x000C4671, 0x0003D17A, 0x0003D173, 0x0019B4D5, 0x00047253, 0x000C4511, 0x0003D184, 0x0015FA4B, 0x000E7516, 0x0004725C, 0x00110409, 0x00061552, 0x00144F5B, 0x00178D69, 0x0022533A, 0x0017FBB6, 0x001FDE70, 0x00087759, 0x00178D68, 0x000A8241, 0x00037102, 0x0017C9D2, 0x0019B4E2, 0x0018E37F, 0x0004AF74, 0x001797A4, 0x000D3E9D, 0x0003D18E, 0x00036FC7, 0x00177E2F, 0x00047255, 0x00063B59, 0x0023C66A, 0x0023C647, 0x001651F3, 0x001768FB, 0x0015FA44, 0x000D3E96, 0x001ABBFF, 0x0018500D, 0x000EEF61, 0x000E37B6, 0x00225327, 0x000C4514, 0x0023C66F, 0x00036FD8, 0x00177E21, 0x000304C9, 0x000E2129, 0x0017C9E9, 0x00178ED8, 0x0001934D, 0x001ABBFA, 0x000A822B, 0x0017FBA1, 0x0017645E, 0x00176499, 0x00177E2D, 0x0008634F, 0x0017648C, 0x001797C5, 0x0023C65A, 0x00140520, 0x00178EE6, 0x00144A5E, 0x000A600F, 0x0023B89B, 0x001764A7, 0x000304EA, 0x0017C9CD, 0x0015FA47, 0x0001FA55, 0x000C4526, 0x00176445, 0x0004AF72, 0x00147716, 0x000A6010, 0x0017976B, 0x001832A2, 0x000A8222, 0x00144F3D, 0x0017C9D8, 0x0014771D, 0x00147728, 0x0017B43B, 0x00048979, 0x001768E6, 0x000EEF5F, 0x00144A61, 0x00179761, 0x00065668, 0x00178EF8, 0x000ED73E, 0x000EEF76, 0x000AE808, 0x000A823F, 0x000AE80A, 0x0017FBA3, 0x00177E13, 0x000FFF59, 0x001797BA, 0x000ECA13, 0x00144F3E, 0x0017646D, 0x00062BCD, 0x00217A8E, 0x00086350, 0x00147715, 0x0018E383, 0x00056A7A, 0x0017B443, 0x0003D175, 0x001797A3, 0x00179777, 0x00036FC4, 0x000E2125, 0x0017FBA5, 0x00036FC5, 0x0017C9F5, 0x0008635E, 0x0021BDDD, 0x00225325, 0x00177E2B, 0x0017C9AD, 0x00036FC6, 0x0017648A, 0x00225322, 0x0019B4E7, 0x00179774, 0x00061563, 0x0017C9C9, 0x000A7282, 0x00144F58, 0x0017B439, 0x00112D18, 0x00225317, 0x000EC832, 0x000EF220, 0x00140531, 0x0017B46F, 0x00176483, 0x00179F6A, 0x00179798, 0x0017B44C, 0x0019B4DF, 0x0017647A, 0x00056A73, 0x00179723, 0x00036FD2, 0x00061540, 0x00179792, 0x00177E1D, 0x00177E3D, 0x001797C8, 0x0017C9A3, 0x0017649B, 0x001764A3, 0x000F47FD, 0x00185011, 0x0006F10D, 0x0023C658, 0x00185026, 0x000A8232, 0x001CB290, 0x0013C6CC, 0x0017B454, 0x00178EE5, 0x00217A94, 0x00144A5F, 0x0003D17E, 0x0019FC8A, 0x00176456, 0x0017C9EF, 0x0019B4EC, 0x000E37BF, 0x0017B475, 0x0017B474, 0x00144F50, 0x00193ADC, 0x0019B4DD, 0x0003D183, 0x0005E6DC, 0x00217A91, 0x000304EF, 0x001A0D1B, 0x001768E2, 0x000A822D, 0x00144F67, 0x0017FB98, 0x001832A7, 0x00185036, 0x001AAD57, 0x0023C64A, 0x001768DE, 0x001768E4, 0x0017648D, 0x000C4521, 0x0017643E, 0x0017C9E8, 0x0017FB93, 0x0006153D, 0x001FDE72, 0x00144F53, 0x0006154C, 0x001797C4, 0x000B0F88, 0x0017977A, 0x001ABC02, 0x001651F5, 0x00144DE6, 0x00036FCA, 0x0017975D, 0x001CB28F, 0x0007815C, 0x00179743, 0x000ED742, 0x0018501A, 0x00178EEC, 0x0017974B, 0x00179736, 0x0017FB99, 0x00177E31, 0x0017C9ED, 0x000A02BF, 0x0017C9EB, 0x00062BCF, 0x00147725, 0x0008634E, 0x00177E38, 0x00036FCB, 0x00225329, 0x00176443, 0x0018E37B, 0x000A7AE5, 0x00179F71, 0x00166AB8, 0x0017B43E, 0x00166AC4, 0x000AEF9E, 0x00176466, 0x0022533E, 0x0019B4DA, 0x0017C9BA, 0x0017C996, 0x0017C9A5, 0x00177E1A, 0x00062BC9, 0x0017B45E, 0x00217A93, 0x00144F3C, 0x00112DDA, 0x000EDF45, 0x0014FEEE, 0x0017977B, 0x00174F89, 0x0023C64F, 0x0014051A, 0x00087755, 0x00078160, 0x0017B44E, 0x00225338, 0x00185024, 0x000A02C0, 0x001797BD, 0x001768EB, 0x001A765C, 0x00149E6C, 0x000FF3D6, 0x0003D178, 0x0004725A, 0x001797B4, 0x000BE813, 0x001768DA, 0x00087752, 0x00144F54, 0x000C4518, 0x00179F69, 0x000ECA0F, 0x000304CE, 0x00177E26, 0x0019B4D8, 0x0022531D, 0x00225326, 0x001797A2, 0x000E0BC6, 0x0017B45B, 0x00086362, 0x000304EB, 0x000AA783, 0x00225337, 0x00047263, 0x00179F6E, 0x00047264, 0x000EDF4B, 0x0017978A, 0x000A8230, 0x00047254, 0x001651ED, 0x0019B4E0, 0x000A3757, 0x0017FB9E, 0x0017C9D6, 0x000ED743, 0x00147720, 0x0017974A, 0x0021636C, 0x00166AB4, 0x00179747, 0x00249D5D, 0x00063B57, 0x0023C661, 0x000E7519, 0x0006154D, 0x001AC2C4, 0x0017B472, 0x0017C9AC, 0x00135769, 0x0017972B, 0x0017647F, 0x000A823C, 0x00135766, 0x000780CC, 0x00144F47, 0x001764A9, 0x00144A5A, 0x0004AF73, 0x001AB26B, 0x0014052E, 0x00140533, 0x000A8231, 0x000AEF9D, 0x00147717, 0x00078141, 0x000E0BC9, 0x0006154F, 0x000C9535, 0x0017FB96, 0x00176496, 0x0017C9FA, 0x0017C9B2, 0x0017B47D, 0x001CB28B, 0x0017B46D, 0x00115316, 0x0001FA56, 0x0017B455, 0x0003D182, 0x0011FBC3, 0x001797A1, 0x0017B45A, 0x00086359, 0x0017C9FF, 0x001A7662, 0x000304DC, 0x00185017, 0x0017FBA0, 0x0018E382, 0x001AAD53, 0x00179742, 0x00166ABD, 0x000304D0, 0x00166AB9, 0x00185022, 0x0008635B, 0x00225314, 0x0017C994, 0x000EF21F, 0x0008774F, 0x00140512, 0x00179754, 0x000304E8, 0x0019B4EE, 0x0023C669, 0x00140516, 0x0017FBAE, 0x0018E380, 0x000FFF5A, 0x00174F84, 0x0017C9E6, 0x001ABBF9, 0x00176DCB, 0x001797BB, 0x00179730, 0x001764AA, 0x001768F3, 0x0017C99E, 0x00186051, 0x00061548, 0x0017975C, 0x001797B3, 0x001797AC, 0x000EC831, 0x0017B478, 0x000EEF68, 0x00225332, 0x000304C3, 0x00144F66, 0x00176467, 0x0017C9CF, 0x00225331, 0x000A823D, 0x00065669, 0x000AE80C, 0x000AA786, 0x00178EE9, 0x000A02BE, 0x000EEF60, 0x0013576C, 0x001797B0, 0x00047259, 0x00177E22, 0x0017C9EA, 0x0023C671, 0x00177E1B, 0x00176442, 0x0022532F, 0x00185019, 0x00179758, 0x00177E27, 0x00100ABD, 0x000E7517, 0x00065670, 0x000A375B, 0x000E19FD, 0x001797B1, 0x0017C9DC, 0x00185023, 0x0023C670, 0x0022533F, 0x0021BDDE, 0x0015FA43, 0x0008774D, 0x00147729, 0x00176DCC, 0x000AEF9C, 0x00179752, 0x001651EB, 0x00176465, 0x00169618, 0x00178EEB, 0x00178EEE, 0x0003D17F, 0x0017647B, 0x00176482, 0x000A8224, 0x000A02BB, 0x001768DC, 0x0019B4D7, 0x0017C9E0, 0x000AE80D, 0x00140525, 0x0021BDEA, 0x00178ED9, 0x0017B460, 0x00179726, 0x001578A3, 0x00179794, 0x0014053A, 0x000D3EA6, 0x00056A79, 0x00179783, 0x00168B88, 0x00177E30, 0x0023C650, 0x001768EE, 0x000BB861, 0x000304D8, 0x0017C9E7, 0x00086363, 0x00019349, 0x00036FD4, 0x001FDE73, 0x00144A59, 0x00037113, 0x0017FBB3, 0x00047250, 0x00149E69, 0x0017C991, 0x00176476, 0x00140523, 0x0017B47B, 0x0017B453, 0x00185013, 0x0017648F, 0x00144F48, 0x000E61FE, 0x00179721, 0x0023B89C, 0x000D3E9B, 0x0001A33F, 0x0017C99A, 0x00140521, 0x0022531B, 0x0017B45F, 0x0017645C, 0x00140511, 0x0017B469, 0x0017B463, 0x0017B444, 0x0017C9C5, 0x0017B47F, 0x001768D7, 0x0006154A, 0x000FF3ED, 0x00176474, 0x00176484, 0x0013576B, 0x00135761, 0x0017C9EC, 0x0021BDDF, 0x000A8236, 0x0022530C, 0x00179771, 0x0017972C, 0x0017C9C6, 0x000ED73D, 0x0006F104, 0x00144A55, 0x001797B8, 0x001AAD52, 0x001439D0, 0x000C453F, 0x0001FA51, 0x000FFC4E, 0x0017C9D3, 0x00019346, 0x0023C664, 0x0017972D, 0x2A191, 0x2A192, 0x4337F, 0x43380, 0x43381, 0x43382, 0x43383, 0x43384, 0x43385, 0xAF8F3, 0xAF8F4, 0xAF8F5, 0x1AC0B6, 0x1AC2C5, 0x1AB26D, 0x1AB272, 0x1ABFDB, 0x1ABFDA, 0x1B9A5B, 0x1AB267, 0x1AC0B7, 0x1AB26E, 0x1AB271,
	};

	for (UInt32 topicInfoID : topicInfoIDs)
	{
		/*switch (topicInfoID)
		{
		case 0x2A191: case 0x2A192: case 0x4337F: case 0x43380: case 0x43381: case 0x43382: case 0x43383: case 0x43384: case 0x43385: case 0xAF8F3: case 0xAF8F4: case 0xAF8F5:
			continue;
		}*/

		if (existingTopicInfoIDs.count(topicInfoID) > 0)
		{
			continue;
		}

		TESForm* topicInfo = (TESForm*)Runtime_DynamicCast(LookupFormByID(topicInfoID), RTTI_TESForm, RTTI_TESTopicInfo);
		if (!topicInfo)
		{
			self.topicInfoRemainders.push_back(topicInfoID);
			//printf("%X ", topicInfoID);
			continue;
		}

		topicInfos.push_back(topicInfo);
		existingTopicInfoIDs.insert(topicInfoID);
	}

	printf("%llu %llu\n", sizeof(topicInfoIDs) / sizeof(UInt32), topicInfos.size());

	tArray<TESForm*>& topics = (*g_dataHandler)->arrDIAL;

	for (UInt32 i = 0; i < topics.count; i++)
	{
		UInt8* ptr = (UInt8*)topics[i];
		TESForm**& infos = *(TESForm***)(ptr + sizeof(TESForm) + 48);
		UInt32& count1 = *(UInt32*)(ptr + sizeof(TESForm) + 64);
		UInt32& count2 = *(UInt32*)(ptr + sizeof(TESForm) + 68);

		for (UInt32 j = 0; j < count1; j++)
		{
			TESForm* info = (TESForm*)Runtime_DynamicCast(infos[j], RTTI_TESForm, RTTI_TESTopicInfo);
			if (!info)
			{
				continue;
			}
			
			if (existingTopicInfoIDs.count(info->formID) > 0)
			{
				continue;
			}
			
			topicInfos.push_back(info);
		}
	}

	printf("%llu %llu\n", sizeof(topicInfoIDs) / sizeof(UInt32), topicInfos.size());

	VMArray<TESForm*> topicInfoArray(topicInfos);

	// TODO: disabled for testing.
	/*GetInstance().papyrus->GetExternalEventRegistrations("OnTopicInfoRegister", &topicInfoArray, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			VMArray<TESForm*>* topicInfos = (VMArray<TESForm*>*)dataPtr;
			SendPapyrusEvent1(handle, scriptName, callbackName, *topicInfos);
		});*/

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
	F4MP& self = GetInstance();
	//librg_tick(&self.ctx);

	std::vector<TESForm*> newTopicInfos;

	for (auto it = self.topicInfoRemainders.begin(); it != self.topicInfoRemainders.end();)
	{
		TESForm* topicInfo = (TESForm*)Runtime_DynamicCast(LookupFormByID(*it), RTTI_TESForm, RTTI_TESTopicInfo);
		if (!topicInfo)
		{
			it++;
			continue;
		}

		newTopicInfos.push_back(topicInfo);
		it = self.topicInfoRemainders.erase(it);
	}

	if (newTopicInfos.size() > 0)
	{
		printf("new topic infos: %llu\n", newTopicInfos.size());

		VMArray<TESForm*> topicInfoArray(newTopicInfos);

		GetInstance().papyrus->GetExternalEventRegistrations("OnAdditionalTopicInfoRegister", &topicInfoArray, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
			{
				VMArray<TESForm*>* topicInfos = (VMArray<TESForm*>*)dataPtr;
				SendPapyrusEvent1(handle, scriptName, callbackName, *topicInfos);
			});
	}

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
	Entity* entity = Entity::Get(GetInstance().FetchEntity(entityID));
	if (!entity)
	{
		throw std::exception();
	}
	
	entity->SetRef(ref);
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
		return Animator::GetStateName(0).c_str();
	}

	return player->GetAnimState().c_str();
}

VMArray<TESObjectREFR*> f4mp::F4MP::GetRefsInCell(StaticFunctionTag* base, TESObjectCELL* cell)
{
	auto refs = std::vector<TESObjectREFR*>(&cell->objectList.entries[0], &cell->objectList.entries[cell->objectList.count]);
	return refs;
}

Float32 f4mp::F4MP::Atan2(StaticFunctionTag* base, Float32 y, Float32 x)
{
	Float32 angle = atan2(y, x);
	return angle - floor(angle / ZPL_TAU) * ZPL_TAU;
}

BSFixedString f4mp::F4MP::GetWalkDir(StaticFunctionTag* base, Float32 dX, Float32 dY, Float32 angleZ)
{
	switch (Player::GetWalkDir(zpl_vec2{ dX, dY }, angleZ))
	{
	case 0:
		return "Forward";
	case 1:
		return "ForwardRight";
	case 2:
		return "Right";
	case 3:
		return "BackwardRight";
	case 4:
		return "Backward";
	case 5:
		return "BackwardLeft";
	case 6:
		return "Left";
	case 7:
		return "ForwardLeft";
	default:
		return "None";
	}
}

bool f4mp::F4MP::AnimLoops(StaticFunctionTag* base, BSFixedString animState)
{
	return Animator::Loops(Animator::GetStateID(animState.c_str()));
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
