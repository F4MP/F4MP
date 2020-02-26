#pragma once

#include "librg.h"

#include <vector>
#include <string>
#include <tuple>

namespace f4mp
{
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

			librg_data_wu32(data, values.size());

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

		template<>
		static void Write(librg_data* data, const bool& value)
		{
			//_MESSAGE("%s", value ? "true" : "false");

			librg_data_wb8(data, value);
		}

		template<>
		static void Write(librg_data* data, const u8& value)
		{
			//_MESSAGE("%u", value);

			librg_data_wu8(data, value);
		}

		template<>
		static void Write(librg_data* data, const u32& value)
		{
			//_MESSAGE("%u", value);

			librg_data_wu32(data, value);
		}

		template<>
		static void Write(librg_data* data, const f32& value)
		{
			//_MESSAGE("%f", value);

			librg_data_wf32(data, value);
		}

		template<>
		static void Write(librg_data* data, const std::string& value)
		{
			//_MESSAGE("%s", value.c_str());

			librg_data_wu32(data, value.size());

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

		template<>
		static void Read(librg_data* data, bool& value)
		{
			value = !!librg_data_rb8(data);
		}

		template<>
		static void Read(librg_data* data, u8& value)
		{
			value = librg_data_ru8(data);
		}

		template<>
		static void Read(librg_data* data, u32& value)
		{
			value = librg_data_ru32(data);
		}

		template<>
		static void Read(librg_data* data, f32& value)
		{
			value = librg_data_rf32(data);
		}

		template<>
		static void Read(librg_data* data, std::string& value)
		{
			value.resize(librg_data_ru32(data));

			for (char& ch : value)
			{
				ch = librg_data_rb8(data);
			}
		}
	};
}