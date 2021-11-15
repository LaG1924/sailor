#pragma once
#include "Core/RefPtr.hpp"
#include "Renderer.h"
#include "Types.h"
#include "GfxDevice/Vulkan/VulkanApi.h"
#include "GfxDevice/Vulkan/VulkanBufferMemory.h"

namespace Sailor::RHI
{
	class Material : public Resource
	{
	public:
#if defined(VULKAN)
		struct
		{
			Sailor::GfxDevice::Vulkan::VulkanPipelinePtr m_pipeline;
			Sailor::GfxDevice::Vulkan::VulkanDescriptorSetPtr m_descriptorSet;
		} m_vulkan;
#endif
		Material(RenderState renderState, ShaderPtr vertexShader, ShaderPtr fragmentShader) :
			m_renderState(std::move(renderState)),
			m_vertexShader(vertexShader),
			m_fragmentShader(fragmentShader)
		{}

		void SetLayoutShaderBindings(std::vector<RHI::ShaderLayoutBinding> layoutBindings);

		const RHI::RenderState& GetRenderState() const { return m_renderState; }
		const std::vector<RHI::ShaderLayoutBinding>& GetLayoutBindings() const { return m_layoutBindings; }

		RHI::ShaderBindingPtr& GetOrCreateShaderBinding(const std::string& parameter);
		
		const std::unordered_map<std::string, RHI::ShaderBindingPtr>& GetShaderBindings() const { return m_shaderBindings; }

		ShaderPtr GetVertexShader() const { return m_vertexShader; }
		ShaderPtr GetFragmentShader() const { return m_fragmentShader; }

	protected:

		RHI::RenderState m_renderState;

		ShaderPtr m_vertexShader;
		ShaderPtr m_fragmentShader;

		std::vector<RHI::ShaderLayoutBinding> m_layoutBindings;
		std::unordered_map<std::string, RHI::ShaderBindingPtr> m_shaderBindings;

		friend class IGfxDevice;
	};
};
