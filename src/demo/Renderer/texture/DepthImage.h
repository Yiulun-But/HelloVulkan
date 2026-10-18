#pragma once 
#include "Image.h"

namespace HelloVulkan {
	class DepthImage : public Image {
	public:
		DepthImage(
			VulkanEngine& engine,
			std::pair<uint32_t, uint32_t> size
		);

		inline const vk::raii::ImageView& getDepthImageView() { return depthImageView; }
		inline const vk::Format getDepthFormat() const { return format; }

	private:

		vk::Format findDepthFormat(VulkanEngine& engine);

		vk::raii::ImageView depthImageView = nullptr;
		vk::Format format;
	};
}