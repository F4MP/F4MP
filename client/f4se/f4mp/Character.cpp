#include "Character.h"
#include "f4mp.h"

#include "f4se/NiNodes.h"

f4mp::Character::Character()
{
	animator = std::make_unique<Animator>(Animator::Human);

}

f4mp::Animator& f4mp::Character::GetAnimator()
{
	return *animator;
}

const f4mp::Animator& f4mp::Character::GetAnimator() const
{
	return *animator;
}

void f4mp::Character::OnEntityUpdate(librg_event* event)
{
	Entity::OnEntityUpdate(event, F4MP::GetInstance().player.get() != this);

	std::vector<float> transforms;
	double syncTime;

	Utils::Read(event->data, transforms);
	Utils::Read(event->data, syncTime);

	if (transforms.size() == 0)
	{
		return;
	}

	float deltaTime = syncTime - transformBuffer.syncTime;

	if (deltaTime < 1e-2f)
	{
		//transformBuffer.syncTime = syncTime;
		return;
	}

	TransformBuffer newTransformBuffer(transforms.size() / 8, syncTime, zpl_time_now(), deltaTime);

	for (size_t i = 0; i < newTransformBuffer.next.size(); i++)
	{
		size_t ti = i * 8;

		Transform& transform = newTransformBuffer.next[i];
		transform.position = { transforms[ti], transforms[ti + 1], transforms[ti + 2] };
		transform.rotation = { transforms[ti + 3], transforms[ti + 4], transforms[ti + 5], transforms[ti + 6] };
		transform.scale = transforms[ti + 7];

		float mag = zpl_quat_mag(transform.rotation);
		if (isnan(mag) || fabsf(mag - 1.f) > 1e-2f)
		{
			transform.rotation = { 0, 0, 0, 0 };
		}
	}

	if (transformBuffer.prev.size() == 0)
	{
		newTransformBuffer.prev = newTransformBuffer.next;
	}

	float t = min((newTransformBuffer.time - transformBuffer.time) / transformBuffer.deltaTime, 1.f);

	for (size_t nodeIndex = 0; nodeIndex < transformBuffer.prev.size(); nodeIndex++)
	{
		const Transform& prevTransform = transformBuffer.prev[nodeIndex];
		const Transform& nextTransform = transformBuffer.next[nodeIndex];
		Transform& curTransform = newTransformBuffer.prev[nodeIndex];

		if (zpl_quat_dot(newTransformBuffer.next[nodeIndex].rotation, nextTransform.rotation) < 0.f)
		{
			newTransformBuffer.next[nodeIndex].rotation *= -1.f;
		}

		zpl_vec3_lerp(&curTransform.position, prevTransform.position, nextTransform.position, t);
		zpl_quat_slerp_approx(&curTransform.rotation, prevTransform.rotation, nextTransform.rotation, t);
		curTransform.scale = zpl_lerp(prevTransform.scale, nextTransform.scale, t);
	}

	while (lock.test_and_set(std::memory_order_acquire));
	transformBuffer = newTransformBuffer;
	lock.clear(std::memory_order_release);
}

