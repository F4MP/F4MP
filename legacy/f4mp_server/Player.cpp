#include "Player.h"

const f4mp::AppearanceData& f4mp::Player::GetAppearance() const
{
	return appearance;
}

const f4mp::WornItemsData& f4mp::Player::GetWornItems() const
{
	return wornItems;
}

void f4mp::Player::OnConnectRequest(librg_event* event)
{
    Character::OnConnectRequest(event);

    AppearanceData& playerAppearance = appearance;
    WornItemsData& playerWornItems = wornItems;

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
}

void f4mp::Player::OnConnectRefuse(librg_event* event)
{
    Character::OnConnectRefuse(event);

    delete this;
    event->user_data = nullptr;
}

void f4mp::Player::OnEntityCreate(librg_event* event)
{
    Character::OnEntityCreate(event);



    AppearanceData& playerAppearance = appearance;
    WornItemsData& playerWornItems = wornItems;

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

void f4mp::Player::OnEntityUpdate(librg_event* event)
{
    Character::OnEntityUpdate(event);
    
    librg_data_wf32(event->data, health);

    librg_data_wi32(event->data, animState);
}

void f4mp::Player::OnClientUpdate(librg_event* event)
{
    Character::OnClientUpdate(event);

    health = librg_data_rf32(event->data);

    animState = librg_data_ri32(event->data);
}