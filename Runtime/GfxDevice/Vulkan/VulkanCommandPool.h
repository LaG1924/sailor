#pragma once
#include "VulkanApi.h"
#include "Core/RefPtr.hpp"
#include "RHI/Types.h"

namespace Sailor::GfxDevice::Vulkan
{
	class VulkanDevice;
	
	class VulkanCommandPool final : public RHI::Resource
	{

	public:

		const VkCommandPool* GetHandle() const { return &m_commandPool; }
		operator VkCommandPool() const { return m_commandPool; }

		void Reset(VkCommandPoolResetFlags flags = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT) const;

		VulkanCommandPool(VulkanDevicePtr device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

		virtual ~VulkanCommandPool() override;

	protected:

		VulkanDevicePtr m_device;
		VkCommandPool m_commandPool;
	};
}
