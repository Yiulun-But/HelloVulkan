#pragma once

#include "ImageTypes.h"

namespace HelloVulkan {
	class Image {
	public:

		Image(
				const vk::raii::Device& device,
				const vk::raii::PhysicalDevice& physicalDevice,
				const ImageCreateContext& imageContext
			);

		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		Image(Image&&) noexcept = default;
		Image& operator=(Image&&) noexcept = default;

	protected:

		void CreateImage(
			const vk::raii::Device& device,
			const vk::raii::PhysicalDevice& physicalDevice,
			const ImageCreateContext& imageContext
		);

		vk::raii::Image        image = nullptr;
		vk::raii::DeviceMemory imageMemory = nullptr;
	};
}