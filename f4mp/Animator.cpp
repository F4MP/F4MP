#include "Animator.h"
#include "Animations.h"
#include "Player.h"

#include "f4se/NiNodes.h"

#include <fstream>

std::vector<std::vector<std::string>> f4mp::Animator::animatedNodes;
std::vector<std::unordered_map<std::string, UInt32>> f4mp::Animator::animatedNodeIndices;

std::vector<std::string> f4mp::Animator::stateNames;
std::unordered_map<std::string, SInt32> f4mp::Animator::stateIDs;

std::unordered_map<std::string, f4mp::Animator::Animation> f4mp::Animator::animations;

f4mp::Animator::Animator(Type type) : type(type), startTime(zpl_time_now())
{
	Play(&animations["idle"]);
}

void f4mp::Animator::Play(const Animation* newAnimation)
{
	animation = newAnimation;
	startTime = zpl_time_now();
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

bool f4mp::Animator::Save(const std::string& path, const Animation& animation, bool binary) const
{
	std::ofstream file(path);
	if (!file)
	{
		return false;
	}

	std::vector<UCHAR> data;

	if (binary)
	{
		const size_t nodes = animation.nodes.size(), frames = animation.frames.size();
		data.insert(data.end(), (UCHAR*)(&animation.loops), (UCHAR*)(&animation.loops + 1));
		data.insert(data.end(), (UCHAR*)(&nodes), (UCHAR*)(&nodes + 1));
		data.insert(data.end(), (UCHAR*)(&frames), (UCHAR*)(&frames + 1));
	}
	else
	{
		file << animation.loops << ' ' << animation.nodes.size() << ' ' << animation.frames.size() << std::endl;
	}

	for (const std::string& nodeName : animation.nodes)
	{
		if (binary)
		{
			const size_t length = nodeName.length();
			data.insert(data.end(), (UCHAR*)(&length), (UCHAR*)(&length + 1));
			data.insert(data.end(), (UCHAR*)(&nodeName[0]), (UCHAR*)(&nodeName[0] + length));
		}
		else
		{
			file << nodeName << ' ';
		}
	}

	if (binary)
	{
		file << std::hex;

		for (UCHAR byte : data)
		{
			file << "0x" << (UINT)byte << ", ";
		}

		data.clear();
	}
	else
	{
		file << std::endl;
	}

	for (const Frame& frame : animation.frames)
	{
		if (binary)
		{
			data.insert(data.end(), (UCHAR*)(&frame.duration), (UCHAR*)(&frame.duration + 1));
		}
		else
		{
			file << frame.duration << ' ';
		}

		for (const Transform& transform : frame.transforms)
		{
			if (binary)
			{
				data.insert(data.end(), (UCHAR*)(&transform), (UCHAR*)(&transform + 1));
			}
			else
			{
				file << transform.position.x << ' ' << transform.position.y << ' ' << transform.position.z << ' ';
				file << transform.rotation.x << ' ' << transform.rotation.y << ' ' << transform.rotation.z << ' ' << transform.rotation.w << ' ';
				file << transform.scale << ' ';
			}
		}

		file << std::endl;

		if (binary)
		{
			for (UCHAR byte : data)
			{
				file << "0x" << (UINT)byte << ", ";
			}

			data.clear();
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

	for (std::string& nodeName : animation.nodes)
	{
		file >> nodeName;
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

void f4mp::Animator::OnClientUpdate(const Player& player)
{
	const Animation* newAnimation = &animations["idle"];

	int walkDir = player.GetWalkDir(player.GetDisplacement().xy, player.GetLookAngle());
	if (walkDir >= 0)
	{
		const Animation* walkAnimations[]
		{
			&animations["jog_forward"],
			&animations["jog_forward_right"],
			&animations["jog_right"],
			&animations["jog_backward_right"],
			&animations["jog_backward"],
			&animations["jog_backward_left"],
			&animations["jog_left"],
			&animations["jog_forward_left"],
		};
		newAnimation = walkAnimations[walkDir];
	}

	if (animation != newAnimation)
	{
		Play(newAnimation);
	}
}

f4mp::Animator::Animation f4mp::Animator::Load(const UCHAR data[])
{
	const UCHAR* curData = data;

	Animation animation;

	animation.loops = *(((const bool*&)curData)++);

	const size_t nodes = *(((const size_t*&)curData)++), frames = *(((const size_t*&)curData)++);
	animation.nodes.resize(nodes);
	animation.frames.resize(frames);

	for (std::string& nodeName : animation.nodes)
	{
		const size_t length = *(((const size_t*&)curData)++);
		nodeName = std::string((const char*)curData, (const char*)curData + length);
		curData += length;
	}

	size_t frameCount = 0;

	for (Frame& frame : animation.frames)
	{
		frame.duration = *(((const float*&)curData)++);
		if (frame.duration <= 0)
		{
			break;
		}

		animation.duration += frame.duration;

		frame.transforms.resize(nodes);

		for (Transform& transform : frame.transforms)
		{
			transform = *(((const Transform*&)curData)++);
		}

		frameCount++;
	}

	animation.frames.resize(frameCount);

	return animation;
}

void f4mp::Animator::Init()
{
	animations["idle"] = Load(animations::idle);
	animations["jog_forward"] = Load(animations::jogForward);
	animations["jog_forward_left"] = Load(animations::jogForwardLeft);
	animations["jog_forward_right"] = Load(animations::jogForwardRight);
	animations["jog_backward"] = Load(animations::jogBackward);
	animations["jog_backward_left"] = Load(animations::jogBackwardLeft);
	animations["jog_backward_right"] = Load(animations::jogBackwardRight);
	animations["jog_left"] = Load(animations::jogLeft);
	animations["jog_right"] = Load(animations::jogRight);

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
	if (id < 0 || id >= stateNames.size())
	{
		id = 0;
	}

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
