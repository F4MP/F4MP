#pragma once

#include "Entity.h"
#include "Animation.h"

#include <memory>
#include <atomic>

namespace f4mp
{
	class Character : public Entity
	{
	public:
		struct Transform
		{
			zpl_vec3 position;
			zpl_quat rotation;
			float scale;
		};

		struct TransformBuffer
		{
			std::vector<Transform> prev, next;

			double syncTime, time;
			float deltaTime;

			TransformBuffer();
			TransformBuffer(size_t transforms, double syncTime, double time, float deltaTime);
		};

		Character();

		Animation& GetAnimation() const;

		void OnEntityUpdate(librg_event* event) override;

		void OnClientUpdate(librg_event* event) override;

		void OnTick() override;

	private:
		std::unique_ptr<Animation> animation;
		
		std::atomic_flag lock = ATOMIC_FLAG_INIT;
		TransformBuffer transformBuffer;
	};
}