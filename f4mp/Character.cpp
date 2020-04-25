#include "Character.h"
#include "f4mp.h"

#include "f4se/NiNodes.h"

f4mp::Character::Character() : prevTransformTime(-1.f), curTransformTime(-1.f), transformDeltaTime(0.f)
{
	animation = std::make_unique<Animation>(Animation::Human);

}

void f4mp::Character::OnEntityUpdate(librg_event* event)
{
	Entity::OnEntityUpdate(event, F4MP::GetInstance().player.get() != this);

	std::vector<float> transforms;

	Utils::Read(event->data, transforms);
	Utils::Read(event->data, transformDeltaTime);

	//prevTransforms.swap(nextTransforms);
	prevTransforms = curTransforms;

	nextTransforms.resize(transforms.size() / 7);

	for (size_t i = 0; i < nextTransforms.size(); i++)
	{
		size_t ti = i * 7;

		NiTransform& transform = nextTransforms[i];
		transform.pos = NiPoint3(transforms[ti], transforms[ti + 1], transforms[ti + 2]);
		transform.rot.SetEulerAngles(transforms[ti + 3], transforms[ti + 4], transforms[ti + 5]);
		transform.scale = transforms[ti + 6];
	}

	if (prevTransforms.size() == 0 || curTransforms.size() == 0)
	{
		prevTransforms = curTransforms = nextTransforms;
	}

	if (curTransformTime < 0.f)
	{
		curTransformTime = zpl_time_now();
	}

	prevTransformTime = curTransformTime;
	curTransformTime = zpl_time_now();

}

void f4mp::Character::OnClientUpdate(librg_event* event)
{
	Entity::OnClientUpdate(event);

	std::vector<float> transforms(animation->GetAllowedNodeCount() * 7);

	TESObjectREFR* ref = GetRef();
	if (ref)
	{
		NiNode* root = ref->GetActorRootNode(false);
		if (root)
		{
			root->Visit([&](NiAVObject* obj)
				{
					NiNode* node = dynamic_cast<NiNode*>(obj);
					if (!node)
					{
						return false;
					}

					UInt32 index = animation->GetNodeIndex(node->m_name.c_str());
					if (index >= animation->GetAllowedNodeCount())
					{
						return false;
					}

					index *= 7;

					float rot[3];
					node->m_localTransform.rot.GetEulerAngles(&rot[0], &rot[1], &rot[2]);

					transforms[index + 0] = node->m_localTransform.pos.x;
					transforms[index + 1] = node->m_localTransform.pos.y;
					transforms[index + 2] = node->m_localTransform.pos.z;
					transforms[index + 3] = rot[0];
					transforms[index + 4] = rot[1];
					transforms[index + 5] = rot[2];
					transforms[index + 6] = node->m_localTransform.scale;

					return false;
				});
		}
	}

	if (curTransformTime < 0.f)
	{
		curTransformTime = zpl_time_now();
	}

	prevTransformTime = curTransformTime;
	curTransformTime = zpl_time_now();

	Utils::Write(event->data, transforms);
	Utils::Write(event->data, curTransformTime - prevTransformTime);
}

void f4mp::Character::OnTick()
{
	F4MP& f4mp = F4MP::GetInstance();

	if (this == f4mp.player.get())
	{
		return;
	}

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

			root->Visit([=](NiAVObject* obj)
				{
					NiNode* node = dynamic_cast<NiNode*>(obj);
					if (!node)
					{
						return false;
					}

					UInt32 nodeIndex = animation->GetNodeIndex(node->m_name.c_str());
					if (nodeIndex >= prevTransforms.size())
					{
						return false;
					}

					NiTransform& prevTransform = prevTransforms[nodeIndex];
					NiTransform& curTransform = curTransforms[nodeIndex];
					NiTransform& nextTransform = nextTransforms[nodeIndex];

					//printf("%f %f\n", zpl_time_now() - prevTransformTime, (zpl_time_now() - prevTransformTime) / transformDeltaTime);

					float t = min((zpl_time_now() - curTransformTime) / transformDeltaTime, 1.f);

					for (int i = 0; i < 12; i++)
					{
						curTransform.rot.arr[i] = prevTransform.rot.arr[i] * (1.f - t) + nextTransform.rot.arr[i] * t;
					}

					curTransform.pos = prevTransform.pos * (1.f - t) + nextTransform.pos * t;
					curTransform.scale = prevTransform.scale * (1.f - t) + nextTransform.scale * t;

					// HACK: find out why it stutters when syncing the position with this. maybe the preicison? no way..
					if (node == root)
					{
						curTransform.pos = node->m_localTransform.pos;
					}

					node->m_localTransform = curTransform;

					//node->m_localTransform = nextFound->second;

					return false;
				});

			//root->UpdateTransforms();

			NiAVObject::NiUpdateData updateData;
			root->UpdateWorldData(&updateData);
		}));
}
