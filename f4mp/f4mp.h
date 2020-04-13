#include "client.h"
#include "Player.h"

#include <librg.h>

#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

namespace f4mp
{
	class F4MP
	{
		friend class Player;

	public:
		static F4MP& GetInstance();

		F4MP();

		virtual ~F4MP();

		bool Init(const F4SEInterface* f4se);

		static std::vector<TESForm*> DecodeWornItems(const WornItemsData& wornItems);

	private:
		static std::vector<std::unique_ptr<F4MP>> instances;
		static size_t activeInstance;

		std::string address;
		SInt32 port;

		librg_ctx ctx;

		PluginHandle handle;

		F4SEMessagingInterface* messaging;
		F4SEPapyrusInterface* papyrus;
		F4SETaskInterface* task;

		std::unique_ptr<Player> player;

		static void OnConnectRequest(librg_event* event);
		static void OnConnectAccept(librg_event* event);
		static void OnConnectRefuse(librg_event* event);
		static void OnDisonnect(librg_event* event);

		//TODO: abstract event handlers as they're basically the same

		static void OnEntityCreate(librg_event* event);
		static void OnEntityUpdate(librg_event* event);
		static void OnEntityRemove(librg_event* event);

		static void OnClientUpdate(librg_event* event);

		static void OnHit(librg_message* msg);
		static void OnFireWeapon(librg_message* msg);
		
		static UInt32 GetClientInstanceID(StaticFunctionTag* base);
		static void SetClient(StaticFunctionTag* base, UInt32 instance);

		static bool Connect(StaticFunctionTag* base, Actor* player, TESNPC* playerActorBase, BSFixedString address, SInt32 port);
		static bool Disconnect(StaticFunctionTag* base);
		static void Tick(StaticFunctionTag* base);

		static UInt32 GetPlayerEntityID(StaticFunctionTag* base);
		static bool IsEntityValid(StaticFunctionTag* base, UInt32 entityID);

		static VMArray<Float32> GetEntityPosition(StaticFunctionTag* base, UInt32 entityID);
		static void SetEntityPosition(StaticFunctionTag* base, UInt32 entityID, float x, float y, float z);

		//TODO: abstract ...EntVar...

		static void SetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name, Float32 value);
		static void SetEntVarAnim(StaticFunctionTag* base, UInt32 entityID, BSFixedString animState);
		static Float32 GetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name);
		static BSFixedString GetEntVarAnim(StaticFunctionTag* base, UInt32 entityID);
		
		static Float32 Atan2(StaticFunctionTag* base, Float32 y, Float32 x);
		static BSFixedString _GetWalkDir(StaticFunctionTag* base, Float32 dX, Float32 dY, Float32 angleZ);
		static bool AnimLoops(StaticFunctionTag* base, BSFixedString animState);
		
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
		
		static void CopyAppearance(StaticFunctionTag* base, TESNPC* src, TESNPC* dest);
		static void CopyWornItems(StaticFunctionTag* base, Actor* src, Actor* dest);
		
		static void PlayerHit(StaticFunctionTag* base, UInt32 hitter, UInt32 hittee, Float32 damage);
		static void PlayerFireWeapon(StaticFunctionTag* base);
	};
}
