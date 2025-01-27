#include "ECS/CameraECS.h"
#include "ECS/TransformECS.h"
#include "Engine/GameObject.h"
#include "RHI/SceneView.h"

using namespace Sailor;
using namespace Sailor::Tasks;

Tasks::ITaskPtr CameraECS::Tick(float deltaTime)
{
	for (auto& data : m_components)
	{
		if (data.m_bIsActive)
		{
			// The origin
			const mat4 origin = glm::mat4(1);// glm::rotate(glm::mat4(1), glm::radians(90.0f), Math::vec3_Up);
			data.m_viewMatrix = origin * glm::inverse(data.m_owner.StaticCast<GameObject>()->GetTransformComponent().GetCachedWorldMatrix());
		}
	}

	return nullptr;
}

glm::mat4 CameraData::GetInvProjection() const
{
	return glm::inverse(m_projectionMatrix);
}

glm::mat4 CameraData::GetInvViewMatrix() const
{
	return glm::inverse(m_viewMatrix);
}

glm::mat4 CameraData::GetInvViewProjection() const
{
	// We do that separately to maximize the accuracy of Math
	return GetInvProjection() * GetInvViewMatrix();
}

void CameraECS::CopyCameraData(RHI::RHISceneViewPtr& outCameras)
{
	outCameras->m_cameras.Clear(false);
	for (auto& data : m_components)
	{
		if (data.m_bIsActive)
		{
			const Sailor::Math::Transform& transform = data.m_owner.StaticCast<GameObject>()->GetTransformComponent().GetTransform();

			outCameras->m_cameras.Add(data);
			outCameras->m_cameraTransforms.Add(transform);
		}
	}
}