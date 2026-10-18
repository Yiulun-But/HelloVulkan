#pragma once
#include "Buffer.h"

namespace HelloVulkan {

	class UniformBuffer : public Buffer {
	public:
		UniformBuffer(
			VulkanEngine& engine,
			vk::DeviceSize size
		);

		~UniformBuffer();

		UniformBuffer(const UniformBuffer&) = delete;
		UniformBuffer& operator=(const UniformBuffer&) = delete;

		UniformBuffer(UniformBuffer&& other) noexcept;
		UniformBuffer& operator=(UniformBuffer&& other) noexcept;

		void mapData(const void* data, vk::DeviceSize size, vk::DeviceSize offset) = delete;

		void update(const void* src, vk::DeviceSize size, vk::DeviceSize offset);

	private:
		void* mappedMem;
	};

}