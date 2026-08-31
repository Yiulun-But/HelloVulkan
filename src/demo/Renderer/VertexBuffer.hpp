#pragma once

#include <vulkan/vulkan.hpp>
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan.hpp;
#endif
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstddef>

namespace HelloVulkan {

	class VertexBuffer
	{
	public:
		struct Vertex
		{
			glm::vec4 pos;
			glm::vec4 col;

			static vk::VertexInputBindingDescription getBindingDescription() {
				return { .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex };
			}

			static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
				return { {
					{ .location = 0, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat,	.offset = offsetof(Vertex, pos) },
					{ .location = 1, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat,	.offset = offsetof(Vertex, col) }
				} };
			}
		};


		const std::vector<Vertex> vertices{
			{ {  0.0f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
			{ {  0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }
		};

		VertexBuffer(
			const vk::raii::Device& device,
			const vk::raii::PhysicalDevice& physicalDevice
		) {
			bufferSize = sizeof(vertices[0]) * vertices.size();
			createVertexBuffer(device);
			allocateBufferMemory(device, physicalDevice);
			mapVertexData();
		}

		void bind(const vk::raii::CommandBuffer& commandBuffer)
		{
			commandBuffer.bindVertexBuffers(0, *vertexBuffer, { 0 });
		}

	private:

		void createVertexBuffer(const vk::raii::Device& device) {
			vk::BufferCreateInfo bufferInfo {
				.size = bufferSize,
				.usage = vk::BufferUsageFlagBits::eVertexBuffer,
				.sharingMode = vk::SharingMode::eExclusive
			};
			vertexBuffer = vk::raii::Buffer(device, bufferInfo);
		}

		void allocateBufferMemory(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice) {
			vk::MemoryRequirements memRequirements = vertexBuffer.getMemoryRequirements();
			vk::MemoryAllocateInfo memoryAllocateInfo{
				.allocationSize = memRequirements.size,
				.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, physicalDevice)
			};
			vertexBufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
			vertexBuffer.bindMemory(*vertexBufferMemory, 0);
		}

		void mapVertexData()
		{
			void* data = vertexBufferMemory.mapMemory(0, bufferSize);
			memcpy(data, vertices.data(), bufferSize);
			vertexBufferMemory.unmapMemory();
		}

		uint32_t findMemoryType(uint32_t typeFilter,
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


		vk::raii::DeviceMemory vertexBufferMemory = nullptr;
		vk::raii::Buffer vertexBuffer = nullptr;
		vk::DeviceSize bufferSize = 0;
	};
}