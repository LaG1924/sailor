#pragma once
#include "VulkanApi.h"
#include "Core/RefPtr.hpp"
#include "RHI/Types.h"

namespace Sailor::GfxDevice::Vulkan
{
	class VulkanCommandBuffer final : public RHI::Resource
	{

	public:

		const VkCommandBuffer* GetHandle() const { return &m_commandBuffer; }
		operator VkCommandBuffer() const { return m_commandBuffer; }

		VulkanCommandPoolPtr GetCommandPool() const;

		VulkanCommandBuffer(VulkanDevicePtr device, VulkanCommandPoolPtr commandPool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		virtual ~VulkanCommandBuffer() override;

		void BeginCommandList(VkCommandBufferUsageFlags flags = 0);
		void EndCommandList();

		void BeginRenderPass(VulkanRenderPassPtr renderPass,
			VulkanFramebufferPtr frameBuffer,
			VkExtent2D extent,
			VkOffset2D offset = { 0,0 },
			VkClearValue clearColor = VulkanApi::DefaultClearColor);

		void EndRenderPass();

		void BindVertexBuffers(std::vector<VulkanBufferPtr> buffers, std::vector <VkDeviceSize> offsets = { 0 }, uint32_t firstBinding = 0, uint32_t bindingCount = 1);
		void BindIndexBuffer(VulkanBufferPtr indexBuffer);
		void BindDescriptorSet(VulkanPipelineLayoutPtr pipelineLayout, std::vector<VulkanDescriptorSetPtr> descriptorSet);
		void BindPipeline(VulkanPipelinePtr pipeline);

		void DrawIndexed(VulkanBufferPtr indexBuffer);

		void Execute(VulkanCommandBufferPtr secondaryCommandBuffer);
		void CopyBuffer(VulkanBufferPtr  src, VulkanBufferPtr dst, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
		void CopyBufferToImage(VulkanBufferPtr src, VulkanImagePtr image, uint32_t width, uint32_t height);

		void SetViewport(VulkanStateViewportPtr viewport);
		void SetScissor(VulkanStateViewportPtr viewport);

		void ImageMemoryBarrier(VulkanImagePtr image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		void Blit(VulkanImagePtr srcImage, VkImageLayout srcImageLayout, VulkanImagePtr dstImage, VkImageLayout dstImageLayout,
			uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);

		void GenerateMipMaps(VulkanImagePtr image);
		void ClearDependencies();
		void Reset();

	protected:

		std::vector<VulkanBufferPtr> m_bufferDependencies;
		std::vector<VulkanImagePtr> m_imageDependencies;

		VulkanDevicePtr m_device;
		VulkanCommandPoolPtr m_commandPool;
		VkCommandBuffer m_commandBuffer;
		VkCommandBufferLevel m_level;
	};
}
