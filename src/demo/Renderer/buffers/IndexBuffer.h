#pragma once

#include "Buffer.h"

namespace HelloVulkan {

	class IndexBuffer : public Buffer
	{
	public:

		IndexBuffer(
			const vk::raii::Device& device,
			const vk::raii::PhysicalDevice& physicalDevice,
			vk::DeviceSize size,
			vk::BufferUsageFlags usage,
			vk::MemoryPropertyFlags properties,
			vk::IndexType indexType
		);

		void bind(const vk::raii::CommandBuffer& commandBuffer);

	private:
		vk::IndexType indexType;
	};
}