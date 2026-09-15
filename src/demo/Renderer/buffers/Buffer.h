#pragma once

#include <vulkan/vulkan.hpp>
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan.hpp;
#endif

#include <vector>
#include <cstddef>
#include <utility>


namespace HelloVulkan {

	class Buffer
	{
	public:

		Buffer(
			const vk::raii::Device& device,
			const vk::raii::PhysicalDevice& physicalDevice,
			vk::DeviceSize size,
			vk::BufferUsageFlags usage,
			vk::MemoryPropertyFlags properties
		);

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		Buffer(Buffer&&) noexcept = default;
		Buffer& operator=(Buffer&&) noexcept = default;

		inline const vk::Buffer& getBufferHandle() { return *buffer; };
		inline const vk::raii::Buffer& getBuffer() { return buffer; };
		inline vk::raii::DeviceMemory& getMemory() { return memory; };
		inline vk::DeviceSize getSize() { return size; };

		void mapData(const void* data, vk::DeviceSize size, vk::DeviceSize offset);


	private:

		void createBuffer(const vk::raii::Device& device);

		void allocateBufferMemory(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);


		vk::DeviceSize			size;
		vk::BufferUsageFlags	usage;
		vk::MemoryPropertyFlags	properties;

		// Reversed order so buffer is destructed first
		vk::raii::DeviceMemory	memory = nullptr;
		vk::raii::Buffer		buffer = nullptr;
	};

}