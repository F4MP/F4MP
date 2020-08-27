#pragma once

#include "Entity.h"

namespace f4mp
{
	class Character : public Entity
	{
	public:
		Character();

		void OnEntityUpdate(librg_event* event) override;

		void OnClientUpdate(librg_event* event) override;

	private:
		std::vector<float> nodeTransforms;

		double transformTime;
	};
}