#pragma once

#include "Buffer.h"

namespace HelloVulkan {

	class VertexBuffer : public Buffer
	{
	public:

		VertexBuffer(
			VulkanEngine& engine,
			vk::DeviceSize size,
			vk::BufferUsageFlags usage,
			vk::MemoryPropertyFlags properties
		);

		void bind (const vk::raii::CommandBuffer& commandBuffer);

	private:
	};
}