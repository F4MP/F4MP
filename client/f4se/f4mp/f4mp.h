#pragma once

#include "Networking.h"

namespace f4mp
{
	using FormID = UInt32;

	class F4MP
	{
	public:
		F4MP();
		virtual ~F4MP();

	private:
		networking::Networking* networking;
	};
}