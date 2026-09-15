#include "Buffer.h"

#include <stdexcept>
#include <cstring>

namespace HelloVulkan {

	static uint32_t findMemoryType(uint32_t typeFilter,
		vk::MemoryPropertyFlags properties,
		const vk::raii::PhysicalDevice& physicalDevice
	) {
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type for buffer!");
	}

	Buffer::Buffer(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties
	) : size(size), usage(usage), properties(properties)
	{
		createBuffer(device);
		allocateBufferMemory(device, physicalDevice);
	}

	void Buffer::mapData(const void* data, vk::DeviceSize size, vk::DeviceSize offset) {

		void* bufferData = memory.mapMemory(offset, size);
		memcpy(bufferData, data, size);
		memory.unmapMemory();
	}

	void Buffer::createBuffer(const vk::raii::Device& device) {
		vk::BufferCreateInfo bufferInfo{
			.size = size,
			.usage = usage,
			.sharingMode = vk::SharingMode::eExclusive
		};
		buffer = vk::raii::Buffer(device, bufferInfo);
	}

	void Buffer::allocateBufferMemory(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice) {
		vk::MemoryRequirements	memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo memoryAllocateInfo{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice)
		};
		memory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
		buffer.bindMemory(*memory, 0);
	}

}