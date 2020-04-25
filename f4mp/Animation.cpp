#include "Animation.h"

std::vector<std::vector<std::string>> f4mp::Animation::allowedNodes;
std::vector<std::unordered_map<std::string, UInt32>> f4mp::Animation::allowedNodeIndices;

std::vector<std::string> f4mp::Animation::stateNames;
std::unordered_map<std::string, SInt32> f4mp::Animation::stateIDs;

f4mp::Animation::Animation(Type type) : type(type)
{

}

size_t f4mp::Animation::GetAllowedNodeCount() const
{
	return allowedNodes[type].size();
}

bool f4mp::Animation::IsAllowedNode(const std::string& nodeName) const
{
	return allowedNodeIndices[type].count(nodeName) != 0;
}

const std::string& f4mp::Animation::GetNodeName(UInt32 nodeIndex) const
{
	return allowedNodes[type][nodeIndex];
}

UInt32 f4mp::Animation::GetNodeIndex(const std::string& nodeName) const
{
	auto foundNode = allowedNodeIndices[type].find(nodeName);
	if (foundNode == allowedNodeIndices[type].end())
	{
		return (UInt32)-1;
	}

	return foundNode->second;
}

void f4mp::Animation::Init()
{
	if (stateNames.size() == 0)
	{
		allowedNodes =
		{
			// Human
			{
				"skeleton.nif"   ,
				"COM"            ,
				"Pelvis"		 ,
				"LLeg_Thigh"	 ,
				"LLeg_Calf"		 ,
				"LLeg_Foot"		 ,
				"LLeg_Toe1"		 ,
				"RLeg_Thigh"	 ,
				"RLeg_Calf"		 ,
				"RLeg_Foot"		 ,
				"RLeg_Toe1"		 ,
				"SPINE1"		 ,
				"SPINE2"		 ,
				"Chest"			 ,
				"LArm_Collarbone",
				"LArm_UpperArm"	 ,
				"LArm_ForeArm1"	 ,
				"LArm_ForeArm2"	 ,
				"LArm_ForeArm3"	 ,
				"LArm_Hand"		 ,
				"LArm_Finger11"	 ,
				"LArm_Finger12"	 ,
				"LArm_Finger13"	 ,
				"LArm_Finger21"	 ,
				"LArm_Finger22"	 ,
				"LArm_Finger23"	 ,
				"LArm_Finger31"	 ,
				"LArm_Finger32"	 ,
				"LArm_Finger33"	 ,
				"LArm_Finger41"	 ,
				"LArm_Finger42"	 ,
				"LArm_Finger43"	 ,
				"LArm_Finger51"	 ,
				"LArm_Finger52"	 ,
				"LArm_Finger53"	 ,
				"Neck"			 ,
				"Head"			 ,
				"RArm_Collarbone",
				"RArm_UpperArm"	 ,
				"RArm_ForeArm1"	 ,
				"RArm_ForeArm2"	 ,
				"RArm_ForeArm3"	 ,
				"RArm_Hand"		 ,
				"RArm_Finger11"	 ,
				"RArm_Finger12"	 ,
				"RArm_Finger13"	 ,
				"RArm_Finger21"	 ,
				"RArm_Finger22"	 ,
				"RArm_Finger23"	 ,
				"RArm_Finger31"	 ,
				"RArm_Finger32"	 ,
				"RArm_Finger33"	 ,
				"RArm_Finger41"	 ,
				"RArm_Finger42"	 ,
				"RArm_Finger43"	 ,
				"RArm_Finger51"	 ,
				"RArm_Finger52"	 ,
				"RArm_Finger53"	 ,
			}
		};

		for (const std::vector<std::string>& allowedNodesInType : allowedNodes)
		{
			std::unordered_map<std::string, UInt32> allowedNodeIndicesInType;

			for (const std::string& allowedNode : allowedNodesInType)
			{
				allowedNodeIndicesInType[allowedNode] = allowedNodeIndicesInType.size();
			}

			allowedNodeIndices.push_back(allowedNodeIndicesInType);
		}

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
