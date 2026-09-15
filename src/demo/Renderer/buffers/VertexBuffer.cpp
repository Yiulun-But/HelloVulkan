#include "VertexBuffer.h"

namespace HelloVulkan{

	VertexBuffer::VertexBuffer(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties
	) : Buffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eVertexBuffer | usage, properties)
	{

	}

	void VertexBuffer::bind(const vk::raii::CommandBuffer& commandBuffer)
	{
		commandBuffer.bindVertexBuffers(0, getBufferHandle(), {0});
	}
}