#include "IndexBuffer.h"

namespace HelloVulkan {

	IndexBuffer::IndexBuffer(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::IndexType indexType
	) : Buffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eIndexBuffer | usage, properties), indexType(indexType)
	{

	}

	void IndexBuffer::bind(const vk::raii::CommandBuffer& commandBuffer)
	{
		commandBuffer.bindIndexBuffer(getBufferHandle(), 0, indexType);
	}
}