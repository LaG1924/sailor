#pragma once
#include "Memory/RefPtr.hpp"
#include "Renderer.h"
#include "Types.h"
#include "Containers/Map.h"
#include "GraphicsDriver/Vulkan/VulkanApi.h"
#include "GraphicsDriver/Vulkan/VulkanBufferMemory.h"

namespace Sailor::RHI
{
	class ShaderBindingSet : public Resource, public IDelayedInitialization
	{
	public:

#if defined(SAILOR_BUILD_WITH_VULKAN)
		struct
		{
			GraphicsDriver::Vulkan::VulkanDescriptorSetPtr m_descriptorSet;
		} m_vulkan;
#endif

		SAILOR_API void AddLayoutShaderBinding(ShaderLayoutBinding layout);
		SAILOR_API void SetLayoutShaderBindings(TVector<RHI::ShaderLayoutBinding> layoutBindings);
		SAILOR_API const TVector<RHI::ShaderLayoutBinding>& GetLayoutBindings() const { return m_layoutBindings; }
		SAILOR_API RHI::ShaderBindingPtr& GetOrCreateShaderBinding(const std::string& binding);
		SAILOR_API const TConcurrentMap<std::string, RHI::ShaderBindingPtr>& GetShaderBindings() const { return m_shaderBindings; }

		static SAILOR_API void ParseParameter(const std::string& parameter, std::string& outBinding, std::string& outVariable);

		SAILOR_API bool HasBinding(const std::string& binding) const;
		SAILOR_API bool HasParameter(const std::string& parameter) const;

		SAILOR_API bool NeedsStorageBuffer() const { return m_bNeedsStorageBuffer; }

	protected:

		SAILOR_API bool PerInstanceDataStoredInSSBO() const;

		TVector<RHI::ShaderLayoutBinding> m_layoutBindings;
		TConcurrentMap<std::string, RHI::ShaderBindingPtr> m_shaderBindings;
		bool m_bNeedsStorageBuffer = false;
	};

	class Material : public Resource
	{
	public:
#if defined(SAILOR_BUILD_WITH_VULKAN)
		struct
		{
			Sailor::GraphicsDriver::Vulkan::VulkanPipelinePtr m_pipeline;
		} m_vulkan;
#endif
		SAILOR_API Material(RenderState renderState, ShaderPtr vertexShader, ShaderPtr fragmentShader) :
			m_renderState(std::move(renderState)),
			m_vertexShader(vertexShader),
			m_fragmentShader(fragmentShader)
		{}

		SAILOR_API const RHI::RenderState& GetRenderState() const { return m_renderState; }

		SAILOR_API ShaderPtr GetVertexShader() const { return m_vertexShader; }
		SAILOR_API ShaderPtr GetFragmentShader() const { return m_fragmentShader; }

		SAILOR_API ShaderBindingSetPtr GetBindings() const { return m_bindings; }
		SAILOR_API void SetBindings(ShaderBindingSetPtr bindings) { m_bindings = bindings; }

	protected:

		RHI::RenderState m_renderState;

		ShaderPtr m_vertexShader;
		ShaderPtr m_fragmentShader;

		ShaderBindingSetPtr m_bindings;

		friend class IGraphicsDriver;
	};
};
