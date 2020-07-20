#include "Player.h"

f4mp::Connection::Connection(const std::string& address, Player* player) : address(address), player(player)
{
}

f4mp::User::User(networking::Entity::ID id, const std::string& username, const Connection& connection) : id(id), username(username), connection(connection)
{
}
