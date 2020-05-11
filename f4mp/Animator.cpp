#include "Animator.h"

#include "f4se/NiNodes.h"

#include <fstream>

std::vector<std::vector<std::string>> f4mp::Animator::animatedNodes;
std::vector<std::unordered_map<std::string, UInt32>> f4mp::Animator::animatedNodeIndices;

std::vector<std::string> f4mp::Animator::stateNames;
std::unordered_map<std::string, SInt32> f4mp::Animator::stateIDs;

f4mp::Animator::Animator(Type type) : type(type), startTime(zpl_time_now())
{

}

size_t f4mp::Animator::GetAnimatedNodeCount() const
{
	return animatedNodes[type].size();
}

bool f4mp::Animator::IsAnimatedNode(const std::string& nodeName) const
{
	return animatedNodeIndices[type].count(nodeName) != 0;
}

const std::string& f4mp::Animator::GetNodeName(UInt32 nodeIndex) const
{
	return animatedNodes[type][nodeIndex];
}

UInt32 f4mp::Animator::GetNodeIndex(const std::string& nodeName) const
{
	auto foundNode = animatedNodeIndices[type].find(nodeName);
	if (foundNode == animatedNodeIndices[type].end())
	{
		return (UInt32)-1;
	}

	return foundNode->second;
}

void f4mp::Animator::SetAnimation(Animation* newAnimation)
{
	animation = newAnimation;
}

const f4mp::Animator::Animation* f4mp::Animator::GetAnimation() const
{
	return animation;
}

std::vector<f4mp::Animator::Transform> f4mp::Animator::GetTransforms() const
{
	if (!animation)
	{
		return {};
	}

	// TODO: non-looping animations

	float t = fmod(zpl_time_now() - startTime, animation->duration);
	size_t frame = 0;

	while (animation->frames[frame].duration < t)
	{
		t -= animation->frames[frame].duration;
		frame++;
		if (frame >= animation->frames.size())
		{
			return {};
		}
	}

	const Frame& prevFrame = animation->frames[frame];
	const Frame& nextFrame = animation->frames[(frame + 1) % animation->frames.size()];
	t /= prevFrame.duration;

	std::vector<Transform> curFrame(animation->nodes.size());

	for (size_t node = 0; node < curFrame.size(); node++)
	{
		Transform& curTransform = curFrame[node];
		const Transform& prevTransform = prevFrame.transforms[node];
		const Transform& nextTransform = nextFrame.transforms[node];
		zpl_vec3_lerp(&curTransform.position, prevTransform.position, nextTransform.position, t);
		zpl_quat_slerp_approx(&curTransform.rotation, prevTransform.rotation, nextTransform.rotation, t);
		curTransform.scale = zpl_lerp(prevTransform.scale, nextTransform.scale, t);
	}
	
	return curFrame;
}

bool f4mp::Animator::ForEachNode(TESObjectREFR* ref, const std::function<bool(NiNode*, UInt32)>& callback)
{
	if (!ref)
	{
		return true;
	}

	NiNode* root = ref->GetActorRootNode(false);
	if (!root)
	{
		return true;
	}

	return root->Visit([&](NiAVObject* obj)
		{
			NiNode* node = DYNAMIC_CAST(obj, NiAVObject, NiNode);
			if (!node)
			{
				return false;
			}

			UInt32 nodeIndex = GetNodeIndex(node->m_name.c_str());
			if (nodeIndex >= GetAnimatedNodeCount())
			{
				return false;
			}

			return callback(node, nodeIndex);
		});
}

bool f4mp::Animator::Save(const std::string& path, const Animation& animation) const
{
	std::ofstream file(path);
	if (!file)
	{
		return false;
	}

	file << animation.loops << ' ' << animation.nodes.size() << ' ' << animation.frames.size() << std::endl;

	for (UInt32 nodeIndex : animation.nodes)
	{
		file << GetNodeName(nodeIndex) << std::endl;
	}

	for (const Frame& frame : animation.frames)
	{
		file << frame.duration << std::endl;
		
		for (const Transform& transform : frame.transforms)
		{
			file << transform.position.x << ' ' << transform.position.y << ' ' << transform.position.z << std::endl;
			file << transform.rotation.x << ' ' << transform.rotation.y << ' ' << transform.rotation.z << ' ' << transform.rotation.w << std::endl;
			file << transform.scale << std::endl;
		}
	}

	return true;
}

f4mp::Animator::Animation f4mp::Animator::Load(const std::string& path) const
{
	std::ifstream file(path);
	if (!file)
	{
		return Animation(-1.f);
	}

	size_t nodes, frames;
	std::string nodeName;
	
	Animation animation;

	file >> animation.loops >> nodes >> frames;

	animation.nodes.resize(nodes);
	animation.frames.resize(frames);

	for (UInt32& nodeIndex : animation.nodes)
	{
		file >> nodeName;
		nodeIndex = GetNodeIndex(nodeName);
	}

	for (Frame& frame : animation.frames)
	{
		file >> frame.duration;
		animation.duration += frame.duration;

		frame.transforms.resize(nodes);

		for (Transform& transform : frame.transforms)
		{
			file >> transform.position.x >> transform.position.y >> transform.position.z;
			file >> transform.rotation.x >> transform.rotation.y >> transform.rotation.z >> transform.rotation.w;
			file >> transform.scale;
		}
	}

	return animation;
}

void f4mp::Animator::Init()
{
	if (stateNames.size() == 0)
	{
		animatedNodes =
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

		for (const std::vector<std::string>& allowedNodesInType : animatedNodes)
		{
			std::unordered_map<std::string, UInt32> allowedNodeIndicesInType;

			for (const std::string& allowedNode : allowedNodesInType)
			{
				allowedNodeIndicesInType[allowedNode] = allowedNodeIndicesInType.size();
			}

			animatedNodeIndices.push_back(allowedNodeIndicesInType);
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

const std::string& f4mp::Animator::GetStateName(SInt32 id)
{
	return stateNames[id];
}

SInt32 f4mp::Animator::GetStateID(const std::string& name)
{
	auto id = stateIDs.find(Lower(name));
	if (id == stateIDs.end())
	{
		return -1;
	}

	return id->second;
}

bool f4mp::Animator::Loops(SInt32 id)
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

f4mp::Animator::Animation::Animation(float duration) : loops(true), duration(duration)
{
}
