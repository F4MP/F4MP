#pragma once

#include <librg.h>

#include <vector>
#include <string>
#include <tuple>

namespace f4mp
{
	struct MessageType
	{
		enum : u16
		{
			Hit = LIBRG_EVENT_LAST + 1u,
			FireWeapon,
			SpawnEntity,
			SyncEntity,
			SpawnBuilding,
			RemoveBuilding,
			Speak
		};
	};

	struct EntityType
	{
		enum : u32
		{
			Player = 0,
			NPC
		};
	};

	struct HitData
	{
		u32 hitter, hittee;
		f32 damage;
	};

	struct SpawnEntityData
	{
		u32 formID;
		zpl_vec3 position;
		zpl_vec3 angles;

		u32 entityID;
		u32 ownerEntityID;
	};

	struct SyncEntityData
	{
		u32 formID;
		zpl_vec3 position;
		zpl_vec3 angles;

		f64 syncedTime;
	};

	struct SpawnBuildingData
	{
		u32 ownerEntityID;
		u32 formID;
		u32 baseFormID; // HACK: 0 if it's just a transform update
		zpl_vec3 position;
		zpl_vec3 angles;
	};

	struct RemoveBuildingData
	{
		u64 uniqueFormID;
	};

	struct TransformData
	{
		u32 formID;
		zpl_vec3 position;
		zpl_vec3 angles;
	};

	struct SpeakData
	{
		u32 clientEntityID;
		u32 speakerFormID; // 0 if client
		u32 topicInfoFormID;
	};

	struct AppearanceData
	{
		bool female;
		std::vector<f32> weights;
		std::string hairColor;
		std::vector<std::string> headParts;
		std::vector<f32> morphSetValue;
		std::vector<u32> morphRegionData1;
		std::vector<std::vector<f32>> morphRegionData2;
		std::vector<u32> morphSetData1;
		std::vector<f32> morphSetData2;

		void Clear()
		{
			headParts.clear();
			morphSetValue.clear();
			morphRegionData1.clear();
			morphRegionData2.clear();
			morphSetData1.clear();
			morphSetData2.clear();
		}
	};

	struct WornItemsData
	{
		std::vector<u8> data1;
		std::vector<std::string> data2;

		void Clear()
		{
			//data.clear();
			data1.clear();
			data2.clear();
		}
	};

	// TODO: the ones for tuple seem to be broken

	class Utils
	{
	private:
		template<class T>
		static void WriteForTuple(librg_data* data, const T& value)
		{
			Write(data, value);
		}

		template<class T, class... Ts>
		static void WriteForTuple(librg_data* data, const T& value, Ts... values)
		{
			Write(data, value);
			WriteForTuple(data, values...);
		}

		template<class T, size_t... I>
		static void WriteTuple(librg_data* data, const T& tuple, std::index_sequence<I...>)
		{
			WriteForTuple(data, std::get<I>(tuple)...);
		}

		template<class T>
		static void ReadForTuple(librg_data* data, T& value)
		{
			Read(data, value);
		}

		template<class T, class... Ts>
		static void ReadForTuple(librg_data* data, T& value, Ts... values)
		{
			Read(data, value);
			ReadForTuple(data, values...);
		}

		template<class T, size_t... I>
		static void ReadTuple(librg_data* data, T& tuple, std::index_sequence<I...>)
		{
			ReadForTuple(data, std::get<I>(tuple)...);
		}

	public:
		template<class T>
		static void Write(librg_data* data, const T& value);

		template<class T>
		static void Write(librg_data* data, const std::vector<T>& values)
		{
			//_MESSAGE("size: %u", values.size());

			librg_data_wu32(data, static_cast<u32>(values.size()));

			for (const T& value : values)
			{
				Write(data, value);
			}
		}

		template<class... Ts>
		static void Write(librg_data* data, const std::tuple<Ts...>& values)
		{
			static constexpr auto size = std::tuple_size<std::tuple<Ts...>>::value;
			WriteTuple(data, values, std::make_index_sequence<size>{});
		}

		static void Write(librg_data* data, const bool& value)
		{
			//_MESSAGE("%s", value ? "true" : "false");

			librg_data_wb8(data, value);
		}

		static void Write(librg_data* data, const u8& value)
		{
			//_MESSAGE("%u", value);

			librg_data_wu8(data, value);
		}

		static void Write(librg_data* data, const u32& value)
		{
			//_MESSAGE("%u", value);

			librg_data_wu32(data, value);
		}

		static void Write(librg_data* data, const f32& value)
		{
			//_MESSAGE("%f", value);

			librg_data_wf32(data, value);
		}

		static void Write(librg_data* data, const f64& value)
		{
			librg_data_wf64(data, value);
		}

		static void Write(librg_data* data, const std::string& value)
		{
			//_MESSAGE("%s", value.c_str());

			librg_data_wu32(data, static_cast<u32>(value.size()));

			for (char ch : value)
			{
				librg_data_wb8(data, ch);
			}
		}

		template<class T>
		static void Read(librg_data* data, T& value);

		template<class T>
		static void Read(librg_data* data, std::vector<T>& values)
		{
			values.resize(librg_data_ru32(data));

			for (T& value : values)
			{
				Read(data, value);
			}
		}

		template<class... Ts>
		static void Read(librg_data* data, std::tuple<Ts...>& values)
		{
			static constexpr auto size = std::tuple_size<std::tuple<Ts...>>::value;
			ReadTuple(data, values, std::make_index_sequence<size>{});
		}

		static void Read(librg_data* data, bool& value)
		{
			value = !!librg_data_rb8(data);
		}

		static void Read(librg_data* data, u8& value)
		{
			value = librg_data_ru8(data);
		}

		static void Read(librg_data* data, u32& value)
		{
			value = librg_data_ru32(data);
		}

		static void Read(librg_data* data, f32& value)
		{
			value = librg_data_rf32(data);
		}

		static void Read(librg_data* data, f64& value)
		{
			value = librg_data_rf64(data);
		}

		static void Read(librg_data* data, std::string& value)
		{
			value.resize(librg_data_ru32(data));

			for (char& ch : value)
			{
				ch = librg_data_rb8(data);
			}
		}
	};

	template<class T>
	std::string Lower(const T& string)
	{
		std::string lower = string;

		for (size_t i = 0; i < lower.length(); i++)
		{
			lower[i] = tolower(lower[i]);
		}

		return lower;
	}

	u64 GetUniqueFormID(u32 ownerEntityID, u32 entityFormID);
}