void f4mp::Character::OnClientUpdate(librg_event* event)
{
	Entity::OnClientUpdate(event);

	std::vector<float> transforms;

	TESObjectREFR* ref = GetRef();
	if (ref)
	{
		NiNode* root = ref->GetActorRootNode(false);
		if (root)
		{
			transforms.resize(animator->GetAnimatedNodeCount() * 8);

			root->Visit([&](NiAVObject* obj)
				{
					NiNode* node = dynamic_cast<NiNode*>(obj);
					if (!node)
					{
						return false;
					}

					UInt32 nodeIndex = animator->GetNodeIndex(node->m_name.c_str());
					if (nodeIndex >= animator->GetAnimatedNodeCount())
					{
						return false;
					}

					UInt32 index = nodeIndex * 8;

					const NiMatrix43 rot = node->m_localTransform.rot;
					zpl_mat4 mat
					{
						rot.data[0][0], rot.data[0][1], rot.data[0][2], rot.data[0][3],
						rot.data[1][0], rot.data[1][1], rot.data[1][2], rot.data[1][3],
						rot.data[2][0], rot.data[2][1], rot.data[2][2], rot.data[2][3],
					};
					zpl_quat quat;
					zpl_quat_from_mat4(&quat, &mat);

					transforms[index + 0] = node->m_localTransform.pos.x;
					transforms[index + 1] = node->m_localTransform.pos.y;
					transforms[index + 2] = node->m_localTransform.pos.z;
					transforms[index + 3] = quat.x;
					transforms[index + 4] = quat.y;
					transforms[index + 5] = quat.z;
					transforms[index + 6] = quat.w;
					transforms[index + 7] = node->m_localTransform.scale;

					return false;
				});

			if (ref->GetObjectRootNode() != root)
			{
				// TODO: temporary
				const Animator::Animation* animation = animator->GetAnimation();
				if (animation)
				{
					std::vector<Animator::Transform> animationTransforms = animator->GetTransforms();

					for (size_t i = 0; i < animationTransforms.size(); i++)
					{
						const Animator::Transform& transform = animationTransforms[i];
						size_t index = animator->GetNodeIndex(animation->nodes[i]) * 8;
						transforms[index + 0] = transform.position.x;
						transforms[index + 1] = transform.position.y;
						transforms[index + 2] = transform.position.z;
						transforms[index + 3] = transform.rotation.x;
						transforms[index + 4] = transform.rotation.y;
						transforms[index + 5] = transform.rotation.z;
						transforms[index + 6] = transform.rotation.w;
						transforms[index + 7] = transform.scale;
					}
				}
			}
		}
	}

	transformBuffer.syncTime = zpl_time_now();
	
	Utils::Write(event->data, transforms);
	Utils::Write(event->data, transformBuffer.syncTime);
}

void f4mp::Character::OnTick()
{
	F4MP& f4mp = F4MP::GetInstance();

	if (this == f4mp.player.get())
	{
		return;
	}

	while (lock.test_and_set(std::memory_order_acquire));
	TransformBuffer transforms = transformBuffer;
	lock.clear(std::memory_order_release);

	f4mp.task->AddTask(new Task([=]()
		{
			TESObjectREFR* ref = GetRef();
			if (!ref)
			{
				return;
			}

			NiNode* root = ref->GetActorRootNode(false);
			if (!root)
			{
				return;
			}

			float t = min((zpl_time_now() - transforms.time) / transforms.deltaTime, 1.f);

			root->Visit([&](NiAVObject* obj)
				{
					NiNode* node = dynamic_cast<NiNode*>(obj);
					if (!node)
					{
						return false;
					}

					UInt32 nodeIndex = animator->GetNodeIndex(node->m_name.c_str());
					if (nodeIndex >= transforms.prev.size())
					{
						return false;
					}

					const Transform& prevTransform = transforms.prev[nodeIndex];
					const Transform& nextTransform = transforms.next[nodeIndex];
					Transform curTransform;

					zpl_vec3_lerp(&curTransform.position, prevTransform.position, nextTransform.position, t);
					zpl_quat_slerp_approx(&curTransform.rotation, prevTransform.rotation, nextTransform.rotation, t);
					curTransform.scale = zpl_lerp(prevTransform.scale, nextTransform.scale, t);

					node->m_localTransform.pos = (NiPoint3&)curTransform.position;
					node->m_localTransform.scale = curTransform.scale;

					if (fabsf(zpl_quat_mag(nextTransform.rotation) - 1.f) > 1e-2f || fabsf(zpl_quat_mag(prevTransform.rotation) - 1.f) > 1e-2f)
					{
						return false;
					}

					zpl_mat4 rot;
					zpl_mat4_from_quat(&rot, curTransform.rotation);
					zpl_float4* m = zpl_float44_m(&rot);

					node->m_localTransform.rot =
					{
						m[0][0], m[0][1], m[0][2], m[0][3],
						m[1][0], m[1][1], m[1][2], m[1][3],
						m[2][0], m[2][1], m[2][2], m[2][3],
					};

					return false;
				});

			NiAVObject::NiUpdateData updateData;
			root->UpdateWorldData(&updateData);
		}));
}

f4mp::Character::TransformBuffer::TransformBuffer() : TransformBuffer(0, -1.0, -1.0, 0.f)
{
}

f4mp::Character::TransformBuffer::TransformBuffer(size_t transforms, double syncTime, double time, float deltaTime) : prev(transforms), next(transforms), syncTime(syncTime), time(time), deltaTime(deltaTime)
{
}
