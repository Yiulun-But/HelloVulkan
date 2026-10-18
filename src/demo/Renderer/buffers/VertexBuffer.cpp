#include "VertexBuffer.h"

namespace HelloVulkan{

	VertexBuffer::VertexBuffer(
		VulkanEngine& engine,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties
	) : Buffer(engine, size, vk::BufferUsageFlagBits::eVertexBuffer | usage, properties)
	{

	}

	void VertexBuffer::bind(const vk::raii::CommandBuffer& commandBuffer)
	{
		commandBuffer.bindVertexBuffers(0, getBufferHandle(), {0});
	}
}