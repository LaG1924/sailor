#include "VulkanBufferMemory.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

using namespace Sailor;
using namespace Sailor::Memory;
using namespace Sailor::GfxDevice::Vulkan;

VulkanBufferMemoryPtr::VulkanBufferMemoryPtr(TRefPtr<Sailor::GfxDevice::Vulkan::VulkanBuffer> buffer) : m_buffer(buffer) {}
VulkanBufferMemoryPtr::VulkanBufferMemoryPtr(TRefPtr<Sailor::GfxDevice::Vulkan::VulkanBuffer> buffer, size_t offset, size_t size) :
	m_buffer(buffer), m_offset(offset), m_size(size) {}

VulkanBufferMemoryPtr& VulkanBufferMemoryPtr::operator=(const TRefPtr<Sailor::GfxDevice::Vulkan::VulkanBuffer>& rhs)
{
	m_buffer = rhs;
	return *this;
}

VulkanBufferMemoryPtr::operator bool() const
{
	return m_buffer;
}

VulkanBufferMemoryPtr GlobalVulkanBufferAllocator::Allocate(size_t size)
{
	auto buffer = Sailor::GfxDevice::Vulkan::VulkanApi::CreateBuffer(Sailor::GfxDevice::Vulkan::VulkanApi::GetInstance()->GetMainDevice(),
		size, m_usage, m_memoryProperties);

	VulkanBufferMemoryPtr memPtr(buffer, 0, size);
	return memPtr;
}

void GlobalVulkanBufferAllocator::Free(VulkanBufferMemoryPtr pData, size_t size)
{
	pData.m_buffer.Clear();
	pData.m_offset = pData.m_size = 0;
}
