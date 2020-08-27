#pragma once

#include <librg.h>

#include "server_common.h"
#include "Entity.h"
#include "Player.h"
#include "NPC.h"

#include <cfloat>
#include <unordered_map>

namespace f4mp
{
	class Server
	{
	private:
		static Server* instance;

		std::string address;
		i32 port;

		librg_ctx ctx;

		std::unordered_map<u32, u32> entityIDs;

		std::unordered_map<u32, f64> entitiesSyncedTime;

		static void on_connect_request(librg_event* event)
		{
			Player* player = new Player{};
			event->user_data = player;
			event->peer->data = player;

			player->OnConnectRequest(event);

			librg_log("connection requested\n");
		}

		static void on_connect_accepted(librg_event* event)
		{
			librg_log("on_connect_accepted %x\n", event->peer);
			librg_entity* blob = event->entity;

			blob->position = zpl_vec3{ 886.134460f, -426.953460f, -1550.012817f };

			librg_log("spawning player %u at: %f %f %f\n",
				event->entity->id,
				blob->position.x,
				blob->position.y,
				blob->position.z
			);

			Server* self = (Server*)event->ctx->user_data;

			enet_peer_timeout(event->peer, UINT32_MAX, UINT32_MAX, UINT32_MAX);
			librg_entity_control_set(event->ctx, event->entity->id, event->peer);
			librg_entity_visibility_set(event->ctx, event->entity->id, LIBRG_ALWAYS_VISIBLE);
			//librg_entity_world_set(event->ctx, event->entity->id, 0);

			event->entity->user_data = event->user_data;

			Entity* entity = Entity::Get(event);
			if (!entity)
			{
				return;
			}

			entity->SetEntityID(event->entity->id);
			entity->OnConnectAccept(event);
		}

		static void on_connect_refused(librg_event* event)
		{
			librg_log("on_connect_refused\n");

			Player* player = (Player*)event->user_data;
			if (!player)
			{
				return;
			}

			player->OnConnectRefuse(event);
		}

		static void on_connect_disconnect(librg_event* event)
		{
			librg_log("entity %d peer disconnected: %x\n", event->entity->id, event->peer);

			Entity* entity = Entity::Get(event);
			if (!entity)
			{
				return;
			}

			entity->OnDisonnect(event);
		}

		static void on_entity_create(librg_event* event)
		{
			Entity* entity = Entity::Get(event);
			if (!entity)
			{
				return;
			}

			entity->OnEntityCreate(event);
		}

		static void on_entity_update(librg_event* event)
		{
			Entity* entity = Entity::Get(event);
			if (!entity)
			{
				return;
			}

			entity->OnEntityUpdate(event);
		}

		static void on_entity_remove(librg_event* event)
		{
			librg_log("remove ent %d for client %x\n", event->entity->id, event->peer);

			Entity* entity = Entity::Get(event);
			if (!entity)
			{
				return;
			}

			entity->OnEntityRemove(event);
		}

		static void on_update(librg_event* event)
		{
			Entity* entity = Entity::Get(event);
			if (!entity)
			{
				return;
			}

			entity->OnClientUpdate(event);
		}

		static void OnHit(librg_message* msg)
		{
			HitData data;
			librg_data_rptr(msg->data, &data, sizeof(HitData));

			librg_message_send_to(msg->ctx, MessageType::Hit, librg_entity_control_get(msg->ctx, data.hittee), &data, sizeof(HitData));
		}

		static void OnFireWeapon(librg_message* msg)
		{
			u32 entity;
			librg_data_rptr(msg->data, &entity, sizeof(u32));

			librg_message_send_except(msg->ctx, MessageType::FireWeapon, msg->peer, &entity, sizeof(u32));
		}

		static void OnSpawnEntity(librg_message* msg)
		{
			SpawnEntityData data;
			librg_data_rptr(msg->data, &data, sizeof(SpawnEntityData));

			auto entityID = instance->entityIDs.find(data.formID);
			if (entityID != instance->entityIDs.end())
			{
				return;
			}

			librg_log("entity spawned: %x (%f, %f, %f) (%f, %f, %f)\n", data.formID, data.position.x, data.position.y, data.position.z, data.angles.x, data.angles.y, data.angles.z);

			// TODO: change it so only the host can add entities?

			librg_entity* entity = librg_entity_create(msg->ctx, EntityType::NPC);
			librg_entity_control_set(msg->ctx, entity->id, msg->peer);
			librg_entity_visibility_set(msg->ctx, entity->id, LIBRG_ALWAYS_VISIBLE);

			entity->position = data.position;

			NPC* npc = Entity::Create(entity, new NPC(data.formID, data.ownerEntityID));

			data.entityID = entity->id;
			data.ownerEntityID = Entity::GetAs<Player>(msg->peer)->GetEntityID();

			instance->entityIDs[data.formID] = data.entityID;

			//librg_message_send_all(msg->ctx, MessageType::SpawnEntity, &data, sizeof(SpawnEntityData));
		}

