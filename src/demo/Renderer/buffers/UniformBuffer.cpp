#include "UniformBuffer.h"

namespace HelloVulkan {
	UniformBuffer::UniformBuffer(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		vk::DeviceSize size

	) : Buffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible)
	{
		mappedMem = getMemory().mapMemory(0, size);
	}

	UniformBuffer::~UniformBuffer() {
		if (mappedMem)
		{
			getMemory().unmapMemory();
		}
	}

	void UniformBuffer::update(const void* src, vk::DeviceSize size, vk::DeviceSize offset) {
		if (size + offset > getSize())
		{
			throw std::runtime_error("Uniform buffer update exceed buffer size!");
		}
		memcpy(static_cast<std::byte*>(mappedMem) + offset, src, static_cast<size_t>(size));
	}

	UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
		: Buffer(std::move(other)), mappedMem(other.mappedMem)
	{
		other.mappedMem = nullptr;
	}

	UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept {
		if (this != &other)
		{
			if (mappedMem)
			{
				getMemory().unmapMemory();
			}
			Buffer::operator=(std::move(other));
			mappedMem = other.mappedMem;
			other.mappedMem = nullptr;
		}
		return *this;
	}
}