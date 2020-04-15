#include "Player.h"
#include "f4mp.h"

void f4mp::Player::OnConnect(Actor* player, TESNPC* playerActorBase)
{
	appearance.Fill(playerActorBase);
	wornItems.Fill(player);
}

void f4mp::Player::OnEntityCreate(librg_event* event)
{
	Entity::OnEntityCreate(event);

	Utils::Read(event->data, appearance.female);
	Utils::Read(event->data, appearance.weights);
	Utils::Read(event->data, appearance.hairColor);
	Utils::Read(event->data, appearance.headParts);
	Utils::Read(event->data, appearance.morphSetValue);
	Utils::Read(event->data, appearance.morphRegionData1);
	Utils::Read(event->data, appearance.morphRegionData2);
	Utils::Read(event->data, appearance.morphSetData1);
	Utils::Read(event->data, appearance.morphSetData2);

	Utils::Read(event->data, wornItems.data1);
	Utils::Read(event->data, wornItems.data2);

	for (const auto& item : wornItems.data2)
	{
		_MESSAGE("%s", item.c_str());
	}

	auto& npcs = (*g_dataHandler)->arrNPC_;
	for (UInt32 i = 0; i < npcs.count; i++)
	{
		if (strcmp(npcs[i]->fullName.name.c_str(), "F4MP Player") == 0)
		{
			SetAppearance(npcs[i], appearance);
			break;
		}
	}

	struct Tmp
	{
		UInt32 id;
		const WornItemsData& wornItems;
	} tmp{ event->entity->id, GetWornItems() };

	F4MP& self = F4MP::GetInstance();

	self.papyrus->GetExternalEventRegistrations("OnEntityCreate", &tmp, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			Tmp* tmp = static_cast<Tmp*>(dataPtr);
			VMArray<TESForm*> wornItems(F4MP::DecodeWornItems(tmp->wornItems));

			SendPapyrusEvent2<UInt32, VMArray<TESForm*>>(handle, scriptName, callbackName, tmp->id, wornItems);
		});
}

void f4mp::Player::OnEntityUpdate(librg_event* event)
{
	Entity::OnEntityUpdate(event);

	SetNumber("angleX", librg_data_rf32(event->data));
	SetNumber("angleY", librg_data_rf32(event->data));
	SetNumber("angleZ", librg_data_rf32(event->data));

	SetNumber("health", librg_data_rf32(event->data));

	SetAnimStateID(librg_data_ri32(event->data));
}

void f4mp::Player::OnEntityRemove(librg_event* event)
{
	Entity::OnEntityRemove(event);
}

void f4mp::Player::OnClientUpdate(librg_event* event)
{
	Entity::OnClientUpdate(event);

	const std::string& animState = GetAnimState();

	// TODO: move this to the Animation class.

	zpl_vec2 displacement = event->entity->position.xy - prevPosition.xy;
	const char* newAnimState;

	switch (GetWalkDir(displacement, GetLookAngle()))
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

	SetAnimState(newAnimState);

	prevPosition = event->entity->position;

	librg_data_wf32(event->data, GetNumber("angleX"));
	librg_data_wf32(event->data, GetNumber("angleY"));
	librg_data_wf32(event->data, GetNumber("angleZ"));

	librg_data_wf32(event->data, GetNumber("health"));

	librg_data_wi32(event->data, GetAnimStateID());
}

Float32 f4mp::Player::GetNumber(const std::string& name) const
{
	return numbers.find(name)->second;
}

SInt32 f4mp::Player::GetInteger(const std::string& name) const
{
	return integers.find(name)->second;
}

void f4mp::Player::SetNumber(const std::string& name, Float32 number)
{
	if (F4MP::GetInstance().player.get() == this)
	{
		for (auto& instance : F4MP::instances)
		{
			instance->player->numbers[name] = number;
		}

		return;
	}

	numbers[name] = number;
}

void f4mp::Player::SetInteger(const std::string& name, SInt32 integer)
{
	if (F4MP::GetInstance().player.get() == this)
	{
		for (auto& instance : F4MP::instances)
		{
			instance->player->integers[name] = integer;
		}

		return;
	}

	integers[name] = integer;
}

SInt32 f4mp::Player::GetAnimStateID() const
{
	return GetInteger("animState");
}

const std::string& f4mp::Player::GetAnimState() const
{
	return Animation::GetStateName(GetAnimStateID());
}

float f4mp::Player::GetLookAngle() const
{
	return GetNumber("angleZ");
}

void f4mp::Player::SetAnimStateID(SInt32 id)
{
	SetInteger("animState", id);
}

void f4mp::Player::SetAnimState(const std::string& name)
{
	SetAnimStateID(Animation::GetStateID(name));
}

UInt32 f4mp::Player::GetEntityID() const
{
	return entityID;
}

const f4mp::client::AppearanceData& f4mp::Player::GetAppearance() const
{
	return appearance;
}

const f4mp::client::WornItemsData& f4mp::Player::GetWornItems() const
{
	return wornItems;
}

void f4mp::Player::OnConnectRequest(librg_event* event)
{
	Entity::OnConnectRequest(event);

	Utils::Write(event->data, appearance.female);
	Utils::Write(event->data, appearance.weights);
	Utils::Write(event->data, appearance.hairColor);
	Utils::Write(event->data, appearance.headParts);
	Utils::Write(event->data, appearance.morphSetValue);
	Utils::Write(event->data, appearance.morphRegionData1);
	Utils::Write(event->data, appearance.morphRegionData2);
	Utils::Write(event->data, appearance.morphSetData1);
	Utils::Write(event->data, appearance.morphSetData2);

	Utils::Write(event->data, wornItems.data1);
	Utils::Write(event->data, wornItems.data2);
}

void f4mp::Player::OnConnectAccept(librg_event* event)
{
	Entity::OnConnectAccept(event);

	entityID = event->entity->id;

	event->entity->user_data = this;
}

void f4mp::Player::OnDisonnect(librg_event* event)
{
	Entity::OnDisonnect(event);
}

int f4mp::Player::GetWalkDir(const zpl_vec2& displacement, float lookAngle)
{
	const float walkThreshold = 1.f;

	float speed = zpl_vec2_mag(displacement);
	printf("%f\n", speed);
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

	return fabsf(dot) > cosf(45.f * deg2rad) ? (dot > 0.f ? 0 : 1) : (cross > 0.f ? 2 : 3);
}

void f4mp::Player::SetAppearance(TESNPC* actorBase, const AppearanceData& appearance)
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

void f4mp::Player::SetWornItems(Actor* actor, const WornItemsData& wornItems)
{
	F4MP& self = F4MP::GetInstance();

	struct Args
	{
		Actor* actor;
		const WornItemsData& wornItems;
	} args{ actor, wornItems };

	self.papyrus->GetExternalEventRegistrations("OnCopyWornItems", &args, [](UInt64 handle, const char* scriptName, const char* callbackName, void* dataPtr)
		{
			Args* args = static_cast<Args*>(dataPtr);
			VMArray<TESForm*> itemsToWear(F4MP::DecodeWornItems(args->wornItems));

			SendPapyrusEvent2<Actor*, VMArray<TESForm*>>(handle, scriptName, callbackName, args->actor, itemsToWear);
		});
}

f4mp::Player::Player() : entityID((UInt32)-1)
{
	animation = std::make_unique<Animation>();
}
