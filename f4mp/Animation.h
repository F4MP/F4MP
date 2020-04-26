#pragma once

#include "client.h"

namespace f4mp
{
	class Animation
	{
	public:
		enum Type
		{
			Human
		};

	private:
		static std::vector<std::vector<std::string>> animatedNodes;
		static std::vector<std::unordered_map<std::string, UInt32>> animatedNodeIndices;
		
		static std::vector<std::string> stateNames;
		static std::unordered_map<std::string, SInt32> stateIDs;

		Type type;

	public:
		Animation(Type type);

		size_t GetAllowedNodeCount() const;

		bool IsAnimatedNode(const std::string& nodeName) const;

		const std::string& GetNodeName(UInt32 nodeIndex) const;
		UInt32 GetNodeIndex(const std::string& nodeName) const;

		static void Init();

		static const std::string& GetStateName(SInt32 id);
		static SInt32 GetStateID(const std::string& name);

		static bool Loops(SInt32 id);
	};
}