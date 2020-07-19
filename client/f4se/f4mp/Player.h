#pragma once

#include "Character.h"

namespace f4mp
{
	class Player;

	class Connection
	{
	public:
		Connection(const std::string& address, Player* player);

	private:
		std::string address;

		Player* player;
	};

	class User
	{
	public:
		User(networking::Entity::ID id, const std::string& username, const Connection& connection);

	private:
		networking::Entity::ID id;

		std::string username;
		
		Connection connection;
	};
}