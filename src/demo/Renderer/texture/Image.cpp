#include "Image.h"

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

	Image::Image(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		const ImageCreateContext& imageContext
	) {
		CreateImage(device, physicalDevice, imageContext);
	}

	void Image::CreateImage(
		const vk::raii::Device& device,
		const vk::raii::PhysicalDevice& physicalDevice,
		const ImageCreateContext& imageContext
	) {
		vk::ImageCreateInfo imageInfo{
			.imageType = vk::ImageType::e2D,
			.format = imageContext.format,
			.extent = {imageContext.width, imageContext.height, 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = vk::SampleCountFlagBits::e1,
			.tiling = imageContext.tiling,
			.usage = imageContext.usage,
			.sharingMode = vk::SharingMode::eExclusive
		};

		image = vk::raii::Image(device, imageInfo);
		vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, imageContext.memoryProperties, physicalDevice)
		};
		imageMemory = vk::raii::DeviceMemory(device, allocInfo);
		image.bindMemory(*imageMemory, 0);
	}
}