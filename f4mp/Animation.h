#pragma once

#include "client.h"

namespace f4mp
{
	class Animation
	{
	private:
		static std::vector<std::string> stateNames;
		static std::unordered_map<std::string, SInt32> stateIDs;

	public:
		static void Init();

		static const std::string& GetStateName(SInt32 id);
		static SInt32 GetStateID(const std::string& name);

		static bool Loops(SInt32 id);
	};
}