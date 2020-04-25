#pragma once

#include "Entity.h"
#include "Animation.h"

#include <memory>

namespace f4mp
{
	class Character : public Entity
	{
	public:
		Character();

		void OnEntityUpdate(librg_event* event) override;

		void OnClientUpdate(librg_event* event) override;

		void OnTick() override;

	private:
		std::unique_ptr<Animation> animation;

		std::vector<NiTransform> prevTransforms, curTransforms, nextTransforms;

		float prevTransformTime, curTransformTime, transformDeltaTime;
	};
}