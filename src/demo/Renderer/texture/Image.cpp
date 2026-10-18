#include "Image.h"

namespace HelloVulkan {

	uint32_t Image::findMemoryType(uint32_t typeFilter,
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
		VulkanEngine& engine,
		const ImageCreateContext& imageContext
	) {
		CreateImage(engine.getDevice(), engine.getPhysicalDevice(), imageContext);
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
			.mipLevels = imageContext.mipLevels,
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
	vk::raii::ImageView Image::createImageView(const vk::raii::Device& device, vk::raii::Image& image, ImageCreateContext context)
	{
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = context.format,
			.subresourceRange = {.aspectMask = context.aspects, .baseMipLevel = 0, .levelCount = context.mipLevels, .baseArrayLayer = 0, .layerCount = 1 }
		};

		return vk::raii::ImageView(device, viewInfo);
	}

	vk::Format Image::findSupportedFormat(
		VulkanEngine& engine,
		const std::vector<vk::Format>& candidates,
		vk::ImageTiling tiling,
		vk::FormatFeatureFlags features
	)
	{
		for (auto format : candidates) {
			vk::FormatProperties props = engine.getPhysicalDevice().getFormatProperties(format);
			if ((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features) ||
				(tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features))
			{
				return format;
			};
		}

		throw std::runtime_error("failed to find supported depth image format!");
	}
}