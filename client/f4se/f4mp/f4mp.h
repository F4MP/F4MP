#pragma once

#include "Networking.h"

#include "f4se/PluginAPI.h"

namespace f4mp
{
	using FormID = UInt32;

	class F4MP
	{
	public:
		F4MP(const F4SEInterface* f4se);
		virtual ~F4MP();

	private:
		networking::Networking* networking;

		PluginHandle pluginHandle;
	};
}