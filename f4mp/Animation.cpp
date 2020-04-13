#include "Animation.h"

std::vector<std::string> f4mp::Animation::stateNames;
std::unordered_map<std::string, SInt32> f4mp::Animation::stateIDs;

void f4mp::Animation::Init()
{
	if (stateNames.size() == 0)
	{
		stateNames =
		{
			"None",

			"JogForward",
			"JogBackward",
			"JogLeft",
			"JogRight",

			"JumpUp",
			"JumpFall",
			"JumpLand",

			"FireWeapon"
		};

		for (SInt32 i = 0; i < static_cast<SInt32>(stateNames.size()); i++)
		{
			const std::string& state = stateNames[i];
			std::string lowerState(state.length(), '\0');

			for (size_t i = 0; i < state.length(); i++)
			{
				lowerState[i] = tolower(state[i]);
			}

			stateIDs[lowerState] = i;
		}
	}
}

const std::string& f4mp::Animation::GetStateName(SInt32 id)
{
	return stateNames[id];
}

SInt32 f4mp::Animation::GetStateID(const std::string& name)
{
	auto id = stateIDs.find(Lower(name));
	if (id == stateIDs.end())
	{
		return -1;
	}

	return id->second;
}

bool f4mp::Animation::Loops(SInt32 id)
{
	std::string animStateName = GetStateName(id);

	if (animStateName == "None")
	{
		return true;
	}
	
	if (animStateName.find("Jog") != std::string::npos)
	{
		return true;
	}

	// TOOD: add more

	return false;
}
