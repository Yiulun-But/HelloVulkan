#include "IndexBuffer.h"

namespace HelloVulkan {

	IndexBuffer::IndexBuffer(
		VulkanEngine& engine,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::IndexType indexType
	) : Buffer(engine, size, vk::BufferUsageFlagBits::eIndexBuffer | usage, properties), indexType(indexType)
	{

	}

	void IndexBuffer::bind(const vk::raii::CommandBuffer& commandBuffer)
	{
		commandBuffer.bindIndexBuffer(getBufferHandle(), 0, indexType);
	}
}