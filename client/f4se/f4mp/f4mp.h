#include "client.h"
#include "Player.h"

#include <librg.h>

#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <atomic>

// TODO: completely separate static functions and member variable access with instance function copies of them.

namespace f4mp
{
	struct SyncDataForPapyrus
	{
		TESObjectREFR* ref;
		zpl_vec3 position;
		zpl_vec3 angles;
	};

	class Topic : public TESForm
	{
	public:
		enum { kTypeID = kFormType_DIAL };

		UInt8 unkTopic1[48];
		
		TESForm** infos;

		UInt8 unkTopic2[8];

		UInt32 count;
		UInt32 count2;

		UInt8 unkTopic3[16];
	};

	class F4MP
	{
		friend class Entity;
		friend class Character;
		friend class Player;
		friend class NPC;

	public:
		static F4MP& GetInstance();

		static std::string GetPath();

		F4MP();

		virtual ~F4MP();

		bool Init(const F4SEInterface* f4se);

		librg_entity* FetchEntity(UInt32 id, const std::string& errorMsg = "no entity with ID '%d'!");

		static std::vector<TESForm*> DecodeWornItems(const WornItemsData& wornItems);

		static void SyncTransform(TESObjectREFR* ref, zpl_vec3 position, zpl_vec3 angles, bool ignoreAngleXY = false);

		static void TranslateTo(TESObjectREFR* ref, zpl_vec3 position, zpl_vec3 angles, Float32 speed, Float32 rotSpeed);
		static void MoveTo(TESObjectREFR* ref, zpl_vec3 position, zpl_vec3 angles);

	private:
		static std::vector<std::unique_ptr<F4MP>> instances;
		static size_t activeInstance, nextActiveInstance;

		Config config;

		std::string address;
		SInt32 port;

		librg_ctx ctx;

		PluginHandle handle;

		F4SEMessagingInterface* messaging;
		F4SEPapyrusInterface* papyrus;
		F4SETaskInterface* task;
		F4SEObjectInterface* object;

		std::unique_ptr<Player> player;

		std::unordered_map<UInt32, UInt32> entityIDs;

		std::unordered_map<UInt64, TransformData> buildings;
		std::unordered_set<UInt32> knownBuildings;
		
		std::unordered_map<UInt32, std::unordered_multiset<UInt32>> linesToSpeak;

		std::list<UInt32> topicInfoRemainders;

		Topic* topicInstance;

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
		static void OnSyncEntity(librg_message* msg);
		static void OnSpawnBuilding(librg_message* msg);
		static void OnRemoveBuilding(librg_message* msg);
		static void OnSpeak(librg_message* msg);

		static UInt32 GetClientInstanceID(StaticFunctionTag* base);
		static void SetClient(StaticFunctionTag* base, UInt32 instance);

		static bool IsConnected(StaticFunctionTag* base);
		static bool Connect(StaticFunctionTag* base, Actor* player, TESNPC* playerActorBase, BSFixedString address, SInt32 port);
		static bool Disconnect(StaticFunctionTag* base);
		static void Tick(StaticFunctionTag* base);
		static void SyncWorld(StaticFunctionTag* base);
		
		static UInt32 GetPlayerEntityID(StaticFunctionTag* base);
		static UInt32 GetEntityID(StaticFunctionTag* base, TESObjectREFR* ref);
		static void SetEntityRef(StaticFunctionTag* base, UInt32 entityID, TESObjectREFR* ref);

		static bool IsEntityValid(StaticFunctionTag* base, UInt32 entityID);

		static VMArray<Float32> GetEntityPosition(StaticFunctionTag* base, UInt32 entityID);
		static void SetEntityPosition(StaticFunctionTag* base, UInt32 entityID, float x, float y, float z);
		
		//TODO: abstract ...EntVar...

		static void SetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name, Float32 value);
		static void SetEntVarAnim(StaticFunctionTag* base, UInt32 entityID, BSFixedString animState);
		static Float32 GetEntVarNum(StaticFunctionTag* base, UInt32 entityID, BSFixedString name);
		static BSFixedString GetEntVarAnim(StaticFunctionTag* base, UInt32 entityID);

		static VMArray<TESObjectREFR*> GetRefsInCell(StaticFunctionTag* base, TESObjectCELL* cell);
		static Float32 Atan2(StaticFunctionTag* base, Float32 y, Float32 x);
		static BSFixedString GetWalkDir(StaticFunctionTag* base, Float32 dX, Float32 dY, Float32 angleZ);
		static bool AnimLoops(StaticFunctionTag* base, BSFixedString animState);

		static void CopyAppearance(StaticFunctionTag* base, TESNPC* src, TESNPC* dest);
		static void CopyWornItems(StaticFunctionTag* base, Actor* src, Actor* dest);

		static void PlayerHit(StaticFunctionTag* base, UInt32 hitter, UInt32 hittee, Float32 damage);
		static void PlayerFireWeapon(StaticFunctionTag* base);
	};
}
