#pragma once

#include <librg.h>

#include "server_common.h"
#include "Entity.h"
#include "Player.h"

#include <unordered_map>

namespace f4mp
{
    class Server
    {
    private:
        static Server* instance;

        std::unordered_map<u32, u32> entityIDs;

        static void on_connect_request(librg_event* event)
        {
            Player* player = new Player{};
            event->user_data = player;
            event->peer->data = player;

            player->OnConnectRequest(event);

            librg_log("connection requested");
        }

        static void on_connect_accepted(librg_event* event)
        {
            librg_log("%d", event->user_data);

            librg_log("on_connect_accepted\n");
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

            librg_log("%p\n", event->user_data);

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
            SpawnData data;
            librg_data_rptr(msg->data, &data, sizeof(SpawnData));

            auto entityID = instance->entityIDs.find(data.formID);
            if (entityID != instance->entityIDs.end())
            {
                return;
            }

            librg_log("entity spawned: %x (%f, %f, %f) (%f, %f, %f)\n", data.formID, data.position.x, data.position.y, data.position.z, data.angles.x, data.angles.y, data.angles.z);

            // TODO: change it so only the host can add entities?

            librg_entity* entity = librg_entity_create(msg->ctx, EntityType::NPC);
            librg_entity_control_set(msg->ctx, entity->id, msg->peer);

            entity->position = data.position;
            
            Entity::Create(entity);

            data.entityID = entity->id;
            data.ownerEntityID = Entity::GetAs<Player>(msg->peer)->GetEntityID();

            instance->entityIDs[data.formID] = data.entityID;
            
            librg_message_send_all(msg->ctx, MessageType::SpawnEntity, &data, sizeof(SpawnData));
        }

        static void OnSyncEntity(librg_message* msg)
        {
            SyncData data;
            librg_data_rptr(msg->data, &data, sizeof(SyncData));

            u32 callerID = Entity::Get(msg->peer)->GetEntityID();
            float minDist = zpl_vec3_mag2(data.position - librg_entity_fetch(msg->ctx, callerID)->position);

            librg_entity_iteratex(msg->ctx, LIBRG_ENTITY_ALIVE, id,
                {
                    if (id == callerID)
                    {
                        continue;
                    }

                    librg_entity* entity = librg_entity_fetch(msg->ctx, id);
                    if (Entity::GetAs<Player>(entity) == nullptr)
                    {
                        continue;
                    }

                    if (zpl_vec3_mag2(data.position - entity->position) < minDist)
                    {
                        return;
                    }
                });

            librg_message_send_except(msg->ctx, MessageType::SyncEntity, msg->peer, &data, sizeof(SyncData));
        }

    public:
        Server(const std::string& address)
        {
            instance = this;

            librg_option_set(LIBRG_MAX_ENTITIES_PER_BRANCH, 4);

            librg_ctx ctx = { 0 };

            ctx.tick_delay = 1.0;
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

            librg_network_start(&ctx, librg_address{ 7779, const_cast<char*>(address.c_str()) });

            librg_log("server started!");

            while (true) {
                librg_tick(&ctx);
                zpl_sleep_ms(1);
            }

            librg_network_stop(&ctx);
            librg_free(&ctx);
        }

        virtual ~Server()
        {
            // just in case..
            if (instance == this)
            {
                instance = nullptr;
            }
        }
    };
}