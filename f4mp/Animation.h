#pragma once

#include "client.h"

namespace f4mp
{
	class Animation
	{
	private:
		static std::vector<std::string> animStates;
		static std::unordered_map<std::string, SInt32> animStateIDs;

	public:
		static void Init();

		static const std::string& GetStateName(SInt32 id);
		static SInt32 GetStateID(const std::string& name);

		static bool Loops(SInt32 id);
	};
}