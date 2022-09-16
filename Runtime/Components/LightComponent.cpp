#include "Components/LightComponent.h"
#include "Engine/GameObject.h"
#include "ECS/TransformECS.h"
#include "ECS/LightingECS.h"
#include "Math/Math.h"

using namespace Sailor;
using namespace Sailor::Tasks;

void LightComponent::BeginPlay()
{
	auto ecs = GetOwner()->GetWorld()->GetECS<LightingECS>();
	m_handle = ecs->RegisterComponent();

	auto& ecsData = GetData();

	ecsData.SetOwner(GetOwner());
}

LightData& LightComponent::GetData()
{
	auto ecs = GetOwner()->GetWorld()->GetECS<LightingECS>();
	return ecs->GetComponentData(m_handle);
}

const LightData& LightComponent::GetData() const
{
	auto ecs = GetOwner()->GetWorld()->GetECS<LightingECS>();
	return ecs->GetComponentData(m_handle);
}

void LightComponent::EndPlay()
{
	GetOwner()->GetWorld()->GetECS<LightingECS>()->UnregisterComponent(m_handle);
}

void LightComponent::OnGizmo()
{
	const glm::vec4 worldPosition = GetOwner()->GetTransformComponent().GetWorldPosition();
	GetOwner()->GetWorld()->GetDebugContext()->DrawOrigin(worldPosition, 30.0f);
}

void LightComponent::SetIntensity(const glm::vec3& value)
{
	LightData& lightData = GetData();

	lightData.m_intensity = value;
	lightData.SetDirty();
}

void LightComponent::SetAttenuation(const glm::vec3& value)
{
	LightData& lightData = GetData();

	lightData.m_attenuation = value;
	lightData.SetDirty();
}

void LightComponent::SetBounds(const glm::vec3& value)
{
	LightData& lightData = GetData();

	lightData.m_bounds = value;
	lightData.SetDirty();
}

void LightComponent::SetLightType(ELightType value)
{
	LightData& lightData = GetData();

	lightData.m_type = value;
	lightData.SetDirty();
}