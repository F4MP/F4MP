#pragma once

#include "client.h"

#include <queue>

namespace f4mp
{
	class Clone
	{
	private:
		std::queue<PlayerData> records;
	};
}