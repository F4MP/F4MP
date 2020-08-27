#pragma once

#include "client.h"

namespace f4mp
{
	class Player;

	class Animator
	{
	public:
		enum Type
		{
			Human
		};

		struct Transform
		{
			zpl_vec3 position;
			zpl_quat rotation;
			float scale;
		};

		struct Frame
		{
			float duration;
			std::vector<Transform> transforms;
		};

		struct Animation
		{
			bool loops;
			float duration;
			std::vector<std::string> nodes;
			std::vector<Frame> frames;

			Animation(float duration = 0.f);
		};

	private:
		static std::vector<std::vector<std::string>> animatedNodes;
		static std::vector<std::unordered_map<std::string, UInt32>> animatedNodeIndices;
		
		static std::vector<std::string> stateNames;
		static std::unordered_map<std::string, SInt32> stateIDs;

		static std::unordered_map<std::string, Animation> animations;

		Type type;

		const Animation* animation;
		double startTime;

	public:
		Animator(Type type);

		void Play(const Animation* newAnimation);

		size_t GetAnimatedNodeCount() const;

		bool IsAnimatedNode(const std::string& nodeName) const;

		const std::string& GetNodeName(UInt32 nodeIndex) const;
		UInt32 GetNodeIndex(const std::string& nodeName) const;

		const Animation* GetAnimation() const;

		std::vector<Transform> GetTransforms() const;

		bool ForEachNode(TESObjectREFR* ref, const std::function<bool(NiNode*, UInt32)>& callback);

		bool Save(const std::string& path, const Animation& animation, bool binary = true) const;
		Animation Load(const std::string& path) const;

		void OnClientUpdate(const Player& player);

		static void Init();

		static Animation Load(const UCHAR data[]);

		static const std::string& GetStateName(SInt32 id);
		static SInt32 GetStateID(const std::string& name);

		static bool Loops(SInt32 id);
	};
}