		static void OnSyncEntity(librg_message* msg)
		{
			SyncEntityData data;
			librg_data_rptr(msg->data, &data, sizeof(SyncEntityData));

			auto syncedTime = instance->entitiesSyncedTime.find(data.formID);
			if (syncedTime != instance->entitiesSyncedTime.end())
			{
				if (data.syncedTime < syncedTime->second)
				{
					return;
				}
			}

			u32 callerID = Entity::Get(msg->peer)->GetEntityID();
			float minDist = zpl_vec3_mag2(data.position - librg_entity_fetch(msg->ctx, callerID)->position);

			librg_entity_iteratex(msg->ctx, LIBRG_ENTITY_ALIVE | LIBRG_ENTITY_CLIENT, id,
				{
					if (id == callerID)
					{
						continue;
					}

					librg_entity* entity = librg_entity_fetch(msg->ctx, id);
					if (zpl_vec3_mag2(data.position - entity->position) < minDist)
					{
						return;
					}
				});

			instance->entitiesSyncedTime[data.formID] = data.syncedTime;

			librg_message_send_except(msg->ctx, MessageType::SyncEntity, msg->peer, &data, sizeof(SyncEntityData));
		}

		static void OnSpawnBuilding(librg_message* msg)
		{
			SpawnBuildingData data;
			librg_data_rptr(msg->data, &data, sizeof(SpawnBuildingData));

			librg_log("building spawned: %llx %f %f %f\n", GetUniqueFormID(data.ownerEntityID, data.formID), data.position.x, data.position.y, data.position.z);

			librg_message_send_except(msg->ctx, MessageType::SpawnBuilding, msg->peer, &data, sizeof(SpawnBuildingData));
		}

		static void OnRemoveBuilding(librg_message* msg)
		{
			RemoveBuildingData data;
			librg_data_rptr(msg->data, &data, sizeof(RemoveBuildingData));

			librg_log("building removed: %llx\n", data.uniqueFormID);

			librg_message_send_except(msg->ctx, MessageType::RemoveBuilding, msg->peer, &data, sizeof(RemoveBuildingData));
		}

		static void OnSpeak(librg_message* msg)
		{
			SpeakData data;
			librg_data_rptr(msg->data, &data, sizeof(SpeakData));

			printf("%u: %X spoke %X\n", data.clientEntityID, data.speakerFormID, data.topicInfoFormID);

			librg_message_send_except(msg->ctx, MessageType::Speak, msg->peer, &data, sizeof(SpeakData));
		}

	public:
		Server(const std::string& address, i32 port) : address(address), port(port), ctx{}
		{
			instance = this;

			librg_option_set(LIBRG_MAX_ENTITIES_PER_BRANCH, 4);

			ctx.tick_delay = 10.0;
			ctx.mode = LIBRG_MODE_SERVER;
			ctx.world_size = zpl_vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
			ctx.min_branch_size = zpl_vec3f(-1, -1, -1);
			ctx.max_entities = 60000;
			ctx.max_connections = 1200;
			ctx.user_data = this;

			librg_init(&ctx);

			librg_event_add(&ctx, LIBRG_CONNECTION_REQUEST, on_connect_request);
			librg_event_add(&ctx, LIBRG_CONNECTION_ACCEPT, on_connect_accepted);
			librg_event_add(&ctx, LIBRG_CONNECTION_REFUSE, on_connect_refused);
			librg_event_add(&ctx, LIBRG_CONNECTION_DISCONNECT, on_connect_disconnect);
			librg_event_add(&ctx, LIBRG_CLIENT_STREAMER_UPDATE, on_update);

			librg_event_add(&ctx, LIBRG_ENTITY_CREATE, on_entity_create);
			librg_event_add(&ctx, LIBRG_ENTITY_UPDATE, on_entity_update);
			librg_event_add(&ctx, LIBRG_ENTITY_REMOVE, on_entity_remove);

			librg_network_add(&ctx, MessageType::Hit, OnHit);
			librg_network_add(&ctx, MessageType::FireWeapon, OnFireWeapon);
			librg_network_add(&ctx, MessageType::SpawnEntity, OnSpawnEntity);
			librg_network_add(&ctx, MessageType::SyncEntity, OnSyncEntity);
			librg_network_add(&ctx, MessageType::SpawnBuilding, OnSpawnBuilding);
			librg_network_add(&ctx, MessageType::RemoveBuilding, OnRemoveBuilding);
			librg_network_add(&ctx, MessageType::Speak, OnSpeak);

			librg_log("\
    F4MP  Copyright (C) 2020  Hyunsung Go\n\
    This program comes with ABSOLUTELY NO WARRANTY.\n\
    This is free software, and you are welcome to redistribute it\n\
    under certain conditions; Read LICENSE.txt for full details.\n\n");
		}

		virtual ~Server()
		{
			librg_network_stop(&ctx);
			librg_free(&ctx);

			// just in case..
			if (instance == this)
			{
				instance = nullptr;
			}
		}

		void Start()
		{
			librg_network_start(&ctx, librg_address{ port, const_cast<char*>(address.c_str()) });

			librg_log("    Server started. Listening on %s:%d\n\n", address.c_str(), port);
		}

		void Tick()
		{
			librg_tick(&ctx);
			zpl_sleep_ms(1);
		}

		librg_ctx* GetContext()
		{
			return &ctx;
		}
	};
}