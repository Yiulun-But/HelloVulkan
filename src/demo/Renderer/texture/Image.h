#pragma once

#include "renderer/context/VulkanEngine.h"
#include "ImageTypes.h"

namespace HelloVulkan {
	class Image {
	public:

		Image(
				VulkanEngine& engine,
				const ImageCreateContext& imageContext
			);

		Image() = default;

		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		Image(Image&&) noexcept = default;
		Image& operator=(Image&&) noexcept = default;

		vk::raii::Image& getImage() { return image; }

	protected:

		void CreateImage(
			const vk::raii::Device& device,
			const vk::raii::PhysicalDevice& physicalDevice,
			const ImageCreateContext& imageContext
		);

		vk::raii::ImageView createImageView(
			const vk::raii::Device& device, 
			vk::raii::Image& image, 
			ImageCreateContext context
		);

		vk::Format findSupportedFormat(
			VulkanEngine& engine, 
			const std::vector<vk::Format>& candidates, 
			vk::ImageTiling tiling, 
			vk::FormatFeatureFlags features
		);

		uint32_t findMemoryType(uint32_t typeFilter,
			vk::MemoryPropertyFlags properties,
			const vk::raii::PhysicalDevice& physicalDevice
		);

		vk::raii::Image        image = nullptr;
		vk::raii::DeviceMemory imageMemory = nullptr;
	};
}