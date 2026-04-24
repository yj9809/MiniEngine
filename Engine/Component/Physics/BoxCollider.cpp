#include "BoxCollider.h"

#include "Actor/Actor.h"

namespace Engine
{
	BoxCollider::BoxCollider(const Vector3& center, const Vector3& halfSize)
		: super(center, false), halfSize(halfSize)
	{
		Init();
	}

	BoxCollider::~BoxCollider()
	{
	}
	
	void BoxCollider::Tick(float deltaTime)
	{
		Vector3 actorPos = GetOwner().GetPosition();

		worldMin = actorPos + center - halfSize;
		worldMax = actorPos + center + halfSize;

		Init();
	}

	bool BoxCollider::Overlap(const Collider& other) const
	{
		Vector3 min = other.GetWorldMin();
		Vector3 max = other.GetWorldMax();

		return worldMin.x <= max.x && worldMax.x >= min.x
			&& worldMin.y <= max.y && worldMax.y >= min.y
			&& worldMin.z <= max.z && worldMax.z >= min.z;
	}

	void BoxCollider::Init()
	{
		// x = 앞, y = 옆, z = 높이
		BoxSize = {
			{center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z},
			{center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z},
			{center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z},
			{center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z},
			{center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z},
			{center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
			{center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z},
			{center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
		};
	}
}