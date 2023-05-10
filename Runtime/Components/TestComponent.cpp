#include "AssetRegistry/FrameGraph/FrameGraphImporter.h"
#include "AssetRegistry/Material/MaterialImporter.h"
#include "AssetRegistry/Texture/TextureImporter.h"
#include "Components/TestComponent.h"
#include "Components/MeshRendererComponent.h"
#include "Components/CameraComponent.h"
#include "Components/LightComponent.h"
#include "Engine/GameObject.h"
#include "Engine/EngineLoop.h"
#include "ECS/TransformECS.h"
#include "glm/glm/gtc/random.hpp"
#include "imgui.h"

using namespace Sailor;
using namespace Sailor::Tasks;

void TestComponent::BeginPlay()
{
	GetWorld()->GetDebugContext()->DrawOrigin(glm::vec4(600, 2, 0, 0), glm::mat4(1), 20.0f, 3000.0f);

	m_mainModel = GetWorld()->Instantiate();
	m_mainModel->GetTransformComponent().SetPosition(vec3(0, 0, 0));
	m_mainModel->GetTransformComponent().SetScale(vec4(1, 1, 1, 1));
	m_mainModel->GetTransformComponent().SetRotation(glm::quat(vec3(0, 0.25f * 3.14f, 0)));
	m_model = m_mainModel->AddComponent<MeshRendererComponent>()->GetModel();

	for (int32_t i = -1000; i < 1000; i += 32)
	{
		for (int32_t j = -1000; j < 1000; j += 32)
		{
			int k = 10;
			//for (int32_t k = -1000; k < 1000; k += 32) 
			{
				m_boxes.Add(Math::AABB(glm::vec3(i, k, j), glm::vec3(1.0f, 1.0f, 1.0f)));
				const auto& aabb = m_boxes[m_boxes.Num() - 1];

				//GetWorld()->GetDebugContext()->DrawAABB(aabb, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f), 5.0f);

				m_octree.Insert(aabb.GetCenter(), aabb.GetExtents(), aabb);
			}
		}
	}

	/*
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
		{
			auto gameObject2 = GetWorld()->Instantiate();
			gameObject2->GetTransformComponent().SetPosition(vec3(j * 300, 0, i * 250));
			gameObject2->GetTransformComponent().SetScale(vec4(10, 10, 10, 1));
			gameObject2->AddComponent<MeshRendererComponent>();
		}
	/**/

	m_dirLight = GetWorld()->Instantiate();
	auto lightComponent = m_dirLight->AddComponent<LightComponent>();
	m_dirLight->GetTransformComponent().SetPosition(vec3(0.0f, 3000.0f, 1000.0f));
	m_dirLight->GetTransformComponent().SetRotation(quat(vec3(-45, 12.5f, 0)));
	lightComponent->SetLightType(ELightType::Directional);

	auto spotLight = GetWorld()->Instantiate();
	lightComponent = spotLight->AddComponent<LightComponent>();
	spotLight->GetTransformComponent().SetPosition(vec3(200.0f, 40.0f, 0.0f));
	spotLight->GetTransformComponent().SetRotation(quat(vec3(-45, 0.0f, 0.0f)));

	lightComponent->SetBounds(vec3(300.0f, 300.0f, 300.0f));
	lightComponent->SetIntensity(vec3(260.0f, 260.0f, 200.0f));
	lightComponent->SetLightType(ELightType::Spot);
	/**/
	/*
	for (int32_t i = -1000; i < 1000; i += 250)
	{
		for (int32_t j = 0; j < 800; j += 250)
		{
			for (int32_t k = -1000; k < 1000; k += 250)
			{
				auto lightGameObject = GetWorld()->Instantiate();
				auto lightComponent = lightGameObject->AddComponent<LightComponent>();
				const float size = 100 + (float)(rand() % 60);

				//lightGameObject->SetMobilityType(EMobilityType::Static);
				lightGameObject->GetTransformComponent().SetPosition(vec3(i, j, k));
				lightComponent->SetBounds(vec3(size, size, size));
				lightComponent->SetIntensity(vec3(rand() % 256, rand() % 256, rand() % 256));

				m_lights.Emplace(std::move(lightGameObject));
				m_lightVelocities.Add(vec4(0));
			}
		}
	}
	/**/
	//m_octree.DrawOctree(*GetWorld()->GetDebugContext(), 10);

	auto& transform = GetOwner()->GetTransformComponent();
	transform.SetPosition(glm::vec4(0.0f, 3000.0f, 1000.0f, 0.0f));
	transform.SetRotation(quat(vec3(-45, 12.5f, 0)));
}

