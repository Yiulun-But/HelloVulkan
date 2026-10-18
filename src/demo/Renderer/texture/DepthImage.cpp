#include "DepthImage.h"

namespace HelloVulkan {


	DepthImage::DepthImage(VulkanEngine& engine, std::pair<uint32_t, uint32_t> size)
		: Image()
	{
		format = findDepthFormat(engine);
		ImageCreateContext depthImageContext{
			size.first,
			size.second,
			1,
			format,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eDepth,
			vk::ImageTiling::eOptimal,
			
		};

		vk::ImageCreateInfo imageInfo{
				.imageType = vk::ImageType::e2D,
				.format = depthImageContext.format,
				.extent = {depthImageContext.width, depthImageContext.height, 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = vk::SampleCountFlagBits::e1,
				.tiling = depthImageContext.tiling,
				.usage = depthImageContext.usage,
				.sharingMode = vk::SharingMode::eExclusive
		};

		image = vk::raii::Image(engine.getDevice(), imageInfo);
		vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = Image::findMemoryType(memRequirements.memoryTypeBits, depthImageContext.memoryProperties, engine.getPhysicalDevice())
		};
		imageMemory = vk::raii::DeviceMemory(engine.getDevice(), allocInfo);
		image.bindMemory(*imageMemory, 0);
		depthImageView = createImageView(engine.getDevice(), image, depthImageContext);
	}


	vk::Format DepthImage::findDepthFormat(VulkanEngine& engine)
	{
		return Image::findSupportedFormat(
			engine,
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment
		);
	}
}