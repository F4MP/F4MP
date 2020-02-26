#pragma once

#include <librg.h>

#include "common.h"

namespace f4mp
{
    struct Player
    {
        float angles[3];

        float health;

        int animState;

        AppearanceData appearance;
        WornItemsData wornItems;
    };

    class Server
    {
    private:
        static void on_connect_request(librg_event* event) {
            //u32 secret = 0;// librg_data_ru32(event->data);

            //if (secret != 42) {
            //    librg_log("intruder!\n");
            //    //librg_event_reject(event);
            //}

            Player* player = new Player{};
            event->user_data = player;

            AppearanceData& playerAppearance = player->appearance;
            WornItemsData& playerWornItems = player->wornItems;

            Utils::Read(event->data, playerAppearance.female);
            Utils::Read(event->data, playerAppearance.weights);
            Utils::Read(event->data, playerAppearance.hairColor);
            Utils::Read(event->data, playerAppearance.headParts);
            Utils::Read(event->data, playerAppearance.morphSetValue);
            Utils::Read(event->data, playerAppearance.morphRegionData1);
            Utils::Read(event->data, playerAppearance.morphRegionData2);
            Utils::Read(event->data, playerAppearance.morphSetData1);
            Utils::Read(event->data, playerAppearance.morphSetData2);

            Utils::Read(event->data, playerWornItems.data1);
            Utils::Read(event->data, playerWornItems.data2);

            librg_log("connection requested");
        }

        static void on_connect_accepted(librg_event* event) {
            librg_log("%d", event->user_data);

            librg_log("on_connect_accepted\n");
            librg_entity* blob = event->entity;

            // blob->position.x = (float)(2000 - rand() % 4000);
            // blob->position.y = (float)(2000 - rand() % 4000);
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

            librg_log("%p\n", event->user_data);

            event->entity->user_data = event->user_data;
        }

        static void on_connect_refused(librg_event* event) {
            librg_log("on_connect_refused\n");

            delete (Player*)event->user_data;
        }

        static void on_connect_disconnect(librg_event* event) {
            librg_log("entity %d peer disconnected: %x\n", event->entity->id, event->peer);

            delete (Player*)event->entity->user_data;
        }

        static void on_entity_create(librg_event* event) {
            Player* player = (Player*)event->entity->user_data;
            if (!player)
            {
                return;
            }

            AppearanceData& playerAppearance = player->appearance;
            WornItemsData& playerWornItems = player->wornItems;

            Utils::Write(event->data, playerAppearance.female);
            Utils::Write(event->data, playerAppearance.weights);
            Utils::Write(event->data, playerAppearance.hairColor);
            Utils::Write(event->data, playerAppearance.headParts);
            Utils::Write(event->data, playerAppearance.morphSetValue);
            Utils::Write(event->data, playerAppearance.morphRegionData1);
            Utils::Write(event->data, playerAppearance.morphRegionData2);
            Utils::Write(event->data, playerAppearance.morphSetData1);
            Utils::Write(event->data, playerAppearance.morphSetData2);

            Utils::Write(event->data, playerWornItems.data1);
            Utils::Write(event->data, playerWornItems.data2);
        }

        static void on_entity_update(librg_event* event) {
            Player* player = (Player*)event->entity->user_data;
            if (!player)
            {
                return;
            }

            librg_data_wf32(event->data, player->angles[0]);
            librg_data_wf32(event->data, player->angles[1]);
            librg_data_wf32(event->data, player->angles[2]);

            librg_data_wf32(event->data, player->health);

            librg_data_wi32(event->data, player->animState);
        }

        static void on_entity_remove(librg_event* event) {
            librg_log("remove ent %d for client %x\n", event->entity->id, event->peer);
        }

        static void on_update(librg_event* e) {
            Player* player = (Player*)e->entity->user_data;
            if (!player)
            {
                return;
            }

            librg_data_read_safe(f32, ax, e->data);
            librg_data_read_safe(f32, ay, e->data);
            librg_data_read_safe(f32, az, e->data);
            librg_data_read_safe(f32, health, e->data);
            librg_data_read_safe(i32, animState, e->data);

            player->angles[0] = ax;
            player->angles[1] = ay;
            player->angles[2] = az;

            player->health = health;

            player->animState = animState;
        }

        static void OnHit(librg_message* msg)
        {
            HitData data;
            librg_data_rptr(msg->data, &data, sizeof(HitData));

            librg_message_send_to(msg->ctx, Hit, librg_entity_control_get(msg->ctx, data.hittee), &data, sizeof(HitData));
        }

        static void OnFireWeapon(librg_message* msg)
        {
            librg_log("on_fire_weapon: %u\n", msg->peer->connectID);
        }

    public:
        Server(const std::string& address)
        {
            librg_option_set(LIBRG_MAX_ENTITIES_PER_BRANCH, 4);

            librg_ctx ctx = { 0 };

            ctx.tick_delay = 10;
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

            librg_network_add(&ctx, Message::Hit, OnHit);
            librg_network_add(&ctx, Message::FireWeapon, OnFireWeapon);

            librg_network_start(&ctx, librg_address{ 7779, const_cast<char*>(address.c_str()) });

            librg_log("server started!");

            // for (isize i = 0; i < 1; i++) {
            //     librg_entity *enemy = librg_entity_create(&ctx, 0);

            //     //librg_attach_foo(&ctx, enemy, NULL);
            //     // enemy->position.x = (float)(2000 - rand() % 4000);
            //     // enemy->position.x = (float)(2000 - rand() % 4000);
            //     enemy->position.x = 10;
            //     enemy->position.y = 10;

            //     batid = enemy->id;
            // }

            //zpl_timer* tick_timer = zpl_timer_add(ctx.timers);
            //tick_timer->user_data = (void*)&ctx; /* provide ctx as a argument to timer */
            //zpl_timer_set(tick_timer, 1.0, -1, measure);
            //zpl_timer_start(tick_timer, 1.0);

            while (true) {
                librg_tick(&ctx);
                zpl_sleep_ms(1);
            }

            librg_network_stop(&ctx);
            librg_free(&ctx);
        }
    };
}