void TestComponent::EndPlay()
{
}

void TestComponent::Tick(float deltaTime)
{
	auto commands = App::GetSubmodule<Sailor::RHI::Renderer>()->GetDriverCommands();

	//ImGui::ShowDemoWindow();

	auto& transform = GetOwner()->GetTransformComponent();
	const vec3 cameraViewDirection = transform.GetRotation() * Math::vec4_Forward;

	const float sensitivity = 1500;

	glm::vec3 delta = glm::vec3(0.0f, 0.0f, 0.0f);
	if (GetWorld()->GetInput().IsKeyDown('A'))
		delta += -cross(cameraViewDirection, Math::vec3_Up);

	if (GetWorld()->GetInput().IsKeyDown('D'))
		delta += cross(cameraViewDirection, Math::vec3_Up);

	if (GetWorld()->GetInput().IsKeyDown('W'))
		delta += cameraViewDirection;

	if (GetWorld()->GetInput().IsKeyDown('S'))
		delta += -cameraViewDirection;

	if (GetWorld()->GetInput().IsKeyDown('X'))
		delta += vec3(1, 0, 0);

	if (GetWorld()->GetInput().IsKeyDown('Y'))
		delta += vec3(0, 1, 0);

	if (GetWorld()->GetInput().IsKeyDown('Z'))
		delta += vec3(0, 0, 1);

	if (glm::length(delta) > 0)
	{
		const float boost = 10.0f;
		vec4 shift = vec4(glm::normalize(delta) * sensitivity * deltaTime, 1.0f);

		//shift *= boost;

		const vec4 newPosition = transform.GetPosition() + shift;

		transform.SetPosition(newPosition);
	}

	if (GetWorld()->GetInput().IsKeyDown(VK_LBUTTON))
	{
		const float speed = 1.0f;
		const vec2 shift = vec2(GetWorld()->GetInput().GetCursorPos() - m_lastCursorPos) * speed;

		m_yaw += -shift.x;
		m_pitch = glm::clamp(m_pitch - shift.y, -85.0f, 85.0f);

		glm::quat hRotation = angleAxis(glm::radians(m_yaw), Math::vec3_Up);
		glm::quat vRotation = angleAxis(glm::radians(m_pitch), hRotation * Math::vec3_Right);

		transform.SetRotation(vRotation * hRotation);
	}

	if (GetWorld()->GetInput().IsKeyPressed('F'))
	{
		if (auto camera = GetOwner()->GetComponent<CameraComponent>())
		{
			m_cachedFrustum = transform.GetTransform().Matrix();

			Math::Frustum frustum;
			frustum.ExtractFrustumPlanes(transform.GetTransform().Matrix(), camera->GetAspect(), camera->GetFov(), camera->GetZNear(), camera->GetZFar());
			
			// Testing ortho matrix
			//const glm::mat4 m = glm::orthoRH_NO(-100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 900.0f) * glm::inverse(m_cachedFrustum);
			//frustum.ExtractFrustumPlanes(m);

			m_octree.Trace(frustum, m_culledBoxes);
		}
	}

	for (const auto& aabb : m_culledBoxes)
	{
		GetWorld()->GetDebugContext()->DrawAABB(aabb, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
	}

	if (auto camera = GetOwner()->GetComponent<CameraComponent>())
	{
		if (m_cachedFrustum != glm::mat4(1))
		{
			Math::Frustum frustum;
			frustum.ExtractFrustumPlanes(m_cachedFrustum, camera->GetAspect(), camera->GetFov(), camera->GetZNear(), camera->GetZFar());
			GetWorld()->GetDebugContext()->DrawFrustum(frustum);
			
			//const auto& lightView = glm::inverse(m_dirLight->GetTransformComponent().GetCachedWorldMatrix());
			//GetWorld()->GetDebugContext()->DrawLightCascades(lightView, m_cachedFrustum, camera->GetAspect(), camera->GetFov(), camera->GetZNear(), camera->GetZFar());

			//const glm::mat4 m = glm::orthoRH_NO(-100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 900.0f) * glm::inverse(m_cachedFrustum);
			//Math::Frustum frustum;
			//frustum.ExtractFrustumPlanes(m);
			//GetWorld()->GetDebugContext()->DrawFrustum(frustum);
			/**/
		}
	}

	m_lastCursorPos = GetWorld()->GetInput().GetCursorPos();

	for (uint32_t i = 0; i < m_lights.Num(); i++)
	{
		if (glm::length(m_lightVelocities[i]) < 1.0f)
		{
			const glm::vec4 velocity = glm::vec4(glm::sphericalRand(75.0f + rand() % 50), 0.0f);
			m_lightVelocities[i] = velocity;
		}

		const auto& position = m_lights[i]->GetTransformComponent().GetWorldPosition();

		m_lightVelocities[i] = Math::Lerp(m_lightVelocities[i], vec4(0, 0, 0, 0), deltaTime * 0.5f);
		m_lights[i]->GetTransformComponent().SetPosition(position + deltaTime * m_lightVelocities[i]);
	}

	if (GetWorld()->GetInput().IsKeyPressed('R'))
	{
		for (auto& go : GetWorld()->GetGameObjects())
		{
			if (auto mr = go->GetComponent<MeshRendererComponent>())
			{
				for (auto& mat : mr->GetMaterials())
				{
					if (mat && mat->IsReady() && mat->GetShaderBindings()->HasParameter("material.albedo"))
					{
						mat = Material::CreateInstance(GetWorld(), mat);

						const glm::vec4 color = glm::vec4(glm::ballRand(1.0f), 1);

						commands->SetMaterialParameter(GetWorld()->GetCommandList(),
							mat->GetShaderBindings(), "material.albedo", color);
					}
				}
			}
		}
	}

	if (auto node = App::GetSubmodule<RHI::Renderer>()->GetFrameGraph()->GetRHI()->GetGraphNode("Sky"))
	{
		auto sky = node.DynamicCast<Framegraph::SkyNode>();

		SkyNode::SkyParams& skyParams = sky->GetSkyParams();

		ImGui::Begin("Sky Settings");
		ImGui::SliderAngle("Sun angle", &m_sunAngleRad, -25.0f, 89.0f, "%.2f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds density", &skyParams.m_cloudsDensity, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_::ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds coverage", &skyParams.m_cloudsCoverage, 0.0f, 2.0f, "%.2f", ImGuiSliderFlags_::ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds attenuation 1", &skyParams.m_cloudsAttenuation1, 0.1f, 0.3f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds attenuation 2", &skyParams.m_cloudsAttenuation2, 0.001f, 0.1f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds phase influence 1", &skyParams.m_phaseInfluence1, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds phase eccentrisy 1", &skyParams.m_eccentrisy1, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds phase influence 2", &skyParams.m_phaseInfluence2, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds phase eccentrisy 2", &skyParams.m_eccentrisy2, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds horizon blend", &skyParams.m_fog, 0.0f, 20.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds sun intensity", &skyParams.m_sunIntensity, 0.0f, 800.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds ambient", &skyParams.m_ambient, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderInt("Clouds scattering steps", &skyParams.m_scatteringSteps, 1, 10, "%d");
		ImGui::SliderFloat("Clouds scattering density", &skyParams.m_scatteringDensity, 0.1f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds scattering intensity", &skyParams.m_scatteringIntensity, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderFloat("Clouds scattering phase", &skyParams.m_scatteringPhase, 0.001f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::SliderInt("Sun Shafts Distance", &skyParams.m_sunShaftsDistance, 1, 100, "%d");
		ImGui::SliderFloat("Sun Shafts Intensity", &skyParams.m_sunShaftsIntensity, 0.001f, 1.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_NoRoundToFormat);
		ImGui::End();

		if (m_skyHash != skyParams.GetHash())
		{
			sky->MarkDirty();
			m_skyHash = skyParams.GetHash();
		}

		glm::vec4 lightPosition = (-skyParams.m_lightDirection) * 9000.0f;

		skyParams.m_lightDirection = normalize(vec4(0.2f, std::sin(-m_sunAngleRad), std::cos(m_sunAngleRad), 0));
		m_dirLight->GetTransformComponent().SetRotation(glm::quatLookAt(skyParams.m_lightDirection.xyz(), Math::vec3_Up));
		m_dirLight->GetTransformComponent().SetPosition(lightPosition);
		m_dirLight->GetComponent<LightComponent>()->SetIntensity(m_sunAngleRad > 0 ? vec3(7.0f, 7.0f, 7.0f) : vec3(0));
	}
}
