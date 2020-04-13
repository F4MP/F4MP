#include "Animation.h"

std::vector<std::string> f4mp::Animation::animStates;
std::unordered_map<std::string, SInt32> f4mp::Animation::animStateIDs;

void f4mp::Animation::Init()
{
	if (animStates.size() == 0)
	{
		animStates =
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

		for (SInt32 i = 0; i < static_cast<SInt32>(animStates.size()); i++)
		{
			const std::string& state = animStates[i];
			std::string lowerState(state.length(), '\0');

			for (size_t i = 0; i < state.length(); i++)
			{
				lowerState[i] = tolower(state[i]);
			}

			animStateIDs[lowerState] = i;
		}
	}
}

const std::string& f4mp::Animation::GetStateName(SInt32 id)
{
	return animStates[id];
}

SInt32 f4mp::Animation::GetStateID(const std::string& name)
{
	auto id = animStateIDs.find(Lower(name));
	if (id == animStateIDs.end())
	{
		return -1;
	}

	return id->second;
}

bool f4mp::Animation::Loops(SInt32 id)
{
	std::string animStateName = GetStateName(id);

	if (animStateName == "none")
	{
		return true;
	}

	if (animStateName.find("jog") != std::string::npos)
	{
		return true;
	}

	// TOOD: add more

	return false;
}
