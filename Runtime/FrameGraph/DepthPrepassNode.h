#pragma once
#include "Core/Defines.h"
#include "Memory/RefPtr.hpp"
#include "Engine/Object.h"
#include "RHI/Types.h"
#include "RHI/Batch.hpp"
#include "FrameGraph/BaseFrameGraphNode.h"
#include "FrameGraph/FrameGraphNode.h"

namespace Sailor
{
	// TODO: We have an issue with enabled MSAA, since MSAA depth target is stored in temporary MSAA target that is handled by VulkanGraphicsDriver
	class DepthPrepassNode : public TFrameGraphNode<DepthPrepassNode>
	{
	public:

		struct PerInstanceData
		{
			alignas(16) glm::mat4 model;
			alignas(16) uint32_t materialInstance = 0;

			bool operator==(const PerInstanceData& rhs) const { return this->model == model; }

			size_t GetHash() const
			{
				hash<glm::mat4> p;
				return p(model);
			}
		};

		SAILOR_API static const char* GetName() { return m_name; }

		SAILOR_API Tasks::TaskPtr<void, void> Prepare(RHI::RHIFrameGraph* frameGraph, const RHI::RHISceneViewSnapshot& sceneView);
		SAILOR_API virtual void Process(RHI::RHIFrameGraph* frameGraph, RHI::RHICommandListPtr transferCommandList, RHI::RHICommandListPtr commandLists, const RHI::RHISceneViewSnapshot& sceneView) override;
		SAILOR_API virtual void Clear() override;
		SAILOR_API RHI::ESortingOrder GetSortingOrder() const;

	protected:

		uint32_t m_numMeshes = 0;
		SpinLock m_syncSharedResources;
		RHI::TDrawCalls<PerInstanceData> m_drawCalls;
		TSet<RHI::RHIBatch> m_batches;

		TMap<RHI::VertexAttributeBits, RHI::RHIMaterialPtr> m_depthOnlyMaterials;
		RHI::RHIShaderBindingSetPtr m_perInstanceData;
		size_t m_sizePerInstanceData = 0;

		RHI::RHIMaterialPtr GetOrAddDepthMaterial(RHI::RHIVertexDescriptionPtr vertex);		
		TVector<RHI::RHIBufferPtr> m_indirectBuffers;

		static const char* m_name;
	};

	template class TFrameGraphNode<DepthPrepassNode>;
};
