#pragma once

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PluginAPI.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/GameThreads.h"
#include "f4se/GameObjects.h"
#include "f4se/GameForms.h"
#include "f4se/GameFormComponents.h"
#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameRTTI.h"

#include "common.h"

#include <unordered_map>

namespace f4mp
{
	template<class T>
	T ToDegrees(const T& radians)
	{
		return radians * 360.0f / ZPL_TAU;
	}

	namespace client
	{
		struct AppearanceData : public f4mp::AppearanceData
		{
			void Fill(TESNPC* actorBase)
			{
				Clear();

				female = CALL_MEMBER_FN(actorBase, GetSex)() == 1;
				weights = { actorBase->weightThin, actorBase->weightMuscular, actorBase->weightLarge };
				hairColor = actorBase->headData->hairColor->fullName.name;

				for (UInt8 i = 0; i < actorBase->numHeadParts; i++)
				{
					headParts.push_back(actorBase->headParts[i]->partName.c_str());
				}

				for (UInt32 i = 0; i < actorBase->morphSetValue->count; i++)
				{
					morphSetValue.push_back((*actorBase->morphSetValue)[i]);
				}

				actorBase->morphRegionData->ForEach([=](TESNPC::FaceMorphRegion* region)
					{
						//morphRegionData.push_back(std::make_tuple(region->index, std::vector<f32>(&region->value[0], &region->value[8])));
						morphRegionData1.push_back(region->index);
						morphRegionData2.push_back(std::vector<f32>(&region->value[0], &region->value[8]));
						return true;
					});

				actorBase->morphSetData->ForEach([&](TESNPC::MorphSetData* data)
					{
						//morphSetData.push_back(std::make_tuple(data->key, data->value));
						morphSetData1.push_back(data->key);
						morphSetData2.push_back(data->value);
						return true;
					});
			}
		};

		struct WornItemsData : public f4mp::WornItemsData
		{
			void Fill(Actor* actor)
			{
				Clear();

				for (UInt32 i = 0; i < ActorEquipData::kMaxSlots; i++)
				{
					TESForm* item = actor->equipData->slots[i].item;
					if (item)
					{
						/*if (std::find_if(data.begin(), data.end(), [&](const std::tuple<u8, std::string>& wornItem)
							{
								return std::get<0>(wornItem) == item->formType && std::get<1>(wornItem).compare(item->GetFullName()) == 0;
							}) == data.end())
						{
							data.push_back(std::make_pair(item->formType, item->GetFullName()));
						}*/

						size_t j;
						for (j = 0; j < data1.size(); j++)
						{
							if (data1[j] == item->formType && data2[j].compare(item->GetFullName()) == 0)
							{
								break;
							}
						}

						if (j == data1.size())
						{
							data1.push_back(item->formType);
							data2.push_back(item->GetFullName());
						}
					}
				}
			}
		};
	}

	class Task : public ITaskDelegate
	{
	public:
		Task(const std::function<void()>& callback) : callback(callback)
		{

		}

		void Run() override
		{
			callback();
		}

		std::function<void()> callback;
	};

	struct Config
	{
		std::string hostAddress;
	};
}